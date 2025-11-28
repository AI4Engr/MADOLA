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

    isReady() {
        return this.module !== null;
    }
}

// Make it available globally
window.MadolaBrowserWrapper = MadolaBrowserWrapper;