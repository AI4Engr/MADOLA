# SymEngine Integration in MADOLA

## Overview

MADOLA integrates [SymEngine](https://github.com/symengine/symengine) to provide symbolic computation capabilities, including symbolic differentiation, simplification, and expression manipulation. SymEngine is a fast C++ symbolic manipulation library that serves as the computational backend for symbolic operations.

## What is SymEngine?

SymEngine is a standalone fast C++ symbolic manipulation library. It is designed to be:

- **Fast**: Written in C++ with performance as a primary goal
- **Lightweight**: Minimal dependencies for core functionality
- **Embeddable**: Easy to integrate into other projects
- **Compatible**: Can be used as a backend for SymPy and other symbolic systems

### Key Features

- Symbolic differentiation and integration
- Expression simplification and expansion
- Polynomial manipulation
- Trigonometric and transcendental functions
- Matrix operations (symbolic matrices)
- Multiple precision arithmetic support

## Integration Architecture

### Build Configuration

SymEngine is integrated as a git submodule in MADOLA:

```
external/
├── symengine/          # SymEngine library (git submodule)
└── boost/              # Boost library (for multiprecision support)
```

The integration is controlled via CMake options:

```cmake
option(WITH_SYMENGINE "Build with SymEngine symbolic computation support" ON)
```

### Compilation Flags

When SymEngine is enabled, the following preprocessor definition is set:

```cpp
#ifdef WITH_SYMENGINE
// SymEngine-dependent code
#endif
```

This allows MADOLA to be built with or without symbolic computation support.

## Dependencies

### Required Dependencies

1. **SymEngine Core** (git submodule)
   - Location: `external/symengine/`
   - Version: Latest from main branch
   - Purpose: Core symbolic computation engine

2. **Boost.Multiprecision** (git submodule)
   - Location: `external/boost/`
   - Type: Header-only library
   - Purpose: Arbitrary-precision arithmetic for SymEngine
   - Note: Only the multiprecision component is used

### Optional Dependencies

SymEngine supports several optional dependencies for enhanced functionality:

- **GMP** (GNU Multiple Precision Arithmetic Library)
  - Faster arbitrary-precision arithmetic than Boost.Multiprecision
  - Recommended for production builds
  - Not required for WASM builds

- **MPFR** (Multiple Precision Floating-Point Reliable Library)
  - Arbitrary-precision floating-point arithmetic
  - Optional, disabled by default

- **MPC** (Multiple Precision Complex Library)
  - Complex number arithmetic
  - Optional, disabled by default

- **ARB** (Ball Arithmetic Library)
  - Interval arithmetic
  - Optional, disabled by default

## Integer Class Configuration

SymEngine requires an integer class for arbitrary-precision arithmetic. MADOLA uses different configurations for different build targets:

### WASM Builds

```cmake
set(INTEGER_CLASS "boostmp" CACHE STRING "Integer class for SymEngine" FORCE)
set(WITH_BOOST ON CACHE BOOL "Build with Boost (header-only for boostmp)" FORCE)
```

**Rationale**: Boost.Multiprecision is header-only and works well with Emscripten without requiring additional linking.

### Native Builds

Currently using Boost.Multiprecision for consistency:

```cmake
set(INTEGER_CLASS "boostmp" CACHE STRING "Integer class for SymEngine" FORCE)
```

**Future Enhancement**: For production Tauri/server builds, GMP can be enabled for better performance:

```cmake
set(INTEGER_CLASS "gmp" CACHE STRING "Integer class for SymEngine" FORCE)
```

To enable GMP:
- **Windows (MSYS2)**: `pacman -S mingw-w64-x86_64-gmp`
- **Linux**: `apt-get install libgmp-dev`
- **macOS**: `brew install gmp`

## Symbolic Computation Features

### Supported Operations

MADOLA exposes the following symbolic operations through SymEngine:

1. **Symbolic Differentiation**
   ```madola
   @eval
   result := diff(x^2, x);  # Returns: 2*x
   ```

2. **Expression Simplification**
   ```madola
   @eval
   simplified := simplify(x^2 + 2*x^2);  # Returns: 3*x^2
   ```

3. **Symbolic Functions**
   - Trigonometric: `sin`, `cos`, `tan`
   - Exponential: `exp`, `log`
   - Power: `sqrt`, `pow`

### Implementation Details

The symbolic computation is implemented in:

- **Header**: [`src/core/evaluator/evaluator.h`](../src/core/evaluator/evaluator.h)
  ```cpp
  #ifdef WITH_SYMENGINE
      Value evaluateSymbolicDiff(const Expression& expr, const std::string& variable);
      Value evaluateSymbolicMatrixDiff(const ArrayValue& matrix, const std::string& variable);
  #endif
  ```

- **Implementation**: [`src/core/evaluator/evaluator_symbolic.cpp`](../src/core/evaluator/evaluator_symbolic.cpp)
  - Converts MADOLA AST expressions to SymEngine expressions
  - Performs symbolic operations
  - Converts results back to MADOLA values

### Expression Conversion

The conversion between MADOLA and SymEngine expressions handles:

- **Numbers**: `Number` → `SymEngine::real_double`
- **Identifiers**: `Identifier` → `SymEngine::symbol`
- **Binary Operations**: `+`, `-`, `*`, `/`, `^`
- **Unary Operations**: `-`, `+`
- **Function Calls**: `sin`, `cos`, `tan`, `exp`, `log`, `sqrt`

## Executable Size Considerations

### Size Impact

SymEngine significantly increases the executable size:

- **MADOLA Core**: ~5-10 MB
- **With SymEngine**: ~100-110 MB
- **Total**: ~114 MB

### Why So Large?

SymEngine includes:
- Comprehensive symbolic manipulation algorithms
- Expression simplification rules
- Polynomial arithmetic
- Transcendental function support
- Multiple precision arithmetic

This is comparable to other symbolic computation systems:
- **Mathematica**: ~3 GB
- **SymPy** (Python): ~50 MB
- **GiNaC**: ~80 MB
- **SymEngine**: ~100 MB

### Size Reduction Options

If executable size is a concern, you can disable SymEngine:

```bash
# Configure without SymEngine
cmake .. -DWITH_SYMENGINE=OFF

# Build
cmake --build .
```

This produces a ~5-10 MB executable without symbolic computation features.

### Dynamic Linking (Not Recommended)

Dynamic linking can reduce the main executable size but requires distributing additional DLLs:

```
madola.exe:      ~10 MB
symengine.dll:   ~100 MB
Total:           ~110 MB (same total size, but split)
```

**Recommendation**: Use static linking for easier distribution unless you have specific requirements for dynamic linking.

## Build Instructions

### Building with SymEngine

**Windows:**
```cmd
# Initialize submodules
git submodule update --init --recursive

# Configure with SymEngine (default)
dev.bat configure

# Build
dev.bat build
```

**Unix/Linux:**
```bash
# Initialize submodules
git submodule update --init --recursive

# Configure with SymEngine (default)
./dev.sh configure

# Build
./dev.sh build
```

### Building without SymEngine

**Windows:**
```cmd
# Configure without SymEngine
cmake -B build -DWITH_SYMENGINE=OFF

# Build
cmake --build build
```

**Unix/Linux:**
```bash
# Configure without SymEngine
cmake -B build -DWITH_SYMENGINE=OFF

# Build
cmake --build build
```

## Testing Symbolic Features

### Test Files

MADOLA includes several test files for symbolic computation:

- [`test_diff_simple.mda`](../test_diff_simple.mda) - Basic differentiation
- [`test_diff.mda`](../test_diff.mda) - Comprehensive differentiation tests
- [`test_symbolic.mda`](../test_symbolic.mda) - General symbolic operations
- [`test_differential.mda`](../test_differential.mda) - Differential equations

### Running Tests

```bash
# Run a symbolic test
./dist/madola test_diff_simple.mda

# Generate HTML output
./dist/madola test_diff.mda --html > test_diff.html
```

## Troubleshooting

### Build Errors

**Issue**: SymEngine build fails with Boost errors

**Solution**: Ensure Boost submodule is initialized:
```bash
git submodule update --init --recursive
```

**Issue**: Missing Boost headers during compilation

**Solution**: The CMake configuration automatically sets up Boost include paths. If you encounter issues, verify that `external/boost/` exists and contains the Boost library.

### Runtime Errors

**Issue**: Symbolic differentiation fails with "Cannot convert expression to symbolic form"

**Solution**: Ensure the expression uses supported operations. Currently supported:
- Arithmetic: `+`, `-`, `*`, `/`, `^`
- Functions: `sin`, `cos`, `tan`, `exp`, `log`, `sqrt`

**Issue**: Undefined reference to SymEngine symbols

**Solution**: Ensure you're linking against the SymEngine library. This should be automatic if `WITH_SYMENGINE=ON`.

## Performance Considerations

### Compilation Time

SymEngine increases compilation time significantly:
- **Without SymEngine**: ~30 seconds
- **With SymEngine**: ~5-10 minutes (first build)
- **Incremental builds**: ~30 seconds (if SymEngine unchanged)

### Runtime Performance

Symbolic operations are generally fast:
- Simple differentiation: < 1 ms
- Complex expressions: 1-10 ms
- Matrix operations: 10-100 ms (depending on size)

### Memory Usage

SymEngine uses dynamic memory allocation for symbolic expressions:
- Small expressions: < 1 KB
- Complex expressions: 1-10 KB
- Large symbolic matrices: 10-100 KB

## Future Enhancements

### Planned Features

1. **Symbolic Integration**
   ```madola
   result := integrate(x^2, x);  # Returns: x^3/3
   ```

2. **Equation Solving**
   ```madola
   solution := solve(x^2 - 4 = 0, x);  # Returns: [-2, 2]
   ```

3. **Expression Simplification**
   ```madola
   simplified := simplify((x+1)^2 - (x^2 + 2*x + 1));  # Returns: 0
   ```

4. **Symbolic Matrix Operations**
   ```madola
   A := [x, 1; 0, x];
   det := A.det();  # Returns: x^2
   ```

### Performance Optimizations

- **GMP Integration**: Enable GMP for faster arbitrary-precision arithmetic
- **Expression Caching**: Cache frequently used symbolic expressions
- **Lazy Evaluation**: Defer symbolic operations until needed

## References

- [SymEngine Documentation](https://github.com/symengine/symengine/wiki)
- [SymEngine API Reference](https://github.com/symengine/symengine/wiki/API-Reference)
- [Boost.Multiprecision Documentation](https://www.boost.org/doc/libs/release/libs/multiprecision/)
- [GMP Documentation](https://gmplib.org/manual/)

## See Also

- [CMake Build System](CMAKE_SYSTEM.md) - Detailed CMake configuration
- [Dependencies](DEPENDENCIES.md) - Complete dependency list
- [Build Configuration](BUILD_CONFIGURATION.md) - Build options and flags
