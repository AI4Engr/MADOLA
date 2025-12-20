# MADOLA Documentation

Welcome to the MADOLA technical documentation. This directory contains comprehensive guides for building, configuring, and understanding the MADOLA system.

## Documentation Index

### Core Documentation

1. **[Architecture](ARCHITECTURE.md)**
   - System architecture overview
   - Core components (Parser, AST Builder, Evaluator, Generators)
   - Mathematical libraries (Eigen, SymEngine, Boost)
   - Data flow and compilation pipeline
   - Memory management and error handling
   - Performance considerations and extensibility
   - Testing strategy and deployment

2. **[SymEngine Integration](SYMENGINE.md)**
   - Overview of SymEngine symbolic computation library
   - Integration architecture and implementation
   - Dependency configuration (Boost, GMP)
   - Symbolic computation features (differentiation, simplification)
   - Executable size considerations (~100-110 MB impact)
   - Build instructions and troubleshooting
   - Performance considerations and future enhancements

3. **[Dependencies](DEPENDENCIES.md)**
   - Complete list of all MADOLA dependencies
   - Core dependencies (CMake, C++17, Tree-sitter, Eigen)
   - Optional dependencies (SymEngine, Boost, Emscripten, Node.js)
   - Git submodule management
   - Platform-specific requirements
   - Installation instructions for all platforms
   - Dependency licenses and commercial use

4. **[CMake Build System](CMAKE_SYSTEM.md)**
   - CMake configuration and architecture
   - Build targets and options
   - Compiler configuration (GCC, Clang, MSVC)
   - Generator selection (Ninja, Make, Visual Studio)
   - SymEngine and WASM configuration
   - Include directories and linking
   - Testing with CTest
   - Advanced configuration options

5. **[Build Configuration](BUILD_CONFIGURATION.md)**
   - Build types (Debug, Release, MinSizeRel, RelWithDebInfo)
   - Feature configuration (SymEngine, Tree-sitter, Debug)
   - Platform-specific configurations (Windows, Linux, macOS)
   - WASM build configuration
   - Optimization flags and compilation speed
   - Static vs dynamic linking
   - Cross-compilation and testing
   - Build presets and troubleshooting

## Quick Start

### For Developers

If you're new to MADOLA development, start here:

1. **Architecture**: Read [Architecture](ARCHITECTURE.md) to understand the system design
2. **Setup**: Read [Dependencies](DEPENDENCIES.md) to install required tools
3. **Build**: Follow [CMake Build System](CMAKE_SYSTEM.md) for basic build instructions
4. **Configure**: Use [Build Configuration](BUILD_CONFIGURATION.md) for specific build scenarios

### For Users

If you're using MADOLA for mathematical computation:

1. **Installation**: See the main [README](../README.md) for installation instructions
2. **Language Guide**: Read [LANGUAGE_GUIDE](../LANGUAGE_GUIDE.md) for syntax and features
3. **Examples**: Check the example files in the root directory

## Documentation Structure

```
docs/
├── README.md                    # This file - documentation index
├── ARCHITECTURE.md              # System architecture overview
├── SYMENGINE.md                 # SymEngine integration guide
├── DEPENDENCIES.md              # Complete dependency documentation
├── CMAKE_SYSTEM.md              # CMake build system guide
├── BUILD_CONFIGURATION.md       # Build configuration reference
├── TODO.md                      # Project roadmap and improvement plan
├── WASM_COMPLETE_GUIDE.md       # Complete WASM build guide
├── interactive_debugging.md     # Interactive debugging guide
└── source_map_debugging.md      # Source map debugging guide
```

## Common Tasks

### Building MADOLA

**Quick build (all features):**
```bash
# Windows
dev.bat build

# Unix/Linux/macOS
./dev.sh build
```

**Minimal build (no SymEngine):**
```bash
cmake -B build -DWITH_SYMENGINE=OFF
cmake --build build
```

**WASM build:**
```bash
# Windows
dev.bat wasm

# Unix/Linux/macOS
./dev.sh wasm
```

See [Build Configuration](BUILD_CONFIGURATION.md) for more options.

### Understanding Dependencies

**What dependencies do I need?**
- See [Dependencies](DEPENDENCIES.md) for complete list

**How do I install dependencies?**
- See platform-specific sections in [Dependencies](DEPENDENCIES.md)

**What is SymEngine and do I need it?**
- See [SymEngine Integration](SYMENGINE.md) for detailed explanation

### Configuring Builds

**How do I configure for production?**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

**How do I reduce executable size?**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel -DWITH_SYMENGINE=OFF
```

**How do I enable debugging?**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DWITH_DEBUG_FEATURES=ON
```

See [Build Configuration](BUILD_CONFIGURATION.md) for all options.

## Key Concepts

### SymEngine

SymEngine is a C++ symbolic computation library that provides:
- Symbolic differentiation
- Expression simplification
- Polynomial manipulation
- Transcendental functions

**Size Impact**: ~100-110 MB added to executable

**When to use**: Mathematical research, equation solving, symbolic computation

**When to disable**: Size-critical builds, no symbolic computation needed

See [SymEngine Integration](SYMENGINE.md) for details.

### Build Types

MADOLA supports four build types:

1. **Debug**: Development and debugging (`-O0 -g`)
2. **Release**: Production deployment (`-O3 -DNDEBUG`)
3. **RelWithDebInfo**: Profiling (`-O2 -g`)
4. **MinSizeRel**: Size optimization (`-Os -DNDEBUG`)

See [Build Configuration](BUILD_CONFIGURATION.md) for details.

### WASM Builds

MADOLA can be compiled to WebAssembly for browser execution:

- Uses Emscripten compiler
- Outputs to `web/runtime/`
- Supports SymEngine (with Boost.Multiprecision)
- Size: ~2-5 MB (without SymEngine), ~50-80 MB (with SymEngine)

See [CMake Build System](CMAKE_SYSTEM.md) and [Build Configuration](BUILD_CONFIGURATION.md) for details.

## Troubleshooting

### Build Issues

**Problem**: Build fails with missing dependencies

**Solution**: Initialize git submodules:
```bash
git submodule update --init --recursive
```

**Problem**: Out of memory during compilation

**Solution**: Reduce parallel jobs:
```bash
cmake --build build --parallel 2
```

**Problem**: Executable too large

**Solution**: Disable SymEngine:
```bash
cmake -B build -DWITH_SYMENGINE=OFF
```

See [Build Configuration](BUILD_CONFIGURATION.md) for more troubleshooting.

### SymEngine Issues

**Problem**: SymEngine build fails

**Solution**: Ensure Boost submodule is initialized:
```bash
git submodule update --init --recursive
```

**Problem**: Symbolic differentiation fails

**Solution**: Check that expression uses supported operations (see [SymEngine Integration](SYMENGINE.md))

### Platform Issues

**Problem**: Windows build fails with path issues

**Solution**: Use Windows Command Prompt (cmd.exe), not Git Bash

**Problem**: macOS build fails with compiler errors

**Solution**: Install Xcode Command Line Tools:
```bash
xcode-select --install
```

## Contributing

When contributing to MADOLA, please:

1. Read the relevant documentation before making changes
2. Update documentation when adding new features
3. Follow the existing code style and conventions
4. Test your changes on multiple platforms if possible

## Additional Resources

### Main Documentation

- [README](../README.md) - Project overview and quick start
- [LANGUAGE_GUIDE](../LANGUAGE_GUIDE.md) - MADOLA language syntax and features
- [Development](../Development.md) - Development guide and workflow
- [CLAUDE](../CLAUDE.md) - AI assistant instructions

### External Documentation

- [CMake Documentation](https://cmake.org/documentation/)
- [SymEngine Documentation](https://github.com/symengine/symengine/wiki)
- [Eigen Documentation](https://eigen.tuxfamily.org/dox/)
- [Tree-sitter Documentation](https://tree-sitter.github.io/tree-sitter/)
- [Emscripten Documentation](https://emscripten.org/docs/)

## Document Maintenance

This documentation is maintained alongside the MADOLA codebase. When making changes:

- **Code changes**: Update relevant documentation
- **New features**: Add documentation for new features
- **Build system changes**: Update CMake and build configuration docs
- **Dependency changes**: Update dependencies documentation

## Version Information

This documentation is current as of:
- **MADOLA Version**: Development (pre-1.0)
- **CMake Version**: 3.16+
- **C++ Standard**: C++17
- **SymEngine**: Latest from main branch
- **Eigen**: Latest from main branch

## License

MADOLA is distributed under the [Apache License 2.0](../Apache_License.md).

All documentation is part of the MADOLA project and follows the same license.

## Contact and Support

- **Issues**: Report issues on [GitHub](https://github.com/AI4Engr/MADOLA/issues)
- **Discussions**: Join discussions on GitHub
- **Website**: [https://ai4engr.com/MADOLA/](https://ai4engr.com/MADOLA/)

## Document History

- **2025-01**: Initial documentation structure created
  - Architecture overview
  - SymEngine integration guide
  - Dependencies documentation
  - CMake build system guide
  - Build configuration reference
