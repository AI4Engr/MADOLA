// Sidebar functionality for MADOLA

(function() {
    'use strict';

    // Sidebar tab switching
    function initSidebarTabs() {
        const tabs = document.querySelectorAll('.sidebar-tab');
        const tabContents = document.querySelectorAll('.sidebar-tab-content');
        const contentPanel = document.getElementById('sidebar-content-panel');

        tabs.forEach(tab => {
            tab.addEventListener('click', () => {
                const tabName = tab.getAttribute('data-tab');

                // Remove active class from all tabs and contents
                tabs.forEach(t => t.classList.remove('active'));
                tabContents.forEach(tc => tc.classList.remove('active'));

                // Add active class to clicked tab and corresponding content
                tab.classList.add('active');
                document.getElementById(`sidebar-${tabName}`).classList.add('active');

                // Always show content panel when clicking a tab
                contentPanel.classList.add('expanded');
            });
        });
    }

    // Sidebar content panel toggle
    function initSidebarToggle() {
        const toggleBtn = document.getElementById('btn-toggle-sidebar');
        const contentPanel = document.getElementById('sidebar-content-panel');
        const resizeHandle = document.getElementById('resize-left');

        if (toggleBtn && contentPanel) {
            toggleBtn.addEventListener('click', () => {
                const isExpanding = !contentPanel.classList.contains('expanded');
                const isMobile = window.innerWidth <= 768;

                // Toggle content panel visibility
                contentPanel.classList.toggle('expanded');

                if (isExpanding) {
                    // When expanding, restore size if it was resized, or use default
                    if (isMobile) {
                        // Mobile: restore height or use default
                        if (!contentPanel.style.height || contentPanel.style.height === '0px') {
                            contentPanel.style.height = '';
                            contentPanel.style.maxHeight = '';
                        }
                    } else {
                        // Desktop: restore width or use default
                        if (!contentPanel.style.width || contentPanel.style.width === '0px') {
                            contentPanel.style.width = '';
                            contentPanel.style.minWidth = '';
                            contentPanel.style.maxWidth = '';
                        }
                    }
                } else {
                    // When collapsing, temporarily set size to 0
                    if (isMobile) {
                        const currentHeight = contentPanel.offsetHeight;
                        contentPanel.setAttribute('data-last-height', currentHeight);
                        contentPanel.style.height = '0px';
                        contentPanel.style.maxHeight = '0px';
                    } else {
                        const currentWidth = contentPanel.offsetWidth;
                        contentPanel.setAttribute('data-last-width', currentWidth);
                        contentPanel.style.width = '0px';
                        contentPanel.style.minWidth = '0px';
                        contentPanel.style.maxWidth = '0px';
                    }
                }

                // Toggle resize handle visibility
                if (resizeHandle) {
                    resizeHandle.classList.toggle('hidden', !isExpanding);
                }
            });
        }
    }

    // Panel resize functionality
    function initPanelResize() {
        const resizeHandles = document.querySelectorAll('.resize-handle');

        resizeHandles.forEach(handle => {
            let isResizing = false;
            let startX = 0;
            let startY = 0;
            let startWidth = 0;
            let startHeight = 0;
            let targetPanel = null;
            let isMobile = false;

            const startResize = (clientX, clientY) => {
                isResizing = true;
                startX = clientX;
                startY = clientY;
                handle.classList.add('resizing');

                // Check if we're on mobile (window width <= 768px)
                isMobile = window.innerWidth <= 768;

                // Determine which panel to resize
                if (handle.id === 'resize-left') {
                    targetPanel = document.querySelector('.sidebar-content-panel');
                    if (isMobile) {
                        startHeight = targetPanel.offsetHeight;
                    } else {
                        startWidth = targetPanel.offsetWidth;
                    }
                } else if (handle.id === 'resize-right') {
                    targetPanel = document.querySelector('.middle-panel');
                    if (isMobile) {
                        startHeight = targetPanel.offsetHeight;
                    } else {
                        startWidth = targetPanel.offsetWidth;
                    }
                }
            };

            handle.addEventListener('mousedown', (e) => {
                startResize(e.clientX, e.clientY);
                e.preventDefault();
            });

            handle.addEventListener('touchstart', (e) => {
                const touch = e.touches[0];
                startResize(touch.clientX, touch.clientY);
                e.preventDefault();
            });

            const doResize = (clientX, clientY) => {
                if (!isResizing || !targetPanel) return;

                if (isMobile) {
                    // Mobile: vertical resize for both handles
                    const delta = clientY - startY;
                    const newHeight = startHeight + delta;
                    const minHeight = 100;
                    const maxHeight = window.innerHeight - 150;

                    if (newHeight >= minHeight && newHeight <= maxHeight) {
                        targetPanel.style.height = newHeight + 'px';
                        targetPanel.style.minHeight = newHeight + 'px';
                        if (handle.id === 'resize-left') {
                            targetPanel.style.maxHeight = newHeight + 'px';
                        }
                        // Remove flex to allow fixed height
                        targetPanel.style.flex = 'none';
                    }
                } else {
                    // Desktop: horizontal resize
                    const delta = clientX - startX;
                    const newWidth = startWidth + delta;

                    if (handle.id === 'resize-left') {
                        const minWidth = 150;
                        const maxWidth = 500;
                        if (newWidth >= minWidth && newWidth <= maxWidth) {
                            targetPanel.style.width = newWidth + 'px';
                            targetPanel.style.minWidth = newWidth + 'px';
                            targetPanel.style.maxWidth = newWidth + 'px';
                        }
                    } else if (handle.id === 'resize-right') {
                        const minWidth = 400;
                        if (newWidth >= minWidth) {
                            targetPanel.style.flex = 'none';
                            targetPanel.style.width = newWidth + 'px';
                        }
                    }
                }
            };

            document.addEventListener('mousemove', (e) => {
                doResize(e.clientX, e.clientY);
            });

            document.addEventListener('touchmove', (e) => {
                if (!isResizing) return;
                const touch = e.touches[0];
                doResize(touch.clientX, touch.clientY);
                e.preventDefault();
            });

            const endResize = () => {
                if (isResizing) {
                    isResizing = false;
                    handle.classList.remove('resizing');
                    targetPanel = null;
                }
            };

            document.addEventListener('mouseup', endResize);
            document.addEventListener('touchend', endResize);
        });
    }

    // Theme toggle in settings
    function initThemeSettings() {
        const themeToggleBtn = document.getElementById('theme-toggle-btn');
        const colorThemeButtons = document.querySelectorAll('.color-theme-btn');

        // Load saved themes
        const savedTheme = localStorage.getItem('madola-theme') || 'light';
        const savedColorTheme = localStorage.getItem('madola-color-theme') || 'blue';

        applyTheme(savedTheme);
        applyColorTheme(savedColorTheme);
        updateColorThemeButtonState(savedColorTheme);

        // Handle light/dark toggle
        if (themeToggleBtn) {
            themeToggleBtn.addEventListener('click', () => {
                const currentTheme = document.body.classList.contains('dark-theme') ? 'dark' : 'light';
                const newTheme = currentTheme === 'light' ? 'dark' : 'light';
                applyTheme(newTheme);
                localStorage.setItem('madola-theme', newTheme);
            });
        }

        // Handle color theme buttons
        if (colorThemeButtons.length > 0) {
            colorThemeButtons.forEach(btn => {
                btn.addEventListener('click', () => {
                    const colorTheme = btn.getAttribute('data-color');
                    applyColorTheme(colorTheme);
                    updateColorThemeButtonState(colorTheme);
                    localStorage.setItem('madola-color-theme', colorTheme);
                });
            });
        }
    }

    function applyColorTheme(color) {
        document.documentElement.setAttribute('data-color-theme', color);
    }

    function updateColorThemeButtonState(color) {
        const colorThemeButtons = document.querySelectorAll('.color-theme-btn');
        colorThemeButtons.forEach(btn => {
            if (btn.getAttribute('data-color') === color) {
                btn.classList.add('active');
            } else {
                btn.classList.remove('active');
            }
        });
    }

    function applyTheme(theme) {
        const themeToggleIcon = document.querySelector('.theme-toggle-icon');

        if (theme === 'dark') {
            document.body.classList.add('dark-theme');
            if (themeToggleIcon) {
                themeToggleIcon.textContent = '🌙';
            }
            // Update Monaco editor theme if app exists
            if (window.madolaApp && window.madolaApp.mainEditor) {
                window.madolaApp.mainEditor.updateOptions({ theme: 'vs-dark' });
            }
            if (window.madolaApp && window.madolaApp.cppEditor) {
                window.madolaApp.cppEditor.updateOptions({ theme: 'vs-dark' });
            }
        } else {
            document.body.classList.remove('dark-theme');
            if (themeToggleIcon) {
                themeToggleIcon.textContent = '☀️';
            }
            // Update Monaco editor theme if app exists
            if (window.madolaApp && window.madolaApp.mainEditor) {
                window.madolaApp.mainEditor.updateOptions({ theme: 'vs' });
            }
            if (window.madolaApp && window.madolaApp.cppEditor) {
                window.madolaApp.cppEditor.updateOptions({ theme: 'vs' });
            }
        }
    }

    // Clear messages in sidebar
    function initSidebarMessages() {
        const clearBtn = document.getElementById('btn-clear-sidebar-messages');
        const messagesContainer = document.getElementById('sidebar-messages-container');

        if (clearBtn && messagesContainer) {
            clearBtn.addEventListener('click', () => {
                messagesContainer.innerHTML = '';
            });
        }
    }

    // Formatting buttons (Bold/Italic)
    function initFormattingButtons() {
        const formatBtns = document.querySelectorAll('.format-btn');

        formatBtns.forEach(btn => {
            btn.addEventListener('click', () => {
                const format = btn.getAttribute('data-format');
                insertFormatting(format);
            });
        });
    }

    function insertFormatting(format) {
        // Get Monaco editor instance if available
        if (window.madolaApp && window.madolaApp.mainEditor) {
            const editor = window.madolaApp.mainEditor;
            const selection = editor.getSelection();
            const selectedText = editor.getModel().getValueInRange(selection);
            const model = editor.getModel();
            const position = editor.getPosition();

            let formattedText = '';
            let newSelection = null;

            if (format === 'bold') {
                formattedText = `**${selectedText}**`;
                if (selectedText === '') {
                    // If no text selected, position cursor between the markers
                    newSelection = new monaco.Range(
                        selection.startLineNumber,
                        selection.startColumn + 2,
                        selection.endLineNumber,
                        selection.endColumn + 2
                    );
                }
            } else if (format === 'italic') {
                formattedText = `*${selectedText}*`;
                if (selectedText === '') {
                    // If no text selected, position cursor between the markers
                    newSelection = new monaco.Range(
                        selection.startLineNumber,
                        selection.startColumn + 1,
                        selection.endLineNumber,
                        selection.endColumn + 1
                    );
                }
            } else if (format === 'align-left' || format === 'align-center' || format === 'align-right') {
                // Handle alignment decorators
                const lineNumber = selection.startLineNumber;
                const lineContent = model.getLineContent(lineNumber);

                // Check if line starts with @h1, @h2, @h3, @h4, or @p (with or without {)
                const headingMatch = lineContent.match(/^@(h[1-4]|p)(\[.*?\])?(\{)?/);

                if (!headingMatch || !headingMatch[1]) {
                    // Not in a heading or paragraph, do nothing
                    window.addMessageToSidebar('Alignment can only be applied to headings (h1-h4) or paragraphs (p)', 'warning');
                    return;
                }

                const elementType = headingMatch[1]; // h1, h2, h3, h4, or p
                const existingDecorator = headingMatch[2]; // existing [decorator] if any

                // Determine alignment value
                let alignment = 'left';
                if (format === 'align-center') alignment = 'center';
                else if (format === 'align-right') alignment = 'right';

                // Find where to insert/replace the alignment decorator
                // Position is right after @elementType (e.g., after "@p")
                const atSymbolPos = lineContent.indexOf('@');
                const decoratorStart = atSymbolPos + 1 + elementType.length + 1; // +1 for @, +length for element, +1 for column offset

                if (existingDecorator) {
                    // Replace existing decorator
                    const decoratorEnd = decoratorStart + existingDecorator.length;
                    editor.executeEdits('', [{
                        range: new monaco.Range(lineNumber, decoratorStart, lineNumber, decoratorEnd),
                        text: `[${alignment}]`
                    }]);
                } else {
                    // Insert new decorator before the { or at end of line
                    const insertPos = decoratorStart;
                    editor.executeEdits('', [{
                        range: new monaco.Range(lineNumber, insertPos, lineNumber, insertPos),
                        text: `[${alignment}]`
                    }]);
                }

                editor.focus();
                return;
            } else if (format === 'h1' || format === 'h2' || format === 'h3' || format === 'h4' || format === 'p') {
                // Insert decorator with {} wrapping
                const lineNumber = selection.startLineNumber;
                const lineContent = model.getLineContent(lineNumber);
                const lineStartColumn = 1;

                // Check if line is empty or if we should insert before selected text
                if (selectedText === '') {
                    // No selection - insert decorator with empty braces on same line
                    formattedText = `@${format}{}`;

                    // Insert at the beginning of the line
                    editor.executeEdits('', [{
                        range: new monaco.Range(lineNumber, lineStartColumn, lineNumber, lineStartColumn),
                        text: formattedText
                    }]);

                    // Position cursor inside the braces
                    newSelection = new monaco.Range(
                        lineNumber,
                        lineStartColumn + format.length + 2,
                        lineNumber,
                        lineStartColumn + format.length + 2
                    );
                } else {
                    // Text is selected - wrap it in decorator with braces
                    formattedText = `@${format}{${selectedText}}`;
                }
            }

            if (format !== 'h1' && format !== 'h2' && format !== 'h3' && format !== 'h4' && format !== 'p' &&
                format !== 'align-left' && format !== 'align-center' && format !== 'align-right') {
                editor.executeEdits('', [{
                    range: selection,
                    text: formattedText
                }]);
            } else if (selectedText !== '') {
                editor.executeEdits('', [{
                    range: selection,
                    text: formattedText
                }]);
            }

            // Set cursor position if no text was selected
            if (newSelection) {
                editor.setSelection(newSelection);
            }

            editor.focus();
        }
    }

    // Insert Greek symbols into editor
    function initGreekSymbols() {
        const symbolBtns = document.querySelectorAll('.symbol-btn-sm');

        symbolBtns.forEach(btn => {
            btn.addEventListener('click', () => {
                const latex = btn.getAttribute('data-latex');
                insertSymbol(latex);
            });
        });
    }

    function insertSymbol(latex) {
        // Insert into Monaco editor
        if (window.madolaApp && window.madolaApp.mainEditor) {
            const editor = window.madolaApp.mainEditor;
            const position = editor.getPosition();
            editor.executeEdits('', [{
                range: new monaco.Range(position.lineNumber, position.column, position.lineNumber, position.column),
                text: latex
            }]);
            editor.focus();
        }
    }

    // Sync messages between main messages panel and sidebar
    window.addMessageToSidebar = function(message, type) {
        const messagesContainer = document.getElementById('sidebar-messages-container');
        if (messagesContainer) {
            const messageDiv = document.createElement('div');
            messageDiv.className = `message ${type}`;
            messageDiv.textContent = message;
            messagesContainer.appendChild(messageDiv);
            messagesContainer.scrollTop = messagesContainer.scrollHeight;
        }
    };

    // Initialize resize handle visibility based on sidebar state
    function initResizeHandleState() {
        const contentPanel = document.getElementById('sidebar-content-panel');
        const resizeHandle = document.getElementById('resize-left');

        if (contentPanel && resizeHandle) {
            // Hide resize handle initially if sidebar is not expanded
            if (!contentPanel.classList.contains('expanded')) {
                resizeHandle.classList.add('hidden');
            }
        }
    }

    // File browser functionality
    function initFileBrowser() {
        const refreshBtn = document.getElementById('btn-refresh-files');

        if (refreshBtn) {
            refreshBtn.addEventListener('click', loadFileTree);
        }

        // Load file tree on initialization
        loadFileTree();
    }

    async function loadFileTree() {
        const container = document.getElementById('file-tree-container');
        if (!container) return;

        // Show loading state
        container.innerHTML = '<div class="file-tree-loading">Loading files...</div>';

        try {
            // Check if we're in Electron/Tauri - read from filesystem
            if (window.electronAPI || window.__TAURI__) {
                await loadFileTreeFromFilesystem(container);
            } else {
                // Web browser - show in-memory generated files
                await loadFileTreeFromMemory(container);
            }
        } catch (error) {
            console.error('Error loading file tree:', error);
            container.innerHTML = '<div class="file-tree-loading">Error loading files. Run MADOLA code with @trove or @gen_cpp decorators to generate files.</div>';
        }
    }

    // Load files from filesystem (Electron/Tauri)
    async function loadFileTreeFromFilesystem(container) {
        try {
            const homeDir = await getHomeDirectory();
            const madolaDir = `${homeDir}/.madola`;

            // Create folder structure
            const folders = [
                { name: 'trove', path: `${madolaDir}/trove` },
                { name: 'gen_cpp', path: `${madolaDir}/gen_cpp` }
            ];

            container.innerHTML = '';

            for (const folder of folders) {
                try {
                    const files = await listFilesFromFilesystem(folder.path);
                    const folderElement = createFolderElement(folder.name, folder.path, files, true);
                    container.appendChild(folderElement);
                } catch (error) {
                    console.error(`Error loading folder ${folder.name}:`, error);
                    const folderElement = createFolderElement(folder.name, folder.path, [], true);
                    container.appendChild(folderElement);
                }
            }

            if (container.children.length === 0) {
                container.innerHTML = '<div class="file-tree-loading">No files found in ~/.madola directory.</div>';
            }
        } catch (error) {
            console.error('Error loading from filesystem:', error);
            container.innerHTML = '<div class="file-tree-loading">Error accessing ~/.madola directory.</div>';
        }
    }

    // Load files from memory (Web browser)
    async function loadFileTreeFromMemory(container) {
        container.innerHTML = '';

        // Get generated files from the app
        const app = window.madolaApp;
        if (!app) {
            container.innerHTML = '<div class="file-tree-loading">Run MADOLA code with @trove or @gen_cpp decorators to see generated files.</div>';
            return;
        }

        // Get gen_cpp files from last execution
        const genCppFiles = app.lastCppFiles || [];

        // Get trove files from last execution (if stored)
        const troveFiles = app.lastTroveFiles || [];

        // Create folders
        if (troveFiles.length > 0) {
            const troveFileNames = troveFiles.map(f => ({
                name: f.filename || f.name,
                content: f.content
            }));
            const folderElement = createFolderElement('trove', 'memory://trove', troveFileNames, false);
            container.appendChild(folderElement);
        }

        if (genCppFiles.length > 0) {
            const cppFileNames = genCppFiles.map(f => ({
                name: f.filename || f.name,
                content: f.content
            }));
            const folderElement = createFolderElement('gen_cpp', 'memory://gen_cpp', cppFileNames, false);
            container.appendChild(folderElement);
        }

        if (container.children.length === 0) {
            container.innerHTML = '<div class="file-tree-loading">No files generated yet. Run MADOLA code with @trove or @gen_cpp decorators.</div>';
        }
    }

    function createFolderElement(folderName, folderPath, files, isFilesystem) {
        const folderDiv = document.createElement('div');
        folderDiv.className = 'file-tree-folder';

        const header = document.createElement('div');
        header.className = 'file-tree-folder-header';
        header.innerHTML = `
            <span class="folder-icon">🔽</span>
            <span class="folder-name">📁 ${folderName}</span>
            <span class="file-count">(${files.length})</span>
        `;

        const filesContainer = document.createElement('div');
        filesContainer.className = 'file-tree-files';

        files.forEach(file => {
            const fileElement = createFileElement(file, folderPath, isFilesystem);
            filesContainer.appendChild(fileElement);
        });

        header.addEventListener('click', () => {
            const icon = header.querySelector('.folder-icon');
            icon.classList.toggle('collapsed');
            header.classList.toggle('expanded');
            filesContainer.classList.toggle('expanded');
        });

        folderDiv.appendChild(header);
        folderDiv.appendChild(filesContainer);

        return folderDiv;
    }

    function createFileElement(file, folderPath, isFilesystem) {
        const fileDiv = document.createElement('div');
        fileDiv.className = 'file-tree-file';

        const fileName = typeof file === 'string' ? file : (file.name || file.filename);
        const fileContent = typeof file === 'object' ? file.content : null;
        const ext = fileName.split('.').pop().toLowerCase();
        const icon = getFileIcon(ext);

        fileDiv.innerHTML = `
            <span class="file-icon-type">${icon}</span>
            <span class="file-name">${fileName}</span>
        `;

        fileDiv.addEventListener('click', () => {
            if (isFilesystem) {
                // Filesystem path
                const fullPath = `${folderPath}/${fileName}`;
                handleFileClick(fullPath, fileName, ext, null, true);
            } else {
                // In-memory file
                handleFileClick(null, fileName, ext, fileContent, false);
            }
        });

        return fileDiv;
    }

    function getFileIcon(ext) {
        const iconMap = {
            'cpp': '📄',
            'h': '📄',
            'hpp': '📄',
            'c': '📄',
            'txt': '📝',
            'md': '📝',
            'json': '📋',
            'xml': '📋'
        };
        return iconMap[ext] || '📄';
    }

    function handleFileClick(filePath, fileName, ext, fileContent, isFilesystem) {
        // For .cpp files, download or open in external editor
        if (ext === 'cpp' || ext === 'h' || ext === 'hpp' || ext === 'c') {
            if (isFilesystem) {
                openInExternalEditor(filePath);
            } else {
                // Web browser - download the file
                downloadFile(fileName, fileContent);
            }
        } else {
            // For other files
            if (isFilesystem) {
                openInExternalEditor(filePath);
            } else {
                downloadFile(fileName, fileContent);
            }
        }
    }

    async function openInExternalEditor(filePath) {
        try {
            // Try Electron API
            if (window.electronAPI && window.electronAPI.openExternal) {
                await window.electronAPI.openExternal(filePath);
                window.addMessageToSidebar(`Opening ${filePath} in external editor...`, 'success');
                return;
            }

            // Try Tauri API
            if (window.__TAURI__ && window.__TAURI__.shell) {
                await window.__TAURI__.shell.open(filePath);
                window.addMessageToSidebar(`Opening ${filePath} in external editor...`, 'success');
                return;
            }

            // Fallback: show message with path
            window.addMessageToSidebar(`Please open: ${filePath}`, 'info');
            console.log('Open file in external editor:', filePath);
        } catch (error) {
            console.error('Error opening file:', error);
            window.addMessageToSidebar(`Error opening file: ${error.message}`, 'error');
        }
    }

    function downloadFile(fileName, content) {
        try {
            // Create blob and download
            const blob = new Blob([content], { type: 'text/plain' });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = fileName;
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            URL.revokeObjectURL(url);

            window.addMessageToSidebar(`Downloaded: ${fileName}`, 'success');
        } catch (error) {
            console.error('Error downloading file:', error);
            window.addMessageToSidebar(`Error downloading file: ${error.message}`, 'error');
        }
    }

    async function getHomeDirectory() {
        try {
            // Try Electron API
            if (window.electronAPI && window.electronAPI.getHomeDir) {
                return await window.electronAPI.getHomeDir();
            }

            // Try Tauri API
            if (window.__TAURI__ && window.__TAURI__.path) {
                return await window.__TAURI__.path.homeDir();
            }
        } catch (error) {
            console.error('Error getting home directory:', error);
        }

        // Fallback to common paths
        if (navigator.platform.toLowerCase().includes('win')) {
            return 'C:/Users/User';
        } else {
            return '/home/user';
        }
    }

    async function listFilesFromFilesystem(dirPath) {
        try {
            // Try Electron API
            if (window.electronAPI && window.electronAPI.listFiles) {
                return await window.electronAPI.listFiles(dirPath);
            }

            // Try Tauri API
            if (window.__TAURI__ && window.__TAURI__.fs) {
                const entries = await window.__TAURI__.fs.readDir(dirPath);
                return entries.filter(e => !e.children).map(e => e.name);
            }
        } catch (error) {
            console.error('Error listing files:', error);
        }

        // Return empty array if API not available
        return [];
    }

    // Export functions for use in other modules
    window.fileBrowser = {
        loadFileTree,
        openInExternalEditor
    };

    // Initialize all sidebar functionality when DOM is ready
    document.addEventListener('DOMContentLoaded', () => {
        initSidebarTabs();
        initSidebarToggle();
        initPanelResize();
        initResizeHandleState();
        initThemeSettings();
        initSidebarMessages();
        initFormattingButtons();
        initGreekSymbols();
        initFileBrowser();
    });

})();
