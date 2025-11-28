# MADOLA Tauri Desktop Application

This directory contains the Tauri-based desktop application for MADOLA.

## Prerequisites

- Rust (latest stable): https://rustup.rs/
- Node.js (for Tauri CLI)
- System dependencies for Tauri:
  - **Windows**: Microsoft C++ Build Tools, WebView2
  - **macOS**: Xcode Command Line Tools
  - **Linux**: See https://tauri.app/v1/guides/getting-started/prerequisites#setting-up-linux

## Development

```bash
# Install dependencies
npm install

# Run in development mode
npm run dev

# Build for production
npm run build

# Build debug version
npm run build-debug
```

## Build Output

Production builds will be located in:
- `target/release/bundle/` (platform-specific installers)

## Architecture

- **Rust Backend**: `src/main.rs` - Handles file I/O, native dialogs, window management
- **Frontend**: `../web/` - Web-based UI using MADOLA WASM runtime
- **Configuration**: `tauri.conf.json` - App settings, permissions, bundle configuration

## Features

- Native file dialogs (Open, Save)
- File system access with security scoping
- Native window controls
- Menu integration
- File drag-and-drop support
- Cross-platform builds (Windows, macOS, Linux)
- **File Browser**: Lists C++ files from `~/.madola/gen_cpp` and WASM modules from `~/.madola/trove`
  - View generated C++ code directly in the app
  - Browse available WASM modules
  - See [FILE_BROWSER_README.md](FILE_BROWSER_README.md) for details

## Comparison with Electron

**Advantages:**
- Smaller binary size (~3-10 MB vs ~100+ MB)
- Lower memory footprint
- Better performance (native Rust backend)
- More secure (explicit permissions)
- Faster startup time

**Trade-offs:**
- Requires Rust toolchain for development
- Smaller ecosystem compared to Electron
- Platform-specific builds require native development tools