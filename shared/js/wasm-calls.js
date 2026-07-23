// wasm-calls.js — the single definition of every MADOLA WASM ccall.
//
// Shared between web/ and app/ (and the web worker). Each function takes an
// already-initialized Emscripten module as its first argument and encapsulates the
// ptr -> UTF8ToString -> free_result idiom. Loading/instantiating the module is left
// to each caller, because that step is environment-specific (a <script> tag in the
// browser vs importScripts in a worker).
//
// Works in both window and worker contexts (no DOM access here).
//
// Public API: window.MadolaCalls / self.MadolaCalls
//   evaluate(module, code)            -> raw JSON string (or null)
//   format(module, code, mode=1)      -> formatted source string (or null)
//   formatHtml(module, code, mode=1)  -> HTML string ('' on null ptr)
//   evalSvgCurve(module, source, curveId, paramName, paramValue, resultVar='')
//                                     -> { success, curveId, d, [resultVar, resultValue] }
//   evalNamedValue(module, source, varName)
//                                     -> { success, name, value } | { success:false, error }

(function () {
  'use strict';

  const root = (typeof window !== 'undefined') ? window : self;

  // Run a ccall that returns a char* pointer, read it as a UTF-8 string, always free it.
  // Returns the string, or null if the call returned a null pointer.
  function callString(module, fn, argTypes, args) {
    const ptr = module.ccall(fn, 'number', argTypes, args);
    if (!ptr) return null;
    try {
      return module.UTF8ToString(ptr);
    } finally {
      module.ccall('free_result', null, ['number'], [ptr]);
    }
  }

  function evaluate(module, code) {
    if (!module) throw new Error('Module not loaded');
    return callString(module, 'evaluate_madola', ['string'], [code]);
  }

  function format(module, code, mode = 1) {
    if (!module) throw new Error('Module not loaded');
    return callString(module, 'format_madola', ['string', 'number'], [code, mode]);
  }

  function formatHtml(module, code, mode = 1) {
    if (!module) throw new Error('Module not loaded');
    const result = callString(module, 'format_madola_html', ['string', 'number'], [code, mode]);
    // Callers historically treat a null pointer as empty output, not an error.
    return result === null ? '' : result;
  }

  function evalSvgCurve(module, source, curveId, paramName, paramValue, resultVar = '') {
    if (!module) throw new Error('Module not loaded');
    const json = callString(
      module,
      'svg_eval_curve',
      ['string', 'string', 'string', 'number', 'string'],
      [source, curveId, paramName, paramValue, resultVar || '']
    );
    if (json === null) return { success: false, error: 'svg_eval_curve returned null' };
    return JSON.parse(json);
  }

  function evalNamedValue(module, source, varName) {
    if (!module) throw new Error('Module not loaded');
    const json = callString(
      module,
      'eval_named_value',
      ['string', 'string'],
      [source, varName]
    );
    if (json === null) return { success: false, error: 'eval_named_value returned null' };
    return JSON.parse(json);
  }

  root.MadolaCalls = { evaluate, format, formatHtml, evalSvgCurve, evalNamedValue };
})();
