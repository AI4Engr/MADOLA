#!/usr/bin/env node

/**
 * Generate C++ header file from CSS file for embedding
 */

const fs = require('fs');
const path = require('path');

function generateHeader(cssFile, headerFile) {
    const cssContent = fs.readFileSync(cssFile, 'utf8');
    const cssBasename = path.basename(cssFile);

    // Generate header content using raw string literal
    const headerContent = `// Auto-generated file - DO NOT EDIT
// Generated from ${cssBasename}

#ifndef MADOLA_EMBEDDED_CSS_H
#define MADOLA_EMBEDDED_CSS_H

#include <string>

namespace madola {

inline const char* getEmbeddedCss() {
    return R"(${cssContent})";
}

} // namespace madola

#endif // MADOLA_EMBEDDED_CSS_H
`;

    fs.writeFileSync(headerFile, headerContent, 'utf8');
    console.log(`Generated ${headerFile} from ${cssFile}`);
}

if (require.main === module) {
    if (process.argv.length !== 4) {
        console.error('Usage: generate_css_header.js <input.css> <output.h>');
        process.exit(1);
    }

    const cssFile = process.argv[2];
    const headerFile = process.argv[3];

    if (!fs.existsSync(cssFile)) {
        console.error(`Error: CSS file not found: ${cssFile}`);
        process.exit(1);
    }

    generateHeader(cssFile, headerFile);
}
