# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Status

MADOLA (Math Domain Language) implementation is complete with core functionality.

## Development Commands

### CMake Commands (Recommended)

**Windows:**

**IMPORTANT**: Run `dev.bat` from **Windows Command Prompt (cmd.exe)**, NOT Git Bash/MSYS2 bash.
Git Bash has issues with Windows temp file paths that cause compiler failures.

```cmd
dev.bat init-submodules  # Initialize Tree-sitter submodule
dev.bat generate-grammar # Generate Tree-sitter parser
dev.bat configure        # Configure CMake (Debug)
dev.bat build            # Build native executable
dev.bat test             # Run test suite via CTest
dev.bat run              # Run with example.mda
dev.bat clean            # Clean build artifacts
dev.bat release          # Build release version
dev.bat wasm             # Build WASM (requires Emscripten)
dev.bat tree-sitter      # Build with Tree-sitter support
```

**Prerequisites:**
- Install MinGW-w64 toolchain: `pacman -S mingw-w64-x86_64-toolchain` (from MSYS2)
- Tree-sitter CLI: `npm install` (installs tree-sitter-cli from package.json)

**Unix/Linux:**
```bash
./dev.sh init-submodules  # Initialize Tree-sitter submodule
./dev.sh generate-grammar # Generate Tree-sitter parser
./dev.sh configure        # Configure CMake (Debug)
./dev.sh build            # Build native executable
./dev.sh test             # Run test suite via CTest
./dev.sh run              # Run with example.mda
./dev.sh clean            # Clean build artifacts
./dev.sh release          # Build release version
./dev.sh wasm             # Build WASM (requires Emscripten)
./dev.sh tree-sitter      # Build with Tree-sitter support
```

### NPM Scripts
```bash
npm run generate-grammar     # Generate Tree-sitter parser
npm run configure            # Configure CMake (Debug)
npm run build                # Build with CMake
npm run build:debug          # Configure + build debug
npm run build:release        # Configure + build release
npm run build:wasm           # Build WASM with Emscripten
npm run build:tree-sitter    # Generate grammar + build with Tree-sitter
npm run test                 # Run CTest
npm run test:regression      # Run regression tests (all)
npm run test:regression:native # Run native regression tests
npm run test:regression:wasm # Run WASM regression tests
npm run clean                # Clean build artifacts
```

### Regression Testing
```bash
# Unix/Linux/macOS/Git Bash:
./regression/run_regression.sh           # Run native tests
./regression/run_regression.sh native    # Run native tests
./regression/run_regression.sh wasm      # Run WASM tests
./regression/run_regression.sh update    # Run native tests and update baselines
./regression/run_regression.sh native update  # Run native tests and update baselines
./regression/run_regression.sh wasm update    # Run WASM tests and update baselines

# Windows Command Prompt:
regression\run_regression.bat           # Run native tests
regression\run_regression.bat native    # Run native tests
regression\run_regression.bat wasm      # Run WASM tests
regression\run_regression.bat update    # Run native tests and update baselines
regression\run_regression.bat native update  # Run native tests and update baselines
regression\run_regression.bat wasm update    # Run WASM tests and update baselines
```

## Architecture Overview

### Core Components
- **AST Nodes** → `src/core/ast/ast.h` (Program, Statement, Expression)
- **Evaluator** → `src/core/evaluator.h/.cpp` (Environment, execution engine)
- **Formatter** → `src/core/markdown_formatter.h/.cpp` (Markdown output)

### Build Targets
- **Native** → `build/madola.exe` → `dist/madola.exe` (debugging and regression testing)
- **WASM** → `web/runtime/madola.js` + `web/runtime/madola.wasm` (direct build to deployment location)

### Build System
- **CMake** → Cross-platform build configuration with MSVC/GCC support
- **CTest** → Integrated testing framework
- **Ninja/MSBuild** → Fast parallel builds

### Test Suite
- **Unit Tests** → `tests/test_runner.cpp` (7 tests covering core functionality)
- **Sample Files** → `tests/*.mda` (test cases for different scenarios)
- **Regression Tests** → `regression/run_regression.sh` (native + WASM regression testing)

## Important Notes

- Native builds work without Tree-sitter for core functionality testing
- WASM builds require Tree-sitter integration (pending dependency resolution)
- All core language features implemented: assignment, print, variables, error handling
- Test suite passes 100% (7/7 tests)

## Current Status
✅ All core MADOLA functionality implemented and tested
✅ CMake build system with cross-platform support (MSVC/GCC)
✅ CTest integration with 100% pass rate (2/2 tests)
✅ WASM build with direct output to deployment location (`web/runtime/`)
✅ Regression testing for both native (`dist/`) and WASM (`web/runtime/`) builds
✅ Zero-copy deployment - all generated files output to final locations
🎯 Ready for web/electron integration - optimized build pipeline!

## Recent Updates
- **CMake Migration**: Updated from manual compilation to CMake build system
- **Cross-Platform**: MSVC and GCC compiler support with proper flag handling
- **CTest Integration**: Automated testing with `ctest --output-on-failure`
- **Enhanced Scripts**: Updated dev.sh/dev.bat with CMake commands
- **Unicode Fixes**: Replaced Unicode characters for MSVC compatibility
- **Tree-sitter Integration**: Added `generate-grammar` command to properly generate parser.c from grammar.js
- **Cleanup**: Removed old Makefile system in favor of CMake
- **Regression Testing**: Added comprehensive regression test suite for both native and WASM builds
- **WASM Production Build**: Direct output to deployment location (`web/runtime/`)
- **Web Demo**: Created HTML demo showing WASM integration for web applications
- **Git Submodule**: Migrated Tree-sitter to proper git submodule for dependency management
- **Consolidated Structure**: Unified all web assets in `web/` directory with zero-copy deployment