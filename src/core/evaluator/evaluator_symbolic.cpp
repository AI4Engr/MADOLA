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
#include <symengine/rational.h>
#include <symengine/visitor.h>
#include <symengine/simplify.h>
#include <sstream>
#include <stdexcept>
#include <regex>
#include <functional>
#include <iomanip>
#include <vector>
#include <iterator>

namespace madola {

using namespace SymEngine;

// Helper function to convert MADOLA expression to SymEngine expression
RCP<const Basic> expressionToSymEngine(const Expression& expr, Evaluator* evaluator);

// Helper function to convert SymEngine expression to MADOLA-compatible string
std::string symEngineToString(const RCP<const Basic>& expr) {
    std::ostringstream oss;
    oss << *expr;
    std::string result = oss.str();

    // Replace ** with ^
    std::regex pattern_power(R"(\*\*)");
    result = std::regex_replace(result, pattern_power, "^");

    // Convert (1/n)*x^k to \frac{x^k}{n} for LaTeX
    std::regex pattern_frac_pow(R"(\(1/(\d+)\)\*x\^(\d+))");
    result = std::regex_replace(result, pattern_frac_pow, "\\frac{x^$2}{$1}");

    // Convert (1/n)*x to \frac{x}{n} for LaTeX
    std::regex pattern_frac_x(R"(\(1/(\d+)\)\*x)");
    result = std::regex_replace(result, pattern_frac_x, "\\frac{x}{$1}");

    return result;
}

// Helper function for basic integration
static RCP<const Basic> integrateBasic(const RCP<const Basic>& expr, const RCP<const Symbol>& var) {
    // Handle addition: ∫(f + g) dx = ∫f dx + ∫g dx
    if (is_a<Add>(*expr)) {
        const auto& add_expr = down_cast<const Add&>(*expr);
        auto args = add_expr.get_args();
        RCP<const Basic> result = integer(0);
        for (const auto& arg : args) {
            result = add(result, integrateBasic(arg, var));
        }
        return result;
    }

    // Handle multiplication: ∫c*f dx = c*∫f dx
    if (is_a<Mul>(*expr)) {
        const auto& mul_expr = down_cast<const Mul&>(*expr);
        auto args = mul_expr.get_args();
        RCP<const Basic> coeff = integer(1);
        RCP<const Basic> func = integer(1);

        for (const auto& arg : args) {
            if (is_a<Integer>(*arg) || is_a<Rational>(*arg) || is_a<RealDouble>(*arg)) {
                coeff = mul(coeff, arg);
            } else {
                func = mul(func, arg);
            }
        }

        return mul(coeff, integrateBasic(func, var));
    }

    // Handle power: ∫x^n dx = x^(n+1)/(n+1)
    if (is_a<Pow>(*expr)) {
        const auto& pow_expr = down_cast<const Pow&>(*expr);
        auto args = pow_expr.get_args();
        if (args.size() >= 2 && eq(*args[0], *var)) {
            if (is_a<Integer>(*args[1])) {
                int n = down_cast<const Integer&>(*args[1]).as_int();
                if (n != -1) {
                    return div(pow(var, integer(n + 1)), integer(n + 1));
                }
            }
        }
    }

    // Handle variable: ∫x dx = x^2/2
    if (eq(*expr, *var)) {
        return div(pow(var, integer(2)), integer(2));
    }

    // Handle constant: ∫c dx = c*x
    if (is_a<Integer>(*expr) || is_a<Rational>(*expr) || is_a<RealDouble>(*expr)) {
        return mul(expr, var);
    }

    // Default: return unevaluated
    std::ostringstream oss;
    oss << "integrate(" << *expr << ", " << *var << ")";
    return parse(oss.str());
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
        double val = num->value;
        // Check if it's an integer
        if (val == std::floor(val) && std::abs(val) < 1e15) {
            return integer(static_cast<long>(val));
        }
        // Check if it's a simple fraction
        for (int denom = 2; denom <= 100; ++denom) {
            double numerator = val * denom;
            if (std::abs(numerator - std::round(numerator)) < 1e-10) {
                return rational(static_cast<long>(std::round(numerator)), denom);
            }
        }
        return SymEngine::real_double(val);
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

// Symbolic integration function
Value Evaluator::evaluateSymbolicIntegral(const Expression& expr, const std::string& variable) {
    try {
        // Convert expression to SymEngine
        auto symExpr = expressionToSymEngine(expr, this);

        // Create symbol for integration variable
        auto var = symbol(variable);

        // Expand and simplify before integration
        auto expanded = expand(symExpr);

        // Perform basic integration
        auto result = integrateBasic(expanded, var);

        // Simplify the result using SymEngine's simplify function
        auto simplified = simplify(result);

        // Convert back to string
        std::string result_str = symEngineToString(simplified);

        return result_str;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Symbolic integration error: ") + e.what());
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
