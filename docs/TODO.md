# MADOLA Improvement Roadmap

## 🎯 Quick Wins (1-2 weeks)
1. Add clang-format configuration - Standardize code formatting
2. Implement basic logging - Add debug/info/error logging levels
3. Create GitHub Actions workflow - Basic CI for builds and tests
4. Add more unit tests - Cover edge cases in evaluator and parser
5. ✅ **Optimize CMake build** (Completed 2025-11-12) - Reduce build times with better dependency management
  - Add matrix dimension validation
  - ✅ Enhance error messages with actual values
  - See [SAFETY_CHECKS.md](SAFETY_CHECKS.md) for details

- [ ] **Code Duplication Removal**
  - Extract complex number formatting logic to unified function
  - Extract matrix LaTeX rendering to template function
  - Consolidate repeated formatting code

- [ ] **Comprehensive Unit Tests** - Increase test coverage to 95%+
  - Empty arrays, division by zero, overflow
  - Array index out of bounds
  - Matrix dimension mismatches

- [ ] **Static Analysis Integration** - Add Clang Static Analyzer or SonarQube
- [ ] **Automated Code Formatting** - Integrate clang-format with pre-commit hooks
- [ ] **Memory Leak Detection** - Add Valgrind/AddressSanitizer to CI pipeline
- [ ] **API Documentation** - Generate Doxygen documentation for all public APIs

### Medium Priority
- [ ] **Type Safety** - Replace dynamic_cast with Visitor pattern for AST traversal
- [ ] **Memory Management** - Review lifetime management for imported AST nodes
- [ ] **Error Handling Standardization** - Replace exceptions with Result<T, Error> pattern
- [ ] **Logging Framework** - Add structured logging with configurable levels
- [ ] **Configuration System** - Add JSON/YAML configuration for compiler options
- [ ] **Plugin Architecture** - Design extensible plugin system for custom functions

### Low Priority
- [ ] Refactor large functions (>100 lines)
- [ ] Const correctness audit
- [ ] RAII compliance audit

## 🚀 Performance Optimizations

### High Priority
- [ ] **Expression Evaluation Cache** - Cache results of expensive mathematical operations
- [ ] **WASM Optimization** - Enable `-O3` and size optimizations for WASM builds
- [ ] **SIMD Math Operations** - Utilize SIMD instructions for vector/matrix operations

### Medium Priority
- [ ] **Memory Pool Allocator** - Replace frequent `std::unique_ptr` allocations
- [ ] **String Interning** - Intern frequently used identifiers to reduce memory usage
- [ ] **Compile-time Constant Folding** - Pre-compute constant expressions during parsing
- [ ] **Parallel Expression Evaluation** - Evaluate independent expressions in parallel

### Low Priority
- [ ] JIT Compilation - Add LLVM backend for native code generation
- [ ] Binary AST Format - Serialize/deserialize AST for faster loading
- [ ] Profile-Guided Optimization
- [ ] Memory-Mapped File I/O for large input files

## 🏗️ Build System & DevOps

### High Priority
- [ ] **GitHub Actions CI/CD** - Automated builds, tests, and releases
- [ ] **Cross-Platform Testing** - Test on Windows, Linux, macOS in CI
- [ ] **Dependency Management** - Use Conan or vcpkg for C++ dependencies
- [ ] **Docker Containerization** - Create development and production Docker images
- [ ] **Release Automation** - Automated versioning, changelog, and binary releases

### Medium Priority
- [x] **CMake Presets** (Completed 2025-11-12) - Add CMakePresets.json for common build configurations
- [x] **Build Cache** (Completed 2025-11-12) - Implement ccache or sccache for faster rebuilds
- [ ] Performance Benchmarking - Automated performance regression testing

### Low Priority
- [ ] Build reproducibility
- [ ] Multi-architecture support (ARM64, RISC-V)

## 📊 Language Features & Extensions

### High Priority

- [ ] **🔥🔝 Generic SVG Output Primitive (DO FIRST — app differentiator)** - First-class SVG drawing for engineering diagrams
  - Emit arbitrary line/circle/arrow/text/path (not just xy line plots)
  - Unblocks: structural free-body diagrams (supports, load arrows), beam deflection
    shapes, generalizing the hard-coded `graph_3d` brick
  - Foundation for interactive/animated visuals (drag a load → redraw)
  - **Impact:** Core differentiator vs Mathcad/BlockPad/EnerCalc — "calculations that move"
  - **Serves app/:** directly feeds the planned SVG tab; this is the near-term reason it leads
  - **See:** existing `graph()` is static D3; `graph_3d` is a placeholder brick

- [ ] **🔥 Higher-Order Functions** - Support functions as first-class values
  - Pass functions as parameters: `fn simpson(f, a, b, n)`
  - Store functions in variables: `g := f;`
  - Return functions from functions
  - Add `FunctionValue` type to value system
  - Update identifier evaluation for function references
  - **Estimated effort:** 2-3 days (basic), 5-7 days (with closures)
  - **Impact:** Enables functional programming patterns, cleaner numerical methods
  - **Current workaround:** Use `math.summation` for integration methods
  - **See:** Simpson's rule example in LANGUAGE_GUIDE.md line 806-830

- [ ] **🔥 Numerical Definite Integration Primitive** - Out-of-the-box, no hand-rolled Simpson
  - Numeric definite integral (handles piecewise/trapezoidal loads where `math.intg` can't)
  - Unblocks: beam deflection (∫∫ M/EI), engineering integration without boilerplate
  - **Pairs with:** Higher-Order Functions above (pass the integrand as `f`)
  - **Note:** ODE/PDE stepper split out and deferred → see Low Priority

- [x] **Standard Library** - Comprehensive math library (Completed 2026-07-15)
  - [x] Trigonometry: sin, cos, tan (pre-existing) + arcsin, arccos, arctan (new, with domain
        validation on asin/acos)
  - [x] Logarithmic: `math.ln` (natural log), `math.log(x)` (base-10), `math.log(x, base)`
        (new); `math.exp` (pre-existing)
  - [x] Statistical: `math.mean`, `math.variance`, `math.std` (new, array-only, population
        convention — divide by N not N-1)
  - [x] Linear algebra: `A.det()`, `A.inv()`, `A.eigenvalues()` (already implemented in
        `evaluator_matrix.cpp` via Eigen — no work needed)
  - [x] Constants: `math.pi()`, `math.e()`, `math.phi()` (new — namespaced as no-arg `math.`
        calls rather than bare identifiers, so they never shadow user variables)
  - **See:** `evaluator_functions.cpp` (`evaluateMethodCall`, math.* dispatch), LaTeX output
    wired in both `formatExpressionAsMath` and `formatExpressionWithValuesAsMath` in
    `html_formatter_methods.cpp`. Docs updated in `LANGUAGE_GUIDE.md`.

- [ ] **Mathematical Notation** - Expand symbol support
  - Greek letters: α, β, γ, λ, θ, etc.
  - Operators: ≠, ≤, ≥, ≈, ∞
  - Set theory: ∈, ∪, ∩, ⊂, ∅
  - Logic: ∀, ∃, ∧, ∨, ¬

- [ ] **Engineering Unit System Rewrite** - Real dimensional algebra (current system silently
      produces wrong numbers for mixed-unit expressions with exponents)
  - **Root cause:** `UnitValue`/`UnitDefinition` (`unit_system.h`) store units as a formatted
    *string* (e.g. `"kip/in^2"`), not a dimension vector. `operator*`/`operator/`/`operator^`
    in `unit_system.cpp` just concatenate/parse those strings and call `simplifyUnit()` (regex/
    string-level composite expansion only, e.g. `ksi → kip/in^2`) — there is no "convert both
    operands to a common base unit before combining" step, and no cross-unit cancellation.
  - **Confirmed broken:** `example.mda`'s beam deflection formula `δ = 5wL⁴/(384EI)` with
    `L in ft`, `w in kip/in`, `E in ksi`, `I in in^4` — `operator^` computes `L^4` using the
    raw `4.75` (ft) value and just appends `^4` to the unit string (`509.066 ft^4`), without
    scaling by the ft→base conversion factor raised to the 4th power. The final division never
    cancels `ft^4` against `in`/`kip` across numerator and denominator, producing `0` instead
    of the correct ≈0.021 in.
  - **Scope for a real fix:**
    - Replace the unit string with a dimension-vector representation (length/mass/time/force
      exponents) + numeric factor to SI base, so `*`/`/`/`^` become vector add/subtract/scale
      instead of string ops
    - `operator^` must scale the numeric value by `conversionFactor^exponent`, not just relabel
    - Need a display-unit heuristic (user writes `ft`, expects `ft^4` in output, not auto-jumping
      to `m^4`) — nontrivial UX design, not just math
    - `toLatex()`/`toString()` simplification logic is also string-based and would need a
      parallel rewrite
    - Must re-verify all existing unit regression fixtures (`kip-in`, `ksi` composite expansion,
      etc.) still pass — this is a module-level rewrite, not a patch
  - **Impact:** blocks correct engineering formulas that mix base units with derived/exponent
    units (very common in real calcs — beam deflection, stress, moment of inertia). Low risk
    today only if usage stays within same-unit-family expressions without exponents.
  - **Additional confirmed gaps (found 2026-07-22, building the `beam_deflection.mda` web
    example):**
    1. A `UnitValue` variable (e.g. `P := 12 kip;`) cannot be passed into a user-defined
       `fn`/`f(x) := expr;` that does arithmetic on it — fails with `Unary operations only
       supported on numbers and arrays`. There is no "unwrap to base-unit number for
       computation, reattach unit to the result" path, so any callable formula must currently
       be written in plain numbers with units only documented in comments (see
       `web/examples/beam_deflection.mda` for the workaround). Any real fix should make
       unit-valued arguments usable inside function bodies, not just at top-level expressions.
    2. The interactive SVG slider (`svg_eval_curve`/`resampleSvgCurve`) rebinds the dragged
       `@input` variable via a raw `double` (`env.define(paramName, paramValue)` in
       `evaluator_functions.cpp`) — this only works if that variable is a plain number. A
       unit-valued slider target has not been tested and is expected to hit the same problem
       as (1) once it flows into the curve's expression.
  - Physical constants library (still open, independent of the rewrite above)

- [x] **Unit Literal Grammar: allow expressions before a unit, not just a bare number** — DONE
      (natural engineering notation like `100/(12*1000) kip/in` now parses and evaluates)
  - **Fix:** `unit_expression` in `tree-sitter-madola/grammar.js` now takes a
    `multiplicative_expression` (not just `$.number`) before the unit, and was moved out of
    `primary_expression` into `multiplicative_expression` itself (so the unit binds the *whole*
    preceding product/quotient, not just its innermost primary). The old hand-resolved
    `conflicts: [$.unit_expression]` entry is gone — the wider rule generates cleanly with no
    unresolved conflicts against `function_call`/`unary_expression`/juxtaposition.
  - `ast_builder_expressions.cpp::buildUnitExpression` now builds the value via the general
    `buildExpression` dispatcher instead of assuming a bare `buildNumber`.
  - Verified: `100/(12*1000) kip/in` → `0.008 kip/in` (matches the old `* kip/in` workaround's
    result exactly); `100 kip`, `5 m^2`, `(3+2) kip`, `2*3 kip` all still parse/evaluate
    correctly. Full native + WASM regression suites show no new failures (remaining failures —
    `equ_layout.html` on native, and CSS/formatting drift in `expected_wasm/` — are pre-existing
    baseline staleness unrelated to this change).

- [~] **🔥 Parse Error Diagnostics** - Replace bare "Parse error detected in source"
  - **Root cause:** `ast_builder_statements.cpp:58` only calls `ts_node_has_error(root)`
    (boolean) then throws a generic string — discards all location info
  - [x] Walk tree to find first error node (`ts_node_is_error` / `ts_node_is_missing`)
  - [x] Report 1-based line/column via existing `getNodeStartPosition()` → `SourceLocation`
  - [x] Include offending source snippet via existing `getNodeText(errorNode)`
  - [x] Distinguish ERROR ("unexpected '...'") vs MISSING ("missing '}'")
  - [ ] Return structured error `{line, column, type, snippet}` from `wasm_interface.cpp`
        instead of an exception string (so the App can highlight the line)
  - [ ] (stretch) Collect ALL error nodes, not just the first
  - **Done (2026-06-16):** `describeParseError()` in `ast_builder_statements.cpp` now emits
    e.g. `Line 3:16 - syntax error: missing '}'`. Message passes through to the WASM
    `error` field unchanged, so the App shows it with no front-end change.
  - **Remaining:** structured JSON + multi-error collection (need error-field reshape
    + front-end parsing).
  - **Impact:** Turns the opaque UI error into a locatable message — baseline for a
    commercial editor

- [ ] **Error Recovery** - Better error messages with suggestions
- [ ] **Debugging Support** - Breakpoints, variable inspection, call stack

### Medium Priority
- [ ] **Type System** - Optional static typing with type inference
- [ ] **Module System Enhancement** - Import/export system improvements
- [ ] **Pattern Matching** - Advanced pattern matching for data structures
- [ ] **Symbolic Math** - Integration with symbolic computation libraries
- [ ] **Output Formats**
  - LaTeX export for academic papers
  - MathML for web standards
  - SVG/PNG for graphics (see Generic SVG Output Primitive under High Priority)
  - Typst → PDF for signable engineering reports (headers/footers/page numbers)

### Low Priority
- [ ] **ODE / PDE / difference-equation Solver Primitive** (split from the integration item, deferred)
  - ODE/difference-equation stepper (e.g. draining-tank demo, dynamic systems), PDE solvers
  - **Why deferred:** heavy numerical work; `app/` doesn't need it for near-term commercial value.
    Revisit once the SVG primitive and definite integration are shipped and a real user asks for it.
- [ ] Macro System - Compile-time code generation
- [ ] Async/Await - Asynchronous computation support
- [ ] GPU Computing - CUDA/OpenCL integration
- [ ] Multi-language interop (Python/JavaScript)

## 🌐 Web Integration & Features

### High Priority
- [ ] **Progressive Web App** - Add PWA manifest, service worker, offline support
- [ ] **Syntax Highlighting** - Enhanced Monaco Editor integration with Tree-sitter
- [ ] **Error Visualization** - Interactive error highlighting and suggestions
- [ ] **Document Mode (WYSIWYG)** - Rich text editor alongside code mode for document-driven workflow
- [ ] **Interactive Code Cells** - Cell-based execution with live preview and re-evaluation
- [ ] **Mathematical Typesetting UI** - Visual equation editor with LaTeX preview

### Medium Priority
- [ ] **Code Completion** - Intelligent autocomplete for variables and functions
- [ ] **Debugger Integration** - Step-through debugging in web interface
- [ ] **Export Formats** - PDF, LaTeX, Jupyter Notebook export options
- [ ] **Theme System** - Dark/light themes with customizable syntax highlighting
- [ ] **Mobile Responsiveness** - Optimize web interface for mobile devices
- [ ] **Engineering Report Templates** - Pre-built templates for technical documentation

### Low Priority
- [ ] Real-time collaboration - WebSocket-based collaborative editing
- [ ] WebGL Visualization - 3D mathematical function plotting
- [ ] Accessibility - WCAG 2.1 AA compliance

## 🔒 Security & Reliability

### High Priority
- [ ] **Input Validation** - Comprehensive input sanitization and validation
- [ ] **Sandboxing** - Secure execution environment for untrusted code
- [ ] **Memory Safety** - Bounds checking and buffer overflow protection
- [ ] **Fuzzing Integration** - Automated fuzz testing with AFL or libFuzzer

### Medium Priority
- [ ] Resource Limits - CPU time, memory, and recursion depth limits
- [ ] Code Signing - Sign binaries and verify integrity
- [ ] Vulnerability Scanning - Automated dependency vulnerability checks

### Low Priority
- [ ] Security Audit - Third-party security assessment
- [ ] Formal Verification - Mathematical proofs of critical algorithms

## 📈 Monitoring & Analytics

### High Priority
- [ ] **Performance Metrics** - Execution time, memory usage, compilation speed
- [ ] **Error Tracking** - Centralized error reporting and analysis
- [ ] **Crash Reporting** - Automated crash dump collection and analysis

### Medium Priority
- [ ] Usage Analytics - Anonymous usage statistics and feature adoption
- [ ] User Feedback - In-app feedback collection and analysis
- [ ] Performance Profiling - Detailed performance profiling tools

---

## 🎓 Vision: Literate Programming Platform

**Goal:** Cross-disciplinary platform combining document-driven authoring with powerful executability.

### Core Principles
1. **Document-first workflow** - WYSIWYG editor with mathematical typesetting and Markdown/LaTeX
2. **Strong executability** - Interactive code cells for engineering calculations and numerical computation
3. **Multi-domain support** - Engineering (with units), symbolic math, general programming, data science
4. **Dual output generation** (Literate Programming tangle/weave):
   - Code extraction - Compilable programs or scripts
   - Documentation export - PDF/HTML/engineering reports
5. **Collaboration-ready** - Git-compatible format for team-based "runnable engineering reports"

### Status
- ✅ **Already have:** Code generation (`@gen_cpp`, `@gen_addon`), markdown export, WASM execution
- 🔨 **Need to add:** WYSIWYG document mode, cell-based execution, richer unit system
- 🎯 **Future:** Multi-language interop, collaborative editing, template library

---

## 📋 Current MADOLA Strengths

- Unique niche: Executable mathematical notation with markdown output
- Solid architecture: Clean AST → Evaluator → Formatter pipeline
- Modern tooling: CMake, WASM support, comprehensive testing
- Rich features: Functions, matrices, summation, piecewise, for loops

---

*This roadmap is a living document. Priorities may shift based on user feedback, performance analysis, and project goals.*
