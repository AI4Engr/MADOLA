# MADOLA Web Runtime

Web-based runtime for MADOLA (Math Domain Language) with WASM support.

## Architecture

### WASM Preloading System

The web runtime uses a **manifest-based preloading** system to automatically load all WASM modules before code execution:

1. **Manifest Discovery** (`web/trove/manifest.json`)
   - Lists all available WASM modules and their functions
   - Scanned at startup by `setupWasmBridge()`

2. **Automatic Preloading** (`web/js/app.js`)
   - Reads `trove/manifest.json`
   - Loads all WASM modules into `window.madolaWasmBridge`
   - Happens before any user code executes

3. **Dynamic Path Resolution**
   - Wrapper files accept `basePath` parameter
   - Browser: Uses relative paths from `web/index.html`
   - Node.js: Uses `__dirname` for module location

### Directory Structure

```
web/
├── index.html              # Main web application
├── runtime/                # WASM runtime files (build output)
│   ├── madola.js          # MADOLA WASM module
│   └── madola.wasm        # MADOLA WASM binary
├── trove/                 # WASM function library
│   ├── manifest.json      # Module/function registry (REQUIRED for web app)
│   └── [moduleName]/      # Module directories
│       ├── [func].js      # JavaScript wrapper
│       └── [func].wasm    # WASM binary
├── js/                    # JavaScript source
│   ├── app.js            # Main application
│   └── madola-browser.js # WASM wrapper
└── css/                   # Styles
    └── styles.css
```

**📖 For detailed information about WASM imports and directory structure, see [WASM Complete Guide](../docs/WASM_COMPLETE_GUIDE.md)**

## How WASM Preloading Works

### 1. Manifest File (`web/trove/manifest.json`)

```json
{
  "example": ["calcPi", "add", "minus"],
  "module2": ["func1", "func2"]
}
```

### 2. Startup Sequence

```javascript
// app.js - setupWasmBridge()
async setupWasmBridge() {
    window.madolaWasmBridge = {};

    // Read manifest
    const manifest = await fetch('trove/manifest.json').then(r => r.json());

    // Preload all modules
    for (const [moduleName, functions] of Object.entries(manifest)) {
        for (const functionName of functions) {
            const addon = await loadWasmFunctionWithPath(
                functionName,
                `trove/${moduleName}/${functionName}.js`,
                `trove/${moduleName}`
            );
            window.madolaWasmBridge[functionName] = (n) => addon[functionName](n);
        }
    }
}
```

### 3. Import Resolution

When MADOLA code uses imports:

```madola
from example import calcPi;

x := calcPi(100000);
print(x);
```

The MADOLA WASM runtime calls the pre-loaded function via `window.madolaWasmBridge[calcPi]`.

### 4. Wrapper Generation

WASM wrappers are generated via `@gen_addon` decorator:

```madola
@gen_addon
fn calcPi(n) {
    // Implementation
    return result;
}
```

Running `./dist/madola.exe gen_calcPi.mda` generates:
- `web/trove/[module]/calcPi.wasm`
- `web/trove/[module]/calcPi.js` (with basePath support)

## Building and Deployment

### Build WASM Runtime

```bash
npm run build:wasm
```

This outputs directly to `web/runtime/`:
- `web/runtime/madola.js`
- `web/runtime/madola.wasm`

### Generate WASM Functions

1. Create a `.mda` file with `@gen_addon`:

```madola
@gen_addon
fn myFunc(n) {
    return n * 2;
}
```

2. Generate WASM:

```bash
./dist/madola.exe myFunc.mda
```

This creates files in `~/.madola/trove/myModule/`:
- `myFunc.wasm`
- `myFunc.js`
- `myFunc.cpp`

3. **Copy to web app:**

```bash
# Windows
xcopy /E /I %USERPROFILE%\.madola\trove\myModule web\trove\myModule

# Linux/Mac
cp -r ~/.madola/trove/myModule web/trove/myModule
```

4. **Update manifest (`web/trove/manifest.json`):**

```json
{
  "myModule": ["myFunc"],
  "calcPi": ["calcPi"]
}
```

**⚠️ IMPORTANT:** The web app **requires** `manifest.json` to be updated for WASM imports to work. Without this, the module will not be loaded.

### Run Web App

Open `web/index.html` in a browser or use a local server:

```bash
npx http-server web -p 8080
```

## Testing

### Regression Tests

```bash
# Test WASM runtime
npm run test:regression:wasm

# Test native (for comparison)
npm run test:regression:native
```

Both modes should produce identical output.

## Key Features

- **Zero manual configuration** - Manifest-driven auto-loading
- **No hard-coded paths** - Dynamic path resolution
- **Generator-based** - All wrappers created via `@gen_addon`
- **Browser + Node.js** - Unified wrapper works in both environments
- **Import-compatible** - Seamless `from module import func` syntax