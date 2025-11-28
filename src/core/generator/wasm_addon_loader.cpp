#include "wasm_addon_loader.h"
#include "../utils/paths.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>

namespace madola {

WasmAddonLoader::WasmAddonLoader() = default;

bool WasmAddonLoader::loadModule(const std::string& moduleName, const std::vector<std::string>& functionNames, std::string& errorMsg) {
    try {
        for (const auto& functionName : functionNames) {
            // Check if function is already loaded
            if (isFunctionAvailable(functionName)) {
                continue; // Skip already loaded functions
            }

            std::string wasmPath, jsPath;
            if (!findWasmFiles(moduleName, functionName, wasmPath, jsPath)) {
                errorMsg = "WASM files not found for function '" + functionName + "' in module '" + moduleName + "'";
                return false;
            }

#ifdef __EMSCRIPTEN__
            // In WASM environment, we can't validate files using filesystem
            // but we still need to register the function for later execution
            auto wasmFunc = std::make_unique<WasmFunction>(functionName, wasmPath, jsPath);
            wasmFunc->isLoaded = true;
            loadedFunctions[functionName] = std::move(wasmFunc);
#else
            // Native environment - validate files exist
            if (!validateFiles(wasmPath, jsPath)) {
                errorMsg = "WASM files invalid or unreadable for function '" + functionName + "'";
                return false;
            }

            // Create and store the function info
            auto wasmFunc = std::make_unique<WasmFunction>(functionName, wasmPath, jsPath);
            wasmFunc->isLoaded = true;
            loadedFunctions[functionName] = std::move(wasmFunc);
#endif
        }

        return true;
    } catch (const std::exception& e) {
        errorMsg = std::string("Failed to load module '") + moduleName + "': " + e.what();
        return false;
    }
}

bool WasmAddonLoader::isFunctionAvailable(const std::string& functionName) const {
    auto it = loadedFunctions.find(functionName);
    return it != loadedFunctions.end() && it->second->isLoaded;
}

const WasmFunction* WasmAddonLoader::getFunction(const std::string& functionName) const {
    auto it = loadedFunctions.find(functionName);
    if (it != loadedFunctions.end()) {
        return it->second.get();
    }
    return nullptr;
}

const std::unordered_map<std::string, std::unique_ptr<WasmFunction>>& WasmAddonLoader::getLoadedFunctions() const {
    return loadedFunctions;
}

bool WasmAddonLoader::findWasmFiles(const std::string& moduleName, const std::string& functionName,
                                   std::string& wasmPath, std::string& jsPath) {
    try {
#ifdef __EMSCRIPTEN__
        // In WASM environment, we assume files are accessible via HTTP
        // Use trove directory paths
        std::string troveDir = utils::getTroveDirectory(moduleName);
        wasmPath = troveDir + "/" + functionName + ".wasm";
        jsPath = troveDir + "/" + functionName + ".js";
        // The actual validation happens during execution
        return true;
#else
        // Native environment - check multiple locations
        // Priority: 1. Current directory, 2. ~/.madola/trove/
        
        std::vector<std::filesystem::path> searchPaths = {
            // Check current directory first (for regression tests)
            std::filesystem::current_path() / moduleName,
            // Check ~/.madola/trove/ (standard location)
            std::filesystem::path(utils::getTroveDirectory(moduleName))
        };

        for (const auto& moduleDir : searchPaths) {
            if (std::filesystem::exists(moduleDir) && std::filesystem::is_directory(moduleDir)) {
                std::string candidateWasmPath = moduleDir.string() + "/" + functionName + ".wasm";
                std::string candidateJsPath = moduleDir.string() + "/" + functionName + ".js";
                
                // Convert backslashes to forward slashes for consistency
                std::replace(candidateWasmPath.begin(), candidateWasmPath.end(), '\\', '/');
                std::replace(candidateJsPath.begin(), candidateJsPath.end(), '\\', '/');
                
                if (std::filesystem::exists(candidateWasmPath) && std::filesystem::exists(candidateJsPath)) {
                    wasmPath = candidateWasmPath;
                    jsPath = candidateJsPath;
                    return true;
                }
            }
        }

        return false;
#endif
    } catch (const std::exception& e) {
        std::cerr << "Error finding WASM files: " << e.what() << std::endl;
        return false;
    }
}

bool WasmAddonLoader::validateFiles(const std::string& wasmPath, const std::string& jsPath) {
    try {
        // Check WASM file
        if (!std::filesystem::exists(wasmPath)) {
            return false;
        }

        std::ifstream wasmFile(wasmPath, std::ios::binary);
        if (!wasmFile.is_open()) {
            return false;
        }
        wasmFile.close();

        // Check JS file
        if (!std::filesystem::exists(jsPath)) {
            return false;
        }

        std::ifstream jsFile(jsPath);
        if (!jsFile.is_open()) {
            return false;
        }
        jsFile.close();

        // Basic validation - files exist and are readable
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error validating WASM files: " << e.what() << std::endl;
        return false;
    }
}

} // namespace madola