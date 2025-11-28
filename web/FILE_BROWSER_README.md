# File Browser Implementation

## Overview

Added a file browser to the web app's left sidebar that displays:
1. **Generated C++** files from `web/gen_cpp/`
2. **WASM Modules** from `web/trove/`

## Features

### ✅ Server-Side File Listing
- **API Endpoint**: `GET /api/files/cpp` - Lists C++ files with metadata
- **API Endpoint**: `GET /api/files/wasm` - Lists WASM modules with files
- **API Endpoint**: `GET /api/files/cpp/{filename}` - View C++ file content

### ✅ User Interface
- Located in left sidebar **Files** tab (📂 icon)
- Two collapsible sections:
  - 📄 **Generated C++** - Shows `.cpp` files
  - 🔧 **WASM Modules** - Shows modules with `.js` and `.wasm` files
- File icons for easy identification:
  - 📄 C++ files
  - 📦 Module folders
  - 📜 JavaScript wrappers
  - ⚙️ WASM binaries
- **Refresh button** (🔄) to reload file lists
- **File sizes** displayed next to each file

### ✅ Interactivity
- Click C++ files to view content in modal dialog
- Click section headers to expand/collapse
- Click module headers to expand/collapse module files
- Hover effects for better UX

## File Structure

### New Files Created
```
web/
├── serve.js                    (UPDATED: Added 3 API endpoints)
├── js/
│   └── app-file-browser.js     (NEW: File browser logic)
├── css/
│   └── styles.css              (UPDATED: Added file browser styles)
└── index.html                  (UPDATED: Added file browser UI)
```

### API Endpoints

#### 1. List C++ Files
```
GET /api/files/cpp

Response:
{
  "success": true,
  "files": [
    {
      "name": "calcPi.cpp",
      "size": 1234,
      "modified": "2025-10-22T10:30:00.000Z"
    },
    ...
  ]
}
```

#### 2. List WASM Modules
```
GET /api/files/wasm

Response:
{
  "success": true,
  "modules": [
    {
      "name": "calcPi",
      "files": [
        {
          "name": "calcPi.js",
          "type": "js",
          "size": 2011,
          "modified": "2025-10-22T10:30:00.000Z"
        },
        {
          "name": "calcPi.wasm",
          "type": "wasm",
          "size": 7574,
          "modified": "2025-10-22T10:30:00.000Z"
        }
      ]
    },
    ...
  ]
}
```

#### 3. View C++ File
```
GET /api/files/cpp/calcPi.cpp

Response:
{
  "success": true,
  "content": "// C++ code here...",
  "filename": "calcPi.cpp"
}
```

## Usage

### For Users

1. **Open Web App**: Start server with `dev.bat serve` or `./dev.sh serve`
2. **Navigate to Files Tab**: Click the 📂 icon in left sidebar
3. **Browse Files**:
   - See all generated C++ files
   - See all WASM modules with their components
4. **View C++ Code**: Click any `.cpp` file to see its content
5. **Refresh**: Click 🔄 button to reload file lists

### For Developers

When you use `@gen_addon` decorator and generate WASM:
1. C++ source appears in `web/gen_cpp/`
2. WASM files appear in `web/trove/[moduleName]/`
3. Files immediately visible in browser after refresh

## Technical Details

### Client-Side (JavaScript)

**File**: `web/js/app-file-browser.js`

```javascript
class FileBrowser {
    async loadFiles()        // Fetches file lists from API
    renderCppFiles()         // Renders C++ file list
    renderWasmModules()      // Renders WASM module list
    viewCppFile(filename)    // Opens C++ file in modal
    toggleModule(name)       // Expands/collapses module
}
```

### Server-Side (Node.js)

**File**: `web/serve.js`

- Scans `web/gen_cpp/` for `.cpp` files
- Scans `web/trove/` subdirectories for WASM modules
- Returns file metadata (name, size, modified date)
- Serves file content on demand

### Styling

**File**: `web/css/styles.css`

- Clean, modern file browser design
- Collapsible sections with smooth transitions
- Hover effects and visual feedback
- Modal dialog for code viewing
- File icons for easy recognition

## Benefits

1. **Transparency**: See exactly what files are generated
2. **Debugging**: Quickly inspect generated C++ code
3. **Verification**: Confirm WASM modules are in place
4. **Organization**: Understand project structure at a glance
5. **Convenience**: No need to browse filesystem manually

## Future Enhancements

Possible improvements:
- [ ] Syntax highlighting in code modal
- [ ] Download button for files
- [ ] Delete file functionality
- [ ] Search/filter files
- [ ] View WASM module manifest info
- [ ] Show last modified date in human-readable format
- [ ] Display WASM file binary size comparison

## Testing

1. Generate a WASM module with `@gen_addon`
2. Open web app: `http://localhost:8080`
3. Click Files tab (📂)
4. Verify C++ appears in "Generated C++"
5. Verify WASM appears in "WASM Modules"
6. Click a C++ file to view content
7. Click refresh button to reload

---

*Implementation completed: 2025-10-22*

