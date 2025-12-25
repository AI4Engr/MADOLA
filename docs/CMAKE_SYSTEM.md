# CMake Build System

## Overview

MADOLA uses CMake as its cross-platform build system. This document provides detailed information about the CMake configuration, build options, and customization.

## CMake Version

**Minimum Required**: CMake 3.16

**Recommended**: CMake 3.20 or higher for best performance and features

## Build System Architecture

### Project Structure

```
madola/
├── CMakeLists.txt              # Main CMake configuration
├── build/                      # Build directory (generated)
│   ├── native/                # Native build artifacts
│   └── wasm/                  # WASM build artifacts
├── src/                       # Source files
├── external/                  # External dependencies (submodules)
│   ├── eigen/
│   ├── symengine/
│   └── boost/
└── vendor/                    # Vendor dependencies
    └── tree-sitter/
```

### Build Targets

MADOLA defines several build targets:

1. **madola_core_obj** - Object library (core functionality)
2. **madola_core** - Static library (for linking)
3. **madola** - Main executable (native)
4. **madola-debug** - Interactive debugger (native)
5. **madola_wasm** - WebAssembly module (WASM)

## CMake Options

### Core Options

```cmake
option(WITH_TREE_SITTER "Build with Tree-sitter support" ON)
option(WITH_SYMENGINE "Build with SymEngine symbolic computation support" ON)
option(WITH_DEBUG_FEATURES "Build with debug features" ON)
```

### Usage

**Enable/Disable Options:**
```bash
# Disable SymEngine
cmake -B build -DWITH_SYMENGINE=OFF

# Disable Tree-sitter
cmake -B build -DWITH_TREE_SITTER=OFF

# Disable debug features
cmake -B build -DWITH_DEBUG_FEATURES=OFF
```

### Option Details

#### WITH_TREE_SITTER

**Default**: ON
**Purpose**: Enable Tree-sitter parser integration
**Impact**:
- Adds Tree-sitter parser sources to build
- Enables advanced parsing features
- Required for syntax highlighting support
- Size impact: ~1 MB

**When to disable**: If you only need basic parsing or want minimal executable size.

#### WITH_SYMENGINE

**Default**: ON
**Purpose**: Enable symbolic computation features
**Impact**:
- Adds SymEngine library and dependencies
- Enables symbolic differentiation, simplification
- Requires Boost.Multiprecision
- Size impact: ~100-110 MB

**When to disable**: If you don't need symbolic computation or want minimal executable size.

#### WITH_DEBUG_FEATURES

**Default**: ON
**Purpose**: Enable interactive debugger and debug utilities
**Impact**:
- Builds `madola-debug` executable
- Adds source mapping and error reporting
- Enables breakpoints and step-through debugging
- Size impact: ~2-3 MB

**When to disable**: For production builds where debugging is not needed.

## Build Types

CMake supports multiple build types with different optimization levels:

### Debug

**Purpose**: Development and debugging
**Flags**: `-g -O0`
**Features**:
- Full debug symbols
- No optimization
- Assertions enabled
- Slower execution

**Usage:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Release

**Purpose**: Production deployment
**Flags**: `-O3 -DNDEBUG`
**Features**:
- Maximum optimization
- No debug symbols
- Assertions disabled
- Fastest execution

**Usage:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### RelWithDebInfo

**Purpose**: Optimized build with debug info
**Flags**: `-O2 -g -DNDEBUG`
**Features**:
- Good optimization
- Debug symbols included
- Assertions disabled
- Good balance for profiling

**Usage:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

### MinSizeRel

**Purpose**: Smallest executable size
**Flags**: `-Os -DNDEBUG`
**Features**:
- Size optimization
- No debug symbols
- Assertions disabled
- Smaller but slower than Release

**Usage:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build
```

## Compiler Configuration

### Compiler Selection

**GCC:**
```bash
cmake -B build -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
```

**Clang:**
```bash
cmake -B build -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

**MSVC:**
```bash
cmake -B build -G "Visual Studio 16 2019"
```

**MinGW-w64 (Windows):**
```bash
cmake -B build -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe
```

### Compiler Flags

MADOLA sets compiler-specific flags for optimal performance:

**GCC/Clang:**
```cmake
-std=c++17
-Wall -Wextra -Wpedantic
-fno-exceptions (for WASM)
```

**MSVC:**
```cmake
/std:c++17
/W4
/EHsc
```

### Custom Compiler Flags

**Add custom flags:**
```bash
cmake -B build -DCMAKE_CXX_FLAGS="-march=native -mtune=native"
```

**Add custom linker flags:**
```bash
cmake -B build -DCMAKE_EXE_LINKER_FLAGS="-static"
```

## Generator Selection

CMake supports multiple build system generators:

### Ninja (Recommended)

**Advantages**:
- Fastest build times
- Parallel builds by default
- Cross-platform

**Usage:**
```bash
cmake -B build -G Ninja
cmake --build build
```

**Installation:**
- **Windows (MSYS2)**: `pacman -S mingw-w64-x86_64-ninja`
- **Linux**: `sudo apt-get install ninja-build`
- **macOS**: `brew install ninja`

### Unix Makefiles

**Advantages**:
- Available on all Unix systems
- Well-tested and stable

**Usage:**
```bash
cmake -B build -G "Unix Makefiles"
cmake --build build -- -j$(nproc)
```

### Visual Studio

**Advantages**:
- Native Windows integration
- IDE support

**Usage:**
```bash
cmake -B build -G "Visual Studio 16 2019"
cmake --build build --config Release
```

### MSBuild

**Advantages**:
- Windows native build tool
- No additional installation needed

**Usage:**
```bash
cmake -B build -G "NMake Makefiles"
cmake --build build
```

## SymEngine Configuration

### Integer Class Selection

SymEngine requires an integer class for arbitrary-precision arithmetic:

**Boost.Multiprecision (Default):**
```cmake
set(INTEGER_CLASS "boostmp" CACHE STRING "Integer class for SymEngine" FORCE)
set(WITH_BOOST ON CACHE BOOL "Build with Boost" FORCE)
```

**GMP (Faster, Optional):**
```cmake
cmake -B build -DINTEGER_CLASS=gmp
```

**Requirements for GMP:**
- **Windows (MSYS2)**: `pacman -S mingw-w64-x86_64-gmp`
- **Linux**: `sudo apt-get install libgmp-dev`
- **macOS**: `brew install gmp`

### SymEngine Options

MADOLA configures SymEngine with minimal dependencies:

```cmake
set(BUILD_TESTS OFF CACHE BOOL "Build SymEngine tests" FORCE)
set(BUILD_BENCHMARKS OFF CACHE BOOL "Build SymEngine benchmarks" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(WITH_MPFR OFF CACHE BOOL "Build with MPFR" FORCE)
set(WITH_MPC OFF CACHE BOOL "Build with MPC" FORCE)
set(WITH_ARB OFF CACHE BOOL "Build with ARB" FORCE)
set(WITH_FLINT OFF CACHE BOOL "Build with FLINT" FORCE)
```

**To enable optional features:**
```bash
cmake -B build -DWITH_MPFR=ON -DWITH_MPC=ON
```

## WASM Configuration

### Emscripten Setup

**Prerequisites:**
```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh  # Unix/Linux/macOS
emsdk_env.bat          # Windows
```

### WASM Build

**Configure:**
```bash
emcmake cmake -B build/wasm -G Ninja -DCMAKE_BUILD_TYPE=Release
```

**Build:**
```bash
cmake --build build/wasm --parallel
```

**Output:**
- `web/runtime/madola.js` - JavaScript module
- `web/runtime/madola.wasm` - WebAssembly binary

### WASM-Specific Options

```cmake
# Emscripten flags
-s WASM=1
-s ALLOW_MEMORY_GROWTH=1
-s EXPORTED_FUNCTIONS='["_compile_madola"]'
-s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'
```

**Custom WASM flags:**
```bash
emcmake cmake -B build/wasm -DCMAKE_CXX_FLAGS="-s INITIAL_MEMORY=64MB"
```

## Include Directories

MADOLA configures include directories for all dependencies:

```cmake
target_include_directories(madola_core_obj PUBLIC
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/external/eigen
)

# SymEngine includes (if enabled)
target_include_directories(madola_core_obj PUBLIC
    ${CMAKE_SOURCE_DIR}/external/symengine
    ${CMAKE_BINARY_DIR}/external/symengine
)

# Boost includes (if enabled)
target_include_directories(madola_core_obj SYSTEM PUBLIC
    ${CMAKE_SOURCE_DIR}/external/boost/libs/multiprecision/include
    ${CMAKE_SOURCE_DIR}/external/boost/libs/config/include
    # ... more Boost modules
)
```

## Linking Configuration

### Static Linking (Default)

```cmake
add_library(madola_core STATIC $<TARGET_OBJECTS:madola_core_obj>)
```

**Advantages**:
- Single executable
- No DLL dependencies
- Easier distribution

### Dynamic Linking (Optional)

```cmake
cmake -B build -DBUILD_SHARED_LIBS=ON
```

**Advantages**:
- Smaller executable
- Shared library reuse
- Faster linking

**Disadvantages**:
- Requires distributing DLLs
- More complex deployment

## Parallel Builds

### Automatic Parallelism

**Ninja**: Parallel by default
```bash
cmake --build build
```

**Make**: Specify jobs
```bash
cmake --build build -- -j$(nproc)
```

**MSBuild**: Specify parallel level
```bash
cmake --build build -- /m
```

### Custom Parallel Level

```bash
cmake --build build --parallel 8
```

## Build Optimization

### Unity Builds

**Enable unity builds for faster compilation:**
```cmake
set(USE_UNITY_BUILD ON CACHE BOOL "Use unity builds" FORCE)
set_target_properties(madola_core_obj PROPERTIES UNITY_BUILD ON)
```

**Benefits**:
- Faster compilation (2-5x)
- Better optimization opportunities

**Drawbacks**:
- Longer incremental builds
- Potential symbol conflicts

### Precompiled Headers

**Enable precompiled headers:**
```cmake
set(USE_PRECOMPILED_HEADERS ON CACHE BOOL "Use precompiled headers" FORCE)
target_precompile_headers(madola_core_obj PRIVATE
    <vector>
    <string>
    <memory>
    <variant>
)
```

**Benefits**:
- Faster compilation (20-40%)
- Reduced memory usage

### Compiler Cache (ccache)

**Enable ccache:**
```bash
# Install ccache
sudo apt-get install ccache  # Linux
brew install ccache          # macOS

# Configure CMake to use ccache
cmake -B build -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

**Benefits**:
- Much faster rebuilds
- Caches compilation results

## Testing Configuration

### CTest Integration

MADOLA uses CTest for testing:

```cmake
enable_testing()
add_test(NAME madola_test_example COMMAND madola ${CMAKE_SOURCE_DIR}/example.mda)
```

**Run tests:**
```bash
cd build
ctest --output-on-failure
```

**Run specific test:**
```bash
ctest -R madola_test_example
```

**Verbose output:**
```bash
ctest -V
```

## Installation

### Install Targets

```cmake
install(TARGETS madola DESTINATION bin)
install(TARGETS madola-debug DESTINATION bin)
```

**Install:**
```bash
cmake --build build --target install
```

**Custom install prefix:**
```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build --target install
```

## Troubleshooting

### Common Issues

**Issue**: CMake can't find compiler

**Solution**: Specify compiler explicitly:
```bash
cmake -B build -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
```

**Issue**: Submodules not initialized

**Solution**:
```bash
# WARNING: Do NOT use "git submodule update --init --recursive" (downloads ~1.2GB)
# Use setup script instead: dev.bat setup (Windows) or ./dev.sh setup (Unix)
```

**Issue**: SymEngine build fails

**Solution**: Ensure Boost submodule is initialized:
```bash
# WARNING: Do NOT use "git submodule update --init --recursive" (downloads ~1.2GB)
# Use setup script instead: dev.bat setup (Windows) or ./dev.sh setup (Unix)
```

**Issue**: Ninja not found

**Solution**: Install Ninja or use different generator:
```bash
cmake -B build -G "Unix Makefiles"
```

**Issue**: Out of memory during compilation

**Solution**: Reduce parallel jobs:
```bash
cmake --build build --parallel 2
```

### Clean Build

**Remove build directory:**
```bash
rm -rf build
cmake -B build
cmake --build build
```

**Clean specific target:**
```bash
cmake --build build --target clean
```

## Advanced Configuration

### Custom Source Files

**Add custom source files:**
```cmake
list(APPEND CORE_SOURCES
    src/custom/my_feature.cpp
)
```

### Custom Definitions

**Add preprocessor definitions:**
```cmake
target_compile_definitions(madola_core_obj PRIVATE
    MY_CUSTOM_DEFINE=1
    ENABLE_FEATURE_X
)
```

### Custom Libraries

**Link custom libraries:**
```cmake
target_link_libraries(madola_core PUBLIC
    my_custom_library
)
```

## Build Configuration Summary

CMake prints a configuration summary at the end:

```
==============================================
MADOLA Build Configuration
==============================================
  Build Type:            Debug
  C++ Standard:          17
  Eigen Support:         Enabled (external/eigen)
  Tree-sitter Support:   ON
  SymEngine Support:     ON
  Unity Build:           OFF
  Precompiled Headers:   OFF
  Build Cache (ccache):  Enabled
  Target:                Native
  Parallel Jobs:         8
==============================================
```

## See Also

- [Dependencies](DEPENDENCIES.md) - Complete dependency list
- [SymEngine Integration](SYMENGINE.md) - SymEngine configuration
- [Build Configuration](BUILD_CONFIGURATION.md) - Build options and flags
- [Development Guide](../Development.md) - Development workflow
