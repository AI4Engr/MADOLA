# Build Configuration Guide

## Overview

This document provides comprehensive information about configuring MADOLA builds for different scenarios, including development, production, WASM, and size-optimized builds.

## Quick Reference

### Common Build Configurations

| Configuration | Command | Use Case |
|--------------|---------|----------|
| **Debug** | `cmake -B build -DCMAKE_BUILD_TYPE=Debug` | Development, debugging |
| **Release** | `cmake -B build -DCMAKE_BUILD_TYPE=Release` | Production, maximum performance |
| **Minimal Size** | `cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel -DWITH_SYMENGINE=OFF` | Embedded systems, size-critical |
| **WASM** | `emcmake cmake -B build/wasm -G Ninja` | Web deployment |
| **With SymEngine** | `cmake -B build -DWITH_SYMENGINE=ON` | Symbolic computation |
| **Without SymEngine** | `cmake -B build -DWITH_SYMENGINE=OFF` | Minimal dependencies |

## Build Types

### 1. Development Build (Debug)

**Purpose**: Active development and debugging

**Configuration:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

**Features:**
- Full debug symbols (`-g`)
- No optimization (`-O0`)
- Assertions enabled
- Debug features enabled
- Source maps for error reporting

**Executable Size**: ~15-20 MB (without SymEngine), ~120-130 MB (with SymEngine)

**Compilation Time**: ~2-3 minutes (first build), ~10-30 seconds (incremental)

**Use Cases:**
- Active development
- Debugging with GDB/LLDB
- Testing new features
- Error investigation

### 2. Production Build (Release)

**Purpose**: Production deployment with maximum performance

**Configuration:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Features:**
- Maximum optimization (`-O3`)
- No debug symbols
- Assertions disabled (`-DNDEBUG`)
- Link-time optimization (LTO) enabled
- Strip symbols

**Executable Size**: ~10-15 MB (without SymEngine), ~110-115 MB (with SymEngine)

**Compilation Time**: ~5-10 minutes (first build), ~30-60 seconds (incremental)

**Performance**: 2-5x faster than Debug build

**Use Cases:**
- Production deployment
- Performance benchmarking
- End-user distribution

### 3. Size-Optimized Build (MinSizeRel)

**Purpose**: Smallest possible executable size

**Configuration:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel -DWITH_SYMENGINE=OFF -DWITH_DEBUG_FEATURES=OFF
cmake --build build
```

**Features:**
- Size optimization (`-Os`)
- No debug symbols
- Assertions disabled
- Minimal features
- Strip symbols

**Executable Size**: ~3-5 MB

**Compilation Time**: ~1-2 minutes (first build), ~10-20 seconds (incremental)

**Performance**: Slower than Release, but smaller

**Use Cases:**
- Embedded systems
- Size-critical deployments
- Minimal installations

### 4. Profiling Build (RelWithDebInfo)

**Purpose**: Optimized build with debug information for profiling

**Configuration:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

**Features:**
- Good optimization (`-O2`)
- Debug symbols included (`-g`)
- Assertions disabled
- Suitable for profiling

**Executable Size**: ~12-18 MB (without SymEngine), ~115-125 MB (with SymEngine)

**Compilation Time**: ~3-5 minutes (first build), ~20-40 seconds (incremental)

**Use Cases:**
- Performance profiling
- Optimization analysis
- Production debugging

## Feature Configuration

### SymEngine (Symbolic Computation)

**Enable (Default):**
```bash
cmake -B build -DWITH_SYMENGINE=ON
```

**Disable:**
```bash
cmake -B build -DWITH_SYMENGINE=OFF
```

**Impact:**
- **Size**: +100-110 MB
- **Compilation Time**: +3-5 minutes
- **Features**: Symbolic differentiation, simplification, expression manipulation

**When to Enable:**
- Need symbolic computation
- Mathematical research
- Equation solving

**When to Disable:**
- Size-critical builds
- No symbolic computation needed
- Faster compilation

### Tree-sitter (Advanced Parsing)

**Enable (Default):**
```bash
cmake -B build -DWITH_TREE_SITTER=ON
```

**Disable:**
```bash
cmake -B build -DWITH_TREE_SITTER=OFF
```

**Impact:**
- **Size**: +1 MB
- **Compilation Time**: +10-20 seconds
- **Features**: Advanced parsing, syntax highlighting, incremental parsing

**When to Enable:**
- Need syntax highlighting
- IDE integration
- Advanced parsing features

**When to Disable:**
- Minimal builds
- Basic parsing sufficient

### Debug Features

**Enable (Default):**
```bash
cmake -B build -DWITH_DEBUG_FEATURES=ON
```

**Disable:**
```bash
cmake -B build -DWITH_DEBUG_FEATURES=OFF
```

**Impact:**
- **Size**: +2-3 MB
- **Compilation Time**: +20-30 seconds
- **Features**: Interactive debugger, source maps, error reporting

**When to Enable:**
- Development builds
- Debugging needed
- Error investigation

**When to Disable:**
- Production builds
- Size-critical builds
- No debugging needed

## Platform-Specific Configurations

### Windows (MinGW-w64)

**Configuration:**
```cmd
cmake -B build -G Ninja -DCMAKE_C_COMPILER=C:/msys64/mingw64/bin/gcc.exe -DCMAKE_CXX_COMPILER=C:/msys64/mingw64/bin/g++.exe
cmake --build build
```

**Recommended Settings:**
- Generator: Ninja (fastest)
- Compiler: MinGW-w64 GCC
- Build Type: Release or Debug

**Common Issues:**
- Path issues with Git Bash (use cmd.exe)
- Missing DLLs (use static linking)

### Windows (MSVC)

**Configuration:**
```cmd
cmake -B build -G "Visual Studio 16 2019"
cmake --build build --config Release
```

**Recommended Settings:**
- Generator: Visual Studio
- Build Type: Release or Debug
- Multi-threaded compilation: `/MP`

**Common Issues:**
- Unicode characters (use ASCII)
- Longer compilation times

### Linux

**Configuration:**
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Recommended Settings:**
- Generator: Ninja or Unix Makefiles
- Compiler: GCC or Clang
- Build Type: Release or Debug

**Optimizations:**
```bash
cmake -B build -DCMAKE_CXX_FLAGS="-march=native -mtune=native"
```

### macOS

**Configuration:**
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Recommended Settings:**
- Generator: Ninja or Unix Makefiles
- Compiler: Clang (default)
- Build Type: Release or Debug

**Apple Silicon Optimizations:**
```bash
cmake -B build -DCMAKE_OSX_ARCHITECTURES=arm64
```

## WASM Configuration

### Standard WASM Build

**Configuration:**
```bash
# Activate Emscripten
source /path/to/emsdk/emsdk_env.sh  # Unix/Linux/macOS
emsdk_env.bat                        # Windows

# Configure
emcmake cmake -B build/wasm -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build/wasm --parallel
```

**Output:**
- `web/runtime/madola.js` - JavaScript module
- `web/runtime/madola.wasm` - WebAssembly binary

**Size**: ~2-5 MB (without SymEngine), ~50-80 MB (with SymEngine)

### WASM with SymEngine

**Configuration:**
```bash
emcmake cmake -B build/wasm -G Ninja -DCMAKE_BUILD_TYPE=Release -DWITH_SYMENGINE=ON
cmake --build build/wasm --parallel
```

**Requirements:**
- Boost.Multiprecision (header-only)
- INTEGER_CLASS=boostmp (automatic)

**Size**: ~50-80 MB

### WASM Size Optimization

**Configuration:**
```bash
emcmake cmake -B build/wasm -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel -DWITH_SYMENGINE=OFF -DCMAKE_CXX_FLAGS="-Oz"
cmake --build build/wasm --parallel
```

**Additional Flags:**
```bash
-s WASM=1
-s ALLOW_MEMORY_GROWTH=1
-s INITIAL_MEMORY=16MB
-Oz  # Aggressive size optimization
```

**Size**: ~1-2 MB

## Optimization Flags

### GCC/Clang Optimization Levels

| Flag | Description | Use Case |
|------|-------------|----------|
| `-O0` | No optimization | Debug builds |
| `-O1` | Basic optimization | Quick builds |
| `-O2` | Moderate optimization | Balanced builds |
| `-O3` | Maximum optimization | Production builds |
| `-Os` | Size optimization | Size-critical builds |
| `-Ofast` | Aggressive optimization | Performance-critical (may break standards) |

### Additional Optimization Flags

**Native CPU optimization:**
```bash
cmake -B build -DCMAKE_CXX_FLAGS="-march=native -mtune=native"
```

**Link-time optimization (LTO):**
```cmake
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
```

**Fast math (may break IEEE compliance):**
```bash
cmake -B build -DCMAKE_CXX_FLAGS="-ffast-math"
```

## Compilation Speed Optimization

### Unity Builds

**Enable:**
```cmake
set(USE_UNITY_BUILD ON CACHE BOOL "Use unity builds" FORCE)
```

**Benefits:**
- 2-5x faster compilation
- Better optimization opportunities

**Drawbacks:**
- Longer incremental builds
- Potential symbol conflicts

### Precompiled Headers

**Enable:**
```cmake
set(USE_PRECOMPILED_HEADERS ON CACHE BOOL "Use precompiled headers" FORCE)
```

**Benefits:**
- 20-40% faster compilation
- Reduced memory usage

**Drawbacks:**
- Initial overhead
- More complex build system

### Compiler Cache (ccache)

**Enable:**
```bash
cmake -B build -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

**Benefits:**
- Much faster rebuilds
- Caches compilation results

**Installation:**
- **Linux**: `sudo apt-get install ccache`
- **macOS**: `brew install ccache`
- **Windows**: Not widely supported

### Parallel Builds

**Ninja (automatic):**
```bash
cmake --build build
```

**Make (manual):**
```bash
cmake --build build -- -j$(nproc)
```

**MSBuild:**
```bash
cmake --build build -- /m
```

## Dependency Configuration

### Minimal Dependencies

**Configuration:**
```bash
cmake -B build -DWITH_SYMENGINE=OFF -DWITH_TREE_SITTER=OFF -DWITH_DEBUG_FEATURES=OFF
```

**Dependencies:**
- CMake
- C++17 compiler
- Eigen (header-only)

**Size**: ~3-5 MB

### Full Dependencies

**Configuration:**
```bash
cmake -B build -DWITH_SYMENGINE=ON -DWITH_TREE_SITTER=ON -DWITH_DEBUG_FEATURES=ON
```

**Dependencies:**
- CMake
- C++17 compiler
- Eigen
- Tree-sitter
- SymEngine
- Boost.Multiprecision

**Size**: ~114 MB

## Static vs Dynamic Linking

### Static Linking (Default)

**Configuration:**
```bash
cmake -B build -DBUILD_SHARED_LIBS=OFF
```

**Advantages:**
- Single executable
- No DLL dependencies
- Easier distribution

**Disadvantages:**
- Larger executable
- Longer linking time

### Dynamic Linking

**Configuration:**
```bash
cmake -B build -DBUILD_SHARED_LIBS=ON
```

**Advantages:**
- Smaller executable
- Faster linking
- Shared library reuse

**Disadvantages:**
- Requires distributing DLLs
- More complex deployment
- DLL version conflicts

## Cross-Compilation

### ARM (Raspberry Pi)

**Configuration:**
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=arm-toolchain.cmake
```

**Toolchain File (arm-toolchain.cmake):**
```cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
```

### Android

**Configuration:**
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a
```

### iOS

**Configuration:**
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=ios.toolchain.cmake -DPLATFORM=OS64
```

## Testing Configuration

### Enable Testing

**Configuration:**
```bash
cmake -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

### Disable Testing

**Configuration:**
```bash
cmake -B build -DBUILD_TESTING=OFF
```

## Installation Configuration

### System Installation

**Configuration:**
```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
```

### User Installation

**Configuration:**
```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=$HOME/.local
cmake --build build
cmake --install build
```

### Custom Installation

**Configuration:**
```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/opt/madola
cmake --build build
cmake --install build
```

## Troubleshooting

### Build Fails with Out of Memory

**Solution**: Reduce parallel jobs:
```bash
cmake --build build --parallel 2
```

### Compilation Too Slow

**Solutions**:
1. Enable ccache
2. Use Ninja generator
3. Enable unity builds
4. Reduce optimization level

### Executable Too Large

**Solutions**:
1. Disable SymEngine: `-DWITH_SYMENGINE=OFF`
2. Use MinSizeRel: `-DCMAKE_BUILD_TYPE=MinSizeRel`
3. Strip symbols: `strip madola`
4. Disable debug features: `-DWITH_DEBUG_FEATURES=OFF`

### Missing Dependencies

**Solution**: Initialize submodules:
```bash
# WARNING: Do NOT use "git submodule update --init --recursive" (downloads ~1.2GB)
# Use setup script instead: dev.bat setup (Windows) or ./dev.sh setup (Unix)
```

## Build Presets

### Preset 1: Development

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DWITH_SYMENGINE=ON \
  -DWITH_TREE_SITTER=ON \
  -DWITH_DEBUG_FEATURES=ON \
  -G Ninja
```

### Preset 2: Production

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DWITH_SYMENGINE=ON \
  -DWITH_TREE_SITTER=ON \
  -DWITH_DEBUG_FEATURES=OFF \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
  -G Ninja
```

### Preset 3: Minimal

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DWITH_SYMENGINE=OFF \
  -DWITH_TREE_SITTER=OFF \
  -DWITH_DEBUG_FEATURES=OFF \
  -G Ninja
```

### Preset 4: WASM

```bash
emcmake cmake -B build/wasm \
  -DCMAKE_BUILD_TYPE=Release \
  -DWITH_SYMENGINE=ON \
  -G Ninja
```

## See Also

- [CMake Build System](CMAKE_SYSTEM.md) - Detailed CMake configuration
- [Dependencies](DEPENDENCIES.md) - Complete dependency list
- [SymEngine Integration](SYMENGINE.md) - SymEngine configuration
- [Development Guide](../Development.md) - Development workflow
