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

      // target is a comma-separated list: each entry is either a curve id (matched
      // against data-curve-id) or a displayed result variable name (matched against
      // data-result-var) — resolved against the DOM in mount(), not here.
      const targets = String(attrs.target).split(',').map((t) => t.trim()).filter(Boolean);

      inputs.push({
        name,
        value,
        attrs,
        targets,
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

  // Re-typeset a single element's math after its $$...$$ text content changes —
  // MathJax replaces $$...$$ with rendered <mjx-container> markup, so a later
  // change needs to reset to the raw source form before re-typesetting.
  async function retypeset(el) {
    if (window.MathJax && window.MathJax.typesetPromise) {
      try {
        if (window.MathJax.typesetClear) window.MathJax.typesetClear([el]);
        await window.MathJax.typesetPromise([el]);
      } catch (e) { /* ignore */ }
    }
  }

  function mount(outputEl, source, wrapper) {
    if (!outputEl || !wrapper) return;

    // Clear any sliders injected by a previous run.
    outputEl.querySelectorAll('.svg-input-slider-row').forEach((el) => el.remove());

    const descriptors = extractSvgSliders(source);
    if (descriptors.length === 0) return;

    // Resolve each descriptor's targets against the rendered DOM once: curve targets
    // attach the slider row after their SVG container; result-variable targets are
    // tracked for live text updates but don't host the slider themselves.
    descriptors.forEach((desc) => {
      let mountedAfter = null;
      const curveIds = [];
      const resultVars = []; // { name, el, prefix, suffix }

      desc.targets.forEach((target) => {
        const pathEl = outputEl.querySelector('[data-curve-id="' + CSS.escape(target) + '"]');
        if (pathEl) {
          const svgContainer = pathEl.closest('.svg-container') || pathEl.closest('svg');
          if (svgContainer && !mountedAfter) mountedAfter = svgContainer;
          curveIds.push(target);
          return;
        }
        const resultEl = outputEl.querySelector('[data-result-var="' + CSS.escape(target) + '"]');
        if (resultEl) {
          // Cache the raw "$$name = value$$" template now, before MathJax replaces
          // this element's text content with rendered <mjx-container> markup.
          const rawText = resultEl.textContent;
          const match = rawText.match(/^([\s\S]*=\s*)([^=]+?)(\$\$\s*)$/);
          if (match) {
            resultVars.push({ name: target, el: resultEl, prefix: match[1], suffix: match[3] });
          }
        }
        // Unmatched target: silently ignored (curve/result not present in this render).
      });

      if (!mountedAfter || curveIds.length === 0) return;

      const { row, range, valueLabel } = buildSliderRow(desc);
      mountedAfter.insertAdjacentElement('afterend', row);

      // Apply a fresh result value to its cached $$...$$ template, then re-typeset it.
      const applyResult = (rv, latexValue) => {
        rv.el.textContent = rv.prefix + latexValue + rv.suffix;
        retypeset(rv.el);
      };

      let seq = 0;
      range.addEventListener('input', async () => {
        const newValue = parseFloat(range.value);
        valueLabel.textContent = formatNumber(newValue);

        const mySeq = ++seq;
        const workingSource = replaceLiteral(source, desc, newValue);

        // The primary curve is recomputed together with the first result variable in a
        // SINGLE evaluate() pass (svg_eval_curve returns both), so a drag never parses/
        // evaluates the same source twice. Any extra curves/results (rare) are handled
        // by follow-up calls to keep behavior correct without complicating the common case.
        const primaryCurve = curveIds[0];
        const primaryResult = resultVars[0];

        const first = await wrapper
          .evalSvgCurve(workingSource, primaryCurve, desc.name, newValue,
                        primaryResult ? primaryResult.name : '')
          .catch(() => null);
        if (mySeq !== seq || !first || !first.success) return;

        const livePath = outputEl.querySelector('[data-curve-id="' + CSS.escape(primaryCurve) + '"]');
        if (livePath) livePath.setAttribute('d', first.d);
        if (primaryResult && first.resultValue != null) applyResult(primaryResult, first.resultValue);

        // Extra curves (beyond the first) — each its own single-pass recompute.
        for (let c = 1; c < curveIds.length; c++) {
          const r = await wrapper.evalSvgCurve(workingSource, curveIds[c], desc.name, newValue).catch(() => null);
          if (mySeq !== seq || !r || !r.success) return;
          const p = outputEl.querySelector('[data-curve-id="' + CSS.escape(curveIds[c]) + '"]');
          if (p) p.setAttribute('d', r.d);
        }

        // Extra result variables (beyond the first) — evaluated by name.
        for (let k = 1; k < resultVars.length; k++) {
          const r = await wrapper.evalNamedValue(workingSource, resultVars[k].name).catch(() => null);
          if (mySeq !== seq || !r || !r.success) return;
          applyResult(resultVars[k], r.value);
        }
      });
    });
  }

  window.SvgInput = { mount };
})();
