// JavaScript wrapper for testSquare WASM function

class testSquareAddon {
    constructor(basePath = '') {
        this.module = null;
        this.isReady = false;
        this.basePath = basePath;
    }

    async load() {
        try {
            let wasmPath;
            let wasmBytes;
            
            // Check if we're in Node.js or browser
            if (typeof window === 'undefined') {
                // Node.js environment
                const fs = require('fs');
                const path = require('path');
                
                if (this.basePath) {
                    wasmPath = path.join(this.basePath, 'testSquare.wasm');
                } else {
                    // Derive path from wrapper file location (__dirname is the directory containing this JS file)
                    wasmPath = path.join(__dirname, 'testSquare.wasm');
                }
                wasmBytes = fs.readFileSync(wasmPath);
            } else {
                // Browser environment
                wasmPath = this.basePath ? `${this.basePath}/testSquare.wasm` : 'testSquare.wasm';
                const response = await fetch(wasmPath);
                if (!response.ok) {
                    throw new Error(`Failed to fetch WASM: ${response.status} ${response.statusText}`);
                }
                wasmBytes = await response.arrayBuffer();
            }
            
            const wasmModule = await WebAssembly.instantiate(wasmBytes, {
                env: {
                    memory: new WebAssembly.Memory({ initial: 256 }),
                    table: new WebAssembly.Table({ initial: 0, element: 'anyfunc' })
                }
            });
            
            this.module = wasmModule.instance;
            this.isReady = true;
            return true;
        } catch (error) {
            console.error('Failed to load WASM module:', error);
            return false;
        }
    }

    testSquare(n) {
        if (!this.isReady) {
            throw new Error('WASM module not loaded. Call load() first.');
        }
        
        return this.module.exports.testSquare_wasm(n);
    }
}

// Export for module usage
if (typeof module !== 'undefined' && module.exports) {
    module.exports = testSquareAddon;
} else {
    window.testSquareAddon = testSquareAddon;
}
