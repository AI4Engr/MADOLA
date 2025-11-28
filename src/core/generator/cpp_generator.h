#pragma once
#include "../ast/ast.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace madola {

class CppGenerator {
public:
    struct GeneratedFile {
        std::string filename;
        std::string content;
    };

    CppGenerator();

    // Generate C++ files for functions with @gen_cpp decorator
    std::vector<GeneratedFile> generateCppFiles(const Program& program);

    // Public method to generate a single C++ function (used by WasmGenerator)
    std::string generateCppFunction(const FunctionDeclaration& func);

private:
    // Track declared variables in current function scope
    std::set<std::string> declaredVariables;

    // Convert MADOLA expression to C++ expression
    std::string convertExpression(const Expression& expr);

    // Convert MADOLA statement to C++ statement
    std::string convertStatement(const Statement& stmt);

    // Helper to get C++ type (using double for all numeric values)
    std::string getCppType() { return "double"; }
};

using CppGeneratorPtr = std::unique_ptr<CppGenerator>;

} // namespace madola