# MADOLA (Math Domain Language)

MADOLA is a domain-specific language designed for mathematical computations with comprehensive WASM support, enabling both native and web-based execution.

**Code-first mathematical programming with automatic documentation generation.** Write algorithms in MADOLA's intuitive syntax, execute them natively or via WASM, and automatically generate beautifully formatted LaTeX documents. Unlike document-centric tools, MADOLA is a true compiler that transforms mathematical code into multiple outputs: native executables, WASM modules, C++ source, and publication-ready documentation.

## 🚀 Features

- **Mathematical Focus**: Purpose-built for mathematical expressions, functions, and algorithms
- **WASM Support**: Compile to WebAssembly for high-performance web applications
- **Native Execution**: Fast native builds for development and testing
- **Tree-sitter Integration**: Advanced parsing with syntax highlighting support
- **Cross-Platform**: Windows, Linux, and macOS support via CMake
- **Function Libraries**: Modular WASM function imports (calcPi, mathematical operations)
- **Multiple Output Formats**: Markdown formatting and evaluation results


## 🏗️ Architecture

### Core Components
- **AST Nodes** (`src/core/ast/ast.h`) - Program, Statement, Expression definitions
- **Evaluator** (`src/core/generator/evaluator.cpp`) - Runtime environment and execution engine
- **Generators** - Multiple output formats (Markdown, C++, WASM)
- **Tree-sitter Parser** (`tree-sitter-madola/`) - Advanced syntax parsing
- **CSS Embedding** (`scripts/generate_css_header.js`) - Auto-generates C++ header from CSS for HTML output

### Build Targets
- **Native** → `build/madola.exe` → `dist/madola.exe` (development, debugging, testing)
- **WASM** → `web/runtime/madola.js` + `web/runtime/madola.wasm` (direct build to deployment location)

## 🛠️ Development Setup

### Prerequisites

**All Platforms:**
- **CMake** 3.16+
- **C++17** compatible compiler (GCC, Clang)
- **Node.js** (for Tree-sitter and web development)
- **Emscripten** (for WASM builds)

**Windows-Specific:**
- **GCC/MinGW** via MSYS2 (recommended) or MinGW-w64
- **Ninja** (build tool, required for Windows with GCC/MSYS2)

#### Installing Build Tools on Windows

**Option 1: MSYS2 (Recommended)**
```bash
# Download and install MSYS2 from https://www.msys2.org/

# In MSYS2 terminal, install required packages:
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja

# Add to PATH: C:\msys64\mingw64\bin
```

**Option 2: Standalone MinGW-w64**
```bash
# Download from: https://github.com/niXman/mingw-builds-binaries/releases
# Extract and add to PATH
```

**Installing Emscripten on Windows:**
```bash
# Clone Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# Install and activate latest version
emsdk install latest
emsdk activate latest

# Add to PATH (run in each session or add to system PATH)
emsdk_env.bat
```

**Installing Node.js on Windows:**
- Download installer from https://nodejs.org/
- Or use package manager: `winget install OpenJS.NodeJS`

This is required before running any build commands as MADOLA uses Tree-sitter for parsing.

### First-Time Setup

Before building, complete these initial setup steps:

```bash
# 1. Install Tree-sitter CLI globally (required for parser generation)
npm install -g tree-sitter-cli

# 2. Install project dependencies
npm install

# 3. Initialize Tree-sitter submodule (Git submodule)
git submodule update --init --recursive
```

### Quick Start

**Windows:**

Make sure to add both the **`mingw64`** path and the **Emscripten** path to your terminal **before** you start.

MODOLA uses:

* **CMake**, **GCC**, and **Ninja** to build `MADOLA.exe`
* **Emscripten** (`emcc`) to build `MADOLA.wasm`

It’s best to update your PATH **first**, then launch your IDE (e.g., `code .` or `cursor .`).

*PowerShell*

```powershell
$env:PATH += ";C:\msys64\mingw64\bin"
$env:PATH += ";C:\emsdk\upstream\emscripten"
```

*CMD*

```cmd
set PATH=%PATH%;C:\msys64\mingw64\bin
set PATH=%PATH%;C:\emsdk\upstream\emscripten
```


```bash
# Initialize and build
dev.bat init-submodules
dev.bat generate-grammar
dev.bat configure
dev.bat build

# Run example
dev.bat run

# Regression tests
dev.bat regression native    # Run native regression tests
dev.bat regression wasm      # Run WASM regression tests
dev.bat regression update    # Update native test baselines
```

**Unix/Linux:**
```bash
# Initialize and build
./dev.sh init-submodules
./dev.sh generate-grammar
./dev.sh configure
./dev.sh build

# Run example
./dev.sh run

# Regression tests
./dev.sh regression native    # Run native regression tests
./dev.sh regression wasm      # Run WASM regression tests
./dev.sh regression update    # Update native test baselines
```

### NPM Scripts
```bash
npm run build:debug          # Configure + build debug
npm run build:release        # Configure + build release
npm run build:wasm           # Build WASM with Emscripten
npm run build:tree-sitter    # Generate grammar + build with Tree-sitter
npm run test                 # Run CTest suite
npm run test:regression      # Run comprehensive regression tests (Unix/Git Bash)
npm run test:regression:win  # Run comprehensive regression tests (Windows)
```

## 🧪 Testing

### Test Suite
- **Unit Tests**: Core functionality testing via CTest (7/7 passing)
- **Sample Files**: Various `.mda` test cases in `tests/`
- **Regression Tests**: Native (uses `dist/`) and WASM (uses `web/runtime/`) validation
- **Web Integration**: HTML demos for WASM functionality

### Running Tests

**Unit Tests (CTest):**
```bash
npm run test                    # Run unit test suite
dev.bat test                    # Windows
./dev.sh test                   # Unix/Linux
```

**Regression Tests:**

*Unix/Linux/macOS/Git Bash:*
```bash
# Run tests
./regression/run_regression.sh              # Run native tests
./regression/run_regression.sh native       # Run native tests (explicit)
./regression/run_regression.sh wasm         # Run WASM tests

# Update baselines
./regression/run_regression.sh update       # Run native tests and update baselines
./regression/run_regression.sh native update  # Run native tests and update baselines
./regression/run_regression.sh wasm update    # Run WASM tests and update baselines
```

*Windows Command Prompt:*
```cmd
REM Run tests
regression\run_regression.bat              REM Run native tests
regression\run_regression.bat native       REM Run native tests (explicit)
regression\run_regression.bat wasm         REM Run WASM tests

REM Update baselines
regression\run_regression.bat update       REM Run native tests and update baselines
regression\run_regression.bat native update  REM Run native tests and update baselines
regression\run_regression.bat wasm update    REM Run WASM tests and update baselines
```

*NPM Scripts:*
```bash
npm run test:regression         # Full regression suite (native + WASM)
npm run test:regression:native  # Native tests only
npm run test:regression:wasm    # WASM tests only
```

### Regression Test Structure
```
regression/
├── fixtures/           # Test input files (.mda)
├── expected/           # Expected outputs (native)
│   ├── evaluation/    # Expected evaluation results (.txt)
│   └── html/          # Expected HTML outputs (.html)
├── expected_wasm/      # Expected outputs (WASM)
├── results/            # Actual test outputs
└── diff/               # Diff files (when tests fail)
```

When tests fail, you can:
1. Review differences in `regression/diff/`
2. Update baselines if changes are intentional using the `update` command
3. Fix code if output is incorrect

## 🌐 Web Integration

### Production Deployment

The web application uses a unified structure with all assets built directly to their deployment locations:

**Structure:**
```
web/
├── runtime/               # WASM runtime (built directly here)
│   ├── madola.js         # WASM JavaScript module
│   └── madola.wasm       # WASM binary
├── trove/                # Function library modules (generated here)
│   ├── manifest.json     # Module registry for web app
│   └── calcPi/
│       ├── calcPi.js     # WASM wrapper
│       └── calcPi.wasm   # WASM module
└── gen_cpp/              # Generated C++ files (output here)
    ├── calcPi.cpp
    └── ...
```

**Zero-copy deployment** - all builds output directly to final locations.

**📖 Documentation:**
- **Complete Guide**: [WASM Complete Guide](docs/WASM_COMPLETE_GUIDE.md) - Includes quick start, usage, testing, and troubleshooting

### Complete Web Directory Structure
```
web/
├── js/
│   ├── app.js              # Main web application
│   └── madola-browser.js   # Browser integration
├── css/
│   └── styles.css          # Web UI styles
├── runtime/                # WASM runtime (built automatically)
│   ├── madola.js          # WASM JavaScript module
│   └── madola.wasm        # WASM binary
├── index.html              # Main web interface
└── trove/                  # Function library modules
    └── example/
        ├── calcPi.js       # WASM wrapper
        ├── calcPi.wasm     # WASM module
        └── ...
```

### Development Server

Start the web application with automatic WASM build:

**Unix/Linux/macOS:**
```bash
./dev.sh serve              # Builds WASM (if needed) and starts server at http://localhost:8080
```

**Windows:**
```cmd
dev.bat serve               # Builds WASM (if needed) and starts server at http://localhost:8080
```

The server automatically:
- Builds WASM to `web/runtime/` if not present
- Serves the web application at http://localhost:8080
- Provides API endpoints for C++ file management and WASM compilation

## 📁 Project Structure

```
madola/
├── src/                    # Source code
│   ├── core/              # Core language implementation
│   │   ├── ast/           # AST definitions and builders
│   │   └── generator/     # Output generators (Markdown, C++, WASM)
│   ├── main.cpp           # Native executable entry
│   └── wasm_interface.cpp # WASM interface
├── tests/                 # Unit tests and sample files
├── regression/            # Regression test suite
├── web/                   # Web application and generated files
│   ├── gen_cpp/          # Generated C++ files
│   ├── runtime/          # WASM runtime files
│   └── trove/            # Function library modules
├── tree-sitter-madola/    # Grammar definition
├── vendor/                # Third-party dependencies
├── CMakeLists.txt         # Build configuration
└── dev.sh / dev.bat       # Development scripts
```

## 🔧 Build System

- **CMake**: Cross-platform build system with MSVC/GCC support
- **CTest**: Integrated testing framework
- **Ninja/MSBuild**: Fast parallel builds
- **Emscripten**: WASM compilation toolchain

## Tree-sitter Submodule Management

### Why a Submodule?

Tree-sitter is included as a Git submodule for proper dependency management. This ensures:
- Version consistency across builds
- Clean separation of external dependencies
- Easy updates and maintenance

### Initial Setup

When cloning the repository for the first time:

```bash
git clone <repository-url>
cd madola
./dev.sh init-submodules  # or dev.bat init-submodules on Windows
./dev.sh generate-grammar # Generate parser from grammar.js
```

### Updating the Submodule

To update Tree-sitter to a newer version:

```bash
cd vendor/tree-sitter
git pull origin master
cd ../..
git add vendor/tree-sitter
git commit -m "Update Tree-sitter submodule"
```
## Troubleshooting

**Issue:** `dev.bat build` does nothing or fails silently
**Solution:** Run from **Windows Command Prompt (cmd.exe)**, NOT Git Bash. Git Bash has path issues with MinGW-w64.

**Issue:** `tree-sitter` directory is empty
**Solution:** Initialize Git submodule:
```bash
git submodule update --init --recursive
```

**Issue:** Parser generation fails
**Solution:** Install Tree-sitter CLI globally:
```bash
npm install -g tree-sitter-cli
```

**Issue:** Compiler errors about missing `stdbool.h` or `<memory>`
**Solution (Windows):** Install MinGW-w64 toolchain:
```bash
pacman -S mingw-w64-x86_64-toolchain
```

**Issue:** When to regenerate Tree-sitter parser?
**Answer:** The build scripts (`dev.bat build` / `./dev.sh build`) automatically regenerate the parser when `grammar.js` is newer than `parser.c`. Manual regeneration: `dev.bat generate-grammar`

## 📊 Current Status

- ✅ **Complete Core Implementation**: All language features working
- ✅ **100% Test Coverage**: 7/7 unit tests passing
- ✅ **Cross-Platform Builds**: Windows, Linux, macOS support
- ✅ **WASM Production Ready**: Direct build to deployment location (`web/runtime/`)
- ✅ **Regression Testing**: Native (`dist/`) + WASM (`web/runtime/`) validation
- ✅ **Tree-sitter Integration**: Advanced parsing and syntax support
- ✅ **Zero-Copy Deployment**: All generated files output to final locations
- 🎯 **Production Ready**: Optimized build pipeline with unified web structure

## 📝 Example Usage

### Running MADOLA Programs

**Execute and view results:**
```bash
# Run and display output to console
.\dist\madola.exe .\example.mda

# Generate HTML output
.\dist\madola.exe .\example.mda --html > out.html

# Unix/Linux
./dist/madola ./example.mda
./dist/madola ./example.mda --html > out.html
```

### Basic Computation
```madola
// Floating-Point Arithmetic Demo
from example import calcPi as calc;

x1 := calc(1000);
print(x1);
```

### Complex Number Arithmetic
```madola
// Complex Number Operations
a := 1+2i;
b := 2+3i;

sum := a + b;          // 3 + 5i
diff := a - b;         // -1 - i
product := a * b;      // -4 + 7i
quotient := a / b;     // 0.615385 + 0.0769231i

print(sum);
print(product);
```

### Matrix Operations
```madola
// Matrix Methods
A := [2, 1, 0;
      1, 3, 1;
      0, 1, 2];

det := A.det();        // Determinant: 8
inv := A.inv();        // Inverse matrix
trace := A.tr();       // Trace: 7
transpose := A.T();    // Transpose
eigenvals := A.eigenvalues();   // Eigenvalues
eigenvecs := A.eigenvectors();  // Eigenvectors

print(det);
print(inv);
```

### Symbolic Derivatives
```madola
// Symbolic Differentiation
// Pass expressions directly to math.diff()
df := math.diff(x^2 + 2*x + 1, x);     // Result: 2.0 + 2.0*x**1.0

dg := math.diff(sin(x) * cos(x), x);   // Result: -sin(x)**2 + cos(x)**2

dh := math.diff(exp(x^2), x);          // Result: 2.0*x**1.0*exp(x**2.0)

print(df);
print(dg);
print(dh);
```

### Variable Substitution
```madola
// Pipe Substitution Operator (|)
// Syntax: expression | var1:value1, var2:value2, ...

// Basic substitution with multiple variables
@eval
result := x^2 + 2*y | x:3, y:4;     // Evaluates to: 9 + 8 = 17

// Single variable substitution
@eval
value := x^3 + 2*x^2 | x:2;         // Evaluates to: 8 + 8 = 16

// Combining derivatives and substitution
@eval
derivative := math.diff(x^3 + 2*x^2, x);  // Result: 4.0*x + 3.0*x**2.0
@eval
x_2 := derivative | x:2;              // Evaluates to: 12 + 8 = 20

print(result);
print(x_2);
```



