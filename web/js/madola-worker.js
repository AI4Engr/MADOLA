// MADOLA Web Worker - Handles code execution in background thread
// This prevents UI blocking during long-running calculations

let madolaModule = null;
let isInitialized = false;

// Initialize the MADOLA WASM module
async function initialize() {
    if (isInitialized) return;

    try {
        console.log('[Worker] Initializing MADOLA WASM module...');

        // Load the MADOLA script in worker context
        importScripts('../runtime/madola.js');

        // Wait for MADOLA function to be available
        if (typeof MADOLA === 'undefined') {
            throw new Error('MADOLA function not available after loading script');
        }

        // Initialize the module
        madolaModule = await MADOLA({
            locateFile: (path) => {
                console.log(`[Worker] Locating file: ${path}`);
                return '../runtime/' + path;
            }
        });

        if (!madolaModule) {
            throw new Error('MADOLA module initialization returned null');
        }

        // Verify required functions
        if (typeof madolaModule.ccall !== 'function') {
            throw new Error('ccall function not available in WASM module');
        }

        isInitialized = true;
        console.log('[Worker] MADOLA WASM module initialized successfully');
        self.postMessage({ type: 'init', success: true });
    } catch (error) {
        console.error('[Worker] Failed to initialize MADOLA:', error);
        self.postMessage({ type: 'init', success: false, error: error.message });
    }
}

// WASM wrapper functions
async function evaluate(code) {
    if (!madolaModule) {
        throw new Error('Module not loaded');
    }

    const resultPtr = madolaModule.ccall('evaluate_madola', 'number', ['string'], [code]);
    if (resultPtr !== 0) {
        const result = madolaModule.UTF8ToString(resultPtr);
        madolaModule.ccall('free_result', null, ['number'], [resultPtr]);
        return result;
    }
    return null;
}

async function format(code, mode = 1) {
    if (!madolaModule) {
        throw new Error('Module not loaded');
    }

    const resultPtr = madolaModule.ccall('format_madola', 'number', ['string', 'number'], [code, mode]);
    if (resultPtr !== 0) {
        const result = madolaModule.UTF8ToString(resultPtr);
        madolaModule.ccall('free_result', null, ['number'], [resultPtr]);
        return result;
    }
    return null;
}

async function formatHtml(code, mode = 1) {
    if (!madolaModule) {
        throw new Error('Module not loaded');
    }

    const resultPtr = madolaModule.ccall('format_madola_html', 'number', ['string', 'number'], [code, mode]);
    if (resultPtr !== 0) {
        const result = madolaModule.UTF8ToString(resultPtr);
        madolaModule.ccall('free_result', null, ['number'], [resultPtr]);
        return result;
    }
    return null;
}

// Handle messages from main thread
self.onmessage = async function(e) {
    const { type, id, code, mode } = e.data;

    try {
        switch (type) {
            case 'init':
                await initialize();
                break;

            case 'evaluate':
                if (!isInitialized) {
                    await initialize();
                }

                if (!madolaModule) {
                    self.postMessage({
                        type: 'error',
                        id,
                        error: 'WASM module not ready'
                    });
                    return;
                }

                const evalResult = await evaluate(code);
                self.postMessage({
                    type: 'evaluate',
                    id,
                    result: evalResult
                });
                break;

            case 'format':
                if (!isInitialized) {
                    await initialize();
                }

                if (!madolaModule) {
                    self.postMessage({
                        type: 'error',
                        id,
                        error: 'WASM module not ready'
                    });
                    return;
                }

                const formatResult = await format(code, mode || 1);
                self.postMessage({
                    type: 'format',
                    id,
                    result: formatResult
                });
                break;

            case 'formatHtml':
                if (!isInitialized) {
                    await initialize();
                }

                if (!madolaModule) {
                    self.postMessage({
                        type: 'error',
                        id,
                        error: 'WASM module not ready'
                    });
                    return;
                }

                const htmlResult = await formatHtml(code, mode || 1);
                self.postMessage({
                    type: 'formatHtml',
                    id,
                    result: htmlResult
                });
                break;

            default:
                console.warn('[Worker] Unknown message type:', type);
        }
    } catch (error) {
        console.error('[Worker] Error processing message:', error);
        self.postMessage({
            type: 'error',
            id,
            error: error.message
        });
    }
};

// Initialize immediately when worker starts
console.log('[Worker] MADOLA Web Worker started');
