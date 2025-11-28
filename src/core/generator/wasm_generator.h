#pragma once
#include "../ast/ast.h"
#include "cpp_generator.h"
#include <string>
#include <vector>
#include <map>

namespace madola {

class WasmGenerator {
public:
    struct GeneratedWasm {
        std::string functionName;
        std::string wasmPath;
        std::string jsWrapperPath;
        std::string cppSourcePath;
        std::string cppContent;  // The actual C++ source code
        bool compilationSuccess;
        std::string errorMessage;
    };

    WasmGenerator();

    // Generate WASM files for functions with @gen_addon decorator
    std::vector<GeneratedWasm> generateWasmFiles(const Program& program, const std::string& mdaFileName = "default");

private:
    CppGenerator cppGen;

    // Generate C++ source with WASM exports
    std::string generateWasmCppSource(const FunctionDeclaration& func);

    // Compile C++ to WASM using Emscripten
    bool compileToWasm(const std::string& cppSource, const std::string& outputPath, const std::string& functionName, std::string& errorMsg);

    // Generate JavaScript wrapper for WASM function
    std::string generateJsWrapper(const FunctionDeclaration& func, const std::string& wasmPath);

    // Create directory structure
    bool createTroveDirectories(const std::string& mdaFileName);

    // Get safe filename from mda name
    std::string getSafeFileName(const std::string& mdaFileName);

    // Check if WASM needs to be rebuilt (source code changed)
    bool needsRebuild(const std::string& cppSource, const std::string& wasmPath, const std::string& jsPath);

    // Get system temp directory for checksum storage
    std::string getSystemTempDir();
};

using WasmGeneratorPtr = std::unique_ptr<WasmGenerator>;

} // namespace madola