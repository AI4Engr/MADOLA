#!/usr/bin/env node

const http = require('http');
const fs = require('fs');
const path = require('path');
const os = require('os');

const PORT = 8080;

// Helper function to get ~/.madola directory
function getMadolaDir() {
    const homeDir = os.homedir();
    const madolaDir = path.join(homeDir, '.madola');
    if (!fs.existsSync(madolaDir)) {
        fs.mkdirSync(madolaDir, { recursive: true });
    }
    return madolaDir;
}

function getGenCppDir() {
    const madolaDir = getMadolaDir();
    const genCppDir = path.join(madolaDir, 'gen_cpp');
    if (!fs.existsSync(genCppDir)) {
        fs.mkdirSync(genCppDir, { recursive: true });
    }
    return genCppDir;
}

function getTroveDir(moduleName = '') {
    const madolaDir = getMadolaDir();
    let troveDir = path.join(madolaDir, 'trove');
    if (moduleName) {
        troveDir = path.join(troveDir, moduleName);
    }
    if (!fs.existsSync(troveDir)) {
        fs.mkdirSync(troveDir, { recursive: true });
    }
    return troveDir;
}

const mimeTypes = {
    '.html': 'text/html',
    '.js': 'application/javascript',
    '.wasm': 'application/wasm',
    '.css': 'text/css',
    '.json': 'application/json',
    '.md': 'text/markdown',
    '.mda': 'text/plain'
};

const server = http.createServer((req, res) => {
    // Handle API endpoint for saving C++ files
    if (req.method === 'POST' && req.url === '/api/save-cpp') {
        let body = '';
        req.on('data', chunk => {
            body += chunk.toString();
        });
        req.on('end', () => {
            try {
                const { filename, content } = JSON.parse(body);
                const outputDir = getGenCppDir();

                const filePath = path.join(outputDir, filename);
                fs.writeFileSync(filePath, content, 'utf8');

                console.log(`[API] Saved C++ file: ${filename} to ${filePath}`);

                res.writeHead(200, {
                    'Content-Type': 'application/json',
                    'Access-Control-Allow-Origin': '*'
                });
                res.end(JSON.stringify({ success: true, path: filePath }));
            } catch (error) {
                console.error(`[API] Error saving C++ file:`, error);
                res.writeHead(500, {
                    'Content-Type': 'application/json',
                    'Access-Control-Allow-Origin': '*'
                });
                res.end(JSON.stringify({ success: false, error: error.message }));
            }
        });
        return;
    }

    // Handle API endpoint for compiling C++ to WASM (with content)
    if (req.method === 'POST' && req.url === '/api/compile-wasm-content') {
        let body = '';
        req.on('data', chunk => {
            body += chunk.toString();
        });
        req.on('end', () => {
            try {
                const { functionName, cppContent, mdaFileName } = JSON.parse(body);

                console.log(`[API] Compiling WASM addon: ${functionName}`);

                // Create necessary directories
                const buildDir = path.join(__dirname, 'build', 'addon');
                const genCppDir = getGenCppDir();
                const troveDir = getTroveDir(mdaFileName || 'example');

                if (!fs.existsSync(buildDir)) {
                    fs.mkdirSync(buildDir, { recursive: true });
                }

                // Save C++ content to build directory
                const cppPath = path.join(buildDir, `${functionName}.cpp`);
                const wasmPath = path.join(troveDir, `${functionName}.wasm`);
                const genCppPath = path.join(genCppDir, `${functionName}.cpp`);

                // Also save to ~/.madola/gen_cpp for reference
                fs.writeFileSync(genCppPath, cppContent, 'utf8');

                // Generate WASM wrapper C++ code
                const wasmCppContent = `#include <emscripten.h>
#include <cmath>

extern "C" {

${cppContent}

EMSCRIPTEN_KEEPALIVE
double ${functionName}_wasm(${extractParams(cppContent, functionName)}) {
    return ${functionName}(${extractParamNames(cppContent, functionName)});
}

}
`;

                fs.writeFileSync(cppPath, wasmCppContent, 'utf8');

                // Call compile_wasm.js script
                const { execSync } = require('child_process');
                const compileScript = path.join(__dirname, 'web', 'compile_wasm.js');
                const cmd = `node "${compileScript}" "${cppPath}" "${wasmPath}" "_${functionName}_wasm"`;

                try {
                    execSync(cmd, { stdio: 'inherit' });

                    // Generate JS wrapper
                    const jsPath = path.join(troveDir, `${functionName}.js`);
                    const jsWrapper = generateJsWrapper(functionName, wasmPath);
                    fs.writeFileSync(jsPath, jsWrapper, 'utf8');

                    // Cleanup build files
                    fs.unlinkSync(cppPath);

                    res.writeHead(200, {
                        'Content-Type': 'application/json',
                        'Access-Control-Allow-Origin': '*'
                    });
                    res.end(JSON.stringify({
                        success: true,
                        wasmPath: wasmPath,
                        jsPath: jsPath,
                        message: 'WASM compilation successful'
                    }));
                } catch (compileError) {
                    res.writeHead(500, {
                        'Content-Type': 'application/json',
                        'Access-Control-Allow-Origin': '*'
                    });
                    res.end(JSON.stringify({
                        success: false,
                        error: `Compilation failed: ${compileError.message}`
                    }));
                }
            } catch (error) {
                console.error(`[API] Error compiling WASM:`, error);
                res.writeHead(500, {
                    'Content-Type': 'application/json',
                    'Access-Control-Allow-Origin': '*'
                });
                res.end(JSON.stringify({ success: false, error: error.message }));
            }
        });
        return;
    }

    // Handle API endpoint for listing C++ files
    if (req.method === 'GET' && req.url === '/api/files/cpp') {
        try {
            const genCppDir = path.join(__dirname, 'gen_cpp');
            
            if (!fs.existsSync(genCppDir)) {
                fs.mkdirSync(genCppDir, { recursive: true });
            }
            
            const files = fs.readdirSync(genCppDir);
            const cppFiles = files
                .filter(f => f.endsWith('.cpp'))
                .map(filename => {
                    const filePath = path.join(genCppDir, filename);
                    const stats = fs.statSync(filePath);
                    return {
                        name: filename,
                        size: stats.size,
                        modified: stats.mtime.toISOString()
                    };
                })
                .sort((a, b) => a.name.localeCompare(b.name));
            
            res.writeHead(200, {
                'Content-Type': 'application/json',
                'Access-Control-Allow-Origin': '*'
            });
            res.end(JSON.stringify({ success: true, files: cppFiles }));
        } catch (error) {
            console.error(`[API] Error listing C++ files:`, error);
            res.writeHead(500, {
                'Content-Type': 'application/json',
                'Access-Control-Allow-Origin': '*'
            });
            res.end(JSON.stringify({ success: false, error: error.message }));
        }
        return;
    }

    // Handle API endpoint for listing WASM modules
    if (req.method === 'GET' && req.url === '/api/files/wasm') {
        try {
            const troveDir = path.join(__dirname, 'trove');
            
            if (!fs.existsSync(troveDir)) {
                fs.mkdirSync(troveDir, { recursive: true });
            }
            
            const modules = [];
            const entries = fs.readdirSync(troveDir, { withFileTypes: true });
            
            for (const entry of entries) {
                if (entry.isDirectory()) {
                    const modulePath = path.join(troveDir, entry.name);
                    const moduleFiles = fs.readdirSync(modulePath);
                    
                    const wasmFile = moduleFiles.find(f => f.endsWith('.wasm'));
                    const jsFile = moduleFiles.find(f => f.endsWith('.js'));
                    
                    if (wasmFile || jsFile) {
                        const files = [];
                        
                        if (jsFile) {
                            const jsPath = path.join(modulePath, jsFile);
                            const jsStats = fs.statSync(jsPath);
                            files.push({
                                name: jsFile,
                                type: 'js',
                                size: jsStats.size,
                                modified: jsStats.mtime.toISOString()
                            });
                        }
                        
                        if (wasmFile) {
                            const wasmPath = path.join(modulePath, wasmFile);
                            const wasmStats = fs.statSync(wasmPath);
                            files.push({
                                name: wasmFile,
                                type: 'wasm',
                                size: wasmStats.size,
                                modified: wasmStats.mtime.toISOString()
                            });
                        }
                        
                        modules.push({
                            name: entry.name,
                            files: files
                        });
                    }
                }
            }
            
            modules.sort((a, b) => a.name.localeCompare(b.name));
            
            res.writeHead(200, {
                'Content-Type': 'application/json',
                'Access-Control-Allow-Origin': '*'
            });
            res.end(JSON.stringify({ success: true, modules: modules }));
        } catch (error) {
            console.error(`[API] Error listing WASM modules:`, error);
            res.writeHead(500, {
                'Content-Type': 'application/json',
                'Access-Control-Allow-Origin': '*'
            });
            res.end(JSON.stringify({ success: false, error: error.message }));
        }
        return;
    }

    // Handle API endpoint for viewing C++ file content
    if (req.method === 'GET' && req.url.startsWith('/api/files/cpp/')) {
        try {
            const filename = decodeURIComponent(req.url.split('/api/files/cpp/')[1]);
            const filePath = path.join(__dirname, 'gen_cpp', filename);
            
            if (!fs.existsSync(filePath)) {
                res.writeHead(404, {
                    'Content-Type': 'application/json',
                    'Access-Control-Allow-Origin': '*'
                });
                res.end(JSON.stringify({ success: false, error: 'File not found' }));
                return;
            }
            
            const content = fs.readFileSync(filePath, 'utf8');
            
            res.writeHead(200, {
                'Content-Type': 'application/json',
                'Access-Control-Allow-Origin': '*'
            });
            res.end(JSON.stringify({ success: true, content: content, filename: filename }));
        } catch (error) {
            console.error(`[API] Error reading C++ file:`, error);
            res.writeHead(500, {
                'Content-Type': 'application/json',
                'Access-Control-Allow-Origin': '*'
            });
            res.end(JSON.stringify({ success: false, error: error.message }));
        }
        return;
    }

    // Handle OPTIONS preflight requests
    if (req.method === 'OPTIONS') {
        res.writeHead(200, {
            'Access-Control-Allow-Origin': '*',
            'Access-Control-Allow-Methods': 'GET, POST, PUT, DELETE, OPTIONS',
            'Access-Control-Allow-Headers': 'Content-Type, Authorization'
        });
        res.end();
        return;
    }

    // Parse URL and route appropriately
    let filePath = req.url;

    console.log(`[${new Date().toISOString()}] ${req.method} ${req.url}`);

    // Route root to index.html for the main web app
    if (filePath === '/') {
        filePath = '/index.html';
    }
    // Route demo.html requests to the root demo
    else if (filePath === '/demo.html') {
        filePath = '/../demo.html';
    }

    // Serve from web directory (current directory) or parent directory
    if (filePath.startsWith('/../')) {
        // Go to parent directory (for demo.html in project root)
        filePath = path.join(__dirname, '..', filePath.substring(3));
    } else {
        // Serve from web directory (current location)
        filePath = path.join(__dirname, filePath);
    }

    // Get file extension
    const ext = path.extname(filePath);
    const contentType = mimeTypes[ext] || 'application/octet-stream';

    // Check if file exists
    fs.access(filePath, fs.constants.F_OK, (err) => {
        if (err) {
            console.log(`[404] File not found: ${filePath}`);
            res.writeHead(404, { 'Content-Type': 'text/plain' });
            res.end('404 Not Found');
            return;
        }

        // Set CORS headers for WASM and cross-origin requests
        res.setHeader('Access-Control-Allow-Origin', '*');
        res.setHeader('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
        res.setHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization');
        
        // Set headers required for SharedArrayBuffer and WASM
        res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
        res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');

        // Read and serve file
        fs.readFile(filePath, (err, data) => {
            if (err) {
                console.log(`[500] Error reading file: ${filePath}`);
                res.writeHead(500, { 'Content-Type': 'text/plain' });
                res.end('500 Internal Server Error');
                return;
            }

            res.writeHead(200, { 'Content-Type': contentType });
            res.end(data);
            console.log(`[200] Served: ${filePath} (${contentType})`);
        });
    });
});

server.listen(PORT, () => {
    console.log(`🚀 MADOLA Demo Server running at http://localhost:${PORT}`);
    console.log(`📄 Main Web App: http://localhost:${PORT}/`);
    console.log(`📄 Simple Demo: http://localhost:${PORT}/demo.html`);
    console.log(`📁 Serving files from: ${__dirname}`);
});

// Helper functions for WASM compilation
function extractParams(cppContent, functionName) {
    // Extract parameters from function signature
    const regex = new RegExp(`double\\s+${functionName}\\s*\\(([^)]*)\\)`);
    const match = cppContent.match(regex);
    if (match && match[1]) {
        return match[1];
    }
    return '';
}

function extractParamNames(cppContent, functionName) {
    const params = extractParams(cppContent, functionName);
    if (!params) return '';

    // Extract just the parameter names (remove types)
    return params.split(',').map(p => {
        const parts = p.trim().split(/\s+/);
        return parts[parts.length - 1];
    }).join(', ');
}

function generateJsWrapper(functionName, wasmPath) {
    const baseName = path.basename(wasmPath);
    return `// JavaScript wrapper for ${functionName} WASM function

class ${functionName}Addon {
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
                    const nodePath = this.basePath.replace(/^trove\\//, 'web/trove/');
                    wasmPath = path.join(nodePath, '${baseName}');
                } else {
                    wasmPath = path.join(__dirname, '${baseName}');
                }
                wasmBytes = fs.readFileSync(wasmPath);
            } else {
                // Browser environment
                wasmPath = this.basePath ? \`\${this.basePath}/${baseName}\` : '${baseName}';
                const response = await fetch(wasmPath);
                if (!response.ok) {
                    throw new Error(\`Failed to fetch WASM: \${response.status} \${response.statusText}\`);
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

    ${functionName}(...args) {
        if (!this.isReady) {
            throw new Error('WASM module not loaded. Call load() first.');
        }

        return this.module.exports.${functionName}_wasm(...args);
    }
}

// Export for module usage
if (typeof module !== 'undefined' && module.exports) {
    module.exports = ${functionName}Addon;
} else {
    window.${functionName}Addon = ${functionName}Addon;
}
`;
}

