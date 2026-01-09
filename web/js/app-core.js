// MADOLA Web App - Core Application Logic (Part 1/2)

class MadolaApp {
    constructor() {
        this.wasmModule = null;
        this.currentFile = null;
        this.hasUnsavedChanges = false;
        this.isElectron = window.electronAPI && window.electronAPI.isElectron;
        this.cppEditor = null; // Monaco Editor instance for C++ files
        this.mainEditor = null; // Monaco Editor instance for DSL editor
        this.isDarkTheme = false;
        this.autoRunEnabled = true; // Flag to control auto-run behavior

        // Initialize MathJax configuration
        this.setupMathJax();

        // Initialize Monaco Editor
        this.initMonacoEditor();

        // Initialize the application
        this.init();
    }

    setupMathJax() {
        // MathJax is already configured in HTML, just ensure it's properly initialized
        // Only configure if not already configured
        if (!window.MathJax) {
            window.MathJax = {
                tex: {
                    inlineMath: [['$', '$'], ['\\(', '\\)']],
                    displayMath: [['$$', '$$'], ['\\[', '\\]']],
                    processEscapes: true,
                    processEnvironments: true
                },
                options: {
                    ignoreHtmlClass: 'tex2jax_ignore',
                    processHtmlClass: 'tex2jax_process'
                },
                startup: {
                    ready: () => {
                        console.log('MathJax is ready');
                        window.MathJax.startup.defaultReady();
                    }
                }
            };
        } else {
            // MathJax already configured, just ensure processEscapes is enabled
            if (window.MathJax.tex) {
                window.MathJax.tex.processEscapes = true;
                window.MathJax.tex.processEnvironments = true;
            }
        }
    }

    initMonacoEditor() {
        // Monaco Editor is now loaded directly without AMD loader
        // No special configuration needed since we disabled AMD
    }

    async initMainEditor() {
        try {
            // Wait for Monaco Editor to be loaded
            await this.waitForMonaco();

            const container = document.getElementById('monaco-editor');
            this.mainEditor = window.monaco.editor.create(container, {
                value: '',
                language: 'madola',
                theme: this.isDarkTheme ? 'madola-dark' : 'madola-light',
                minimap: { enabled: false },
                scrollBeyondLastLine: false,
                fontSize: 14,
                lineNumbers: 'on',
                folding: true,
                automaticLayout: true,
                wordWrap: 'on'
            });

            // Set up change listener
            this.mainEditor.onDidChangeModelContent(() => {
                this.hasUnsavedChanges = true;
                this.updateTitle();

                // Auto-run on change (debounced) - only if auto-run is enabled
                if (this.autoRunEnabled) {
                    clearTimeout(this.autoRunTimeout);
                    this.autoRunTimeout = setTimeout(() => this.runCode(), 1000);
                }
            });

        } catch (error) {
            console.error('Failed to load Monaco Editor:', error);
            this.addMessage('error', 'Failed to load Monaco Editor');
        }
    }

    async waitForMonaco() {
        return new Promise((resolve, reject) => {
            if (window.monaco) {
                resolve();
                return;
            }

            let attempts = 0;
            const maxAttempts = 50; // 5 seconds maximum wait
            const checkMonaco = () => {
                attempts++;
                if (window.monaco) {
                    resolve();
                } else if (attempts >= maxAttempts) {
                    reject(new Error('Monaco Editor failed to load within timeout'));
                } else {
                    setTimeout(checkMonaco, 100);
                }
            };

            checkMonaco();
        });
    }

    async init() {
        try {
            this.addMessage('info', 'Initializing MADOLA...');

            // Initialize main Monaco Editor
            await this.initMainEditor();

            // Load WASM module (non-blocking)
            await this.loadWasm();

            // Setup event listeners
            this.setupEventListeners();

            // Initialize tab system
            this.initTabSystem();

            // Setup LaTeX panel
            this.setupLatexPanel();

            // Initialize mobile toggles
            this.initMobileToggles();

            // Setup Electron integration if available
            if (this.isElectron) {
                this.setupElectronIntegration();
            }

            // Initialize with default content
            this.loadDefaultContent();

            const status = this.wasmModule ? 'with WASM support' : 'in fallback mode';
            this.addMessage('success', `MADOLA initialized successfully ${status}!`);
        } catch (error) {
            this.addMessage('error', `Failed to initialize: ${error.message}`);
        }
    }

    async loadWasm() {
        try {
            this.addMessage('info', 'Loading MADOLA WASM module...');

            // Use the browser wrapper for MADOLA
            this.madolaWrapper = new MadolaBrowserWrapper();
            this.wasmModule = await this.madolaWrapper.loadModule();

            this.addMessage('success', 'WASM module loaded successfully');

            // Setup WASM function bridge for external functions
            await this.setupWasmBridge();

            // Test the module
            this.addMessage('info', 'Testing WASM module...');
            const testResult = await this.madolaWrapper.evaluate('x := 42; print(x);');
            if (testResult) {
                this.addMessage('success', 'WASM module tested successfully');
            } else {
                this.addMessage('warning', 'WASM module loaded but test failed');
            }

        } catch (error) {
            this.addMessage('error', `WASM loading failed: ${error.message}`);
            this.addMessage('info', 'Running in fallback mode without WASM');
            this.wasmModule = null;
            this.madolaWrapper = null;
        }
    }

    async setupWasmBridge() {
        try {
            this.addMessage('info', 'Setting up WASM function bridge...');

            // Create the bridge object that will hold external WASM functions
            window.madolaWasmBridge = {};

            // Scan and load all WASM modules from trove directory
            await this.scanAndLoadTrove();

            this.addMessage('success', 'WASM bridge setup complete - all modules loaded');
        } catch (error) {
            this.addMessage('warning', `WASM bridge setup failed: ${error.message}`);
            // Ensure bridge exists even if loading fails
            window.madolaWasmBridge = {};
        }
    }

    async scanAndLoadTrove() {
        try {
            // Fetch the trove directory listing
            const response = await fetch('trove/manifest.json');
            if (!response.ok) {
                throw new Error('No manifest.json found - cannot auto-load WASM modules');
            }

            const manifest = await response.json();

            // Load each module and its functions
            for (const [moduleName, functions] of Object.entries(manifest)) {
                for (const functionName of functions) {
                    const jsWrapperPath = `trove/${moduleName}/${functionName}.js`;
                    const basePath = `trove/${moduleName}`;

                    const addon = await this.loadWasmFunctionWithPath(functionName, jsWrapperPath, basePath);
                    if (addon) {
                        window.madolaWasmBridge[functionName] = (n) => addon[functionName](n);
                        console.log(`[MADOLA] Loaded ${functionName} from ${moduleName}`);
                    }
                }
            }
        } catch (error) {
            console.warn('Could not scan trove directory:', error.message);
            console.log('WASM functions will be loaded on-demand from imports');
        }
    }

    async loadWasmFunction(functionName, jsWrapperPath) {
        const basePath = jsWrapperPath.replace(/\/[^\/]+\.js$/, '');
        return this.loadWasmFunctionWithPath(functionName, jsWrapperPath, basePath);
    }

    async loadWasmFunctionWithPath(functionName, jsWrapperPath, basePath) {
        try {
            // Load the JavaScript wrapper
            const response = await fetch(jsWrapperPath);
            if (!response.ok) {
                throw new Error(`Failed to load ${jsWrapperPath}: ${response.statusText}`);
            }

            const jsCode = await response.text();

            // Create a script element to execute the wrapper code
            const script = document.createElement('script');
            script.textContent = jsCode;
            document.head.appendChild(script);

            // Get the constructor from the global scope
            const constructorName = `${functionName}Addon`;
            if (typeof window[constructorName] !== 'function') {
                throw new Error(`Constructor ${constructorName} not found after loading wrapper`);
            }

            // Create and load the addon with base path
            const addon = new window[constructorName](basePath);
            await addon.load();

            // Clean up the script
            document.head.removeChild(script);

            return addon;
        } catch (error) {
            console.error(`Failed to load WASM function ${functionName}:`, error);
            return null;
        }
    }

    setupEventListeners() {
        // Menu buttons
        document.getElementById('btn-new').addEventListener('click', () => this.newFile());
        document.getElementById('btn-open').addEventListener('click', () => this.openFile());
        document.getElementById('btn-save').addEventListener('click', () => this.saveFile());
        document.getElementById('btn-save-as').addEventListener('click', () => this.saveAsFile());
        document.getElementById('btn-about').addEventListener('click', () => this.showAbout());

        // Editor controls
        const btnThemeToggle = document.getElementById('btn-theme-toggle');
        if (btnThemeToggle) {
            btnThemeToggle.addEventListener('click', () => this.toggleTheme());
        }
        document.getElementById('btn-run').addEventListener('click', () => this.runCode());
        const btnExport = document.getElementById('btn-export');
        if (btnExport) {
            btnExport.addEventListener('click', () => this.exportFiles());
        }

        // Example selector
        document.getElementById('example-dropdown').addEventListener('change', (e) => this.loadExample(e.target.value));

        // Output controls
        document.getElementById('btn-copy-output').addEventListener('click', () => this.copyOutput());
        document.getElementById('btn-download-html').addEventListener('click', () => this.downloadHtml());
        document.getElementById('btn-print-pdf').addEventListener('click', () => this.printPDF());

        // Messages control
        const btnClearMessages = document.getElementById('btn-clear-messages');
        if (btnClearMessages) {
            btnClearMessages.addEventListener('click', () => this.clearMessages());
        }
        const btnClearSidebarMessages = document.getElementById('btn-clear-sidebar-messages');
        if (btnClearSidebarMessages) {
            btnClearSidebarMessages.addEventListener('click', () => this.clearMessages());
        }

        // Modal controls
        document.getElementById('modal-close').addEventListener('click', () => this.hideAbout());
        document.getElementById('about-modal').addEventListener('click', (e) => {
            if (e.target.id === 'about-modal') this.hideAbout();
        });

        // File input (only for web version)
        if (!this.isElectron) {
            document.getElementById('file-input').addEventListener('change', (e) => this.handleFileOpen(e));
        }

        // Keyboard shortcuts (only for web version, Electron handles these natively)
        if (!this.isElectron) {
            document.addEventListener('keydown', (e) => this.handleKeyboard(e));
        }
    }

    setupLatexPanel() {
        // Setup LaTeX symbol button click handlers
        const symbolButtons = document.querySelectorAll('.symbol-btn');
        symbolButtons.forEach(btn => {
            btn.addEventListener('click', () => {
                const latexCode = btn.dataset.latex;
                this.insertLatexSymbol(latexCode);
            });
        });
    }

    insertLatexSymbol(latexCode) {
        if (!this.mainEditor) return;

        const selection = this.mainEditor.getSelection();
        const id = { major: 1, minor: 1 };
        const text = latexCode;
        const op = { identifier: id, range: selection, text: text, forceMoveMarkers: true };
        this.mainEditor.executeEdits("insert-latex", [op]);
        this.mainEditor.focus();
    }

    toggleTheme() {
        this.isDarkTheme = !this.isDarkTheme;
        const themeBtn = document.getElementById('btn-theme-toggle');

        if (this.isDarkTheme) {
            document.body.classList.add('dark-theme');
            if (themeBtn) {
                themeBtn.textContent = '☀️ Light';
            }
            if (this.mainEditor) {
                this.mainEditor.updateOptions({ theme: 'madola-dark' });
            }
            if (this.cppEditor) {
                this.cppEditor.updateOptions({ theme: 'vs-dark' });
            }
        } else {
            document.body.classList.remove('dark-theme');
            if (themeBtn) {
                themeBtn.textContent = '🌙 Dark';
            }
            if (this.mainEditor) {
                this.mainEditor.updateOptions({ theme: 'madola-light' });
            }
            if (this.cppEditor) {
                this.cppEditor.updateOptions({ theme: 'vs' });
            }
        }

        // Update theme select in settings if it exists
        const themeSelect = document.getElementById('theme-select');
        if (themeSelect) {
            themeSelect.value = this.isDarkTheme ? 'dark' : 'light';
        }
    }

    exportFiles() {
        // Export functionality - could export multiple formats
        this.downloadHtml();
        this.addMessage('success', 'Files exported successfully');
    }

    setupElectronIntegration() {
        // Listen for menu actions from main process
        window.electronAPI.onMenuAction((action) => {
            switch (action) {
                case 'new':
                    this.newFile();
                    break;
                case 'save':
                    this.saveFileElectron();
                    break;
                case 'run':
                    this.runCode();
                    break;
                case 'format':
                    this.formatCode();
                    break;
                case 'about':
                    this.showAbout();
                    break;
            }
        });

        // Listen for file opened from main process
        window.electronAPI.onFileOpened((fileData) => {
            if (this.mainEditor) {
                // Temporarily disable auto-run to prevent double execution
                const wasAutoRunEnabled = this.autoRunEnabled;
                this.autoRunEnabled = false;
                
                this.mainEditor.setValue(fileData.content);
                
                // Re-enable auto-run
                this.autoRunEnabled = wasAutoRunEnabled;
            }
            this.currentFile = fileData.name;
            this.hasUnsavedChanges = false;
            this.updateTitle();
            this.addMessage('success', `Opened: ${fileData.name}`);
            
            // Clear any pending auto-run timeouts and run once
            clearTimeout(this.autoRunTimeout);
            setTimeout(() => this.runCode(), 100);
        });

        // Listen for save as from main process
        window.electronAPI.onSaveAs((filePath) => {
            this.saveFileToPath(filePath);
        });
    }

    loadDefaultContent() {
        const defaultCode = `
@version 0.01
// HTML Formatter Demo
// This demonstrates math, text, and graphs

// Greek letters and mathematical expressions
@layout1x3
\\alpha := 2;
\\beta := 3;
\\gamma := 1;

// Quadratic formula with step-by-step solution
@resolveAlign
\\Delta := \\beta^2 - 4*\\alpha*\\gamma;

@resolveAlign
x_1 := (-\\beta + sqrt(\\Delta))/(2*\\alpha);

print("Solutions:");
print(x_1);

// Function with algorithmic structure
fn factorial(n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n-1);
    }
}

print("Factorial of 6:");
print(factorial(6));

// For loop creating data for graph
for i in 0...8 {
    x[i] := i;
    y[i] := i^2 + 2*i + 1;
}

graph(x, y, "Quadratic Function");`;

        if (this.mainEditor) {
            // Temporarily disable auto-run to prevent double execution
            const wasAutoRunEnabled = this.autoRunEnabled;
            this.autoRunEnabled = false;
            
            this.mainEditor.setValue(defaultCode);
            
            // Re-enable auto-run
            this.autoRunEnabled = wasAutoRunEnabled;
        }
        
        // Clear any pending auto-run timeouts and run once
        clearTimeout(this.autoRunTimeout);
        setTimeout(() => this.runCode(), 100);
    }
}