#include "evaluator.h"
#include "../ast/ast.h"

#ifdef WITH_SYMENGINE

#include <symengine/basic.h>
#include <symengine/symbol.h>
#include <symengine/add.h>
#include <symengine/mul.h>
#include <symengine/pow.h>
#include <symengine/functions.h>
#include <symengine/parser.h>
#include <symengine/eval_double.h>
#include <symengine/matrix.h>
#include <symengine/real_double.h>
#include <symengine/integer.h>
#include <symengine/visitor.h>
#include <sstream>
#include <stdexcept>

namespace madola {

using namespace SymEngine;

// Helper function to convert MADOLA expression to SymEngine expression
RCP<const Basic> expressionToSymEngine(const Expression& expr, Evaluator* evaluator);

// Helper function to convert SymEngine expression to string
std::string symEngineToString(const RCP<const Basic>& expr) {
    std::ostringstream oss;
    oss << *expr;
    return oss.str();
}

// Helper function to convert MADOLA Value to SymEngine
RCP<const Basic> valueToSymEngine(const Value& value) {
    if (std::holds_alternative<double>(value)) {
        return SymEngine::real_double(std::get<double>(value));
    } else if (std::holds_alternative<std::string>(value)) {
        // Try to parse as symbol or expression
        std::string str = std::get<std::string>(value);
        try {
            return parse(str);
        } catch (...) {
            return symbol(str);
        }
    }
    throw std::runtime_error("Cannot convert value to symbolic expression");
}

// Convert MADOLA expression to SymEngine expression
RCP<const Basic> expressionToSymEngine(const Expression& expr, Evaluator* evaluator) {
    // Handle Number
    if (const auto* num = dynamic_cast<const Number*>(&expr)) {
        return SymEngine::real_double(num->value);
    }

    // Handle Identifier (variable/symbol)
    if (const auto* id = dynamic_cast<const Identifier*>(&expr)) {
        return symbol(id->name);
    }

    // Handle BinaryExpression
    if (const auto* binExpr = dynamic_cast<const BinaryExpression*>(&expr)) {
        auto left = expressionToSymEngine(*binExpr->left, evaluator);
        auto right = expressionToSymEngine(*binExpr->right, evaluator);

        if (binExpr->operator_str == "+") {
            return add(left, right);
        } else if (binExpr->operator_str == "-") {
            return sub(left, right);
        } else if (binExpr->operator_str == "*") {
            return mul(left, right);
        } else if (binExpr->operator_str == "/") {
            return div(left, right);
        } else if (binExpr->operator_str == "^") {
            return pow(left, right);
        } else {
            throw std::runtime_error("Unsupported binary operator for symbolic computation: " + binExpr->operator_str);
        }
    }

    // Handle UnaryExpression
    if (const auto* unaryExpr = dynamic_cast<const UnaryExpression*>(&expr)) {
        auto operand = expressionToSymEngine(*unaryExpr->operand, evaluator);

        if (unaryExpr->operator_str == "-") {
            return neg(operand);
        } else if (unaryExpr->operator_str == "+") {
            return operand;
        } else {
            throw std::runtime_error("Unsupported unary operator for symbolic computation: " + unaryExpr->operator_str);
        }
    }

    // Handle FunctionCall (for symbolic functions like sin, cos, etc.)
    if (const auto* funcCall = dynamic_cast<const FunctionCall*>(&expr)) {
        if (funcCall->arguments.size() == 1) {
            auto arg = expressionToSymEngine(*funcCall->arguments[0], evaluator);

            if (funcCall->function_name == "sin") {
                return sin(arg);
            } else if (funcCall->function_name == "cos") {
                return cos(arg);
            } else if (funcCall->function_name == "tan") {
                return tan(arg);
            } else if (funcCall->function_name == "exp") {
                return exp(arg);
            } else if (funcCall->function_name == "log") {
                return log(arg);
            } else if (funcCall->function_name == "sqrt") {
                return sqrt(arg);
            }
        }
    }

    throw std::runtime_error("Cannot convert expression to symbolic form");
}

// Symbolic differentiation function
Value Evaluator::evaluateSymbolicDiff(const Expression& expr, const std::string& variable) {
    try {
        // Convert expression to SymEngine
        auto symExpr = expressionToSymEngine(expr, this);

        // Create symbol for differentiation variable
        auto var = symbol(variable);

        // Differentiate
        auto derivative = symExpr->diff(var);

        // Simplify the result
        auto simplified = derivative;

        // Convert back to string
        std::string result = symEngineToString(simplified);

        return result;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Symbolic differentiation error: ") + e.what());
    }
}

// Symbolic matrix differentiation
Value Evaluator::evaluateSymbolicMatrixDiff(const ArrayValue& matrix, const std::string& variable) {
    try {
        if (!matrix.isMatrix) {
            throw std::runtime_error("Expected matrix for symbolic matrix differentiation");
        }

        // Create result matrix
        std::vector<std::vector<std::string>> resultMatrix;

        // Differentiate each element
        for (const auto& row : matrix.matrixRows) {
            std::vector<std::string> resultRow;
            for (double element : row) {
                // Create a number expression
                Number numExpr(element);

                // Differentiate (constant derivative is 0)
                resultRow.push_back("0");
            }
            resultMatrix.push_back(resultRow);
        }

        // For now, return a simple string representation
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < resultMatrix.size(); i++) {
            if (i > 0) oss << "; ";
            oss << "[";
            for (size_t j = 0; j < resultMatrix[i].size(); j++) {
                if (j > 0) oss << ", ";
                oss << resultMatrix[i][j];
            }
            oss << "]";
        }
        oss << "]";

        return oss.str();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Symbolic matrix differentiation error: ") + e.what());
    }
}

// Evaluate a symbolic expression (string) with current environment values
Value Evaluator::evaluateSymbolicExpression(const std::string& symbolicExpr) {
    try {
        // Parse the symbolic expression using SymEngine
        auto expr = parse(symbolicExpr);

        // Get all free symbols in the expression
        auto symbols = free_symbols(*expr);

        // Substitute current environment values for all symbols
        map_basic_basic subs;
        for (const auto& sym : symbols) {
            std::string symName = sym->__str__();
            try {
                Value val = env.get(symName);
                if (std::holds_alternative<double>(val)) {
                    subs[sym] = SymEngine::real_double(std::get<double>(val));
                }
            } catch (...) {
                // Symbol not in environment, leave it as is
            }
        }

        // Substitute and evaluate
        auto substituted = expr->subs(subs);

        // Try to evaluate to a double
        double result = eval_double(*substituted);
        return result;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Symbolic expression evaluation error: ") + e.what());
    }
}

} // namespace madola

#endif // WITH_SYMENGINE
