// svg-input.js — SVG-only @input slider support for the web build.
//
// Ports the minimal slice of app/js/mda-parser.js + app/js/ui-generator.js needed to drive
// an interactive svg() curve() from a slider: parse `// @input slider ... target="curveId"`
// annotations, inject a <input type="range"> right below the matching <svg>, and on drag
// recompute just that curve via svg_eval_curve (no full re-run, no editor write-back).
//
// Public API:
//   SvgInput.mount(outputEl, source, wrapper)

(function () {
  'use strict';

  const ATTR_RE = /(\w+)\s*=\s*("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'|true|false|[-+]?\d+(?:\.\d+)?|\w+)/g;

  function parseAttrs(rest) {
    const out = {};
    let m;
    ATTR_RE.lastIndex = 0;
    while ((m = ATTR_RE.exec(rest))) {
      let val = m[2];
      if ((val[0] === '"' && val.endsWith('"')) || (val[0] === "'" && val.endsWith("'"))) {
        val = val.slice(1, -1).replace(/\\(.)/g, '$1');
      } else if (val === 'true') val = true;
      else if (val === 'false') val = false;
      else if (/^[-+]?\d/.test(val)) val = parseFloat(val);
      out[m[1]] = val;
    }
    return out;
  }

  // Extract only slider @inputs that carry a target="<curveId>" attribute — everything
  // else (matrix/vector/select/plain number, or sliders with no target) is out of scope here.
  function extractSvgSliders(source) {
    const lines = source.split('\n');
    const inputs = [];
    let offset = 0;

    for (let i = 0; i < lines.length; i++) {
      const line = lines[i];
      const lineStart = offset;
      offset += line.length + 1;

      const annIdx = line.search(/\/\/\s*@input\b/);
      if (annIdx < 0) continue;

      const codePart = line.slice(0, annIdx);
      const assignIdx = codePart.indexOf(':=');
      if (assignIdx < 0) continue;

      const nameMatch = codePart.slice(0, assignIdx).match(/([A-Za-z_]\w*)\s*$/);
      if (!nameMatch) continue;
      const name = nameMatch[1];

      const afterAssign = codePart.slice(assignIdx + 2);
      const semiIdx = afterAssign.lastIndexOf(';');
      const literalEndInAfter = semiIdx >= 0 ? semiIdx : afterAssign.length;
      const literalRaw = afterAssign.slice(0, literalEndInAfter);
      const literalTrimmed = literalRaw.trim();
      if (!literalTrimmed) continue;

      const literalStartInLine = assignIdx + 2 + literalRaw.indexOf(literalTrimmed);
      const literalStart = lineStart + literalStartInLine;
      const literalEnd = literalStart + literalTrimmed.length;

      const annRest = line.slice(annIdx).replace(/^\/\/\s*@input\s*/, '');
      const firstToken = (annRest.match(/^\w+/) || [''])[0];
      const attrs = parseAttrs(annRest);

      const isSlider = firstToken === 'slider' || attrs.slider;
      if (!isSlider || !attrs.target) continue;

      const value = parseFloat(literalTrimmed);
      if (Number.isNaN(value)) continue;

      inputs.push({
        name,
        value,
        attrs,
        literalStart,
        literalEnd,
      });
    }
    return inputs;
  }

  function replaceLiteral(source, desc, newValue) {
    const before = source.slice(0, desc.literalStart);
    const after = source.slice(desc.literalEnd);
    return before + formatNumber(newValue) + after;
  }

  function formatNumber(n) {
    if (Number.isInteger(n)) return String(n);
    return parseFloat(n.toFixed(6)).toString();
  }

  function findSvgContainer(outputEl, idx) {
    return (
      outputEl.querySelector('[data-svg-index="' + idx + '"]') ||
      outputEl.querySelectorAll('svg')[idx]
    );
  }

  function buildSliderRow(desc) {
    const row = document.createElement('div');
    row.className = 'svg-input-slider-row';
    row.style.cssText = 'display:flex;align-items:center;gap:8px;margin:6px 0;font-size:13px;';

    const label = document.createElement('span');
    label.textContent = (desc.attrs.label || desc.name) + ':';
    label.style.cssText = 'min-width:80px;';

    const range = document.createElement('input');
    range.type = 'range';
    range.min = desc.attrs.min != null ? desc.attrs.min : 0;
    range.max = desc.attrs.max != null ? desc.attrs.max : 100;
    range.step = desc.attrs.step != null ? desc.attrs.step : 1;
    range.value = desc.value;
    range.style.cssText = 'flex:1;max-width:240px;';

    const valueLabel = document.createElement('span');
    valueLabel.textContent = formatNumber(desc.value);
    valueLabel.style.cssText = 'min-width:40px;text-align:right;';

    row.appendChild(label);
    row.appendChild(range);
    row.appendChild(valueLabel);

    return { row, range, valueLabel };
  }

  function mount(outputEl, source, wrapper) {
    if (!outputEl || !wrapper) return;

    // Clear any sliders injected by a previous run.
    outputEl.querySelectorAll('.svg-input-slider-row').forEach((el) => el.remove());

    const descriptors = extractSvgSliders(source);
    if (descriptors.length === 0) return;

    // Group by target curveId so each curve's SVG container gets its own slider(s) below it.
    const byCurve = new Map();
    descriptors.forEach((desc) => {
      const curveId = desc.attrs.target;
      if (!byCurve.has(curveId)) byCurve.set(curveId, []);
      byCurve.get(curveId).push(desc);
    });

    byCurve.forEach((descs, curveId) => {
      const pathEl = outputEl.querySelector('[data-curve-id="' + CSS.escape(curveId) + '"]');
      if (!pathEl) return; // curve not present in this render; skip silently

      const svgContainer = pathEl.closest('.svg-container') || pathEl.closest('svg');
      if (!svgContainer) return;

      let seq = 0;
      descs.forEach((desc) => {
        const { row, range, valueLabel } = buildSliderRow(desc);
        svgContainer.insertAdjacentElement('afterend', row);

        range.addEventListener('input', async () => {
          const newValue = parseFloat(range.value);
          valueLabel.textContent = formatNumber(newValue);

          const mySeq = ++seq;
          const workingSource = replaceLiteral(source, desc, newValue);
          let result;
          try {
            result = await wrapper.evalSvgCurve(workingSource, curveId, desc.name, newValue);
          } catch (e) {
            return;
          }
          if (mySeq !== seq) return; // superseded by a newer drag event
          if (!result || !result.success) return;

          const livePath = outputEl.querySelector('[data-curve-id="' + CSS.escape(curveId) + '"]');
          if (livePath) livePath.setAttribute('d', result.d);
        });
      });
    });
  }

  window.SvgInput = { mount };
})();
