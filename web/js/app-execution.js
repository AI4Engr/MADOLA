// MADOLA Web App - Code Execution and File Operations (Part 2A/2)
// This file extends MadolaApp with file operations, code execution, and HTML rendering

Object.assign(MadolaApp.prototype, {
    // File operations
    newFile() {
        if (this.hasUnsavedChanges) {
            if (!confirm('You have unsaved changes. Are you sure you want to create a new file?')) {
                return;
            }
        }

        if (this.mainEditor) {
            // Temporarily disable auto-run to prevent unnecessary execution on empty content
            const wasAutoRunEnabled = this.autoRunEnabled;
            this.autoRunEnabled = false;
            
            this.mainEditor.setValue('');
            
            // Re-enable auto-run
            this.autoRunEnabled = wasAutoRunEnabled;
        }
        
        document.getElementById('output').innerHTML = '<div class="output-placeholder"><p>Enter your MADOLA code to see the output here.</p></div>';
        this.currentFile = null;
        this.hasUnsavedChanges = false;
        this.updateTitle();
        this.addMessage('info', 'New file created');
        
        // Clear any pending auto-run timeouts
        clearTimeout(this.autoRunTimeout);
    },

    openFile() {
        if (this.isElectron) {
            // Electron will handle the file dialog
            return;
        }
        document.getElementById('file-input').click();
    },

    handleFileOpen(event) {
        const file = event.target.files[0];
        if (!file) return;

        const reader = new FileReader();
        reader.onload = (e) => {
            if (this.mainEditor) {
                // Temporarily disable auto-run to prevent double execution
                const wasAutoRunEnabled = this.autoRunEnabled;
                this.autoRunEnabled = false;
                
                this.mainEditor.setValue(e.target.result);
                
                // Re-enable auto-run
                this.autoRunEnabled = wasAutoRunEnabled;
            }
            this.currentFile = file.name;
            this.hasUnsavedChanges = false;
            this.updateTitle();
            this.addMessage('success', `Opened: ${file.name}`);
            
            // Clear any pending auto-run timeouts and run once
            clearTimeout(this.autoRunTimeout);
            setTimeout(() => this.runCode(), 100);
        };
        reader.readAsText(file);
    },

    saveFile() {
        if (this.isElectron) {
            this.saveFileElectron();
        } else {
            if (this.currentFile) {
                const content = this.mainEditor ? this.mainEditor.getValue() : '';
                this.downloadFile(this.currentFile, content);
            } else {
                this.saveAsFile();
            }
        }
    },

    saveAsFile() {
        if (this.isElectron) {
            // Electron will handle the save dialog
            return;
        }

        const filename = prompt('Save as filename:', this.currentFile || 'untitled.mda');
        if (filename) {
            const content = this.mainEditor ? this.mainEditor.getValue() : '';
            this.downloadFile(filename, content);
            this.currentFile = filename;
            this.hasUnsavedChanges = false;
            this.updateTitle();
        }
    },

    // Electron-specific save operations
    async saveFileElectron() {
        if (!this.isElectron) return;

        const currentFile = await window.electronAPI.getCurrentFile();
        if (currentFile) {
            await this.saveFileToPath(currentFile);
        }
    },

    async saveFileToPath(filePath) {
        if (!this.isElectron) return;

        try {
            const content = this.mainEditor ? this.mainEditor.getValue() : '';
            const result = await window.electronAPI.saveFile({ path: filePath, content });

            if (result.success) {
                this.currentFile = filePath.split(/[/\\]/).pop(); // Get filename from path
                this.hasUnsavedChanges = false;
                this.updateTitle();
                await window.electronAPI.setCurrentFile(filePath);
                this.addMessage('success', `Saved: ${this.currentFile}`);
            } else {
                this.addMessage('error', `Save failed: ${result.error}`);
            }
        } catch (error) {
            this.addMessage('error', `Save error: ${error.message}`);
        }
    },

    downloadFile(filename, content) {
        const blob = new Blob([content], { type: 'text/plain' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        a.click();
        URL.revokeObjectURL(url);
        this.addMessage('success', `Saved: ${filename}`);
    },

    // Code operations
    async formatCode() {
        if (!this.madolaWrapper || !this.madolaWrapper.isReady()) {
            this.addMessage('error', 'WASM module not loaded');
            return;
        }

        try {
            // Set waiting cursor and allow browser to render it
            document.body.classList.add('computing');
            await new Promise(resolve => setTimeout(resolve, 50));

            const code = this.mainEditor ? this.mainEditor.getValue() : '';
            const formatted = await this.madolaWrapper.format(code, 1);

            if (formatted && this.mainEditor) {
                this.mainEditor.setValue(formatted);
                this.hasUnsavedChanges = true;
                this.updateTitle();
                this.addMessage('success', 'Code formatted successfully');
            } else {
                this.addMessage('error', 'Formatting failed');
            }
        } catch (error) {
            this.addMessage('error', `Formatting error: ${error.message}`);
        } finally {
            // Remove waiting cursor
            document.body.classList.remove('computing');
        }
    },

    async loadExample(filename) {
        if (!filename) return;

        try {
            const response = await fetch(`examples/${filename}`);
            if (!response.ok) {
                throw new Error(`Failed to load example: ${response.status}`);
            }

            const content = await response.text();
            if (this.mainEditor) {
                // Temporarily disable auto-run to prevent double execution
                const wasAutoRunEnabled = this.autoRunEnabled;
                this.autoRunEnabled = false;
                
                this.mainEditor.setValue(content);
                this.hasUnsavedChanges = false;
                this.currentFile = filename;
                this.updateTitle();
                this.addMessage('success', `Loaded example: ${filename}`);

                // Re-enable auto-run and run once
                this.autoRunEnabled = wasAutoRunEnabled;
                
                // Clear any pending auto-run timeouts
                clearTimeout(this.autoRunTimeout);
                
                // Run the example once with a small delay to ensure DOM is ready
                setTimeout(() => this.runCode(), 100);
            }
        } catch (error) {
            this.addMessage('error', `Failed to load example: ${error.message}`);
        }
    },

    async runCode() {
        if (!this.madolaWrapper || !this.madolaWrapper.isReady()) {
            this.addMessage('warning', 'WASM module not available. Showing demo output.');
            await this.showFallbackOutput();
            return;
        }

        try {
            const code = this.mainEditor ? this.mainEditor.getValue().trim() : '';
            if (!code) {
                document.getElementById('output').innerHTML = '<div class="output-placeholder"><p>Enter your MADOLA code to see the output here.</p></div>';
                return;
            }

            // Set waiting cursor and allow browser to render it
            document.body.classList.add('computing');
            this.addMessage('info', 'Running MADOLA code...');

            // Use setTimeout to allow browser to update cursor before heavy computation
            await new Promise(resolve => setTimeout(resolve, 50));

            // Pre-load any required WASM functions by parsing import statements
            await this.preloadWasmFunctions(code);

            // First, try to format the code to get HTML output
            const htmlOutput = await this.madolaWrapper.formatHtml(code, 1);

            if (htmlOutput) {
                // Display the HTML directly in the output container
                document.getElementById('output').innerHTML = htmlOutput;

                // Execute embedded scripts (innerHTML doesn't execute scripts automatically)
                this.executeEmbeddedScripts(document.getElementById('output'));

                // Render MathJax for the HTML content
                await this.renderMathJax(document.getElementById('output'));

                this.addMessage('success', 'Code executed and formatted successfully');

                // Also evaluate to get C++ files and graphs
                const evaluated = await this.madolaWrapper.evaluate(code);
                if (evaluated) {
                    const evalResult = JSON.parse(evaluated);
                    if (evalResult.success) {
                        // Update C++ files
                        if (evalResult.cppFiles && evalResult.cppFiles.length > 0) {
                            this.updateCppFiles(evalResult.cppFiles);
                            this.addMessage('info', `Generated ${evalResult.cppFiles.length} C++ file(s)`);
                        } else {
                            this.updateCppFiles([]);
                        }

                        // Handle WASM addon files (@gen_addon)
                        if (evalResult.wasmFiles && evalResult.wasmFiles.length > 0) {
                            this.handleWasmFiles(evalResult.wasmFiles);
                        }

                        // Handle graphs (both 2D and 3D)
                        const totalGraphs = (evalResult.graphs ? evalResult.graphs.length : 0) +
                                          (evalResult.graphs3d ? evalResult.graphs3d.length : 0);

                        // Replace graph placeholders with actual graphs if any exist
                        if (evalResult.graphs && evalResult.graphs.length > 0) {
                            const outputDiv = document.getElementById('output');
                            let htmlContent = outputDiv.innerHTML;
                            let placeholderFound = false;

                            evalResult.graphs.forEach((graph, index) => {
                                const placeholder = `<!-- GRAPH_PLACEHOLDER_${index} -->`;
                                if (htmlContent.includes(placeholder)) {
                                    placeholderFound = true;
                                }
                            });

                            let graphContainersReady = false;

                            if (placeholderFound) {
                                // Replace each graph placeholder with actual graph HTML
                                evalResult.graphs.forEach((graph, index) => {
                                    const placeholder = `<!-- GRAPH_PLACEHOLDER_${index} -->`;
                                    const graphTitle = graph.title || `Graph ${index + 1}`;
                                    const graphHtml = `<div class="graph-container">
                                        <div class="graph-title">${graphTitle}</div>
                                        <div id="web-graph-${index}" class="web-graph"></div>
                                    </div>`;
                                    htmlContent = htmlContent.replace(placeholder, graphHtml);
                                });

                                outputDiv.innerHTML = htmlContent;
                                await this.renderMathJax(outputDiv);
                                graphContainersReady = true;
                            } else {
                                // Try to reuse legacy graph containers (graph0, graph1, ...)
                                const legacyConverted = this.convertLegacyGraphContainers(evalResult.graphs.length);
                                if (legacyConverted) {
                                    graphContainersReady = true;
                                } else {
                                    // Fallback: append graphs at the end
                                    evalResult.graphs.forEach((graph, index) => {
                                        const graphTitle = graph.title || `Graph ${index + 1}`;
                                        const graphHtml = `<div class="graph-container">
                                            <div class="graph-title">${graphTitle}</div>
                                            <div id="web-graph-${index}" class="web-graph"></div>
                                        </div>`;
                                        htmlContent += graphHtml;
                                    });

                                    outputDiv.innerHTML = htmlContent;
                                    await this.renderMathJax(outputDiv);
                                    graphContainersReady = true;
                                }
                            }

                            if (graphContainersReady) {
                                setTimeout(() => {
                                    this.renderGraphs(evalResult.graphs);
                                }, 0);
                                this.addMessage('info', `Generated ${evalResult.graphs.length} 2D graph(s) in code order`);
                            }
                        }

                        // Handle 3D graphs - they are already embedded in the HTML by the formatter
                        if (evalResult.graphs3d && evalResult.graphs3d.length > 0) {
                            this.addMessage('info', `Generated ${evalResult.graphs3d.length} 3D graph(s)`);
                        }

                        if (totalGraphs === 0) {
                            // No graphs detected
                        }
                    } else {
                        this.updateCppFiles([]);
                    }
                } else {
                    this.updateCppFiles([]);
                }
            } else {
                // If formatting fails, try evaluation
                const evaluated = await this.madolaWrapper.evaluate(code);

                if (evaluated) {
                    const evalResult = JSON.parse(evaluated);
                    if (evalResult.success && evalResult.outputs) {
                        let output = `<div class="output-content"><h3>Execution Result</h3><pre><code>${evalResult.outputs.join('\n')}</code></pre>`;

                        // Add graphs if any exist
                        if (evalResult.graphs && evalResult.graphs.length > 0) {
                            output += '<h3>Graphs</h3>';
                            evalResult.graphs.forEach((graph, index) => {
                                const graphTitle = graph.title || `Graph ${index + 1}`;
                                output += `<div class="graph-container">
                                    <div class="graph-title">${graphTitle}</div>
                                    <div id="web-graph-${index}" class="web-graph"></div>
                                </div>`;
                            });
                        }

                        output += '</div>';
                        document.getElementById('output').innerHTML = output;

                        // Render graphs if any exist
                        if (evalResult.graphs && evalResult.graphs.length > 0) {
                            setTimeout(() => {
                                this.renderGraphs(evalResult.graphs);
                            }, 0);
                            this.addMessage('success', `Code executed successfully with ${evalResult.graphs.length} graph(s)`);
                        } else {
                            this.addMessage('success', 'Code executed successfully');
                        }

                        // Update C++ files if any were generated
                        if (evalResult.cppFiles && evalResult.cppFiles.length > 0) {
                            this.updateCppFiles(evalResult.cppFiles);
                            this.addMessage('info', `Generated ${evalResult.cppFiles.length} C++ file(s)`);
                        } else {
                            this.updateCppFiles([]);
                        }

                        // Handle WASM addon files (@gen_addon)
                        if (evalResult.wasmFiles && evalResult.wasmFiles.length > 0) {
                            this.handleWasmFiles(evalResult.wasmFiles);
                        }
                    } else if (evalResult.error) {
                        this.addMessage('error', evalResult.error);
                        document.getElementById('output').innerHTML = `<div class="output-content"><div class="message error">${evalResult.error}</div></div>`;
                        this.updateCppFiles([]);
                    }
                } else {
                    this.addMessage('error', 'Execution failed');
                    this.updateCppFiles([]);
                }
            }
        } catch (error) {
            this.addMessage('error', `Execution error: ${error.message}`);
        } finally {
            // Remove waiting cursor
            document.body.classList.remove('computing');
        }
    },

    executeEmbeddedScripts(container) {
        const scripts = container.querySelectorAll('script');

        scripts.forEach((script) => {
            try {
                // Skip scripts that try to load libraries we already have
                if (script.src) {
                    const src = script.src.toLowerCase();
                    if (src.includes('d3js.org') || src.includes('d3.v') || 
                        src.includes('three.js') || src.includes('three.min.js')) {
                        console.log('Skipping library script that is already loaded:', script.src);
                        return;
                    }
                }

                // Create a new script element and copy the content
                const newScript = document.createElement('script');
                if (script.src) {
                    newScript.src = script.src;
                } else {
                    // For inline scripts, check if they're trying to load libraries
                    const content = script.textContent || '';
                    if (content.includes('d3js.org') || content.includes('d3.v')) {
                        console.log('Skipping inline script that tries to load D3.js');
                        return;
                    }
                    newScript.textContent = content;
                }

                // Replace the old script with the new one to trigger execution
                script.parentNode.replaceChild(newScript, script);
            } catch (error) {
                console.error('Error executing embedded script:', error);
            }
        });
    },

    convertLegacyGraphContainers(graphCount) {
        const outputDiv = document.getElementById('output');
        if (!outputDiv || !graphCount) {
            return false;
        }

        const legacyContainers = Array.from(outputDiv.querySelectorAll('div[id^="graph"]'));
        if (!legacyContainers.length) {
            return false;
        }

        let converted = false;

        legacyContainers.forEach((container) => {
            const match = container.id && container.id.match(/^graph(\d+)$/);
            if (!match) {
                return;
            }

            const index = parseInt(match[1], 10);
            if (Number.isNaN(index) || index >= graphCount) {
                return;
            }

            const newId = `web-graph-${index}`;
            container.id = newId;
            container.classList.add('web-graph');
            converted = true;

            // Remove inline script that targets this legacy container
            let sibling = container.nextElementSibling;
            while (sibling && sibling.tagName === 'SCRIPT') {
                const scriptContent = sibling.textContent || '';
                if (scriptContent.includes(`#graph${index}`)) {
                    sibling.remove();
                    break;
                }
                sibling = sibling.nextElementSibling;
            }
        });

        return converted;
    },

    async preloadWasmFunctions(code) {
        try {
            // Parse import statements from the code - support all syntax patterns
            // Pattern 1: import funcName from "module"
            // Pattern 2: from module import funcName;
            // Pattern 3: import funcName; (simple import, module = funcName)
            const importMatches1 = code.match(/import\s+(.+?)\s+from\s+["'](.+?)["']/g) || [];
            const importMatches2 = code.match(/from\s+(\w+)\s+import\s+(.+?);/g) || [];
            const importMatches3 = code.match(/^import\s+(\w+);/gm) || [];

            const allMatches = [...importMatches1, ...importMatches2, ...importMatches3];

            if (allMatches.length === 0) {
                return; // No imports found
            }

            this.addMessage('info', 'Pre-loading WASM functions...');

            for (const importMatch of allMatches) {
                let functionNames = [];
                let moduleName = '';

                // Handle pattern 1: import funcName from "module"
                const match1 = importMatch.match(/import\s+(.+?)\s+from\s+["'](.+?)["']/);
                if (match1) {
                    functionNames = match1[1].split(',').map(name => name.trim());
                    moduleName = match1[2];
                } else {
                    // Handle pattern 2: from module import funcName;
                    const match2 = importMatch.match(/from\s+(\w+)\s+import\s+(.+?);/);
                    if (match2) {
                        moduleName = match2[1];
                        functionNames = match2[2].split(',').map(name => name.trim());
                    } else {
                        // Handle pattern 3: import funcName; (simple import)
                        const match3 = importMatch.match(/^import\s+(\w+);/);
                        if (match3) {
                            const funcName = match3[1];
                            functionNames = [funcName];
                            moduleName = funcName; // For simple imports, module name = function name
                        }
                    }
                }

                for (const functionName of functionNames) {
                    if (!window.madolaWasmBridge[functionName]) {
                        const jsWrapperPath = `trove/${moduleName}/${functionName}.js`;
                        const addon = await this.loadWasmFunction(functionName, jsWrapperPath);
                        if (addon) {
                            window.madolaWasmBridge[functionName] = (n) => addon[functionName](n);
                            this.addMessage('success', `${functionName} function loaded successfully`);
                        } else {
                            this.addMessage('warning', `Failed to load ${functionName} function`);
                        }
                    }
                }
            }
        } catch (error) {
            this.addMessage('warning', `Error pre-loading WASM functions: ${error.message}`);
        }
    },

    async showFallbackOutput() {
        const code = this.mainEditor ? this.mainEditor.getValue().trim() : '';
        if (!code) {
            document.getElementById('output').innerHTML = '<div class="output-placeholder"><p>Enter your MADOLA code to see the output here.</p></div>';
            return;
        }

        // Create a demo output showing what the formatted result would look like
        const demoHtml = `<div class="demo-output">
            <h2>MADOLA Code Execution</h2>
            <p><strong>Input Code:</strong></p>
            <pre><code>${code}</code></pre>
            <p><strong>Demo Output:</strong></p>
            <p>This is a demonstration of what MADOLA output would look like.</p>
            <p><strong>Mathematical Expression:</strong></p>
            $$f(x) = x^2 + 2x + 1$$
            <p><strong>Note:</strong> WASM module not loaded. This is placeholder content.
            To see actual results, ensure the WASM module loads properly.</p>
        </div>`;

        document.getElementById('output').innerHTML = demoHtml;

        // Render MathJax for the demo content
        await this.renderMathJax(document.getElementById('output'));

        this.addMessage('info', 'Showing demo output (WASM not available)');
    },

    processMathExpressions(html) {
        // Ensure math expressions are properly formatted for MathJax
        // Handle both inline and display math

        // Fix common issues with backslashes in math expressions
        let processed = html.replace(/\$\$([\s\S]*?)\$\$/g, (match, mathContent) => {
            // Clean up math content
            let cleaned = mathContent.trim();
            // Ensure proper escaping for common LaTeX commands
            cleaned = cleaned.replace(/\\([a-zA-Z]+)/g, '\\$1');
            return `$$${cleaned}$$`;
        });

        processed = processed.replace(/\$([^$\n]+?)\$/g, (match, mathContent) => {
            // Clean up inline math content
            let cleaned = mathContent.trim();
            cleaned = cleaned.replace(/\\([a-zA-Z]+)/g, '\\$1');
            return `$${cleaned}$`;
        });

        return processed;
    },

    async renderMathJax(container) {
        try {
            // Wait for MathJax to be available
            let attempts = 0;
            const maxAttempts = 50;

            while (!window.MathJax && attempts < maxAttempts) {
                await new Promise(resolve => setTimeout(resolve, 100));
                attempts++;
            }

            if (!window.MathJax) {
                console.warn('MathJax not available for rendering');
                return;
            }

            // Wait for MathJax to be fully ready
            if (window.MathJax.startup && !window.MathJax.startup.document.state.ready) {
                await new Promise(resolve => {
                    window.MathJax.startup.promise.then(resolve);
                });
            }

            // Re-render MathJax
            if (window.MathJax.typesetPromise) {
                console.log('Rendering MathJax for container', container);
                await window.MathJax.typesetPromise([container]);
                console.log('MathJax rendering complete');
            } else if (window.MathJax.typeset) {
                window.MathJax.typeset([container]);
            }
        } catch (error) {
            console.error('MathJax rendering error:', error);
        }
    }
});