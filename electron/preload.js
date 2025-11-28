const { contextBridge, ipcRenderer } = require('electron');

// Expose protected methods that allow the renderer process to use
// the ipcRenderer without exposing the entire object
contextBridge.exposeInMainWorld('electronAPI', {
    // File operations
    saveFile: (data) => ipcRenderer.invoke('save-file', data),
    saveCppFile: (data) => ipcRenderer.invoke('save-cpp-file', data),
    getCurrentFile: () => ipcRenderer.invoke('get-current-file'),
    setCurrentFile: (filePath) => ipcRenderer.invoke('set-current-file', filePath),

    // Window operations
    updateTitle: (title) => ipcRenderer.send('update-title', title),

    // Listeners for main process events
    onMenuAction: (callback) => {
        ipcRenderer.on('menu-action', (event, action) => callback(action));
    },

    onFileOpened: (callback) => {
        ipcRenderer.on('file-opened', (event, fileData) => callback(fileData));
    },

    onSaveAs: (callback) => {
        ipcRenderer.on('save-as', (event, filePath) => callback(filePath));
    },

    // Remove listeners
    removeAllListeners: (channel) => {
        ipcRenderer.removeAllListeners(channel);
    },

    // Platform info
    platform: process.platform,

    // Check if running in Electron
    isElectron: true,

    // File browser operations
    getCppFiles: () => ipcRenderer.invoke('get-cpp-files'),
    getWasmModules: () => ipcRenderer.invoke('get-wasm-modules'),
    getCppFileContent: (filename) => ipcRenderer.invoke('get-cpp-file-content', filename)
});