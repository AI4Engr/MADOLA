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

  // @input parsing / literal rewriting come from the shared MdaInputs module
  // (shared/js/mda-inputs.js) — the same parser the app's low-code UI uses.
  const replaceLiteral = window.MdaInputs.replaceLiteral;
  const formatNumber = window.MdaInputs.formatNumber;

  // Extract only slider @inputs that carry a target="<curveId>" attribute — a thin filter
  // over the shared full parser. Everything else (matrix/vector/select/plain number, or
  // sliders with no target) is out of scope for the SVG slider UI.
  function extractSvgSliders(source) {
    return window.MdaInputs.extractInputs(source)
      .filter((d) => (d.type === 'slider' || d.attrs.slider) && d.attrs.target)
      .map((d) => ({
        name: d.name,
        type: d.type,
        value: d.value,
        attrs: d.attrs,
        literalStart: d.literalStart,
        literalEnd: d.literalEnd,
        // target is a comma-separated list: each entry is either a curve id (matched
        // against data-curve-id) or a displayed result variable name (matched against
        // data-result-var) — resolved against the DOM in mount(), not here.
        targets: String(d.attrs.target).split(',').map((t) => t.trim()).filter(Boolean),
      }));
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

    // Resolve targets, mount the slider row, and wire the drag handler. Target
    // resolution and the recompute algorithm are shared (SvgCurveSync); web only owns
    // the slider-injection UI (where the range input is placed).
    descriptors.forEach((desc) => {
      const resolved = window.SvgCurveSync.resolveTargets(outputEl, desc);
      if (!resolved.mountedAfter || resolved.curveIds.length === 0) return;

      const { row, range, valueLabel } = buildSliderRow(desc);
      resolved.mountedAfter.insertAdjacentElement('afterend', row);

      const seqRef = { value: 0 };
      range.addEventListener('input', () => {
        const newValue = parseFloat(range.value);
        valueLabel.textContent = formatNumber(newValue);
        window.SvgCurveSync.recompute({
          wrapper,
          outputEl,
          source: replaceLiteral(source, desc, newValue),
          desc,
          newValue,
          resolved,
          seqRef,
          retypeset,
        });
      });
    });
  }

  window.SvgInput = { mount };
})();
