# WASM Import Regression Tests

This document explains how WASM imports work in regression tests for both native and WASM builds.

## Overview

The regression test suite now includes tests for WASM module imports in both environments:
- **Native executable** tests (`run_regression.bat native`)
- **WASM runtime** tests (`run_regression.bat wasm`)

## Test Files

### WASM Import Test

**File:** `fixtures/wasm_import.mda`

```madola
// WASM Import Test
// Test importing functions from WASM modules in the same directory

from testWasm import testSquare, testDouble;

// Test WASM square function
result1 := testSquare(5);
print(result1);

// Test WASM double function
result2 := testDouble(7);
print(result2);

// Use WASM functions in expressions
result3 := testSquare(3) + testDouble(4);
print(result3);

// Test with larger numbers
result4 := testSquare(100);
print(result4);
```

### WASM Module Files

**Location:** `fixtures/testWasm/`

```
fixtures/
├── wasm_import.mda          # Test file
└── testWasm/                # WASM module directory
    ├── testSquare.js        # JavaScript wrapper
    ├── testSquare.wasm      # WASM binary
    ├── testDouble.js        # JavaScript wrapper
    └── testDouble.wasm      # WASM binary
```

**Important:** WASM files are in the same directory as the test file, allowing tests to import from the local directory.

## How It Works

### Native Executable

The WASM loader (`src/core/generator/wasm_addon_loader.cpp`) searches for WASM modules in:

1. **Current directory** (priority 1)
   - `./testWasm/testSquare.{js,wasm}`
   - Enables regression tests to use local WASM files

2. **~/.madola/trove/** (priority 2)
   - `~/.madola/trove/testWasm/testSquare.{js,wasm}`
   - Standard location for user modules

### WASM Runtime

The WASM runtime uses the web app's manifest system:

1. **Manifest registration** - `web/trove/manifest.json` lists modules
2. **Preloading** - Modules loaded into `window.madolaWasmBridge`
3. **Import resolution** - Functions called through bridge

## Directory Structure

### Regression Tests (Native)

```
regression/
├── fixtures/
│   ├── wasm_import.mda         # Test that imports WASM
│   └── testWasm/               # WASM module (same dir as test)
│       ├── testSquare.js
│       ├── testSquare.wasm
│       ├── testDouble.js
│       └── testDouble.wasm
├── expected/
│   ├── evaluation/
│   │   └── wasm_import.txt     # Expected console output
│   └── html/
│       └── wasm_import.html    # Expected HTML output
└── results/                    # Generated during test runs
```

### Web App Tests (WASM)

```
web/
└── trove/
    ├── manifest.json           # Must include testWasm
    └── testWasm/               # Copy of regression test module
        ├── testSquare.js
        ├── testSquare.wasm
        ├── testDouble.js
        └── testDouble.wasm
```

## Running Tests

### Native Tests

```bash
# Run all native tests (including WASM imports)
.\regression\run_regression.bat native

# Run and update baselines
.\regression\run_regression.bat native update
```

**Output:**
```
Running regression tests in native mode...
Running test: wasm_import
Ran 18 test cases

Comparing results...
[PASS] wasm_import.txt (evaluation)
[PASS] wasm_import.html (html)

Results: 32 passed, 0 failed
All tests passed!
```

### WASM Tests

```bash
# Run all WASM tests (including WASM imports)
.\regression\run_regression.bat wasm

# Run and update baselines
.\regression\run_regression.bat wasm update
```

## Creating WASM Module for Tests

### 1. Create Generator File

**File:** `fixtures/test_wasm_generator.mda`

```madola
@gen_addon
fn testSquare(n) {
    return n * n;
}

@gen_addon
fn testDouble(n) {
    return n * 2;
}
```

### 2. Generate WASM Files

```bash
# Generate WASM files
dist/madola.exe regression/fixtures/test_wasm_generator.mda

# Files created in ~/.madola/trove/test_wasm_generator/
```

### 3. Copy to Regression Fixtures

```bash
# Create module directory
mkdir regression/fixtures/testWasm

# Copy WASM files
copy %USERPROFILE%\.madola\trove\test_wasm_generator\testSquare.* regression\fixtures\testWasm\
copy %USERPROFILE%\.madola\trove\test_wasm_generator\testDouble.* regression\fixtures\testWasm\
```

### 4. Copy to Web App (for WASM tests)

```bash
# Create module directory
mkdir web/trove/testWasm

# Copy WASM files
copy regression\fixtures\testWasm\* web\trove\testWasm\

# Update manifest
# Edit web/trove/manifest.json:
{
  "testWasm": ["testSquare", "testDouble"]
}
```

## Expected Test Output

### Evaluation Output

**File:** `expected/evaluation/wasm_import.txt`

```
Imported WASM function: testSquare from testWasm
Imported WASM function: testDouble from testWasm
Execution completed successfully
Output:
  25
  14
  17
  10000
```

### HTML Output

**File:** `expected/html/wasm_import.html`

Contains formatted HTML with:
- Function import messages
- Computation results
- Formatted mathematical expressions

## Key Implementation Details

### 1. WASM Loader Search Path

**Code:** `src/core/generator/wasm_addon_loader.cpp`

```cpp
std::vector<std::filesystem::path> searchPaths = {
    // Check current directory first (for regression tests)
    std::filesystem::current_path() / moduleName,
    // Check ~/.madola/trove/ (standard location)
    std::filesystem::path(utils::getTroveDirectory(moduleName))
};
```

**Benefits:**
- Regression tests can use local WASM files
- No need to install test modules globally
- Clean test isolation

### 2. Test Execution Context

**Regression script runs from fixtures directory:**

```bat
pushd "%FIXTURES_DIR%"
"!MADOLA_CMD!" "!base_name!.mda" > "%OUTPUT_DIR%\evaluation\!base_name!.txt"
popd
```

**Result:**
- Current directory = `regression/fixtures/`
- Test can import from `./testWasm/`
- WASM loader finds files in current directory

## Advantages of This Approach

### ✅ Test Isolation
- Each test has its own WASM modules
- No global state pollution
- Tests can use different module versions

### ✅ Portability
- WASM files committed with test files
- No external dependencies
- Works on any machine with MADOLA built

### ✅ Consistency
- Same WASM files used for native and WASM tests
- Same import syntax in both environments
- Identical expected outputs

### ✅ Simplicity
- No special setup required
- Tests work immediately after clone
- Clear directory structure

## Troubleshooting

### Test Fails with "Cannot find module"

**Problem:** WASM loader can't find module files

**Solution:**
1. Check `fixtures/testWasm/` exists
2. Verify `testSquare.js` and `testSquare.wasm` are present
3. Ensure test runs from fixtures directory

### Test Passes Native but Fails WASM

**Problem:** Web manifest not updated

**Solution:**
1. Copy WASM files to `web/trove/testWasm/`
2. Update `web/trove/manifest.json`:
   ```json
   {
     "testWasm": ["testSquare", "testDouble"]
   }
   ```
3. Rebuild WASM: `dev.bat wasm`

### Expected Output Mismatch

**Problem:** Timing or formatting differences

**Solution:**
```bash
# Update expected outputs
.\regression\run_regression.bat native update
.\regression\run_regression.bat wasm update
```

## Future Enhancements

- [ ] Add multi-parameter WASM function tests
- [ ] Test WASM modules with dependencies
- [ ] Test error handling for missing WASM files
- [ ] Add performance benchmarks (WASM vs interpreted)

## Summary

The regression test suite now fully supports WASM imports:
- ✅ Native executable tests WASM imports from local directory
- ✅ WASM runtime tests WASM imports via bridge
- ✅ Same test files for both environments
- ✅ WASM files co-located with test files
- ✅ Automated comparison of expected vs actual results

WASM import testing is production-ready! 🚀

