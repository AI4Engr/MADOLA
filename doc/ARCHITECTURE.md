# MADOLA Architecture

## Overview

MADOLA (Math Domain Language) is a domain-specific language designed for mathematical computations with comprehensive WASM support. This document provides a detailed overview of the system architecture, component interactions, and design decisions.

## System Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      MADOLA System                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐ │
│  │   Parser     │───▶│  AST Builder │───▶│  Evaluator   │ │
│  │ (Tree-sitter)│    │              │    │              │ │
│  └──────────────┘    └──────────────┘    └──────────────┘ │
│         │                    │                    │        │
│         │                    │                    │        │
│         ▼                    ▼                    ▼        │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐ │
│  │   Grammar    │    │     AST      │    │ Environment  │ │
│  │  (grammar.js)│    │   Nodes      │    │  (Variables) │ │
│  └──────────────┘    └──────────────┘    └──────────────┘ │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              Output Generators                       │  │
│  ├──────────────┬──────────────┬──────────────────────┤  │
│  │   Markdown   │     C++      │        WASM          │  │
│  │  Formatter   │  Generator   │      Generator       │  │
│  └──────────────┴──────────────┴──────────────────────┘  │
│                                                             │
│  ┌──────────────────────────────────────────────────────┐  │
│  │           Mathematical Libraries                     │  │
│  ├──────────────┬──────────────┬──────────────────────┤  │
│  │    Eigen     │  SymEngine   │   Boost.Multiprecision│ │
│  │ (Linear Alg) │  (Symbolic)  │  (Arbitrary Precision)│ │
│  └──────────────┴──────────────┴──────────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. Parser (Tree-sitter)

**Location**: `vendor/tree-sitter/`, `tree-sitter-madola/`

**Purpose**: Parses MADOLA source code into a concrete syntax tree (CST)

**Key Features**:
- Incremental parsing for better performance
- Error recovery for partial parsing
- Syntax highlighting support
- Language server protocol (LSP) integration

**Grammar Definition**: `tree-sitter-madola/grammar.js`

**Generated Files**:
- `tree-sitter-madola/src/parser.c` - Generated parser
- `tree-sitter-madola/src/tree_sitter/parser.h` - Parser header

**Workflow**:
```
grammar.js → tree-sitter generate → parser.c → compiled into MADOLA
```

### 2. AST Builder

**Location**: `src/core/ast/`

**Purpose**: Converts Tree-sitter CST into MADOLA Abstract Syntax Tree (AST)

**Key Files**:
- `ast.h` - AST node definitions
- `ast_builder_statements.cpp` - Statement AST building
- `ast_builder_expressions.cpp` - Expression AST building

**AST Node Types**:

**Statements**:
- `AssignmentStatement` - Variable assignments
- `PrintStatement` - Output statements
- `FunctionDeclaration` - Function definitions
- `ForLoop` - Loop constructs
- `IfStatement` - Conditional statements
- `ReturnStatement` - Function returns

**Expressions**:
- `Number` - Numeric literals
- `Identifier` - Variable references
- `BinaryExpression` - Binary operations (+, -, *, /, ^)
- `UnaryExpression` - Unary operations (-, +, !)
- `FunctionCall` - Function invocations
- `ArrayExpression` - Array/matrix literals
- `ComplexNumber` - Complex number literals

**Design Pattern**: Visitor pattern for AST traversal

### 3. Evaluator

**Location**: `src/core/evaluator/`

**Purpose**: Executes MADOLA programs by evaluating AST nodes

**Key Files**:
- `evaluator.h` - Evaluator interface
- `evaluator.cpp` - Core evaluation logic
- `evaluator_matrix.cpp` - Matrix operations
- `evaluator_symbolic.cpp` - Symbolic computation (optional)

**Value Types**:
```cpp
using Value = std::variant<
    double,                    // Numeric values
    std::string,               // String values
    ComplexValue,              // Complex numbers
    ArrayValue,                // Arrays/matrices
    FunctionValue,             // Function references
    GraphData,                 // Graph data
    TableData                  // Table data
>;
```

**Environment**:
- Variable storage (name → value mapping)
- Function definitions
- Scope management
- Built-in function registry

**Execution Model**:
1. Traverse AST depth-first
2. Evaluate expressions recursively
3. Update environment with assignments
4. Execute statements sequentially

### 4. Output Generators

#### Markdown Formatter

**Location**: `src/core/generator/markdown_formatter.cpp`

**Purpose**: Generates formatted Markdown/HTML output with LaTeX math

**Features**:
- LaTeX equation rendering
- Code block formatting
- Table generation
- Graph embedding
- MathJax integration

**Output Format**:
```html
<!DOCTYPE html>
<html>
<head>
    <script src="https://polyfill.io/v3/polyfill.min.js?features=es6"></script>
    <script id="MathJax-script" async src="https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js"></script>
</head>
<body>
    <!-- Generated content -->
</body>
</html>
```

#### C++ Generator

**Location**: `src/core/generator/cpp_generator.cpp`

**Purpose**: Generates optimized C++ code from MADOLA functions

**Features**:
- Function translation
- Type inference
- Optimization passes
- Standard library usage

**Output**: C++ source files in `web/gen_cpp/`

#### WASM Generator

**Location**: `src/core/generator/wasm_generator.cpp`

**Purpose**: Generates WebAssembly modules from MADOLA functions

**Features**:
- WASM binary generation
- JavaScript wrapper generation
- Memory management
- Function exports

**Output**: WASM modules in `web/trove/`

## Mathematical Libraries

### 1. Eigen (Linear Algebra)

**Location**: `external/eigen/` (git submodule)

**Purpose**: High-performance linear algebra operations

**Features Used**:
- Dense matrix operations
- Matrix decompositions (LU, QR, SVD)
- Eigenvalue computation
- Linear system solving
- SIMD optimizations

**Integration**:
```cpp
#include <Eigen/Dense>

Eigen::MatrixXd matrix(3, 3);
matrix << 1, 2, 3,
          4, 5, 6,
          7, 8, 9;

double det = matrix.determinant();
Eigen::MatrixXd inv = matrix.inverse();
```

**Matrix Methods in MADOLA**:
- `.det()` - Determinant
- `.inv()` - Inverse
- `.T()` - Transpose
- `.tr()` - Trace
- `.eigenvalues()` - Eigenvalues
- `.eigenvectors()` - Eigenvectors

### 2. SymEngine (Symbolic Computation)

**Location**: `external/symengine/` (git submodule)

**Purpose**: Symbolic mathematics and computer algebra

**Features Used**:
- Symbolic differentiation
- Expression simplification
- Polynomial manipulation
- Transcendental functions

**Integration**:
```cpp
#ifdef WITH_SYMENGINE
#include <symengine/basic.h>
#include <symengine/symbol.h>

using namespace SymEngine;

auto x = symbol("x");
auto expr = pow(x, integer(2));
auto derivative = expr->diff(x);  // 2*x
#endif
```

**Symbolic Functions in MADOLA**:
- `diff(expr, var)` - Differentiation
- `simplify(expr)` - Simplification (planned)
- `integrate(expr, var)` - Integration (planned)
- `solve(equation, var)` - Equation solving (planned)

**See Also**: [SymEngine Integration](SYMENGINE.md)

### 3. Boost.Multiprecision

**Location**: `external/boost/` (git submodule)

**Purpose**: Arbitrary-precision arithmetic for SymEngine

**Type**: Header-only library

**Integration**:
- Used by SymEngine for exact arithmetic
- No linking required
- Modular structure (`libs/*/include/`)

**Configuration**:
```cmake
set(INTEGER_CLASS "boostmp" CACHE STRING "Integer class for SymEngine" FORCE)
```

## Build System

### CMake Configuration

**Main File**: `CMakeLists.txt`

**Build Targets**:

1. **madola_core_obj** (Object Library)
   - Core MADOLA functionality
   - Compiled once, used by multiple targets
   - Includes AST, evaluator, generators

2. **madola_core** (Static Library)
   - Links object library
   - Used by executables

3. **madola** (Executable)
   - Main compiler/interpreter
   - Native execution
   - Output: `dist/madola.exe` or `dist/madola`

4. **madola-debug** (Executable)
   - Interactive debugger
   - Breakpoints and step-through
   - Output: `dist/madola-debug.exe` or `dist/madola-debug`

5. **madola_wasm** (WASM Module)
   - WebAssembly compilation
   - Browser execution
   - Output: `web/runtime/madola.js` + `madola.wasm`

**Build Options**:
```cmake
option(WITH_TREE_SITTER "Build with Tree-sitter support" ON)
option(WITH_SYMENGINE "Build with SymEngine symbolic computation support" ON)
option(WITH_DEBUG_FEATURES "Build with debug features" ON)
```

**See Also**: [CMake Build System](CMAKE_SYSTEM.md)

## Data Flow

### Compilation Pipeline

```
Source Code (.mda)
    │
    ▼
Tree-sitter Parser
    │
    ▼
Concrete Syntax Tree (CST)
    │
    ▼
AST Builder
    │
    ▼
Abstract Syntax Tree (AST)
    │
    ├──────────────┬──────────────┬──────────────┐
    ▼              ▼              ▼              ▼
Evaluator    Markdown Gen    C++ Gen      WASM Gen
    │              │              │              │
    ▼              ▼              ▼              ▼
Results        HTML/MD        C++ Code      WASM Module
```

### Evaluation Pipeline

```
AST Node
    │
    ▼
Evaluator.evaluate(node)
    │
    ├─ Statement? ──▶ Execute statement ──▶ Update environment
    │
    └─ Expression? ──▶ Evaluate expression ──▶ Return value
                            │
                            ├─ Number ──▶ Return numeric value
                            ├─ Identifier ──▶ Lookup in environment
                            ├─ BinaryOp ──▶ Evaluate left & right, apply operator
                            ├─ FunctionCall ──▶ Lookup function, evaluate args, call
                            └─ ArrayExpr ──▶ Evaluate elements, create array
```

## Memory Management

### Value Storage

**Value Type**: `std::variant<...>`
- Type-safe union
- No manual memory management
- Automatic cleanup

**Array Storage**: `std::vector<double>`
- Dynamic sizing
- Contiguous memory
- Cache-friendly

**Matrix Storage**: `Eigen::MatrixXd`
- Column-major storage
- SIMD-optimized
- Lazy evaluation

### Environment Storage

**Variables**: `std::unordered_map<std::string, Value>`
- Fast lookup (O(1) average)
- String keys
- Value semantics

**Functions**: `std::unordered_map<std::string, FunctionDeclaration>`
- Function definitions
- Closure support (planned)

## Error Handling

### Error Types

1. **Parse Errors**
   - Syntax errors
   - Reported by Tree-sitter
   - Line and column information

2. **Semantic Errors**
   - Type mismatches
   - Undefined variables
   - Invalid operations

3. **Runtime Errors**
   - Division by zero
   - Matrix dimension mismatches
   - Function call errors

### Error Reporting

**Debug Mode**:
- Full stack traces
- Source location mapping
- Variable state inspection

**Production Mode**:
- User-friendly error messages
- Minimal overhead

## Performance Considerations

### Optimization Strategies

1. **AST Optimization**
   - Constant folding
   - Dead code elimination
   - Common subexpression elimination (planned)

2. **Evaluation Optimization**
   - Lazy evaluation for matrices (Eigen)
   - SIMD vectorization (Eigen)
   - Expression templates (Eigen)

3. **Compilation Optimization**
   - Unity builds (optional)
   - Precompiled headers (optional)
   - Link-time optimization (LTO)

### Performance Characteristics

**Parsing**: O(n) where n = source code size
**AST Building**: O(n) where n = number of nodes
**Evaluation**: O(n) where n = number of operations
**Matrix Operations**: O(n³) for dense matrices (Eigen optimized)

## Extensibility

### Adding New Features

1. **New Syntax**
   - Update `grammar.js`
   - Regenerate parser
   - Add AST node type
   - Implement evaluation

2. **New Built-in Function**
   - Add to built-in function registry
   - Implement in evaluator
   - Document in language guide

3. **New Output Format**
   - Create new generator class
   - Implement generation logic
   - Add command-line option

### Plugin System (Planned)

- WASM module imports
- Custom function libraries
- External data sources

## Security Considerations

### Sandboxing

**WASM Builds**:
- Sandboxed execution
- No file system access
- Limited memory

**Native Builds**:
- No sandboxing by default
- File I/O planned (with restrictions)

### Input Validation

- Parser validates syntax
- Evaluator validates types
- Bounds checking for arrays

## Testing Strategy

### Unit Tests

**Location**: `tests/`

**Framework**: CTest

**Coverage**:
- Parser tests
- AST builder tests
- Evaluator tests
- Generator tests

### Regression Tests

**Location**: `regression/`

**Types**:
- Native regression tests
- WASM regression tests
- HTML output validation

**Workflow**:
```bash
./regression/run_regression.sh native
./regression/run_regression.sh wasm
```

### Integration Tests

- End-to-end compilation
- WASM execution in browser
- C++ code generation and compilation

## Deployment

### Native Deployment

**Output**: Single executable
- `dist/madola.exe` (Windows)
- `dist/madola` (Unix/Linux/macOS)

**Size**:
- Without SymEngine: ~5-10 MB
- With SymEngine: ~114 MB

**Dependencies**: None (static linking)

### WASM Deployment

**Output**:
- `web/runtime/madola.js` - JavaScript module
- `web/runtime/madola.wasm` - WebAssembly binary

**Size**:
- Without SymEngine: ~2-5 MB
- With SymEngine: ~50-80 MB

**Dependencies**: Modern web browser with WASM support

## Future Architecture

### Planned Enhancements

1. **JIT Compilation**
   - LLVM backend
   - Runtime optimization
   - Adaptive compilation

2. **Parallel Execution**
   - Multi-threaded evaluation
   - GPU acceleration (WebGPU)
   - Distributed computation

3. **Advanced Type System**
   - Static type checking
   - Type inference
   - Generic functions

4. **Module System**
   - Package management
   - Dependency resolution
   - Version control

## See Also

- [SymEngine Integration](SYMENGINE.md) - Symbolic computation details
- [Dependencies](DEPENDENCIES.md) - Complete dependency list
- [CMake Build System](CMAKE_SYSTEM.md) - Build system configuration
- [Build Configuration](BUILD_CONFIGURATION.md) - Build options
- [Language Guide](../LANGUAGE_GUIDE.md) - MADOLA language syntax
