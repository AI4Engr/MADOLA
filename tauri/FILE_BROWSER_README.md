# Tauri File Browser Integration

The MADOLA Tauri desktop app now includes a file browser that lists C++ files and WASM modules from the native file system.

## Features

### Native File System Access
The Tauri app reads files directly from the user's home directory:
- **C++ files**: `~/.madola/gen_cpp/` - Lists all `.cpp` files
- **WASM modules**: `~/.madola/trove/` - Lists directories containing `.wasm` and `.js` files

### Tauri Commands

Three Rust commands are exposed to the frontend via Tauri's IPC:

#### 1. `get_cpp_files`
Lists all C++ files from `~/.madola/gen_cpp/`

```javascript
const result = await window.__TAURI__.invoke('get_cpp_files');
// Returns: { success: true, files: [...], error?: string }
```

**Response format**:
```json
{
  "success": true,
  "files": [
    {
      "name": "calcPi.cpp",
      "size": 1234,
      "modified": "SystemTime { ... }"
    }
  ]
}
```

#### 2. `get_wasm_modules`
Lists WASM modules (directories with `.wasm` and `.js` files) from `~/.madola/trove/`

```javascript
const result = await window.__TAURI__.invoke('get_wasm_modules');
// Returns: { success: true, modules: [...], error?: string }
```

**Response format**:
```json
{
  "success": true,
  "modules": [
    {
      "name": "calcPi",
      "files": [
        { "name": "calcPi.wasm", "type": "wasm", "size": 5678, "modified": "..." },
        { "name": "calcPi.js", "type": "js", "size": 2345, "modified": "..." }
      ]
    }
  ]
}
```

#### 3. `get_cpp_file_content`
Reads the content of a specific C++ file

```javascript
const result = await window.__TAURI__.invoke('get_cpp_file_content', { 
  filename: 'calcPi.cpp' 
});
// Returns: { success: true, content: "...", filename: "...", error?: string }
```

**Response format**:
```json
{
  "success": true,
  "content": "// C++ code here...",
  "filename": "calcPi.cpp"
}
```

## Implementation Details

### Rust Backend (`tauri/src/main.rs`)

The backend uses:
- **`dirs`** crate: To get the user's home directory (`dirs::home_dir()`)
- **`std::fs`**: For file system operations
- **`serde`**: For JSON serialization/deserialization

Key features:
- Automatic directory creation if `~/.madola/gen_cpp` or `~/.madola/trove` don't exist
- Sorted file/module lists by name
- Error handling with descriptive messages
- Type-safe data structures

### Frontend (`web/js/app-file-browser.js`)

The frontend detects the environment and uses the appropriate API:

```javascript
const isTauri = window.__TAURI__ !== undefined;

if (isTauri) {
    // Use Tauri IPC
    data = await window.__TAURI__.invoke('get_cpp_files');
} else if (isElectron) {
    // Use Electron IPC
    data = await window.electronAPI.getCppFiles();
} else {
    // Use HTTP API
    const response = await fetch('/api/files/cpp');
    data = await response.json();
}
```

This allows the same JavaScript code to work in:
1. **Tauri app**: Native file system access via Rust
2. **Electron app**: Native file system access via Node.js
3. **Web browser**: HTTP API via Node.js server

## UI Features

The file browser appears in the left sidebar under the "Files" tab:

- **📄 Generated C++**: Collapsible section showing all `.cpp` files
  - Click any file to view its content in a modal
  - Shows file name and size

- **📦 WASM Modules**: Collapsible section showing module directories
  - Each module can be expanded to show its files (`.wasm` and `.js`)
  - Shows file names, types (⚙️ for `.wasm`, 📜 for `.js`), and sizes

- **🔄 Refresh button**: Reloads the file list from the file system

## Dependencies

Add to `tauri/Cargo.toml`:
```toml
[dependencies]
dirs = "5.0"
```

## Building

To build the Tauri app with file browser support:

```bash
cd tauri
npm install        # Install Node.js dependencies
npm run tauri dev  # Run in development mode
npm run tauri build # Build production app
```

## Platform Support

The file browser works on all platforms supported by Tauri:
- **Windows**: `C:\Users\<username>\.madola\`
- **macOS**: `/Users/<username>/.madola/`
- **Linux**: `/home/<username>/.madola/`

The `dirs` crate handles cross-platform path resolution automatically.

## Comparison: Tauri vs Electron

| Feature | Tauri | Electron |
|---------|-------|----------|
| **Language** | Rust | Node.js |
| **Bundle Size** | ~3-10 MB | ~50-100 MB |
| **Memory Usage** | Lower | Higher |
| **Startup Time** | Faster | Slower |
| **Security** | More secure (no Node.js runtime in frontend) | Less secure |
| **File Access** | Via Rust commands | Via IPC to Node.js |
| **Build Time** | Slower (Rust compilation) | Faster |

Both implementations provide the same user experience, but Tauri offers better performance and security.

