#!/usr/bin/env node

const MADOLA = require('./web/runtime/madola.js');
const fs = require('fs');
const path = require('path');

// Setup global bridge for WASM imports
// We create a window object with madolaWasmBridge that wrappers can check but also access
const madolaBridge = {};
const windowProxy = {
    madolaWasmBridge: madolaBridge
};

// Set global.window for WASM runtime access
global.window = windowProxy;

// Function to load WASM module dynamically
async function loadWasmModule(moduleName, functionName) {
    try {
        const jsWrapperPath = path.join(__dirname, 'web', 'trove', moduleName, `${functionName}.js`);
        
        if (!fs.existsSync(jsWrapperPath)) {
            return null;
        }

        // Delete window temporarily so wrapper detects Node.js
        const savedWindow = global.window;
        delete global.window;
        
        // Load the wrapper class
        const WrapperClass = require(jsWrapperPath);
        const basePath = path.join(__dirname, 'web', 'trove', moduleName);
        
        // Create instance and load (it will use Node.js fs.readFileSync)
        const addon = new WrapperClass(basePath);
        const loaded = await addon.load();
        
        // Restore window
        global.window = savedWindow;
        
        if (!loaded) {
            return null;
        }
        
        return addon;
    } catch (error) {
        // Restore window if error
        if (!global.window && windowProxy) {
            global.window = windowProxy;
        }
        return null;
    }
}

// Preload WASM modules from manifest
async function setupWasmBridge() {
    try {
        const manifestPath = path.join(__dirname, 'web', 'trove', 'manifest.json');
        if (!fs.existsSync(manifestPath)) {
            // No manifest - WASM imports won't work but that's ok for tests without imports
            return;
        }

        const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
        
        for (const [moduleName, functions] of Object.entries(manifest)) {
            for (const functionName of functions) {
                const addon = await loadWasmModule(moduleName, functionName);
                if (addon) {
                    madolaBridge[functionName] = (...args) => addon[functionName](...args);
                    // Successfully loaded
                }
            }
        }
    } catch (error) {
        // Silent failure - tests without WASM imports will still work
    }
}

async function main() {
    try {
        const args = process.argv.slice(2);
        if (args.length < 1) {
            console.error('Usage: node madola_runner.js <input.mda> [--format|--html] [--output <file>]');
            process.exit(1);
        }

        const inputFile = args[0];
        const isFormat = args.includes('--format');
        const isHtml = args.includes('--html');
        const outputIndex = args.indexOf('--output');
        const outputFile = outputIndex !== -1 ? args[outputIndex + 1] : null;

        // Setup WASM bridge before initializing MADOLA
        await setupWasmBridge();

        // Initialize WASM module
        const runtimeLogs = [];
        const madola = await MADOLA({
            print: (text) => {
                if (typeof text === 'string' && text.length > 0) {
                    runtimeLogs.push(text);
                }
            },
            printErr: (text) => {
                if (typeof text === 'string' && text.length > 0) {
                    runtimeLogs.push(text);
                }
            }
        });

        // Read input file
        const testContent = fs.readFileSync(inputFile, 'utf8');

        let result;
        if (isHtml) {
            // HTML format mode
            const htmlResultPtr = madola.ccall('format_madola_html', 'number', ['string', 'number'], [testContent, 1]);
            if (htmlResultPtr !== 0) {
                result = madola.UTF8ToString(htmlResultPtr);
                madola.ccall('free_result', null, ['number'], [htmlResultPtr]);
            } else {
                result = 'Error: WASM HTML formatting failed';
            }
        } else if (isFormat) {
            // Markdown format mode
            const formatResultPtr = madola.ccall('format_madola', 'number', ['string', 'number'], [testContent, 1]);
            if (formatResultPtr !== 0) {
                result = madola.UTF8ToString(formatResultPtr);
                madola.ccall('free_result', null, ['number'], [formatResultPtr]);
            } else {
                result = 'Error: WASM formatting failed';
            }
        } else {
            // Evaluation mode
            try {
                const evalResultPtr = madola.ccall('evaluate_madola', 'number', ['string'], [testContent]);
                if (evalResultPtr !== 0) {
                    const evaluated = madola.UTF8ToString(evalResultPtr);
                    madola.ccall('free_result', null, ['number'], [evalResultPtr]);

                    // Parse the JSON response and extract outputs with better diagnostics
                    let evalResult;
                    try {
                        evalResult = JSON.parse(evaluated);
                    } catch (parseErr) {
                        const preview = (evaluated || '').slice(0, 200);
                        throw new Error('Malformed JSON from WASM evaluate_madola. Preview: ' + preview);
                    }

                    if (evalResult.success) {
                        const lines = ['Execution completed successfully'];

                        if (evalResult.outputs && evalResult.outputs.length > 0) {
                            lines.push('Output:');
                            for (const outputLine of evalResult.outputs) {
                                lines.push(`  ${outputLine}`);
                            }
                        }

                        if (evalResult.cppFiles && evalResult.cppFiles.length > 0) {
                            for (const cppFile of evalResult.cppFiles) {
                                lines.push(`Generated C++ file: ${cppFile.filename}`);
                            }
                        }

                        if (evalResult.wasmFiles && evalResult.wasmFiles.length > 0) {
                            for (const wasmFile of evalResult.wasmFiles) {
                                if (wasmFile.cppSourcePath) {
                                    lines.push(`Generated C++ file: ${wasmFile.cppSourcePath}`);
                                }
                                if (wasmFile.wasmPath) {
                                    lines.push(`Generated WASM addon: ${wasmFile.wasmPath}`);
                                }
                                if (wasmFile.jsWrapperPath) {
                                    lines.push(`Generated JS wrapper: ${wasmFile.jsWrapperPath}`);
                                }
                                if (wasmFile.errorMessage) {
                                    lines.push(`WASM generation error: ${wasmFile.errorMessage}`);
                                }
                            }
                        }

                        result = lines.join('\n');
                    } else if (evalResult.error) {
                        result = evalResult.error;
                    } else if (evalResult && evalResult.success === false) {
                        result = 'Error: Unknown evaluation failure';
                    } else {
                        result = 'Error: No output or error from evaluation';
                    }
                } else {
                    result = 'Error: WASM evaluation failed (returned null pointer)';
                }
            } catch (evalError) {
                result = 'Error: WASM evaluation threw exception: ' + (evalError.message || evalError);
            }
        }

        // Replay runtime logs to console for visibility
        if (runtimeLogs.length > 0) {
            for (const logLine of runtimeLogs) {
                console.log(logLine);
            }
        }

        // Prepend runtime logs to result when writing to output
        if (runtimeLogs.length > 0) {
            const logsJoined = runtimeLogs.join('\n');
            result = logsJoined.length > 0 ? logsJoined + '\n' + result : result;
        }

        // Output result
        if (outputFile) {
            // Normalize line endings to match platform (Windows uses \r\n)
            const normalizedResult = result.replace(/\r?\n/g, '\r\n');
            // Add final newline to match native output (only for evaluation mode)
            const finalResult = (isFormat || isHtml) ? normalizedResult : normalizedResult + '\r\n';
            fs.writeFileSync(outputFile, finalResult, 'utf8');
        } else {
            // Normalize line endings to match platform on console output too
            const normalizedResult = process.platform === 'win32' ?
                result.replace(/\r?\n/g, '\r\n') : result;
            process.stdout.write(normalizedResult + (process.platform === 'win32' ? '\r\n' : '\n'));
        }

        // Allow a brief moment for WASM cleanup before natural process exit
        // Avoid explicit process.exit on Windows to prevent libuv assertion
        await new Promise((resolve) => setImmediate(resolve));

    } catch (error) {
        console.error('Error:', (error && (error.stack || error.message)) || error);
        // Set exit code and allow natural exit after a microtask tick
        process.exitCode = 1;
        await new Promise((resolve) => setImmediate(resolve));
    }
}

main();

