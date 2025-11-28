# MADOLA Electron App

This is the Electron wrapper for the MADOLA Web App, providing a native desktop experience.

## Features

- Native desktop application for Windows, macOS, and Linux
- Full integration with operating system (file associations, native menus, etc.)
- Loads the MADOLA web app with WASM support
- File operations through native dialogs
- Keyboard shortcuts and menu integration

## Development

### Prerequisites

Make sure you have Node.js installed, then install dependencies:

```bash
cd electron
npm install
```

### Running in Development

```bash
npm start
```

This will start the Electron app in development mode with DevTools enabled.

### Building for Production

Build for current platform:
```bash
npm run build
```

Build for specific platforms:
```bash
npm run build-win    # Windows
npm run build-mac    # macOS
npm run build-linux  # Linux
```

The built application will be available in the `dist/` directory.

## File Structure

- `main.js` - Main Electron process, handles window creation and native menus
- `preload.js` - Preload script for secure communication between main and renderer
- `package.json` - NPM configuration with build settings
- `../web/` - The web app that gets loaded in the Electron window

## Features

### Native Integration

- **File Menu**: New, Open, Save, Save As with native file dialogs
- **Edit Menu**: Standard edit operations (Cut, Copy, Paste, etc.)
- **Code Menu**: Run and Format commands
- **View Menu**: Zoom, reload, and DevTools access
- **Help Menu**: About dialog

### Keyboard Shortcuts

- `Ctrl+N` / `Cmd+N` - New file
- `Ctrl+O` / `Cmd+O` - Open file
- `Ctrl+S` / `Cmd+S` - Save file
- `Ctrl+Shift+S` / `Cmd+Shift+S` - Save As
- `F5` - Run code
- `Ctrl+Shift+F` / `Cmd+Shift+F` - Format code
- Standard edit shortcuts (Ctrl+C, Ctrl+V, etc.)

### Build Configuration

The app is configured to build for multiple platforms:

- **Windows**: NSIS installer (.exe)
- **macOS**: DMG package (.dmg)
- **Linux**: AppImage (.AppImage)

### Security

- Context isolation enabled
- Node integration disabled in renderer
- Secure IPC communication through preload script
- Web security partially disabled to allow local WASM loading