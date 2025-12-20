# MADOLA Dependencies

## Overview

This document provides a comprehensive overview of all dependencies used in the MADOLA project, including their purpose, version requirements, and installation instructions.

## Dependency Categories

### Core Dependencies (Required)

These dependencies are required for basic MADOLA functionality:

1. **CMake** (3.16+)
2. **C++17 Compiler** (GCC 7+, Clang 5+, MSVC 2017+)
3. **Tree-sitter** (git submodule)
4. **Eigen** (git submodule)

### Optional Dependencies

These dependencies enable additional features:

1. **SymEngine** (git submodule) - Symbolic computation
2. **Boost** (git submodule) - Multiprecision arithmetic for SymEngine
3. **Emscripten** - WASM compilation
4. **Node.js** - Web development and Tree-sitter CLI

## Detailed Dependency Information

### 1. CMake

**Version**: 3.16 or higher
**Purpose**: Cross-platform build system
**License**: BSD 3-Clause

**Installation:**

- **Windows**: Download from [cmake.org](https://cmake.org/download/) or use package manager:
  ```cmd
  winget install Kitware.CMake
  ```

- **Linux**:
  ```bash
  # Ubuntu/Debian
  sudo apt-get install cmake

  # Fedora
  sudo dnf install cmake
  ```

- **macOS**:
  ```bash
  brew install cmake
  ```

**Usage in MADOLA:**
- Configures build system for native and WASM targets
- Manages compiler flags and optimization settings
- Handles dependency linking and include paths

### 2. C++17 Compiler

**Minimum Versions**:
- GCC 7.0+
- Clang 5.0+
- MSVC 2017+ (Visual Studio 15.0)

**Purpose**: Compiles MADOLA source code
**License**: Various (GPL for GCC, Apache for Clang, Proprietary for MSVC)

**Installation:**

- **Windows (MSYS2/MinGW-w64)**:
  ```bash
  # Install MSYS2 from https://www.msys2.org/
  # Then in MSYS2 terminal:
  pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-toolchain
  ```

- **Linux**:
  ```bash
  # Ubuntu/Debian
  sudo apt-get install build-essential

  # Fedora
  sudo dnf install gcc-c++
  ```

- **macOS**:
  ```bash
  xcode-select --install
  ```

**C++17 Features Used:**
- `std::variant` - Type-safe unions for Value types
- `std::optional` - Optional return values
- `std::string_view` - Efficient string handling
- Structured bindings - Cleaner code
- `if constexpr` - Compile-time conditionals

### 3. Tree-sitter

**Version**: Latest from main branch
**Type**: Git submodule
**Location**: `vendor/tree-sitter/`
**Purpose**: Parser generator and parsing library
**License**: MIT

**Installation:**
```bash
# Initialize submodule
git submodule update --init --recursive
```

**Tree-sitter CLI:**
```bash
# Install globally
npm install -g tree-sitter-cli

# Or use local installation
npm install
```

**Usage in MADOLA:**
- Parses `.mda` source files into Abstract Syntax Trees (AST)
- Provides syntax highlighting support for editors
- Enables incremental parsing for better performance
- Grammar definition in `tree-sitter-madola/grammar.js`

**Build Integration:**
- Parser generated from `grammar.js` → `parser.c`
- Automatically regenerated when grammar changes
- Compiled into MADOLA executable

### 4. Eigen

**Version**: Latest from main branch
**Type**: Git submodule
**Location**: `external/eigen/`
**Purpose**: Linear algebra library
**License**: MPL2 (Mozilla Public License 2.0)

**Installation:**
```bash
# Initialize submodule
git submodule update --init --recursive
```

**Usage in MADOLA:**
- Matrix operations (multiplication, inversion, determinant)
- Vector operations (dot product, cross product)
- Eigenvalue and eigenvector computation
- Linear system solving
- Matrix decompositions (LU, QR, SVD)

**Features Used:**
- Dense matrix operations
- Matrix methods (`.det()`, `.inv()`, `.T()`)
- Eigenvalue computation (`.eigenvalues()`, `.eigenvectors()`)
- High-performance SIMD optimizations

### 5. SymEngine (Optional)

**Version**: Latest from main branch
**Type**: Git submodule
**Location**: `external/symengine/`
**Purpose**: Symbolic computation library
**License**: MIT
**CMake Option**: `WITH_SYMENGINE` (default: ON)

**Installation:**
```bash
# Initialize submodule
git submodule update --init --recursive
```

**Usage in MADOLA:**
- Symbolic differentiation
- Expression simplification
- Symbolic matrix operations
- Polynomial manipulation

**Size Impact**: ~100-110 MB added to executable

**See Also**: [SymEngine Integration](SYMENGINE.md)

### 6. Boost (Optional)

**Version**: Latest from main branch
**Type**: Git submodule
**Location**: `external/boost/`
**Purpose**: Multiprecision arithmetic for SymEngine
**License**: Boost Software License 1.0
**CMake Option**: Enabled automatically when `WITH_SYMENGINE=ON`

**Installation:**
```bash
# Initialize submodule
git submodule update --init --recursive
```

**Components Used:**
- **Boost.Multiprecision** (header-only)
  - Arbitrary-precision integers
  - Arbitrary-precision floating-point
  - Used by SymEngine for exact arithmetic

**Note**: Only header files are used; no linking required.

**Modular Structure:**
Boost uses a modular structure with headers in `libs/*/include/`:
- `libs/multiprecision/include/` - Main multiprecision headers
- `libs/config/include/` - Configuration macros
- `libs/core/include/` - Core utilities
- `libs/integer/include/` - Integer utilities
- And many more...

### 7. Emscripten (Optional)

**Version**: Latest stable
**Purpose**: Compile C++ to WebAssembly (WASM)
**License**: MIT/Apache 2.0

**Installation:**

```bash
# Clone Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk

# Install and activate latest version
./emsdk install latest
./emsdk activate latest

# Add to PATH (run in each session or add to shell profile)
source ./emsdk_env.sh  # Unix/Linux/macOS
emsdk_env.bat          # Windows
```

**Usage in MADOLA:**
- Compiles MADOLA to WASM for web deployment
- Generates JavaScript glue code
- Enables browser-based execution
- Supports WASM function imports/exports

**Build Target:**
```bash
# Build WASM
./dev.sh wasm      # Unix/Linux/macOS
dev.bat wasm       # Windows
```

**Output:**
- `web/runtime/madola.js` - JavaScript module
- `web/runtime/madola.wasm` - WebAssembly binary

### 8. Node.js (Optional)

**Version**: 14.0 or higher
**Purpose**: Web development and Tree-sitter CLI
**License**: MIT

**Installation:**

- **Windows**: Download from [nodejs.org](https://nodejs.org/) or:
  ```cmd
  winget install OpenJS.NodeJS
  ```

- **Linux**:
  ```bash
  # Ubuntu/Debian
  sudo apt-get install nodejs npm

  # Fedora
  sudo dnf install nodejs npm
  ```

- **macOS**:
  ```bash
  brew install node
  ```

**Usage in MADOLA:**
- Tree-sitter CLI for parser generation
- Development server for web demo
- NPM scripts for build automation
- Package management

**NPM Dependencies:**
See [`package.json`](../package.json) for complete list.

## Git Submodules

MADOLA uses git submodules for C++ dependencies to ensure version consistency and easy updates.

### Submodule List

```
.gitmodules:
[submodule "vendor/tree-sitter"]
    path = vendor/tree-sitter
    url = https://github.com/tree-sitter/tree-sitter.git

[submodule "external/eigen"]
    path = external/eigen
    url = https://gitlab.com/libeigen/eigen.git

[submodule "external/symengine"]
    path = external/symengine
    url = https://github.com/symengine/symengine.git

[submodule "external/boost"]
    path = external/boost
    url = https://github.com/boostorg/boost.git
```

### Initializing Submodules

**First-time setup:**
```bash
git submodule update --init --recursive
```

**Updating submodules:**
```bash
# Update all submodules to latest
git submodule update --remote --recursive

# Update specific submodule
cd external/symengine
git pull origin main
cd ../..
git add external/symengine
git commit -m "Update SymEngine submodule"
```

## Platform-Specific Dependencies

### Windows

**Required:**
- MinGW-w64 or MSVC compiler
- Ninja build tool (for MinGW builds)

**Installation (MSYS2):**
```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja
```

**PATH Configuration:**
```cmd
set PATH=%PATH%;C:\msys64\mingw64\bin
```

### Linux

**Required:**
- GCC or Clang
- Make or Ninja

**Installation (Ubuntu/Debian):**
```bash
sudo apt-get install build-essential cmake ninja-build
```

### macOS

**Required:**
- Xcode Command Line Tools
- Homebrew (recommended)

**Installation:**
```bash
xcode-select --install
brew install cmake ninja
```

## Optional Production Dependencies

### GMP (GNU Multiple Precision Arithmetic Library)

**Purpose**: Faster arbitrary-precision arithmetic for SymEngine
**License**: LGPL v3 / GPL v2
**Status**: Optional, not currently used

**Installation:**

- **Windows (MSYS2)**:
  ```bash
  pacman -S mingw-w64-x86_64-gmp
  ```

- **Linux**:
  ```bash
  sudo apt-get install libgmp-dev
  ```

- **macOS**:
  ```bash
  brew install gmp
  ```

**To Enable:**
```cmake
cmake .. -DINTEGER_CLASS=gmp
```

**Benefits:**
- 2-5x faster than Boost.Multiprecision
- Recommended for production builds
- Not compatible with WASM (use Boost for WASM)

## Dependency Graph

```
MADOLA
├── CMake (build system)
├── C++17 Compiler
│   ├── GCC/Clang/MSVC
│   └── Standard Library
├── Tree-sitter (parsing)
│   └── Tree-sitter CLI (Node.js)
├── Eigen (linear algebra)
├── SymEngine (optional, symbolic computation)
│   └── Boost.Multiprecision (arbitrary precision)
│       └── Boost headers (modular)
└── Emscripten (optional, WASM)
    └── Node.js
```

## Dependency Sizes

### Source Code Size

- **Tree-sitter**: ~2 MB
- **Eigen**: ~10 MB (header-only)
- **SymEngine**: ~50 MB
- **Boost**: ~500 MB (full), ~5 MB (used headers only)

### Compiled Size Impact

- **MADOLA Core**: ~5-10 MB
- **Tree-sitter**: ~1 MB
- **Eigen**: ~1 MB (header-only, minimal code generation)
- **SymEngine**: ~100-110 MB
- **Total (with SymEngine)**: ~114 MB
- **Total (without SymEngine)**: ~5-10 MB

## Troubleshooting

### Submodule Issues

**Issue**: Submodules not initialized

**Solution**:
```bash
git submodule update --init --recursive
```

**Issue**: Submodule update fails

**Solution**:
```bash
# Reset submodules
git submodule deinit -f .
git submodule update --init --recursive
```

### Compiler Issues

**Issue**: C++17 features not available

**Solution**: Update compiler to minimum version:
- GCC 7.0+
- Clang 5.0+
- MSVC 2017+

**Issue**: Missing standard library headers

**Solution**: Install complete toolchain:
```bash
# Windows (MSYS2)
pacman -S mingw-w64-x86_64-toolchain

# Linux
sudo apt-get install build-essential
```

### Build Tool Issues

**Issue**: CMake not found

**Solution**: Install CMake and add to PATH:
```bash
# Verify installation
cmake --version
```

**Issue**: Ninja not found (Windows)

**Solution**:
```bash
# MSYS2
pacman -S mingw-w64-x86_64-ninja

# Or use MSBuild instead
cmake .. -G "Visual Studio 16 2019"
```

## Dependency Licenses

| Dependency | License | Commercial Use |
|------------|---------|----------------|
| CMake | BSD 3-Clause | ✅ Yes |
| GCC | GPL v3+ | ✅ Yes (runtime exception) |
| Clang | Apache 2.0 | ✅ Yes |
| Tree-sitter | MIT | ✅ Yes |
| Eigen | MPL2 | ✅ Yes |
| SymEngine | MIT | ✅ Yes |
| Boost | Boost License | ✅ Yes |
| Emscripten | MIT/Apache 2.0 | ✅ Yes |
| Node.js | MIT | ✅ Yes |
| GMP | LGPL v3 / GPL v2 | ⚠️ LGPL allows dynamic linking |

**Note**: All dependencies allow commercial use. GMP requires dynamic linking for commercial use due to LGPL license.

## See Also

- [SymEngine Integration](SYMENGINE.md) - Detailed SymEngine documentation
- [CMake Build System](CMAKE_SYSTEM.md) - Build system configuration
- [Build Configuration](BUILD_CONFIGURATION.md) - Build options and flags
- [Development Guide](../Development.md) - Development setup and workflow
