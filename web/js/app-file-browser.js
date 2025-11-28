// File browser for C++ and WASM files

class FileBrowser {
    constructor() {
        this.cppFiles = [];
        this.wasmModules = [];
    }

    async init() {
        await this.loadFiles();
        this.setupEventListeners();
    }

    async loadFiles() {
        try {
            // Detect environment: Tauri, Electron, or Web
            const isTauri = window.__TAURI__ !== undefined;
            const isElectron = window.electronAPI && window.electronAPI.isElectron;
            
            console.log('[FileBrowser] Environment detected:', {
                isTauri,
                isElectron,
                isWeb: !isTauri && !isElectron
            });
            
            // Load C++ files
            let cppData;
            if (isTauri) {
                console.log('[FileBrowser] Loading C++ files via Tauri...');
                cppData = await window.__TAURI__.invoke('get_cpp_files');
                console.log('[FileBrowser] C++ data received:', cppData);
            } else if (isElectron) {
                console.log('[FileBrowser] Loading C++ files via Electron...');
                cppData = await window.electronAPI.getCppFiles();
            } else {
                console.log('[FileBrowser] Loading C++ files via HTTP...');
                const cppResponse = await fetch('/api/files/cpp');
                cppData = await cppResponse.json();
            }
            
            if (cppData.success) {
                this.cppFiles = cppData.files;
                console.log('[FileBrowser] Loaded', cppData.files.length, 'C++ files');
                this.renderCppFiles();
            } else {
                console.error('[FileBrowser] Failed to load C++ files:', cppData.error);
            }

            // Load WASM modules
            let wasmData;
            if (isTauri) {
                console.log('[FileBrowser] Loading WASM modules via Tauri...');
                wasmData = await window.__TAURI__.invoke('get_wasm_modules');
                console.log('[FileBrowser] WASM data received:', wasmData);
            } else if (isElectron) {
                console.log('[FileBrowser] Loading WASM modules via Electron...');
                wasmData = await window.electronAPI.getWasmModules();
            } else {
                console.log('[FileBrowser] Loading WASM modules via HTTP...');
                const wasmResponse = await fetch('/api/files/wasm');
                wasmData = await wasmResponse.json();
            }
            
            if (wasmData.success) {
                this.wasmModules = wasmData.modules;
                console.log('[FileBrowser] Loaded', wasmData.modules.length, 'WASM modules');
                this.renderWasmModules();
            } else {
                console.error('[FileBrowser] Failed to load WASM modules:', wasmData.error);
            }
        } catch (error) {
            console.error('[FileBrowser] Error loading files:', error);
        }
    }

    setupEventListeners() {
        // Toggle C++ section
        const cppHeader = document.getElementById('cpp-section-header');
        if (cppHeader) {
            cppHeader.addEventListener('click', () => {
                const list = document.getElementById('cpp-file-list');
                const icon = cppHeader.querySelector('.toggle-icon');
                if (list.style.display === 'none') {
                    list.style.display = 'block';
                    icon.textContent = '▼';
                } else {
                    list.style.display = 'none';
                    icon.textContent = '▶';
                }
            });
        }

        // Toggle WASM section
        const wasmHeader = document.getElementById('wasm-section-header');
        if (wasmHeader) {
            wasmHeader.addEventListener('click', () => {
                const list = document.getElementById('wasm-module-list');
                const icon = wasmHeader.querySelector('.toggle-icon');
                if (list.style.display === 'none') {
                    list.style.display = 'block';
                    icon.textContent = '▼';
                } else {
                    list.style.display = 'none';
                    icon.textContent = '▶';
                }
            });
        }

        // Refresh button
        const refreshBtn = document.getElementById('refresh-files-btn');
        if (refreshBtn) {
            refreshBtn.addEventListener('click', () => this.loadFiles());
        }
    }

    renderCppFiles() {
        const container = document.getElementById('cpp-file-list');
        if (!container) return;

        if (this.cppFiles.length === 0) {
            container.innerHTML = '<div class="file-item empty">No C++ files</div>';
            return;
        }

        container.innerHTML = this.cppFiles.map(file => `
            <div class="file-item" data-filename="${file.name}" onclick="fileBrowser.viewCppFile('${file.name}')">
                <span class="file-icon">📄</span>
                <span class="file-name">${file.name}</span>
                <span class="file-size">${this.formatSize(file.size)}</span>
            </div>
        `).join('');
    }

    renderWasmModules() {
        const container = document.getElementById('wasm-module-list');
        if (!container) return;

        if (this.wasmModules.length === 0) {
            container.innerHTML = '<div class="file-item empty">No WASM modules</div>';
            return;
        }

        container.innerHTML = this.wasmModules.map(module => `
            <div class="module-item">
                <div class="module-header" onclick="fileBrowser.toggleModule('${module.name}')">
                    <span class="module-icon">📦</span>
                    <span class="module-name">${module.name}</span>
                    <span class="module-toggle">▼</span>
                </div>
                <div class="module-files" id="module-${module.name}">
                    ${module.files.map(file => `
                        <div class="file-item">
                            <span class="file-icon">${file.type === 'wasm' ? '⚙️' : '📜'}</span>
                            <span class="file-name">${file.name}</span>
                            <span class="file-size">${this.formatSize(file.size)}</span>
                        </div>
                    `).join('')}
                </div>
            </div>
        `).join('');
    }

    toggleModule(moduleName) {
        const filesDiv = document.getElementById(`module-${moduleName}`);
        const header = filesDiv.previousElementSibling;
        const toggle = header.querySelector('.module-toggle');
        
        if (filesDiv.style.display === 'none') {
            filesDiv.style.display = 'block';
            toggle.textContent = '▼';
        } else {
            filesDiv.style.display = 'none';
            toggle.textContent = '▶';
        }
    }

    async viewCppFile(filename) {
        try {
            const isTauri = window.__TAURI__ !== undefined;
            const isElectron = window.electronAPI && window.electronAPI.isElectron;
            let data;
            
            if (isTauri) {
                data = await window.__TAURI__.invoke('get_cpp_file_content', { filename });
            } else if (isElectron) {
                data = await window.electronAPI.getCppFileContent(filename);
            } else {
                const response = await fetch(`/api/files/cpp/${encodeURIComponent(filename)}`);
                data = await response.json();
            }
            
            if (data.success) {
                this.showCodeModal(filename, data.content);
            } else {
                console.error('Error loading file:', data.error);
            }
        } catch (error) {
            console.error('Error viewing file:', error);
        }
    }

    showCodeModal(filename, content) {
        // Create modal if it doesn't exist
        let modal = document.getElementById('code-modal');
        if (!modal) {
            modal = document.createElement('div');
            modal.id = 'code-modal';
            modal.className = 'modal code-modal';
            modal.innerHTML = `
                <div class="modal-content">
                    <div class="modal-header">
                        <h3 id="modal-title"></h3>
                        <button class="close-btn modal-close" onclick="fileBrowser.closeModal()">&times;</button>
                    </div>
                    <div class="modal-body">
                        <pre id="modal-code"></pre>
                    </div>
                </div>
            `;
            document.body.appendChild(modal);
            modal.style.display = 'none';
        }

        document.getElementById('modal-title').textContent = filename;
        document.getElementById('modal-code').textContent = content;
        modal.style.display = 'flex';
    }

    closeModal() {
        const modal = document.getElementById('code-modal');
        if (modal) {
            modal.style.display = 'none';
        }
    }

    formatSize(bytes) {
        if (bytes < 1024) return bytes + ' B';
        if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
        return (bytes / (1024 * 1024)).toFixed(1) + ' MB';
    }
}

// Global instance
const fileBrowser = new FileBrowser();

// Initialize when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => fileBrowser.init());
} else {
    fileBrowser.init();
}

