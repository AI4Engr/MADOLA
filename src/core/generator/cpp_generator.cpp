#include "cpp_generator.h"
#include <sstream>
#include <cmath>

namespace madola {

CppGenerator::CppGenerator() = default;

std::vector<CppGenerator::GeneratedFile> CppGenerator::generateCppFiles(const Program& program) {
    std::vector<GeneratedFile> files;

    for (const auto& stmt : program.statements) {
        if (const auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(stmt.get())) {
            if (funcDecl->hasDecorator("gen_cpp")) {
                GeneratedFile file;
                file.filename = funcDecl->name + ".cpp";
                file.content = generateCppFunction(*funcDecl);
                files.push_back(file);
            }
        }
    }

    return files;
}

std::string CppGenerator::generateCppFunction(const FunctionDeclaration& func) {
    std::stringstream ss;

    // Clear declared variables for this function
    declaredVariables.clear();
    
    // Add function parameters to declared variables
    for (const auto& param : func.parameters) {
        declaredVariables.insert(param);
    }

    // Add includes
    ss << "#include <cmath>\n\n";

    // Function signature
    ss << getCppType() << " " << func.name << "(";
    for (size_t i = 0; i < func.parameters.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << getCppType() << " " << func.parameters[i];
    }
    ss << ")\n{\n";

    // Function body
    for (const auto& stmt : func.body) {
        ss << "    " << convertStatement(*stmt) << "\n";
    }

    ss << "}\n";

    return ss.str();
}

std::string CppGenerator::convertStatement(const Statement& stmt) {
    if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(&stmt)) {
        std::string result;
        if (declaredVariables.find(assignment->variable) == declaredVariables.end()) {
            // First time declaring this variable
            result = getCppType() + " " + assignment->variable + " = " + convertExpression(*assignment->expression) + ";";
            declaredVariables.insert(assignment->variable);
        } else {
            // Variable already declared, just assign
            result = assignment->variable + " = " + convertExpression(*assignment->expression) + ";";
        }
        return result;
    } else if (const auto* returnStmt = dynamic_cast<const ReturnStatement*>(&stmt)) {
        return "return " + convertExpression(*returnStmt->expression) + ";";
    } else if (const auto* forStmt = dynamic_cast<const ForStatement*>(&stmt)) {
        std::stringstream ss;
        if (const auto* rangeExpr = dynamic_cast<const RangeExpression*>(forStmt->range.get())) {
            std::string start = convertExpression(*rangeExpr->start);
            std::string end = convertExpression(*rangeExpr->end);
            ss << "for (int " << forStmt->variable << " = " << start << "; " << forStmt->variable << " <= " << end << "; " << forStmt->variable << "++) {\n";
            for (const auto& bodyStmt : forStmt->body) {
                ss << "        " << convertStatement(*bodyStmt) << "\n";
            }
            ss << "    }";
        }
        return ss.str();
    } else if (const auto* ifStmt = dynamic_cast<const IfStatement*>(&stmt)) {
        std::stringstream ss;
        ss << "if (" << convertExpression(*ifStmt->condition) << ") {\n";
        for (const auto& bodyStmt : ifStmt->then_body) {
            ss << "        " << convertStatement(*bodyStmt) << "\n";
        }
        if (!ifStmt->else_body.empty()) {
            ss << "    } else {\n";
            for (const auto& bodyStmt : ifStmt->else_body) {
                ss << "        " << convertStatement(*bodyStmt) << "\n";
            }
        }
        ss << "    }";
        return ss.str();
    }
    return "// Unknown statement";
}

std::string CppGenerator::convertExpression(const Expression& expr) {
    if (const auto* identifier = dynamic_cast<const Identifier*>(&expr)) {
        return identifier->name;
    } else if (const auto* number = dynamic_cast<const Number*>(&expr)) {
        // Format as integer if it's a whole number, otherwise as double
        if (number->value == static_cast<int>(number->value)) {
            return std::to_string(static_cast<int>(number->value));
        } else {
            return std::to_string(number->value);
        }
    } else if (const auto* binary = dynamic_cast<const BinaryExpression*>(&expr)) {
        std::string left = convertExpression(*binary->left);
        std::string right = convertExpression(*binary->right);

        if (binary->operator_str == "^") {
            return "pow(" + left + ", " + right + ")";
        } else {
            return "(" + left + " " + binary->operator_str + " " + right + ")";
        }
    } else if (const auto* unary = dynamic_cast<const UnaryExpression*>(&expr)) {
        return unary->operator_str + convertExpression(*unary->operand);
    } else if (const auto* functionCall = dynamic_cast<const FunctionCall*>(&expr)) {
        std::stringstream ss;
        ss << functionCall->function_name << "(";
        for (size_t i = 0; i < functionCall->arguments.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << convertExpression(*functionCall->arguments[i]);
        }
        ss << ")";
        return ss.str();
    }

    return "0"; // Default fallback
}

} // namespace madola