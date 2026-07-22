// Browser compatibility wrapper for MADOLA WASM module
// This wraps the Node.js-compiled WASM to work in browsers

class MadolaBrowserWrapper {
    constructor() {
        this.module = null;
    }

    async loadModule() {
        try {
            // Simple and direct approach: just load the WASM module
            console.log('Loading MADOLA WASM module...');
            
            // The WASM module is already compiled for web environment, so we can load it directly
            if (typeof MADOLA === 'undefined') {
                // Load the script if MADOLA is not already available
                await this.loadMadolaScript();
            }

            if (typeof MADOLA === 'undefined') {
                throw new Error('MADOLA function not available after loading script');
            }

            // Initialize the module
            console.log('Initializing MADOLA WASM module...');
            this.module = await MADOLA({
                locateFile: (path) => {
                    console.log(`Locating file: ${path}`);
                    return './runtime/' + path;
                }
            });

            if (!this.module) {
                throw new Error('MADOLA module initialization returned null');
            }

            // Verify required functions are available
            if (typeof this.module.ccall !== 'function') {
                throw new Error('ccall function not available in WASM module');
            }
            
            if (typeof this.module.UTF8ToString !== 'function') {
                throw new Error('UTF8ToString function not available in WASM module');
            }
            
            console.log('MADOLA WASM module loaded and verified successfully');
            return this.module;

        } catch (error) {
            console.error('Failed to load MADOLA module:', error);
            throw error;
        }
    }

    async loadMadolaScript() {
        return new Promise((resolve, reject) => {
            const script = document.createElement('script');
            script.src = './runtime/madola.js';
            script.async = true;
            
            script.onload = () => {
                console.log('MADOLA script loaded successfully');
                resolve();
            };
            
            script.onerror = (error) => {
                console.error('Failed to load MADOLA script:', error);
                reject(new Error('Failed to load MADOLA script'));
            };
            
            document.head.appendChild(script);
        });
    }

    async evaluate(code) {
        if (!this.module) {
            throw new Error('Module not loaded');
        }

        const resultPtr = this.module.ccall('evaluate_madola', 'number', ['string'], [code]);
        if (resultPtr !== 0) {
            const result = this.module.UTF8ToString(resultPtr);
            this.module.ccall('free_result', null, ['number'], [resultPtr]);
            return result;
        }
        return null;
    }

    async format(code, mode = 1) {
        if (!this.module) {
            throw new Error('Module not loaded');
        }

        const resultPtr = this.module.ccall('format_madola', 'number', ['string', 'number'], [code, mode]);
        if (resultPtr !== 0) {
            const result = this.module.UTF8ToString(resultPtr);
            this.module.ccall('free_result', null, ['number'], [resultPtr]);
            return result;
        }
        return null;
    }

    async formatHtml(code, mode = 1) {
        if (!this.module) {
            throw new Error('Module not loaded');
        }

        const resultPtr = this.module.ccall('format_madola_html', 'number', ['string', 'number'], [code, mode]);
        if (resultPtr !== 0) {
            const result = this.module.UTF8ToString(resultPtr);
            this.module.ccall('free_result', null, ['number'], [resultPtr]);
            return result;
        }
        return null;
    }

    /**
     * Recompute a single interactive svg() curve after rebinding one variable (e.g. a
     * dragged @input value), without re-running/re-rendering the whole program. Returns
     * { success, curveId, d } or { success: false, error }. Mirrors the Electron app's
     * runtime-bridge.js::evalSvgCurve — same ccall signature, shared WASM export.
     */
    async evalSvgCurve(source, curveId, paramName, paramValue, resultVar = '') {
        if (!this.module) {
            throw new Error('Module not loaded');
        }

        const resultPtr = this.module.ccall(
            'svg_eval_curve',
            'number',
            ['string', 'string', 'string', 'number', 'string'],
            [source, curveId, paramName, paramValue, resultVar || '']
        );
        if (!resultPtr) return { success: false, error: 'svg_eval_curve returned null' };
        try {
            return JSON.parse(this.module.UTF8ToString(resultPtr));
        } finally {
            this.module.ccall('free_result', null, ['number'], [resultPtr]);
        }
    }

    /**
     * Recompute a single named variable's final value after a full evaluate() pass
     * (the caller has already rebound the dragged @input's literal in `source`).
     * Returns { success, name, value } or { success: false, error }. Used to keep a
     * displayed @result value in sync with a slider drag without re-rendering the
     * whole HTML document.
     */
    async evalNamedValue(source, varName) {
        if (!this.module) {
            throw new Error('Module not loaded');
        }

        const resultPtr = this.module.ccall(
            'eval_named_value',
            'number',
            ['string', 'string'],
            [source, varName]
        );
        if (!resultPtr) return { success: false, error: 'eval_named_value returned null' };
        try {
            return JSON.parse(this.module.UTF8ToString(resultPtr));
        } finally {
            this.module.ccall('free_result', null, ['number'], [resultPtr]);
        }
    }

    isReady() {
        return this.module !== null;
    }
}

// Make it available globally
window.MadolaBrowserWrapper = MadolaBrowserWrapper;