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

- [ ] **🔥 Generic SVG Output Primitive** - First-class SVG drawing for engineering diagrams
  - Emit arbitrary line/circle/arrow/text/path (not just xy line plots)
  - Unblocks: structural free-body diagrams (supports, load arrows), beam deflection
    shapes, generalizing the hard-coded `graph_3d` brick
  - Foundation for interactive/animated visuals (drag a load → redraw)
  - **Impact:** Core differentiator vs Mathcad/BlockPad/EnerCalc — "calculations that move"
  - **See:** existing `graph()` is static D3; `graph_3d` is a placeholder brick

- [ ] **🔥 Numerical Integration / ODE Solver Primitive** - Out-of-the-box, no hand-rolled Simpson
  - Numeric definite integral (handles piecewise/trapezoidal loads where `math.intg` can't)
  - ODE/difference-equation stepper (e.g. draining-tank demo, dynamic systems)
  - Unblocks: beam deflection (∫∫ M/EI), education-grade dynamic visualizations
  - **Pairs with:** Higher-Order Functions above (pass the integrand/derivative as `f`)

- [ ] **Standard Library** - Comprehensive math library
  - Trigonometry: sin, cos, tan, arcsin, arccos, arctan
  - Logarithmic: log, ln, exp
  - Statistical: mean, std, variance
  - Linear algebra: det, inv, eigenvalues
  - Constants: π, e, φ

- [ ] **Mathematical Notation** - Expand symbol support
  - Greek letters: α, β, γ, λ, θ, etc.
  - Operators: ≠, ≤, ≥, ≈, ∞
  - Set theory: ∈, ∪, ∩, ⊂, ∅
  - Logic: ∀, ∃, ∧, ∨, ¬

- [ ] **Engineering Unit System** - Full dimensional analysis with automatic conversion
  - Unit support: meters, seconds, kilograms
  - Dimensional analysis validation
  - Physical constants library

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
