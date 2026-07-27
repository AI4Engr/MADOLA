// AUTO-GENERATED COPY — do not edit here. Source of truth: shared/js/
// Regenerate with: node scripts/sync-shared.js
// mda-inputs.js — extract `// @input` annotations from MADOLA source.
//
// Shared between app/ (left-pane low-code widgets) and web/ (SVG slider injection).
// Pure string logic, no DOM access — works in window and worker contexts.
//
// Supported forms (annotation is the // @input comment on the same line as the assignment):
//   x := 10;                       // @input label="Temperature" min=0 max=100 step=0.1 unit="°C"
//   gain := 1.5;                   // @input slider min=0 max=10 step=0.01
//   A := [[1,0],[0,1]];            // @input matrix label="Transform"
//   v := [1, 2, 3];                // @input vector label="Position"
//
// Output: array of input descriptors with metadata + literal location for in-place editing.
//
// Public API: window.MdaInputs / self.MdaInputs
//   extractInputs(source)          -> [{ name, type, value, attrs, lineIndex, literalStart, literalEnd, raw }]
//   replaceLiteral(source, desc, v) -> source with the literal at [start,end) replaced
//   formatLiteral(value, type)     -> string
//   parseAttrs(rest)               -> attrs object (exposed for callers that need it)

(function () {
  'use strict';

  const root = (typeof window !== 'undefined') ? window : self;

  // Match the FIRST token after `// @input` to detect explicit type (matrix|vector|slider|number).
  // Then parse the rest as key=value pairs (value may be quoted, number, or bareword).
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

  // Detect type from the value literal if not explicitly given.
  function detectType(literal) {
    const t = literal.trim();
    if (t.startsWith('[[')) return 'matrix';
    if (t.startsWith('[')) return 'vector';
    return 'number';
  }

  // Parse a matrix literal like [[1,2],[3,4]] → [[1,2],[3,4]]
  // Returns null if not a well-formed numeric matrix.
  function parseMatrixLiteral(s) {
    try {
      // Replace MADOLA-friendly chars and parse as JSON-ish.
      const cleaned = s.replace(/;/g, ',').trim();
      const v = JSON.parse(cleaned);
      if (!Array.isArray(v)) return null;
      if (v.length === 0) return [[]];
      if (Array.isArray(v[0])) {
        if (!v.every((r) => Array.isArray(r) && r.every((x) => typeof x === 'number'))) return null;
        return v;
      }
      if (v.every((x) => typeof x === 'number')) return [v]; // treat as row vector
      return null;
    } catch {
      return null;
    }
  }

  function parseVectorLiteral(s) {
    const m = parseMatrixLiteral(s);
    if (!m) return null;
    if (m.length === 1) return m[0];
    return m.map((r) => r[0]);
  }

  /**
   * Extract input descriptors from source.
   * Returns: [{ name, type, value, attrs, lineIndex, literalStart, literalEnd, raw }]
   */
  function extractInputs(source) {
    const lines = source.split('\n');
    const inputs = [];
    let offset = 0;

    for (let i = 0; i < lines.length; i++) {
      const line = lines[i];
      const lineStart = offset;
      offset += line.length + 1; // +1 for newline

      // Find the @input comment
      const annIdx = line.search(/\/\/\s*@input\b/);
      if (annIdx < 0) continue;

      // Find assignment ":=" before the comment
      const codePart = line.slice(0, annIdx);
      const assignIdx = codePart.indexOf(':=');
      if (assignIdx < 0) continue;

      // Extract name (last identifier before :=)
      const nameMatch = codePart.slice(0, assignIdx).match(/([A-Za-z_]\w*)\s*$/);
      if (!nameMatch) continue;
      const name = nameMatch[1];

      // Find literal between := and either ; or end of code part
      const afterAssign = codePart.slice(assignIdx + 2);
      const semiIdx = afterAssign.lastIndexOf(';');
      const literalEndInAfter = semiIdx >= 0 ? semiIdx : afterAssign.length;
      const literalRaw = afterAssign.slice(0, literalEndInAfter);
      const literalTrimmed = literalRaw.trim();
      if (!literalTrimmed) continue;

      // Compute absolute offsets of the literal in the full source
      const literalStartInLine = assignIdx + 2 + literalRaw.indexOf(literalTrimmed);
      const literalStart = lineStart + literalStartInLine;
      const literalEnd = literalStart + literalTrimmed.length;

      // Parse annotation
      const annRest = line.slice(annIdx).replace(/^\/\/\s*@input\s*/, '');
      const firstToken = (annRest.match(/^\w+/) || [''])[0];
      const explicitType = ['matrix', 'vector', 'slider', 'number', 'select'].includes(firstToken) ? firstToken : null;
      const attrs = parseAttrs(annRest);

      // Parse select options: "Label1:val1,Label2:val2"
      if (attrs.options && typeof attrs.options === 'string') {
        attrs.options = attrs.options.split(',').map((pair) => {
          const [label, val] = pair.trim().split(':');
          return { label: label.trim(), value: parseFloat(val) };
        }).filter((o) => !Number.isNaN(o.value));
      }

      const type = explicitType || (attrs.matrix && 'matrix') || (attrs.vector && 'vector')
                                || (attrs.slider && 'slider') || (attrs.options && 'select')
                                || detectType(literalTrimmed);

      let value;
      if (type === 'matrix') value = parseMatrixLiteral(literalTrimmed);
      else if (type === 'vector') value = parseVectorLiteral(literalTrimmed);
      else value = parseFloat(literalTrimmed);

      if (value === null || (typeof value === 'number' && Number.isNaN(value))) continue;

      inputs.push({
        name,
        type,
        value,
        attrs,
        lineIndex: i,
        literalStart,
        literalEnd,
        raw: literalTrimmed,
      });
    }
    return inputs;
  }

  /**
   * Apply a new value to the source by replacing the literal at [literalStart, literalEnd).
   * Returns the new source string.
   */
  function replaceLiteral(source, descriptor, newValue) {
    const newLiteral = formatLiteral(newValue, descriptor.type);
    const before = source.slice(0, descriptor.literalStart);
    const after = source.slice(descriptor.literalEnd);
    return before + newLiteral + after;
  }

  function formatLiteral(value, type) {
    if (type === 'matrix') {
      return '[' + value.map((row) => '[' + row.map(formatNumber).join(', ') + ']').join(', ') + ']';
    }
    if (type === 'vector') {
      return '[' + value.map(formatNumber).join(', ') + ']';
    }
    return formatNumber(value);
  }

  function formatNumber(n) {
    if (Number.isInteger(n)) return String(n);
    // Trim trailing zeros, keep up to 6 significant decimals
    return parseFloat(n.toFixed(6)).toString();
  }

  root.MdaInputs = { extractInputs, replaceLiteral, formatLiteral, parseAttrs, formatNumber };
})();
