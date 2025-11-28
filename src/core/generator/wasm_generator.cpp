#include "wasm_generator.h"
#include "../utils/paths.h"
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace madola {

WasmGenerator::WasmGenerator() = default;

std::vector<WasmGenerator::GeneratedWasm> WasmGenerator::generateWasmFiles(const Program& program, const std::string& mdaFileName) {
    std::vector<GeneratedWasm> wasmFiles;

    // Create trove directories
    if (!createTroveDirectories(mdaFileName)) {
        return wasmFiles; // Return empty if directory creation fails
    }

    // Create build/addon directory for temporary compilation
    std::filesystem::create_directories("build/addon");

    for (const auto& stmt : program.statements) {
        if (const auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(stmt.get())) {
            if (funcDecl->hasDecorator("gen_addon")) {
                GeneratedWasm wasmFile;
                wasmFile.functionName = funcDecl->name;

                std::string safeFileName = getSafeFileName(mdaFileName);

                // 1. Generate C++ in ~/.madola/gen_cpp/ folder
                std::string genCppDir = utils::getGenCppDirectory();
                std::string genCppPath = genCppDir + "/" + funcDecl->name + ".cpp";
                std::string cppSource = cppGen.generateCppFunction(*funcDecl);

                // Store C++ content for web app display
                wasmFile.cppContent = cppSource;

                // Write to ~/.madola/gen_cpp/ directory
                std::ofstream genCppFile(genCppPath);
                if (genCppFile.is_open()) {
                    genCppFile << cppSource;
                    genCppFile.close();
                }

                // 2. Copy to build/addon/ and add WASM wrapper code
                std::string buildCppPath = "build/addon/" + funcDecl->name + ".cpp";
                std::string wasmCppSource = generateWasmCppSource(*funcDecl);

                std::ofstream buildCppFile(buildCppPath);
                if (buildCppFile.is_open()) {
                    buildCppFile << wasmCppSource;
                    buildCppFile.close();

                    // 3. Check if rebuild is needed
                    std::string troveDir = utils::getTroveDirectory(safeFileName);
                    std::string finalWasmPath = troveDir + "/" + funcDecl->name + ".wasm";
                    std::string finalJsPath = troveDir + "/" + funcDecl->name + ".js";

                    if (!needsRebuild(wasmCppSource, finalWasmPath, finalJsPath)) {
                        // No rebuild needed, files are up to date
                        wasmFile.compilationSuccess = true;
                        wasmFile.cppSourcePath = genCppPath;
                        wasmFile.wasmPath = finalWasmPath;
                        wasmFile.jsWrapperPath = finalJsPath;
                        wasmFile.errorMessage = "No rebuild needed - files up to date";

                        wasmFiles.push_back(wasmFile);
                        continue;
                    }

                    // 4. Compile WASM in build/addon/
                    std::string buildWasmPath = "build/addon/" + funcDecl->name + ".wasm";
                    std::string errorMsg;

                    wasmFile.compilationSuccess = compileToWasm(buildCppPath, buildWasmPath, funcDecl->name, errorMsg);
                    wasmFile.errorMessage = errorMsg;

                    if (wasmFile.compilationSuccess) {
                        // 5. Move final files to trove/ folder
                        wasmFile.cppSourcePath = genCppPath; // Point to gen_cpp version
                        wasmFile.wasmPath = finalWasmPath;
                        wasmFile.jsWrapperPath = finalJsPath;

                        // Copy WASM to final location
                        std::filesystem::copy_file(buildWasmPath, wasmFile.wasmPath, std::filesystem::copy_options::overwrite_existing);

                        // Generate and save JavaScript wrapper
                        std::string jsWrapper = generateJsWrapper(*funcDecl, wasmFile.wasmPath);
                        std::ofstream jsFile(wasmFile.jsWrapperPath);
                        if (jsFile.is_open()) {
                            jsFile << jsWrapper;
                            jsFile.close();
                        }

                        // 6. Clean up build/addon/ files
                        std::filesystem::remove(buildCppPath);
                        std::filesystem::remove(buildWasmPath);
                    }
                } else {
                    wasmFile.compilationSuccess = false;
                    wasmFile.errorMessage = "Failed to write C++ source file";
                }

                wasmFiles.push_back(wasmFile);
            }
        }
    }

    // Clean up build/addon directory if empty
    try {
        if (std::filesystem::is_empty("build/addon")) {
            std::filesystem::remove("build/addon");
        }
    } catch (...) {
        // Ignore cleanup errors
    }

    return wasmFiles;
}

std::string WasmGenerator::generateWasmCppSource(const FunctionDeclaration& func) {
    std::stringstream ss;

    // Add necessary includes and extern "C" for WASM
    ss << "#include <emscripten.h>\n";
    ss << "#include <cmath>\n\n";

    ss << "extern \"C\" {\n\n";

    // Generate the main function
    std::string cppFunction = cppGen.generateCppFunction(func);

    // Remove the includes from the generated function since we add our own
    size_t includePos = cppFunction.find("#include");
    if (includePos != std::string::npos) {
        size_t newlinePos = cppFunction.find("\n\n", includePos);
        if (newlinePos != std::string::npos) {
            cppFunction = cppFunction.substr(newlinePos + 2);
        }
    }

    ss << cppFunction << "\n";

    // Add WASM export
    ss << "EMSCRIPTEN_KEEPALIVE\n";
    ss << "double " << func.name << "_wasm(";
    for (size_t i = 0; i < func.parameters.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << "double " << func.parameters[i];
    }
    ss << ") {\n";
    ss << "    return " << func.name << "(";
    for (size_t i = 0; i < func.parameters.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << func.parameters[i];
    }
    ss << ");\n";
    ss << "}\n\n";

    ss << "} // extern \"C\"\n";

    return ss.str();
}

bool WasmGenerator::compileToWasm(const std::string& cppSource, const std::string& outputPath, const std::string& functionName, std::string& errorMsg) {
    try {
        std::string exportedFunction = "_" + functionName + "_wasm";

#ifdef __EMSCRIPTEN__
        // Running in WASM (web/electron/tauri): Call Node.js to compile
        std::string cmd = "node compile_wasm.js \"" + cppSource + "\" \"" + outputPath + "\" \"" + exportedFunction + "\"";
#else
        // Native build: Use emcc directly
        std::string cmd = "emcc \"" + cppSource + "\" -o \"" + outputPath + "\" "
                         "-s STANDALONE_WASM=1 "
                         "-s EXPORTED_FUNCTIONS=\"['" + exportedFunction + "']\" "
                         "-s EXPORTED_RUNTIME_METHODS=\"['ccall','cwrap']\" "
                         "-s ALLOW_MEMORY_GROWTH=1 "
                         "--no-entry "
                         "-O2";

        std::cout << cmd << std::endl;
#endif

        int result = std::system(cmd.c_str());
        if (result == 0) {
            return true;
        } else {
            errorMsg = "Emscripten compilation failed with exit code " + std::to_string(result);
            return false;
        }
    } catch (const std::exception& e) {
        errorMsg = std::string("WASM compilation error: ") + e.what();
        return false;
    }
}

std::string WasmGenerator::generateJsWrapper(const FunctionDeclaration& func, const std::string& /* wasmPath */) {
    std::stringstream ss;

    ss << "// JavaScript wrapper for " << func.name << " WASM function\n\n";

    ss << "class " << func.name << "Addon {\n";
    ss << "    constructor(basePath = '') {\n";
    ss << "        this.module = null;\n";
    ss << "        this.isReady = false;\n";
    ss << "        this.basePath = basePath;\n";
    ss << "    }\n\n";

    ss << "    async load() {\n";
    ss << "        try {\n";
    ss << "            let wasmPath;\n";
    ss << "            let wasmBytes;\n";
    ss << "            \n";
    ss << "            // Check if we're in Node.js or browser\n";
    ss << "            if (typeof window === 'undefined') {\n";
    ss << "                // Node.js environment\n";
    ss << "                const fs = require('fs');\n";
    ss << "                const path = require('path');\n";
    ss << "                \n";
    ss << "                if (this.basePath) {\n";
    ss << "                    wasmPath = path.join(this.basePath, '" << func.name << ".wasm');\n";
    ss << "                } else {\n";
    ss << "                    // Derive path from wrapper file location (__dirname is the directory containing this JS file)\n";
    ss << "                    wasmPath = path.join(__dirname, '" << func.name << ".wasm');\n";
    ss << "                }\n";
    ss << "                wasmBytes = fs.readFileSync(wasmPath);\n";
    ss << "            } else {\n";
    ss << "                // Browser environment\n";
    ss << "                wasmPath = this.basePath ? `${this.basePath}/" << func.name << ".wasm` : '" << func.name << ".wasm';\n";
    ss << "                const response = await fetch(wasmPath);\n";
    ss << "                if (!response.ok) {\n";
    ss << "                    throw new Error(`Failed to fetch WASM: ${response.status} ${response.statusText}`);\n";
    ss << "                }\n";
    ss << "                wasmBytes = await response.arrayBuffer();\n";
    ss << "            }\n";
    ss << "            \n";
    ss << "            const wasmModule = await WebAssembly.instantiate(wasmBytes, {\n";
    ss << "                env: {\n";
    ss << "                    memory: new WebAssembly.Memory({ initial: 256 }),\n";
    ss << "                    table: new WebAssembly.Table({ initial: 0, element: 'anyfunc' })\n";
    ss << "                }\n";
    ss << "            });\n";
    ss << "            \n";
    ss << "            this.module = wasmModule.instance;\n";
    ss << "            this.isReady = true;\n";
    ss << "            return true;\n";
    ss << "        } catch (error) {\n";
    ss << "            console.error('Failed to load WASM module:', error);\n";
    ss << "            return false;\n";
    ss << "        }\n";
    ss << "    }\n\n";

    ss << "    " << func.name << "(";
    for (size_t i = 0; i < func.parameters.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << func.parameters[i];
    }
    ss << ") {\n";
    ss << "        if (!this.isReady) {\n";
    ss << "            throw new Error('WASM module not loaded. Call load() first.');\n";
    ss << "        }\n";
    ss << "        \n";
    ss << "        return this.module.exports." << func.name << "_wasm(";
    for (size_t i = 0; i < func.parameters.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << func.parameters[i];
    }
    ss << ");\n";
    ss << "    }\n";
    ss << "}\n\n";

    ss << "// Export for module usage\n";
    ss << "if (typeof module !== 'undefined' && module.exports) {\n";
    ss << "    module.exports = " << func.name << "Addon;\n";
    ss << "} else {\n";
    ss << "    window." << func.name << "Addon = " << func.name << "Addon;\n";
    ss << "}\n";

    return ss.str();
}

bool WasmGenerator::createTroveDirectories(const std::string& mdaFileName) {
    try {
        std::string safeFileName = getSafeFileName(mdaFileName);
        std::string baseDir = utils::getTroveDirectory(safeFileName);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create trove directories: " << e.what() << std::endl;
        return false;
    }
}

std::string WasmGenerator::getSafeFileName(const std::string& mdaFileName) {
    if (mdaFileName.empty() || mdaFileName == "default") {
        return "default";
    }

    std::string safe = mdaFileName;

    // Remove .mda extension if present
    if (safe.length() > 4 && safe.substr(safe.length() - 4) == ".mda") {
        safe = safe.substr(0, safe.length() - 4);
    }

    // Replace invalid characters with underscore
    for (char& c : safe) {
        if (!std::isalnum(c) && c != '_' && c != '-') {
            c = '_';
        }
    }

    return safe;
}

std::string WasmGenerator::getSystemTempDir() {
    std::string tempDir;

#ifdef _WIN32
    // Windows: Use GetTempPath
    char tempPath[MAX_PATH];
    DWORD result = GetTempPathA(MAX_PATH, tempPath);
    if (result > 0 && result < MAX_PATH) {
        tempDir = std::string(tempPath);
    } else {
        tempDir = "C:\\temp\\"; // Fallback
    }
#else
    // Unix/Linux: Check environment variables
    const char* tmpDir = std::getenv("TMPDIR");
    if (!tmpDir) tmpDir = std::getenv("TMP");
    if (!tmpDir) tmpDir = std::getenv("TEMP");
    if (!tmpDir) tmpDir = "/tmp"; // Default fallback

    tempDir = std::string(tmpDir);
    if (!tempDir.empty() && tempDir.back() != '/') {
        tempDir += '/';
    }
#endif

    // Create madola subdirectory in temp
    tempDir += "madola_wasm_checksums";
    std::filesystem::create_directories(tempDir);

    return tempDir;
}

bool WasmGenerator::needsRebuild(const std::string& cppSource, const std::string& wasmPath, const std::string& jsPath) {
    try {
        // Check if output files exist
        if (!std::filesystem::exists(wasmPath) || !std::filesystem::exists(jsPath)) {
            return true; // Need to build if files don't exist
        }

        // Calculate hash of current source code
        std::hash<std::string> hasher;
        size_t sourceHash = hasher(cppSource);

        // Create checksum file path in temp directory
        std::filesystem::path wasmPathObj(wasmPath);
        std::string tempDir = getSystemTempDir();

        // Create unique checksum filename based on full path to avoid conflicts
        std::string fullPath = std::filesystem::absolute(wasmPathObj).string();
        std::hash<std::string> pathHasher;
        size_t pathHash = pathHasher(fullPath);

        std::string checksumPath = tempDir + "/" + wasmPathObj.stem().string() + "_" + std::to_string(pathHash) + ".checksum";

        // Check if checksum file exists and matches
        if (std::filesystem::exists(checksumPath)) {
            std::ifstream checksumFile(checksumPath);
            if (checksumFile.is_open()) {
                size_t storedHash;
                checksumFile >> storedHash;
                checksumFile.close();

                if (storedHash == sourceHash) {
                    return false; // No rebuild needed - source hasn't changed
                }
            }
        }

        // Source has changed, update checksum file
        std::ofstream checksumFile(checksumPath);
        if (checksumFile.is_open()) {
            checksumFile << sourceHash;
            checksumFile.close();
        }

        return true; // Need to rebuild
    } catch (const std::exception& e) {
        // If we can't determine, assume rebuild is needed
        return true;
    }
}

} // namespace madola