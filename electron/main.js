const { app, BrowserWindow, Menu, ipcMain, dialog } = require('electron');
const path = require('path');
const fs = require('fs');

class MadolaElectronApp {
    constructor() {
        this.mainWindow = null;
        this.currentFile = null;
    }

    createWindow() {
        // Create the browser window
        this.mainWindow = new BrowserWindow({
            width: 1400,
            height: 900,
            minWidth: 800,
            minHeight: 600,
            webPreferences: {
                nodeIntegration: false,
                contextIsolation: true,
                enableRemoteModule: false,
                preload: path.join(__dirname, 'preload.js'),
                webSecurity: false // Allow loading local WASM files
            },
            icon: path.join(__dirname, 'assets', 'icon.png'),
            show: false,
            titleBarStyle: 'default'
        });

        // Load the web app
        let webAppPath;
        if (app.isPackaged) {
            // In production, files are in extraResources
            webAppPath = path.join(process.resourcesPath, 'web', 'index.html');
        } else {
            // In development, files are in the parent directory
            webAppPath = path.join(__dirname, '..', 'web', 'index.html');
        }

        console.log('Loading web app from:', webAppPath);
        this.mainWindow.loadFile(webAppPath);

        // Show window when ready to prevent visual flash
        this.mainWindow.once('ready-to-show', () => {
            this.mainWindow.show();
        });

        // Handle window closed
        this.mainWindow.on('closed', () => {
            this.mainWindow = null;
        });

        // Create application menu
        this.createMenu();

        // Development mode - open DevTools
        if (process.env.NODE_ENV === 'development') {
            this.mainWindow.webContents.openDevTools();
        }
    }

    createMenu() {
        const template = [
            {
                label: 'File',
                submenu: [
                    {
                        label: 'New',
                        accelerator: 'CmdOrCtrl+N',
                        click: () => this.newFile()
                    },
                    {
                        label: 'Open...',
                        accelerator: 'CmdOrCtrl+O',
                        click: () => this.openFile()
                    },
                    { type: 'separator' },
                    {
                        label: 'Save',
                        accelerator: 'CmdOrCtrl+S',
                        click: () => this.saveFile()
                    },
                    {
                        label: 'Save As...',
                        accelerator: 'CmdOrCtrl+Shift+S',
                        click: () => this.saveAsFile()
                    },
                    { type: 'separator' },
                    {
                        label: 'Exit',
                        accelerator: process.platform === 'darwin' ? 'Cmd+Q' : 'Ctrl+Q',
                        click: () => app.quit()
                    }
                ]
            },
            {
                label: 'Edit',
                submenu: [
                    { role: 'undo' },
                    { role: 'redo' },
                    { type: 'separator' },
                    { role: 'cut' },
                    { role: 'copy' },
                    { role: 'paste' },
                    { role: 'selectall' }
                ]
            },
            {
                label: 'Code',
                submenu: [
                    {
                        label: 'Run',
                        accelerator: 'F5',
                        click: () => this.runCode()
                    },
                    {
                        label: 'Format',
                        accelerator: 'CmdOrCtrl+Shift+F',
                        click: () => this.formatCode()
                    }
                ]
            },
            {
                label: 'View',
                submenu: [
                    { role: 'reload' },
                    { role: 'forceReload' },
                    { role: 'toggleDevTools' },
                    { type: 'separator' },
                    { role: 'resetZoom' },
                    { role: 'zoomIn' },
                    { role: 'zoomOut' },
                    { type: 'separator' },
                    { role: 'togglefullscreen' }
                ]
            },
            {
                label: 'Help',
                submenu: [
                    {
                        label: 'About MADOLA',
                        click: () => this.showAbout()
                    }
                ]
            }
        ];

        // macOS specific menu adjustments
        if (process.platform === 'darwin') {
            template.unshift({
                label: app.getName(),
                submenu: [
                    { role: 'about' },
                    { type: 'separator' },
                    { role: 'services' },
                    { type: 'separator' },
                    { role: 'hide' },
                    { role: 'hideOthers' },
                    { role: 'unhide' },
                    { type: 'separator' },
                    { role: 'quit' }
                ]
            });

            // Remove Exit from File menu on macOS
            template[1].submenu.pop();
            template[1].submenu.pop(); // Remove separator too
        }

        const menu = Menu.buildFromTemplate(template);
        Menu.setApplicationMenu(menu);
    }

    // File operations
    newFile() {
        this.mainWindow.webContents.send('menu-action', 'new');
    }

    async openFile() {
        const result = await dialog.showOpenDialog(this.mainWindow, {
            title: 'Open MADOLA File',
            filters: [
                { name: 'MADOLA Files', extensions: ['mda'] },
                { name: 'Text Files', extensions: ['txt'] },
                { name: 'All Files', extensions: ['*'] }
            ],
            properties: ['openFile']
        });

        if (!result.canceled && result.filePaths.length > 0) {
            const filePath = result.filePaths[0];
            try {
                const content = fs.readFileSync(filePath, 'utf8');
                this.currentFile = filePath;
                this.mainWindow.webContents.send('file-opened', {
                    path: filePath,
                    name: path.basename(filePath),
                    content: content
                });
            } catch (error) {
                dialog.showErrorBox('Error Opening File', `Could not open file: ${error.message}`);
            }
        }
    }

    async saveFile() {
        if (this.currentFile) {
            // Get content from renderer and save to current file
            this.mainWindow.webContents.send('menu-action', 'save');
        } else {
            this.saveAsFile();
        }
    }

    async saveAsFile() {
        const result = await dialog.showSaveDialog(this.mainWindow, {
            title: 'Save MADOLA File',
            defaultPath: this.currentFile || 'untitled.mda',
            filters: [
                { name: 'MADOLA Files', extensions: ['mda'] },
                { name: 'Text Files', extensions: ['txt'] },
                { name: 'All Files', extensions: ['*'] }
            ]
        });

        if (!result.canceled) {
            this.currentFile = result.filePath;
            this.mainWindow.webContents.send('save-as', result.filePath);
        }
    }

    // Code operations
    runCode() {
        this.mainWindow.webContents.send('menu-action', 'run');
    }

    formatCode() {
        this.mainWindow.webContents.send('menu-action', 'format');
    }

    showAbout() {
        this.mainWindow.webContents.send('menu-action', 'about');
    }

    // Application lifecycle
    init() {
        // Handle app ready
        app.whenReady().then(() => {
            this.createWindow();

            app.on('activate', () => {
                // On macOS, re-create window when dock icon is clicked
                if (BrowserWindow.getAllWindows().length === 0) {
                    this.createWindow();
                }
            });
        });

        // Handle window closed
        app.on('window-all-closed', () => {
            // On macOS, keep app running even when all windows are closed
            if (process.platform !== 'darwin') {
                app.quit();
            }
        });

        // Setup IPC handlers
        this.setupIPC();

        // Handle file associations (when user double-clicks .mda file)
        app.on('open-file', (event, filePath) => {
            event.preventDefault();
            if (this.mainWindow) {
                this.openSpecificFile(filePath);
            } else {
                // Store file to open after window is created
                this.fileToOpen = filePath;
            }
        });
    }

    setupIPC() {
        // Handle file save from renderer
        ipcMain.handle('save-file', async (event, data) => {
            try {
                fs.writeFileSync(data.path, data.content, 'utf8');
                return { success: true };
            } catch (error) {
                return { success: false, error: error.message };
            }
        });

        // Handle C++ file save from renderer
        ipcMain.handle('save-cpp-file', async (event, data) => {
            try {
                // Determine the output directory for C++ files
                let outputDir;
                if (app.isPackaged) {
                    // In production, save to app's data directory
                    outputDir = path.join(app.getPath('userData'), 'gen_cpp');
                } else {
                    // In development, save to web/gen_cpp relative to project root
                    outputDir = path.join(__dirname, '..', 'web', 'gen_cpp');
                }

                // Create directory if it doesn't exist
                if (!fs.existsSync(outputDir)) {
                    fs.mkdirSync(outputDir, { recursive: true });
                }

                const filePath = path.join(outputDir, data.filename);
                fs.writeFileSync(filePath, data.content, 'utf8');
                return { success: true, path: filePath };
            } catch (error) {
                return { success: false, error: error.message };
            }
        });

        // Handle get current file path
        ipcMain.handle('get-current-file', () => {
            return this.currentFile;
        });

        // Handle set current file path
        ipcMain.handle('set-current-file', (event, filePath) => {
            this.currentFile = filePath;
            this.updateWindowTitle();
        });

        // Handle window title update
        ipcMain.on('update-title', (event, title) => {
            if (this.mainWindow) {
                this.mainWindow.setTitle(title);
            }
        });

        // File browser: List C++ files from ~/.madola/gen_cpp
        ipcMain.handle('get-cpp-files', async () => {
            try {
                const os = require('os');
                const genCppDir = path.join(os.homedir(), '.madola', 'gen_cpp');
                
                if (!fs.existsSync(genCppDir)) {
                    fs.mkdirSync(genCppDir, { recursive: true });
                }
                
                const files = fs.readdirSync(genCppDir);
                const cppFiles = files
                    .filter(f => f.endsWith('.cpp'))
                    .map(filename => {
                        const filePath = path.join(genCppDir, filename);
                        const stats = fs.statSync(filePath);
                        return {
                            name: filename,
                            size: stats.size,
                            modified: stats.mtime.toISOString()
                        };
                    })
                    .sort((a, b) => a.name.localeCompare(b.name));
                
                return { success: true, files: cppFiles };
            } catch (error) {
                return { success: false, error: error.message };
            }
        });

        // File browser: List WASM modules from ~/.madola/trove
        ipcMain.handle('get-wasm-modules', async () => {
            try {
                const os = require('os');
                const troveDir = path.join(os.homedir(), '.madola', 'trove');
                
                if (!fs.existsSync(troveDir)) {
                    fs.mkdirSync(troveDir, { recursive: true });
                }
                
                const modules = [];
                const entries = fs.readdirSync(troveDir, { withFileTypes: true });
                
                for (const entry of entries) {
                    if (entry.isDirectory()) {
                        const modulePath = path.join(troveDir, entry.name);
                        const moduleFiles = fs.readdirSync(modulePath);
                        
                        const wasmFile = moduleFiles.find(f => f.endsWith('.wasm'));
                        const jsFile = moduleFiles.find(f => f.endsWith('.js'));
                        
                        if (wasmFile || jsFile) {
                            const files = [];
                            
                            if (jsFile) {
                                const jsPath = path.join(modulePath, jsFile);
                                const jsStats = fs.statSync(jsPath);
                                files.push({
                                    name: jsFile,
                                    type: 'js',
                                    size: jsStats.size,
                                    modified: jsStats.mtime.toISOString()
                                });
                            }
                            
                            if (wasmFile) {
                                const wasmPath = path.join(modulePath, wasmFile);
                                const wasmStats = fs.statSync(wasmPath);
                                files.push({
                                    name: wasmFile,
                                    type: 'wasm',
                                    size: wasmStats.size,
                                    modified: wasmStats.mtime.toISOString()
                                });
                            }
                            
                            modules.push({
                                name: entry.name,
                                files: files
                            });
                        }
                    }
                }
                
                modules.sort((a, b) => a.name.localeCompare(b.name));
                
                return { success: true, modules: modules };
            } catch (error) {
                return { success: false, error: error.message };
            }
        });

        // File browser: Get C++ file content
        ipcMain.handle('get-cpp-file-content', async (event, filename) => {
            try {
                const os = require('os');
                const filePath = path.join(os.homedir(), '.madola', 'gen_cpp', filename);
                
                if (!fs.existsSync(filePath)) {
                    return { success: false, error: 'File not found' };
                }
                
                const content = fs.readFileSync(filePath, 'utf8');
                return { success: true, content: content, filename: filename };
            } catch (error) {
                return { success: false, error: error.message };
            }
        });
    }

    updateWindowTitle() {
        if (this.mainWindow && this.currentFile) {
            const fileName = path.basename(this.currentFile);
            this.mainWindow.setTitle(`${fileName} - MADOLA Editor`);
        }
    }

    async openSpecificFile(filePath) {
        try {
            const content = fs.readFileSync(filePath, 'utf8');
            this.currentFile = filePath;
            this.mainWindow.webContents.send('file-opened', {
                path: filePath,
                name: path.basename(filePath),
                content: content
            });
        } catch (error) {
            dialog.showErrorBox('Error Opening File', `Could not open file: ${error.message}`);
        }
    }
}

// Create and initialize the application
const madolaApp = new MadolaElectronApp();
madolaApp.init();