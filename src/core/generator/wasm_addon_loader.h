#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace madola {

struct WasmFunction {
    std::string name;
    std::string wasmPath;
    std::string jsWrapperPath;
    std::vector<std::string> parameters;
    bool isLoaded;

    WasmFunction(const std::string& functionName, const std::string& wasm, const std::string& js)
        : name(functionName), wasmPath(wasm), jsWrapperPath(js), isLoaded(false) {}
};

class WasmAddonLoader {
public:
    WasmAddonLoader();

    // Load WASM functions from a module (looks in trove/{moduleName}/)
    bool loadModule(const std::string& moduleName, const std::vector<std::string>& functionNames, std::string& errorMsg);

    // Check if a function is available
    bool isFunctionAvailable(const std::string& functionName) const;

    // Get function info
    const WasmFunction* getFunction(const std::string& functionName) const;

    // Get all loaded functions
    const std::unordered_map<std::string, std::unique_ptr<WasmFunction>>& getLoadedFunctions() const;

private:
    std::unordered_map<std::string, std::unique_ptr<WasmFunction>> loadedFunctions;

    // Find WASM files in trove directory
    bool findWasmFiles(const std::string& moduleName, const std::string& functionName,
                       std::string& wasmPath, std::string& jsPath);

    // Validate WASM and JS files exist and are readable
    bool validateFiles(const std::string& wasmPath, const std::string& jsPath);
};

using WasmAddonLoaderPtr = std::unique_ptr<WasmAddonLoader>;

} // namespace madola