// AUTO-GENERATED COPY — do not edit here. Source of truth: shared/js/
// Regenerate with: node scripts/sync-shared.js
// svg-curve-sync.js — interactive svg() curve + @result recompute core.
//
// Shared between app/ (low-code widget panel) and web/ (slider injected under the SVG).
// The rendered HTML (from the WASM formatter) uses the same DOM conventions in both:
//   - a sampled curve is a <path data-curve-id="...">
//   - a live @result value sits in an element with data-result-var="..."
//   - a curve lives inside a .svg-container (or bare <svg>)
// so target resolution and the drag recompute are identical; only WHERE each app mounts
// its slider UI differs (kept in each caller).
//
// A slider's `target` attribute is a comma-separated list; each entry is either a
// curve id or a displayed result-variable name, resolved against the DOM here.
//
// Public API: window.SvgCurveSync / self.SvgCurveSync
//   resolveTargets(outputEl, desc) -> { mountedAfter, curveIds, resultVars }
//   recompute(opts) -> Promise<void>   (see recompute() for opts shape)

(function () {
  'use strict';

  const root = (typeof window !== 'undefined') ? window : self;

  // Split a descriptor's `target` attr into trimmed entries (comma-separated list).
  function targetsOf(desc) {
    if (Array.isArray(desc.targets)) return desc.targets;
    const t = desc.attrs && desc.attrs.target;
    if (!t) return [];
    return String(t).split(',').map((s) => s.trim()).filter(Boolean);
  }

  // Resolve a descriptor's targets against the rendered DOM:
  //   - curve targets ([data-curve-id]) yield a curve id, and the first one's
  //     .svg-container becomes the slider mount anchor.
  //   - result targets ([data-result-var]) cache their raw "$$name = value$$" text
  //     (split into prefix/suffix around the value) BEFORE MathJax replaces it.
  function resolveTargets(outputEl, desc) {
    let mountedAfter = null;
    const curveIds = [];
    const resultVars = []; // { name, el, prefix, suffix }

    for (const target of targetsOf(desc)) {
      const pathEl = outputEl.querySelector('[data-curve-id="' + CSS.escape(target) + '"]');
      if (pathEl) {
        const svgContainer = pathEl.closest('.svg-container') || pathEl.closest('svg');
        if (svgContainer && !mountedAfter) mountedAfter = svgContainer;
        curveIds.push(target);
        continue;
      }
      const resultEl = outputEl.querySelector('[data-result-var="' + CSS.escape(target) + '"]');
      if (resultEl) {
        const rawText = resultEl.textContent;
        const match = rawText.match(/^([\s\S]*=\s*)([^=]+?)(\$\$\s*)$/);
        if (match) {
          resultVars.push({ name: target, el: resultEl, prefix: match[1], suffix: match[3] });
        }
      }
      // Unmatched target: silently ignored (curve/result not present in this render).
    }
    return { mountedAfter, curveIds, resultVars };
  }

  // Recompute one drag: rebind the dragged variable in source, recompute the primary
  // curve + first result variable in a SINGLE evaluate() pass, then patch the DOM.
  // Extra curves/results (rare) get follow-up calls. A monotonic seq guard drops stale
  // async results from superseded drags.
  //
  // opts:
  //   wrapper     — object with evalSvgCurve(source, curveId, paramName, value, resultVar?)
  //                 and evalNamedValue(source, varName); shape shared by app & web bridges.
  //                 (app passes window.MadolaRuntime; web passes its MadolaBrowserWrapper.)
  //   outputEl    — root element to query curves/results within
  //   source      — the working .mda source WITH the dragged literal already rebound
  //   desc        — the input descriptor (for its name)
  //   newValue    — the new numeric value
  //   resolved    — { curveIds, resultVars } from resolveTargets (pass to avoid re-querying)
  //   seqRef      — { value } monotonic counter shared across drags of one slider
  //   retypeset   — async fn(el) to re-render a result element's math (caller-provided;
  //                 app may no-op if it has no MathJax result blocks)
  async function recompute(opts) {
    const { wrapper, outputEl, source, desc, newValue, resolved, seqRef, retypeset } = opts;
    const curveIds = resolved.curveIds;
    const resultVars = resolved.resultVars;
    if (!curveIds.length) return;

    const mySeq = ++seqRef.value;

    const applyResult = (rv, latexValue) => {
      rv.el.textContent = rv.prefix + latexValue + rv.suffix;
      if (retypeset) retypeset(rv.el);
    };

    const primaryCurve = curveIds[0];
    const primaryResult = resultVars[0];

    const first = await wrapper
      .evalSvgCurve(source, primaryCurve, desc.name, newValue, primaryResult ? primaryResult.name : '')
      .catch(() => null);
    if (mySeq !== seqRef.value || !first || !first.success) return;

    const livePath = outputEl.querySelector('[data-curve-id="' + CSS.escape(primaryCurve) + '"]');
    if (livePath) livePath.setAttribute('d', first.d);
    if (primaryResult && first.resultValue != null) applyResult(primaryResult, first.resultValue);

    // Extra curves (beyond the first) — each its own single-pass recompute.
    for (let c = 1; c < curveIds.length; c++) {
      const r = await wrapper.evalSvgCurve(source, curveIds[c], desc.name, newValue).catch(() => null);
      if (mySeq !== seqRef.value || !r || !r.success) return;
      const p = outputEl.querySelector('[data-curve-id="' + CSS.escape(curveIds[c]) + '"]');
      if (p) p.setAttribute('d', r.d);
    }

    // Extra result variables (beyond the first) — evaluated by name.
    for (let k = 1; k < resultVars.length; k++) {
      const r = await wrapper.evalNamedValue(source, resultVars[k].name).catch(() => null);
      if (mySeq !== seqRef.value || !r || !r.success) return;
      applyResult(resultVars[k], r.value);
    }
  }

  root.SvgCurveSync = { resolveTargets, recompute };
})();
