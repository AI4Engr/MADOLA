// MADOLA Web App - UI Operations and Graph Rendering (Part 2B/2)
// This file extends MadolaApp with output operations, UI management, and graph rendering

Object.assign(MadolaApp.prototype, {
    // Output operations
    copyOutput() {
        const output = document.getElementById('output');
        const text = output.textContent || output.innerText;

        navigator.clipboard.writeText(text).then(() => {
            this.addMessage('success', 'Output copied to clipboard');
        }).catch(() => {
            this.addMessage('error', 'Failed to copy output');
        });
    },

    downloadHtml() {
        const output = document.getElementById('output');
        const html = output.innerHTML; // Get the HTML content

        // Create a complete HTML document
        const completeHtml = `<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MADOLA Output</title>
    <script>
        window.MathJax = {
            tex: {
                displayMath: [['$$', '$$'], ['\\[', '\\]']],
                inlineMath: [['$', '$'], ['\\(', '\\)']],
                processEscapes: true,
                processEnvironments: true
            },
            chtml: {
                scale: 0.9,
                displayAlign: 'left'
            }
        };
    </script>
    <script id="MathJax-script" async src="https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js"></script>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .output-container { max-width: 800px; margin: 0 auto; }
    </style>
</head>
<body>
    <div class="output-container">
        ${html}
    </div>
</body>
</html>`;

        // Generate filename: if currentFile exists, replace .mda with .html, otherwise use demo.html
        let filename;
        if (this.currentFile) {
            filename = this.currentFile.replace(/\.mda$/, '.html');
        } else {
            filename = 'demo.html';
        }
        this.downloadFile(filename, completeHtml);
    },

    async printPDF() {
        try {
            const output = document.getElementById('output');
            if (!output || !output.innerHTML.trim()) {
                this.addMessage('error', 'No output content to print');
                return;
            }

            // Wait for MathJax to complete rendering
            if (window.MathJax && window.MathJax.typesetPromise) {
                await window.MathJax.typesetPromise([output]);
            }

            // Wait a bit for any graphs to finish rendering
            await new Promise(resolve => setTimeout(resolve, 500));

            this.addMessage('info', 'Opening print dialog...');
            
            // Trigger browser print dialog
            // User can choose "Save as PDF" in the print dialog
            window.print();

            this.addMessage('success', 'Print dialog opened. Select "Save as PDF" to export.');
        } catch (error) {
            this.addMessage('error', `Print failed: ${error.message}`);
            console.error('Print error:', error);
        }
    },

    // UI operations
    addMessage(type, text) {
        const timestamp = `[${new Date().toLocaleTimeString()}]`;
        const fullText = `${timestamp} ${text}`;

        // Add to sidebar messages container (new UI)
        const sidebarMessages = document.getElementById('sidebar-messages-container');
        if (sidebarMessages) {
            const messageDiv = document.createElement('div');
            messageDiv.className = `message ${type}`;
            messageDiv.textContent = fullText;
            sidebarMessages.appendChild(messageDiv);
            sidebarMessages.scrollTop = sidebarMessages.scrollHeight;
        }

        // Also add to old messages container if it exists (backward compatibility)
        const messagesContainer = document.getElementById('messages');
        if (messagesContainer) {
            const messageDiv = document.createElement('div');
            messageDiv.className = `message ${type}`;
            messageDiv.textContent = fullText;
            messagesContainer.appendChild(messageDiv);
            messagesContainer.scrollTop = messagesContainer.scrollHeight;
        }
    },

    clearMessages() {
        const sidebarMessages = document.getElementById('sidebar-messages-container');
        if (sidebarMessages) {
            sidebarMessages.innerHTML = '';
        }
        const messages = document.getElementById('messages');
        if (messages) {
            messages.innerHTML = '';
        }
    },

    showAbout() {
        document.getElementById('about-modal').classList.remove('hidden');
    },

    hideAbout() {
        document.getElementById('about-modal').classList.add('hidden');
    },

    updateTitle() {
        const title = this.currentFile ?
            `MADOLA - ${this.currentFile}${this.hasUnsavedChanges ? '*' : ''}` :
            `MADOLA${this.hasUnsavedChanges ? '*' : ''}`;

        document.title = title;

        // Update Electron window title if running in Electron
        if (this.isElectron && window.electronAPI) {
            window.electronAPI.updateTitle(title);
        }
    },

    handleKeyboard(event) {
        // Ctrl+N - New file
        if (event.ctrlKey && event.key === 'n') {
            event.preventDefault();
            this.newFile();
        }
        // Ctrl+O - Open file
        else if (event.ctrlKey && event.key === 'o') {
            event.preventDefault();
            this.openFile();
        }
        // Ctrl+S - Save file
        else if (event.ctrlKey && event.key === 's') {
            event.preventDefault();
            if (event.shiftKey) {
                this.saveAsFile();
            } else {
                this.saveFile();
            }
        }
        // F5 or Ctrl+R - Run code
        else if (event.key === 'F5' || (event.ctrlKey && event.key === 'r')) {
            event.preventDefault();
            this.runCode();
        }
        // Ctrl+Shift+F - Format code
        else if (event.ctrlKey && event.shiftKey && event.key === 'F') {
            event.preventDefault();
            this.formatCode();
        }
    },

    // Tab Management
    initTabSystem() {
        const tabOutput = document.getElementById('tab-output');
        const tabCpp = document.getElementById('tab-cpp');
        const outputContent = document.getElementById('output-content');
        const cppContent = document.getElementById('cpp-content');

        tabOutput.addEventListener('click', () => {
            this.switchTab('output');
        });

        tabCpp.addEventListener('click', () => {
            this.switchTab('cpp');
        });
    },

    switchTab(tabName) {
        // Remove active class from all tabs and content
        document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
        document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));

        // Add active class to selected tab and content
        if (tabName === 'output') {
            document.getElementById('tab-output').classList.add('active');
            document.getElementById('output-content').classList.add('active');
        } else if (tabName === 'cpp') {
            document.getElementById('tab-cpp').classList.add('active');
            document.getElementById('cpp-content').classList.add('active');
        }
    },

    // C++ Files Management
    updateCppFiles(cppFiles) {
        // Store for later use by WASM compilation
        this.lastCppFiles = cppFiles;

        const fileList = document.getElementById('cpp-file-list');
        const fileViewer = document.getElementById('cpp-file-content');
        const fileName = document.getElementById('cpp-file-name');

        if (!cppFiles || cppFiles.length === 0) {
            // Dispose Monaco Editor if exists
            if (this.cppEditor) {
                this.cppEditor.dispose();
                this.cppEditor = null;
            }

            fileList.innerHTML = '<div class="no-files-message">No C++ files generated yet. Use @gen_cpp decorator on functions.</div>';
            fileViewer.innerHTML = '<div class="cpp-placeholder">Select a C++ file from the list to view its content.</div>';
            fileName.textContent = 'Select a file';

            // Refresh file browser
            if (window.fileBrowser && window.fileBrowser.loadFileTree) {
                window.fileBrowser.loadFileTree();
            }
            return;
        }

        // Save C++ files to disk automatically
        this.saveCppFilesToDisk(cppFiles);

        // Refresh file browser in sidebar
        if (window.fileBrowser && window.fileBrowser.loadFileTree) {
            window.fileBrowser.loadFileTree();
        }

        // Clear existing files
        fileList.innerHTML = '';

        // Add each C++ file to the list
        cppFiles.forEach((file, index) => {
            const fileItem = document.createElement('div');
            fileItem.className = 'file-item';
            fileItem.innerHTML = `
                <span class="file-icon">📄</span>
                <span class="file-name">${file.filename}</span>
            `;

            fileItem.addEventListener('click', () => {
                this.selectCppFile(file, fileItem);
            });

            fileList.appendChild(fileItem);

            // Select first file by default
            if (index === 0) {
                this.selectCppFile(file, fileItem);
            }
        });
    },

    async saveCppFilesToDisk(cppFiles) {
        try {
            // Check if running in Electron
            if (window.electronAPI && window.electronAPI.saveCppFile) {
                for (const file of cppFiles) {
                    const result = await window.electronAPI.saveCppFile({
                        filename: file.filename,
                        content: file.content
                    });
                    if (result.success) {
                        console.log(`Saved C++ file: ${file.filename} to ${result.path}`);
                        this.addMessage('success', `Saved ${file.filename}`);
                    } else {
                        console.error(`Failed to save C++ file: ${file.filename}`, result.error);
                    }
                }
            }
            // Check if running in Tauri
            else if (window.__TAURI__) {
                const { writeTextFile, BaseDirectory } = window.__TAURI__.fs;
                for (const file of cppFiles) {
                    try {
                        await writeTextFile(`gen_cpp/${file.filename}`, file.content, {
                            dir: BaseDirectory.App
                        });
                        console.log(`Saved C++ file: ${file.filename}`);
                        this.addMessage('success', `Saved ${file.filename}`);
                    } catch (err) {
                        console.error(`Failed to save C++ file: ${file.filename}`, err);
                    }
                }
            }
            // For web browser, send to server to save
            else {
                // Try to save via server API
                const savePromises = cppFiles.map(async (file) => {
                    try {
                        const response = await fetch('/api/save-cpp', {
                            method: 'POST',
                            headers: {
                                'Content-Type': 'application/json',
                            },
                            body: JSON.stringify({
                                filename: file.filename,
                                content: file.content
                            })
                        });

                        const result = await response.json();
                        if (result.success) {
                            console.log(`Saved C++ file: ${file.filename} to ${result.path}`);
                            return true;
                        } else {
                            console.error(`Failed to save C++ file: ${file.filename}`, result.error);
                            return false;
                        }
                    } catch (err) {
                        console.error(`Failed to save C++ file via server: ${file.filename}`, err);
                        return false;
                    }
                });

                const results = await Promise.all(savePromises);
                const successCount = results.filter(r => r).length;

                if (successCount > 0) {
                    this.addMessage('success', `Saved ${successCount} C++ file(s) to ~/.madola/gen_cpp/`);
                } else {
                    this.addMessage('warning', 'Failed to save C++ files - server may not be available');
                }
            }
        } catch (error) {
            console.error('Error saving C++ files to disk:', error);
            this.addMessage('error', `Failed to save C++ files: ${error.message}`);
        }
    },

    async promptForCppOutputDirectory(cppFiles) {
        try {
            const dirHandle = await window.showDirectoryPicker();
            this.cppOutputDir = dirHandle;
            // Now save the files
            for (const file of cppFiles) {
                const fileHandle = await dirHandle.getFileHandle(file.filename, { create: true });
                const writable = await fileHandle.createWritable();
                await writable.write(file.content);
                await writable.close();
            }
            this.addMessage('success', `Saved ${cppFiles.length} C++ file(s) to ${dirHandle.name}`);
        } catch (err) {
            if (err.name !== 'AbortError') {
                this.addMessage('error', `Failed to save C++ files: ${err.message}`);
            }
        }
    },

    // Handle WASM addon files (@gen_addon)
    async handleWasmFiles(wasmFiles) {
        console.log('Handling WASM addon files:', wasmFiles);

        for (const wasmFile of wasmFiles) {
            this.addMessage('info', `@gen_addon: ${wasmFile.functionName}`);

            // Find the corresponding C++ file content
            const cppContent = wasmFile.cppContent || this.findCppContent(wasmFile.functionName);

            if (!cppContent) {
                this.addMessage('warning', `No C++ content found for ${wasmFile.functionName}`);
                continue;
            }

            // Try to compile via server API
            try {
                const response = await fetch('/api/compile-wasm-content', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({
                        functionName: wasmFile.functionName,
                        cppContent: cppContent,
                        mdaFileName: this.currentFile || 'example'
                    })
                });

                const result = await response.json();
                if (result.success) {
                    this.addMessage('success', `✓ Compiled WASM addon: ${wasmFile.functionName}.wasm`);
                    console.log(`WASM addon saved to: ${result.wasmPath}`);
                } else {
                    this.addMessage('warning', `⚠ WASM compilation failed: ${result.error || 'Unknown error'}`);
                    console.log(`WASM addon not compiled: ${result.error}`);
                }
            } catch (error) {
                // Server API not available (expected in browser-only mode)
                this.addMessage('info', `C++ generated for ${wasmFile.functionName} - start server with 'node web/serve.js' to compile WASM`);
                console.log('Server API not available, WASM compilation skipped:', error);
            }
        }
    },

    // Find C++ content from the currently displayed C++ files
    findCppContent(functionName) {
        // Try to find in the current evaluation result
        const cppFiles = this.lastCppFiles || [];
        const cppFile = cppFiles.find(f => f.filename === `${functionName}.cpp`);
        return cppFile ? cppFile.content : null;
    },

    selectCppFile(file, fileItem) {
        // Remove selection from other items
        document.querySelectorAll('.file-item').forEach(item => {
            item.classList.remove('selected');
        });

        // Select current item
        fileItem.classList.add('selected');

        // Update file name
        document.getElementById('cpp-file-name').textContent = file.filename;

        // Display file content with Monaco Editor
        this.displayCppFileWithMonaco(file.content);
    },

    async displayCppFileWithMonaco(content) {
        const container = document.getElementById('cpp-file-content');

        // Clear existing content
        container.innerHTML = '';

        try {
            // Wait for Monaco Editor to be available
            await this.waitForMonaco();

            // Dispose existing editor if any
            if (this.cppEditor) {
                this.cppEditor.dispose();
            }

            // Create a div for the Monaco Editor
            const editorDiv = document.createElement('div');
            editorDiv.className = 'cpp-editor';
            container.appendChild(editorDiv);

            // Create Monaco Editor instance
            this.cppEditor = window.monaco.editor.create(editorDiv, {
                value: content,
                language: 'cpp',
                theme: this.isDarkTheme ? 'vs-dark' : 'vs',
                readOnly: true,
                minimap: { enabled: false },
                scrollBeyondLastLine: false,
                fontSize: 14,
                lineNumbers: 'on',
                folding: true,
                automaticLayout: true
            });

        } catch (error) {
            console.error('Failed to display C++ file with Monaco Editor:', error);
            // Fallback to simple syntax highlighting
            container.innerHTML = `<div class="cpp-code">${this.highlightCppSyntax(content)}</div>`;
        }
    },

    highlightCppSyntax(code) {
        // Simple syntax highlighting for C++
        return code
            .replace(/\n/g, '\n')
            .replace(/(#include|double|int|float|char|void|return|for|while|if|else)/g, '<span class="keyword">$1</span>')
            .replace(/(\w+)(\s*\()/g, '<span class="function">$1</span>$2')
            .replace(/\/\/(.*)/g, '<span class="comment">//$1</span>')
            .replace(/"([^"]*)"/g, '<span class="string">"$1"</span>')
            .replace(/\b(\d+\.?\d*)\b/g, '<span class="number">$1</span>');
    },

    // Graph Rendering
    renderGraphs(graphs) {
        console.log('DEBUG: renderGraphs called with:', graphs);

        // Check for D3.js availability - try multiple ways
        let d3Available = null;
        if (typeof window.d3 !== 'undefined') {
            d3Available = window.d3;
        } else if (typeof d3 !== 'undefined') {
            d3Available = d3;
            window.d3 = d3; // Store on window for consistency
        }

        console.log('DEBUG: D3 available:', !!d3Available);

        if (!d3Available) {
            console.error('D3.js not loaded - cannot render graphs');
            console.log('DEBUG: Attempting to retry D3.js loading...');

            // Try to load D3.js and retry
            this.loadD3AndRetryGraphs(graphs);
            return;
        }

        console.log('DEBUG: D3.js is loaded, rendering', graphs.length, 'graphs');
        graphs.forEach((graph, index) => {
            const containerId = `web-graph-${index}`;
            console.log(`DEBUG: Rendering graph ${index} in container ${containerId}`, graph);
            this.renderSingleGraph(graph, containerId);
        });
    },

    loadD3AndRetryGraphs(graphs) {
        console.log('DEBUG: D3.js not found, checking for alternative sources...');
        
        // Check if D3 is already being loaded or available
        if (document.querySelector('script[src*="d3"]')) {
            console.log('DEBUG: D3.js script already exists, waiting for it to load...');
            // Wait a bit longer for the existing script to load
            setTimeout(() => {
                if (typeof window.d3 !== 'undefined' || typeof d3 !== 'undefined') {
                    console.log('DEBUG: D3.js now available, retrying graphs...');
                    if (typeof d3 !== 'undefined' && typeof window.d3 === 'undefined') {
                        window.d3 = d3;
                    }
                    this.renderGraphs(graphs);
                } else {
                    this.showD3LoadError(graphs);
                }
            }, 1000);
            return;
        }

        // Try alternative CDN
        console.log('DEBUG: Loading D3.js from alternative CDN...');
        const script = document.createElement('script');
        script.src = 'https://cdn.jsdelivr.net/npm/d3@7/dist/d3.min.js';
        script.onload = () => {
            console.log('DEBUG: D3.js loaded successfully from alternative CDN, retrying graph rendering...');
            // Ensure D3.js is available on window object
            if (typeof d3 !== 'undefined') {
                window.d3 = d3;
            }
            // Small delay to ensure D3.js is fully initialized
            setTimeout(() => {
                this.renderGraphs(graphs);
            }, 100);
        };
        script.onerror = () => {
            console.error('DEBUG: Failed to load D3.js from alternative CDN');
            this.showD3LoadError(graphs);
        };
        document.head.appendChild(script);
    },

    showD3LoadError(graphs) {
        console.error('DEBUG: All D3.js loading attempts failed, showing error to user');
        // Show error message to user
        graphs.forEach((graph, index) => {
            const containerId = `web-graph-${index}`;
            const container = document.getElementById(containerId);
            if (container) {
                container.innerHTML = '<div class="error">Unable to load D3.js library. Graphs cannot be rendered.</div>';
            }
        });
    },

    renderSingleGraph(graphData, containerId) {
        console.log(`DEBUG: renderSingleGraph called for ${containerId}`, graphData);

        // Wait a bit more for DOM to be ready
        setTimeout(() => {
            const container = document.getElementById(containerId);
            if (!container) {
                console.error(`Graph container ${containerId} not found`);
                console.log('DEBUG: Available containers:',
                    Array.from(document.querySelectorAll('[id*="web-graph"]')).map(el => el.id));
                return;
            }
            console.log(`DEBUG: Found container ${containerId}`, container);

            // Get D3 reference
            const d3Ref = window.d3 || (typeof d3 !== 'undefined' ? d3 : null);
            if (!d3Ref) {
                console.error('D3 not available in renderSingleGraph');
                container.innerHTML = '<div class="error">D3.js not available for rendering graphs</div>';
                return;
            }

        // Clear existing content
        container.innerHTML = '';

        // Get actual container width dynamically
        const containerRect = container.getBoundingClientRect();
        const containerWidth = containerRect.width || container.offsetWidth || container.clientWidth;
        
        const margin = { top: 20, right: 20, bottom: 40, left: 50 };
        // Use 100% of available container width minus margins
        const width = containerWidth - margin.left - margin.right;
        const height = width * 0.5; // Proportional height based on width

        // Create fully responsive SVG with viewBox
        const svg = d3Ref.select(container)
            .append("svg")
            .attr("viewBox", `0 0 ${width + margin.left + margin.right} ${height + margin.top + margin.bottom}`)
            .attr("preserveAspectRatio", "xMidYMid meet")
            .style("width", "100%")
            .style("height", "auto");

        const g = svg.append("g")
            .attr("transform", `translate(${margin.left},${margin.top})`);

        // Prepare data
        const data = graphData.x_values.map((x, i) => ({
            x: x,
            y: graphData.y_values[i]
        }));

        // Set up scales
        const xScale = d3Ref.scaleLinear()
            .domain(d3Ref.extent(data, d => d.x))
            .range([0, width]);

        const yScale = d3Ref.scaleLinear()
            .domain(d3Ref.extent(data, d => d.y))
            .range([height, 0]);

        // Create line generator
        const line = d3Ref.line()
            .x(d => xScale(d.x))
            .y(d => yScale(d.y));

        // Add X axis
        g.append("g")
            .attr("class", "axis")
            .attr("transform", `translate(0,${height})`)
            .call(d3Ref.axisBottom(xScale));

        // Add Y axis
        g.append("g")
            .attr("class", "axis")
            .call(d3Ref.axisLeft(yScale));

        // Add line
        g.append("path")
            .datum(data)
            .attr("class", "line")
            .attr("fill", "none")
            .attr("stroke", "steelblue")
            .attr("stroke-width", 2)
            .attr("d", line);

        // Add dots
        g.selectAll(".dot")
            .data(data)
            .enter().append("circle")
            .attr("class", "dot")
            .attr("cx", d => xScale(d.x))
            .attr("cy", d => yScale(d.y))
            .attr("r", 4)
            .attr("fill", "steelblue");
        }, 100); // Close setTimeout with 100ms delay
    },

    // 3D Graph Rendering
    render3DGraphs(graphs3d) {
        console.log('DEBUG: render3DGraphs called with:', graphs3d);

        // Check for Three.js availability
        if (typeof THREE === 'undefined') {
            console.error('Three.js not loaded - cannot render 3D graphs');
            graphs3d.forEach((graph, index) => {
                const containerId = `web-graph3d-${index}`;
                const container = document.getElementById(containerId);
                if (container) {
                    container.innerHTML = '<div class="error">Three.js library not loaded. 3D graphs cannot be rendered.</div>';
                }
            });
            return;
        }

        console.log('DEBUG: Three.js is loaded, rendering', graphs3d.length, '3D graphs');

        // Add 3D graphs to the output container
        const outputDiv = document.getElementById('output');
        graphs3d.forEach((graph, index) => {
            const containerId = `web-graph3d-${index}`;
            console.log(`DEBUG: Rendering 3D graph ${index} in container ${containerId}`, graph);

            // Create container for the 3D graph
            const graphContainer = document.createElement('div');
            graphContainer.className = 'graph-container';
            graphContainer.innerHTML = `
                <div class="graph-title">${graph.title}</div>
                <div id="${containerId}" style="width: 100%; aspect-ratio: 4 / 3;"></div>
            `;
            outputDiv.appendChild(graphContainer);

            // Small delay to ensure DOM is ready
            setTimeout(() => {
                this.renderSingle3DGraph(graph, containerId);
            }, 100);
        });
    },

    renderSingle3DGraph(graphData, containerId) {
        console.log(`DEBUG: renderSingle3DGraph called for ${containerId}`, graphData);

        const container = document.getElementById(containerId);
        if (!container) {
            console.error(`DEBUG: Container ${containerId} not found`);
            return;
        }

        // Create Three.js scene
        const scene = new THREE.Scene();
        const renderer = new THREE.WebGLRenderer({ antialias: true });
        const DPR = (window.devicePixelRatio || 1);
        function getContainerSize() {
            const rect = container.getBoundingClientRect();
            const w = Math.max(1, Math.floor(rect.width));
            const h = Math.max(1, Math.floor(rect.height));
            return { width: w, height: h };
        }
        const initSize = getContainerSize();
        const camera = new THREE.PerspectiveCamera(75, initSize.width / initSize.height, 0.1, 1000);
        renderer.setPixelRatio(DPR);
        renderer.setSize(initSize.width, initSize.height);
        renderer.setClearColor(0xf0f0f0);
        renderer.shadowMap.enabled = true;
        renderer.shadowMap.type = THREE.PCFSoftShadowMap;
        container.appendChild(renderer.domElement);
        renderer.domElement.style.width = '100%';
        renderer.domElement.style.height = '100%';

        // Add lighting
        const ambientLight = new THREE.AmbientLight(0x404040, 0.6);
        scene.add(ambientLight);
        const directionalLight = new THREE.DirectionalLight(0xffffff, 1);
        directionalLight.position.set(10, 10, 5);
        directionalLight.castShadow = true;
        scene.add(directionalLight);

        // Create brick with hole geometry using the graph data
        const brickWidth = graphData.width;
        const brickHeight = graphData.height;
        const brickDepth = graphData.depth;
        const holeWidth = graphData.hole_width;
        const holeHeight = graphData.hole_height;
        const holeDepth = graphData.hole_depth;

        const brickMaterial = new THREE.MeshLambertMaterial({ color: 0x8B4513, transparent: true, opacity: 0.8 });
        const group = new THREE.Group();

        // Create brick frame structure to show hole (6 parts)
        const parts = [
            // Bottom part
            {
                geometry: new THREE.BoxGeometry(brickWidth, (brickHeight - holeHeight) / 2, brickDepth),
                position: [0, -(brickHeight + holeHeight) / 4, 0]
            },
            // Top part
            {
                geometry: new THREE.BoxGeometry(brickWidth, (brickHeight - holeHeight) / 2, brickDepth),
                position: [0, (brickHeight + holeHeight) / 4, 0]
            },
            // Left part
            {
                geometry: new THREE.BoxGeometry((brickWidth - holeWidth) / 2, holeHeight, brickDepth),
                position: [-(brickWidth + holeWidth) / 4, 0, 0]
            },
            // Right part
            {
                geometry: new THREE.BoxGeometry((brickWidth - holeWidth) / 2, holeHeight, brickDepth),
                position: [(brickWidth + holeWidth) / 4, 0, 0]
            },
            // Front part
            {
                geometry: new THREE.BoxGeometry(holeWidth, holeHeight, (brickDepth - holeDepth) / 2),
                position: [0, 0, (brickDepth + holeDepth) / 4]
            },
            // Back part
            {
                geometry: new THREE.BoxGeometry(holeWidth, holeHeight, (brickDepth - holeDepth) / 2),
                position: [0, 0, -(brickDepth + holeDepth) / 4]
            }
        ];

        parts.forEach(part => {
            const mesh = new THREE.Mesh(part.geometry, brickMaterial);
            mesh.position.set(...part.position);
            mesh.castShadow = true;
            mesh.receiveShadow = true;
            group.add(mesh);
        });

        scene.add(group);

        // Add wireframe overlay
        const wireframeGeometry = new THREE.BoxGeometry(brickWidth, brickHeight, brickDepth);
        const wireframeMaterial = new THREE.WireframeGeometry(wireframeGeometry);
        const wireframe = new THREE.LineSegments(wireframeMaterial, new THREE.LineBasicMaterial({ color: 0x000000, linewidth: 2 }));
        scene.add(wireframe);

        // Fit camera to object utility
        function fitCameraToObject(object, padding = 1.2) {
            const box = new THREE.Box3().setFromObject(object);
            const center = box.getCenter(new THREE.Vector3());
            const sizeVec = box.getSize(new THREE.Vector3());
            const radius = 0.5 * Math.sqrt(sizeVec.x*sizeVec.x + sizeVec.y*sizeVec.y + sizeVec.z*sizeVec.z);
            const fovy = THREE.MathUtils.degToRad(camera.fov);
            const fovh = 2 * Math.atan(Math.tan(fovy/2) * camera.aspect);
            const distV = (radius * padding) / Math.tan(fovy/2);
            const distH = (radius * padding) / Math.tan(fovh/2);
            const distance = Math.max(distV, distH);
            return { center, distance };
        }

        // Enhanced mouse controls
        let mouseDown = false;
        let rightMouseDown = false;
        let mouseX = 0;
        let mouseY = 0;
        let targetRotationX = 0;
        let targetRotationY = 0;
        let rotationX = 0;
        let rotationY = 0;
        let cameraDistance = 12;
        let targetCameraX = 0;
        let targetCameraY = 0;
        let cameraX = 0;
        let cameraY = 0;

        const fit = fitCameraToObject(group, 1.2);
        cameraDistance = fit.distance;
        const fitCenter = fit.center;
        camera.position.set(fitCenter.x + cameraDistance, fitCenter.y + cameraDistance*0.75, fitCenter.z + cameraDistance);
        camera.lookAt(fitCenter);

        function updateCameraPosition() {
            const distance = cameraDistance;
            const x = fitCenter.x + Math.cos(targetRotationY) * distance + cameraX;
            const z = fitCenter.z + Math.sin(targetRotationY) * distance;
            const y = fitCenter.y + Math.sin(targetRotationX) * distance + cameraY;
            camera.position.set(x, y, z);
            camera.lookAt(fitCenter.x + cameraX, fitCenter.y + cameraY, fitCenter.z);
        }

        container.addEventListener('mousedown', function(event) {
            event.preventDefault();
            if (event.button === 0) {
                mouseDown = true;
            } else if (event.button === 2) {
                rightMouseDown = true;
            }
            mouseX = event.clientX;
            mouseY = event.clientY;
        });

        container.addEventListener('mouseup', function(event) {
            event.preventDefault();
            if (event.button === 0) {
                mouseDown = false;
            } else if (event.button === 2) {
                rightMouseDown = false;
            }
        });

        container.addEventListener('contextmenu', function(event) {
            event.preventDefault();
        });

        container.addEventListener('mousemove', function(event) {
            event.preventDefault();
            if (mouseDown) {
                const deltaX = event.clientX - mouseX;
                const deltaY = event.clientY - mouseY;
                targetRotationY += deltaX * 0.01;
                targetRotationX += deltaY * 0.01;
                targetRotationX = Math.max(-Math.PI/2, Math.min(Math.PI/2, targetRotationX));
            } else if (rightMouseDown) {
                const deltaX = event.clientX - mouseX;
                const deltaY = event.clientY - mouseY;
                targetCameraX -= deltaX * 0.02;
                targetCameraY += deltaY * 0.02;
            }
            mouseX = event.clientX;
            mouseY = event.clientY;
        });

        container.addEventListener('wheel', function(event) {
            event.preventDefault();
            const zoomSpeed = 0.1;
            if (event.deltaY > 0) {
                cameraDistance = Math.min(50, cameraDistance + zoomSpeed);
            } else {
                cameraDistance = Math.max(0.5, cameraDistance - zoomSpeed);
            }
        });

        // Resize handling
        const handleResize = () => {
            const s = getContainerSize();
            renderer.setSize(s.width, s.height);
            camera.aspect = s.width / s.height;
            camera.updateProjectionMatrix();
            const nf = fitCameraToObject(group, 1.2);
            cameraDistance = nf.distance;
        };
        if (typeof ResizeObserver !== 'undefined') {
            const ro = new ResizeObserver(handleResize);
            ro.observe(container);
        } else {
            window.addEventListener('resize', handleResize);
        }

        function animate() {
            requestAnimationFrame(animate);

            // Smooth interpolation
            rotationX += (targetRotationX - rotationX) * 0.1;
            rotationY += (targetRotationY - rotationY) * 0.1;
            cameraX += (targetCameraX - cameraX) * 0.1;
            cameraY += (targetCameraY - cameraY) * 0.1;

            updateCameraPosition();

            renderer.render(scene, camera);
        }

        handleResize();
        updateCameraPosition();
        animate();
    },

    // Mobile toggle functionality
    initMobileToggles() {
        const latexToggleBtn = document.getElementById('btn-toggle-latex');
        const messagesToggleBtn = document.getElementById('btn-toggle-messages');
        const latexPanel = document.querySelector('.latex-panel');
        const messagesPanel = document.querySelector('.progress-panel');
        const overlay = document.getElementById('mobile-overlay');
        const closeLatexBtn = document.getElementById('close-latex-panel');
        const closeMessagesBtn = document.getElementById('close-messages-panel');

        // Toggle LaTeX panel
        if (latexToggleBtn) {
            latexToggleBtn.addEventListener('click', () => {
                this.toggleMobilePanel(latexPanel, overlay, latexToggleBtn);
            });
        }

        // Toggle Messages panel
        if (messagesToggleBtn) {
            messagesToggleBtn.addEventListener('click', () => {
                this.toggleMobilePanel(messagesPanel, overlay, messagesToggleBtn);
            });
        }

        // Close buttons
        if (closeLatexBtn) {
            closeLatexBtn.addEventListener('click', (e) => {
                e.preventDefault();
                e.stopPropagation();
                this.closeMobilePanel(latexPanel, overlay, latexToggleBtn);
            });
        }

        if (closeMessagesBtn) {
            closeMessagesBtn.addEventListener('click', (e) => {
                e.preventDefault();
                e.stopPropagation();
                this.closeMobilePanel(messagesPanel, overlay, messagesToggleBtn);
            });
        }

        // Close on overlay click
        if (overlay) {
            overlay.addEventListener('click', () => {
                this.closeAllMobilePanels(overlay);
            });
        }

        // Close on escape key
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape') {
                this.closeAllMobilePanels(overlay);
            }
        });
    },

    toggleMobilePanel(panel, overlay, toggleBtn) {
        if (panel && overlay && toggleBtn) {
            const isVisible = panel.classList.contains('mobile-visible');

            if (isVisible) {
                this.closeMobilePanel(panel, overlay, toggleBtn);
            } else {
                // Close any other open panels first
                this.closeAllMobilePanels(overlay, false);

                // Open this panel
                panel.classList.add('mobile-visible');
                overlay.classList.add('visible');
                toggleBtn.classList.add('active');

                // Prevent body scroll
                document.body.style.overflow = 'hidden';
            }
        }
    },

    closeMobilePanel(panel, overlay, toggleBtn) {
        if (panel && overlay && toggleBtn) {
            panel.classList.remove('mobile-visible');
            overlay.classList.remove('visible');
            toggleBtn.classList.remove('active');

            // Restore body scroll
            document.body.style.overflow = '';
        }
    },

    closeAllMobilePanels(overlay, restoreScroll = true) {
        const panels = document.querySelectorAll('.latex-panel, .progress-panel');
        const toggleBtns = document.querySelectorAll('#btn-toggle-latex, #btn-toggle-messages');

        panels.forEach(panel => {
            panel.classList.remove('mobile-visible');
        });

        toggleBtns.forEach(btn => {
            btn.classList.remove('active');
        });

        if (overlay) {
            overlay.classList.remove('visible');
        }

        if (restoreScroll) {
            // Restore body scroll
            document.body.style.overflow = '';
        }
    }
});

// Initialize the application when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    window.madolaApp = new MadolaApp();
});