#!/usr/bin/env node

/**
 * compile_wasm.js - Node.js script to compile C++ to WASM using Emscripten
 *
 * Usage: node compile_wasm.js <cppSource> <outputPath> <exportedFunction>
 *
 * This script is called from the WASM build when running in web/electron/tauri
 * to compile C++ addons to WASM format.
 */

const { execSync } = require('child_process');
const path = require('path');
const fs = require('fs');

// Parse command line arguments
const args = process.argv.slice(2);
if (args.length < 3) {
    console.error('Usage: node compile_wasm.js <cppSource> <outputPath> <exportedFunction>');
    process.exit(1);
}

const [cppSource, outputPath, exportedFunction] = args;

// Verify input file exists
if (!fs.existsSync(cppSource)) {
    console.error(`Error: C++ source file not found: ${cppSource}`);
    process.exit(1);
}

// Build emcc command
const emccCmd = `emcc "${cppSource}" -o "${outputPath}" ` +
    `-s STANDALONE_WASM=1 ` +
    `-s EXPORTED_FUNCTIONS="['${exportedFunction}']" ` +
    `-s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" ` +
    `-s ALLOW_MEMORY_GROWTH=1 ` +
    `--no-entry ` +
    `-O2`;

console.log(emccCmd);

try {
    // Execute emcc compilation
    execSync(emccCmd, { stdio: 'inherit' });
    process.exit(0);
} catch (error) {
    console.error(`Compilation failed: ${error.message}`);
    process.exit(1);
}
