#include "html_formatter.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <regex>
#include <cctype>
#include <cmath>

namespace madola {

// Helper function to get operator precedence
static int getOperatorPrecedence(const std::string& op) {
    if (op == "||") return 1;
    if (op == "&&") return 2;
    if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") return 3;
    if (op == "+" || op == "-") return 4;
    if (op == "*" || op == "/") return 5;
    if (op == "^") return 6;
    return 0;
}

// Helper function to check if parentheses are needed
static bool needsParentheses(const Expression* expr, const std::string& parentOp, bool isRight) {
    const auto* binary = dynamic_cast<const BinaryExpression*>(expr);
    if (!binary) return false;

    int parentPrec = getOperatorPrecedence(parentOp);
    int childPrec = getOperatorPrecedence(binary->operator_str);

    // Special case: addition/subtraction in numerator of division doesn't need parentheses
    if (parentOp == "/" && !isRight && (binary->operator_str == "+" || binary->operator_str == "-")) {
        return false;
    }

    // Lower precedence always needs parentheses
    if (childPrec < parentPrec) return true;

    // For equal precedence, right side of non-associative operators needs parentheses
    if (childPrec == parentPrec && isRight && (parentOp == "-" || parentOp == "/" || parentOp == "^")) {
        return true;
    }

    return false;
}

// Helper function to check if an expression is simple (doesn't need \left( \right))
// Simple expressions: single numbers, identifiers, simple unary like -1
// Complex expressions: fractions, nested operations, function calls with multiple terms
static bool isSimpleExpression(const Expression* expr) {
    // Numbers and identifiers are simple
    if (dynamic_cast<const Number*>(expr) || dynamic_cast<const Identifier*>(expr)) {
        return true;
    }

    // Simple unary expressions like -1, -x are simple
    if (const auto* unary = dynamic_cast<const UnaryExpression*>(expr)) {
        // Only simple if the operand is a number or identifier
        return dynamic_cast<const Number*>(unary->operand.get()) ||
               dynamic_cast<const Identifier*>(unary->operand.get());
    }

    // Everything else (binary expressions, function calls, etc.) is complex
    return false;
}

// Helper function to format double with 3 decimal places, removing trailing zeros
// For small numbers (< 0.001), use scientific notation or more precision
static std::string formatDouble(double val) {
    if (val == 0.0) {
        return "0";
    }

    if (val == static_cast<int>(val)) {
        return std::to_string(static_cast<int>(val));
    }

    double absVal = std::abs(val);

    // For very small numbers (absolute value < 0.001), use LaTeX scientific notation
    if (absVal < 0.001 && absVal > 0) {
        std::stringstream ss;
        ss << std::scientific << std::setprecision(2) << val;
        std::string result = ss.str();

        // Convert to LaTeX format: "1.00e-05" -> "1 \times 10^{-5}"
        size_t ePos = result.find('e');
        if (ePos != std::string::npos) {
            std::string mantissa = result.substr(0, ePos);
            std::string exponent = result.substr(ePos + 1); // Skip 'e'

            // Remove trailing zeros from mantissa
            size_t decimalPos = mantissa.find('.');
            if (decimalPos != std::string::npos) {
                mantissa.erase(mantissa.find_last_not_of('0') + 1, std::string::npos);
                if (mantissa.back() == '.') {
                    mantissa.pop_back();
                }
            }

            // Remove leading zeros from exponent (e.g., "-05" -> "-5", "+03" -> "3")
            bool isNegative = (!exponent.empty() && exponent[0] == '-');
            bool isPositive = (!exponent.empty() && exponent[0] == '+');
            if (isNegative || isPositive) {
                std::string sign = exponent.substr(0, 1);
                std::string expNum = exponent.substr(1);
                // Convert to int and back to string to remove leading zeros
                int expValue = std::stoi(expNum);
                exponent = (isNegative ? "-" : "") + std::to_string(expValue);
            } else if (!exponent.empty()) {
                int expValue = std::stoi(exponent);
                exponent = std::to_string(expValue);
            }

            // Format as LaTeX: mantissa \times 10^{exponent}
            result = mantissa + " \\times 10^{" + exponent + "}";
        }

        return result;
    }

    // For normal numbers, use fixed notation with 3 decimal places
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3) << val;
    std::string result = ss.str();

    // Remove trailing zeros after decimal point
    size_t decimalPos = result.find('.');
    if (decimalPos != std::string::npos) {
        result.erase(result.find_last_not_of('0') + 1, std::string::npos);
        // Remove trailing decimal point if all decimals were zeros
        if (result.back() == '.') {
            result.pop_back();
        }
    }

    return result;
}

static std::string formatUnitForLatex(const std::string& unit) {
    // Handle units with numeric suffixes (like in3, mm2, ft3)
    // Convert "in3" -> "\text{ in}^3", "mm2" -> "\text{ mm}^2"

    // Check if unit ends with a digit
    if (unit.length() > 1 && std::isdigit(static_cast<unsigned char>(unit[unit.length() - 1]))) {
        // Find where the number starts
        size_t numStart = unit.length() - 1;
        while (numStart > 0 && std::isdigit(static_cast<unsigned char>(unit[numStart - 1]))) {
            numStart--;
        }

        std::string baseUnit = unit.substr(0, numStart);
        std::string exponent = unit.substr(numStart);
        return "\\text{ " + baseUnit + "}^{" + exponent + "}";
    }

    // No numeric suffix, just wrap in \text{}
    return "\\text{ " + unit + "}";
}

std::string HtmlFormatter::formatStatementAsMathInFunction(const Statement& stmt, Evaluator& evaluator, int depth) {
    if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(&stmt)) {
        // Format assignment statements inside functions/loops
        std::string varName = convertToMathJax(assignment->variable);
        std::string exprStr = formatExpressionAsMath(*assignment->expression, evaluator);
        std::string result = varName + " = " + exprStr;
        if (!assignment->inlineComment.empty()) {
            if (assignment->commentBefore) {
                result = "\\text{" + assignment->inlineComment + "} \\quad " + varName + " = " + exprStr;
            } else {
                result += " \\quad \\text{" + assignment->inlineComment + "}";
            }
        }
        return result;
    } else if (const auto* arrayAssignment = dynamic_cast<const ArrayAssignmentStatement*>(&stmt)) {
        // Format array assignment statements inside functions/loops
        std::string arrayName = convertToMathJax(arrayAssignment->arrayName);
        std::string indexStr = formatExpressionAsMath(*arrayAssignment->index, evaluator);
        std::string exprStr = formatExpressionAsMath(*arrayAssignment->expression, evaluator);
        return arrayName + "[" + indexStr + "] = " + exprStr;
    } else if (const auto* returnStmt = dynamic_cast<const ReturnStatement*>(&stmt)) {
        std::string exprStr = formatExpressionAsMath(*returnStmt->expression, evaluator);
        return "\\text{return } " + exprStr;
    } else if (dynamic_cast<const BreakStatement*>(&stmt)) {
        return "\\text{break}";
    } else if (const auto* ifStmt = dynamic_cast<const IfStatement*>(&stmt)) {
        std::stringstream ss;

        // Format condition
        std::string condition = formatExpressionAsMath(*ifStmt->condition, evaluator);

        // The function body formatter adds \quad to each statement
        // Vertical bars need explicit indentation to align with their keywords
        ss << "\\textbf{if } " << condition << " \\textbf{ then} \\\\\n";
        ss << "\\quad \\left|\n\\begin{array}{l}\n";

        // Format then statements
        for (size_t i = 0; i < ifStmt->then_body.size(); ++i) {
            std::string bodyMath = formatStatementAsMathInFunction(*ifStmt->then_body[i], evaluator, depth + 1);
            if (!bodyMath.empty()) {
                ss << "\\quad " << bodyMath;
                if (i < ifStmt->then_body.size() - 1) {
                    ss << " \\\\\n";  // Add LaTeX line break between statements
                } else {
                    ss << "\n";
                }
            }
        }

        ss << "\\end{array}\n\\right.";

        // Only format else block if there are else statements
        if (!ifStmt->else_body.empty()) {
            ss << "\\\\\n";
            ss << "\\quad \\textbf{else} \\\\\n";
            ss << "\\quad \\left|\n\\begin{array}{l}\n";

            // Format else statements
            for (size_t i = 0; i < ifStmt->else_body.size(); ++i) {
                std::string bodyMath = formatStatementAsMathInFunction(*ifStmt->else_body[i], evaluator, depth + 1);
                if (!bodyMath.empty()) {
                    ss << "\\quad " << bodyMath;
                    if (i < ifStmt->else_body.size() - 1) {
                        ss << " \\\\\n";  // Add LaTeX line break between statements
                    } else {
                        ss << "\n";
                    }
                }
            }

            ss << "\\end{array}\n\\right.";
        }

        return ss.str();
    } else if (const auto* forStmt = dynamic_cast<const ForStatement*>(&stmt)) {
        // Format for loop inside functions
        std::stringstream ss;
        ss << "\\textbf{for } " << convertToMathJax(forStmt->variable) << " \\textbf{ in } ";

        // Format the range
        if (const auto* rangeExpr = dynamic_cast<const RangeExpression*>(forStmt->range.get())) {
            if (rangeExpr->start && rangeExpr->end) {
                ss << formatExpressionAsMath(*rangeExpr->start, evaluator) << "..." << formatExpressionAsMath(*rangeExpr->end, evaluator);
            } else {
                ss << "INVALID_RANGE";
            }
        } else if (forStmt->range) {
            ss << formatExpressionAsMath(*forStmt->range, evaluator);
        } else {
            ss << "NULL_RANGE";
        }

        ss << " \\\\\n";
        ss << "\\quad \\left|\n\\begin{array}{l}\n";

        // Format loop body with support for @layout decorators
        for (size_t i = 0; i < forStmt->body.size(); ++i) {
            const auto* decoratedStmt = dynamic_cast<const DecoratedStatement*>(forStmt->body[i].get());

            // Check if this statement has @layoutNxM decorator
            if (decoratedStmt && decoratedStmt->hasLayoutDecorator()) {
                const Decorator* arrayDec = decoratedStmt->getLayoutDecorator();
                // Check if it's a 1xN layout decorator (single row, multiple columns)
                if (arrayDec && arrayDec->rows == 1 && arrayDec->cols >= 2) {
                    // Format the decorated statement and the next (cols-1) statements on the same line
                    if (decoratedStmt->statement && i + arrayDec->cols - 1 < forStmt->body.size()) {
                        std::vector<std::string> statements;
                        statements.push_back(formatStatementAsMathInFunction(*decoratedStmt->statement, evaluator, depth + 1));

                        // Collect the next (cols-1) statements
                        for (int j = 1; j < arrayDec->cols; ++j) {
                            statements.push_back(formatStatementAsMathInFunction(*forStmt->body[i + j], evaluator, depth + 1));
                        }

                        // Check if all statements formatted successfully
                        bool allValid = true;
                        for (const auto& stmt : statements) {
                            if (stmt.empty()) {
                                allValid = false;
                                break;
                            }
                        }

                        if (allValid) {
                            // Output all statements on the same line with extra spacing
                            ss << "\\quad ";
                            for (size_t j = 0; j < statements.size(); ++j) {
                                if (j > 0) ss << " \\qquad ";
                                ss << statements[j];
                            }
                            ss << " \\\\\n";
                            i += arrayDec->cols - 1; // Skip the statements we already processed
                        }
                    }
                }
            } else {
                std::string bodyMath = formatStatementAsMathInFunction(*forStmt->body[i], evaluator, depth + 1);
                if (!bodyMath.empty()) {
                    ss << "\\quad " << bodyMath << " \\\\\n";
                }
            }
        }

        ss << "\\end{array}\n\\right.";
        return ss.str();
    } else if (const auto* whileStmt = dynamic_cast<const WhileStatement*>(&stmt)) {
        // Format while loop inside functions
        std::stringstream ss;
        ss << "\\textbf{while } " << formatExpressionAsMath(*whileStmt->condition, evaluator) << " \\\\\n";
        ss << "\\quad \\left|\n\\begin{array}{l}\n";

        // Format while loop body
        for (size_t i = 0; i < whileStmt->body.size(); ++i) {
            std::string bodyMath = formatStatementAsMathInFunction(*whileStmt->body[i], evaluator, depth + 1);
            if (!bodyMath.empty()) {
                ss << "\\quad " << bodyMath << " \\\\\n";
            }
        }

        ss << "\\end{array}\n\\right.";
        return ss.str();
    } else if (const auto* decoratedStmt = dynamic_cast<const DecoratedStatement*>(&stmt)) {
        // Handle decorated statements inside functions
        if (decoratedStmt->hasLayoutDecorator()) {
            const Decorator* arrayDec = decoratedStmt->getLayoutDecorator();
            // For @layout1xN decorators, format the statement and return it
            // The calling code will handle combining multiple statements
            if (arrayDec && arrayDec->rows == 1 && arrayDec->cols >= 1) {
                // Just format the underlying statement
                if (decoratedStmt->statement) {
                    return formatStatementAsMathInFunction(*decoratedStmt->statement, evaluator, depth);
                }
            }
        } else {
            // For other decorators, just format the underlying statement
            if (decoratedStmt->statement) {
                return formatStatementAsMathInFunction(*decoratedStmt->statement, evaluator, depth);
            }
        }
    }

    return "";
}

std::string HtmlFormatter::formatExpressionAsMath(const Expression& expr, Evaluator& evaluator) {
    if (const auto* identifier = dynamic_cast<const Identifier*>(&expr)) {
        return convertToMathJax(identifier->name);
    } else if (const auto* number = dynamic_cast<const Number*>(&expr)) {
        return formatDouble(number->value);
    } else if (const auto* stringLit = dynamic_cast<const StringLiteral*>(&expr)) {
        return "\\text{\"" + stringLit->value + "\"}";
    } else if (const auto* complexNum = dynamic_cast<const ComplexNumber*>(&expr)) {
        std::stringstream ss;
        if (complexNum->real == 0 && complexNum->imaginary == 0) {
            return "0";
        }

        bool needsReal = (complexNum->real != 0);
        bool needsImag = (complexNum->imaginary != 0);
        double imag = complexNum->imaginary;

        if (needsReal) {
            if (complexNum->real == static_cast<int>(complexNum->real)) {
                ss << static_cast<int>(complexNum->real);
            } else {
                ss << complexNum->real;
            }
        }

        if (needsImag) {
            if (needsReal && complexNum->imaginary > 0) {
                ss << " + ";
            } else if (needsReal && complexNum->imaginary < 0) {
                ss << " - ";
                imag = -complexNum->imaginary;
            }

            if (imag == 1.0) {
                ss << "i";
            } else if (imag == -1.0 && !needsReal) {
                ss << "-i";
            } else if (imag == static_cast<int>(imag)) {
                ss << static_cast<int>(imag) << "i";
            } else {
                ss << imag << "i";
            }
        }

        return ss.str();
    } else if (const auto* unitExpr = dynamic_cast<const UnitExpression*>(&expr)) {
        std::string valueStr = formatExpressionAsMath(*unitExpr->value, evaluator);
        std::string unitFormatted = formatUnitForLatex(unitExpr->unit);
        return valueStr + unitFormatted;
    } else if (const auto* binary = dynamic_cast<const BinaryExpression*>(&expr)) {
        std::string op = binary->operator_str;

        // Special handling for division: collect numerator and all denominators
        if (op == "/") {
            std::string numerator;
            std::vector<std::string> denominators;
            std::string coefficient;  // Numeric coefficient to extract

            // Check if left side is also a division (nested fraction)
            const BinaryExpression* leftBinary = dynamic_cast<const BinaryExpression*>(binary->left.get());
            if (leftBinary && leftBinary->operator_str == "/") {
                // Extract numerator from nested division
                numerator = formatExpressionAsMath(*leftBinary->left, evaluator);
                // Add first denominator
                std::string firstDenom = formatExpressionAsMath(*leftBinary->right, evaluator);
                if (needsParentheses(leftBinary->right.get(), "/", true)) {
                    firstDenom = "\\left(" + firstDenom + "\\right)";
                }
                denominators.push_back(firstDenom);
            } else if (leftBinary && leftBinary->operator_str == "*") {
                // Check if left side is multiplication: extract numeric coefficient if present
                // e.g., 25 * m/s should be formatted as 25 * (m/s), not (25*m)/s
                const Number* leftNum = dynamic_cast<const Number*>(leftBinary->left.get());
                if (leftNum) {
                    // Extract the numeric coefficient
                    coefficient = formatExpressionAsMath(*leftBinary->left, evaluator);
                    // Use only the right side of multiplication as numerator
                    numerator = formatExpressionAsMath(*leftBinary->right, evaluator);
                } else {
                    // No numeric coefficient, use entire multiplication as numerator
                    numerator = formatExpressionAsMath(*binary->left, evaluator);
                }
                if (needsParentheses(binary->left.get(), "/", false) && coefficient.empty()) {
                    numerator = "\\left(" + numerator + "\\right)";
                }
            } else {
                numerator = formatExpressionAsMath(*binary->left, evaluator);
                if (needsParentheses(binary->left.get(), "/", false)) {
                    numerator = "\\left(" + numerator + "\\right)";
                }
            }

            // Add the current denominator
            std::string currentDenom = formatExpressionAsMath(*binary->right, evaluator);
            if (needsParentheses(binary->right.get(), "/", true)) {
                currentDenom = "\\left(" + currentDenom + "\\right)";
            }
            denominators.push_back(currentDenom);

            // Build denominator string
            std::string denominator = denominators[0];
            for (size_t i = 1; i < denominators.size(); ++i) {
                denominator += " \\cdot " + denominators[i];
            }

            // Return with coefficient if present
            if (!coefficient.empty()) {
                return coefficient + " \\frac{" + numerator + "}{" + denominator + "}";
            } else {
                return "\\frac{" + numerator + "}{" + denominator + "}";
            }
        }

        // Check if children need parentheses
        std::string left = formatExpressionAsMath(*binary->left, evaluator);
        if (needsParentheses(binary->left.get(), op, false)) {
            left = "\\left(" + left + "\\right)";
        }

        std::string right = formatExpressionAsMath(*binary->right, evaluator);
        if (needsParentheses(binary->right.get(), op, true)) {
            right = "\\left(" + right + "\\right)";
        }

        // Convert operators to LaTeX
        if (op == "^") {
            // Check if left side is a unary expression (like -1) that needs braces and parentheses
            const auto* leftUnary = dynamic_cast<const UnaryExpression*>(binary->left.get());
            if (leftUnary) {
                // Use simple () for simple expressions like -1, use \left( \right) for complex ones
                if (isSimpleExpression(binary->left.get())) {
                    return "{(" + left + ")}^{" + right + "}";
                } else {
                    return "{\\left(" + left + "\\right)}^{" + right + "}";
                }
            }
            return left + "^{" + right + "}";
        } else if (op == "*") {
            return left + " \\cdot " + right;
        } else if (op == "<=") {
            return left + " \\leq " + right;
        } else if (op == ">=") {
            return left + " \\geq " + right;
        } else if (op == "!=") {
            return left + " \\ne " + right;
        } else if (op == "&&") {
            return left + " \\;\\land\\; " + right;
        } else if (op == "||") {
            return left + " \\;\\lor\\; " + right;
        } else if (op == "%") {
            return left + " \\% " + right;
        } else {
            return left + " " + op + " " + right;
        }
    } else if (const auto* func = dynamic_cast<const FunctionCall*>(&expr)) {
        // Handle special mathematical functions with LaTeX formatting
        if (func->function_name == "sqrt") {
            if (func->arguments.size() == 1) {
                return "\\sqrt{" + formatExpressionAsMath(*func->arguments[0], evaluator) + "}";
            }
        } else if (func->function_name == "summation") {
            if (func->arguments.size() == 4) {
                // Format as LaTeX summation: \sum_{var=lower}^{upper} expression
                std::string expression = formatExpressionAsMath(*func->arguments[0], evaluator);
                std::string variable = formatExpressionAsMath(*func->arguments[1], evaluator);
                std::string lower = formatExpressionAsMath(*func->arguments[2], evaluator);
                std::string upper = formatExpressionAsMath(*func->arguments[3], evaluator);

                return "\\sum_{" + variable + "=" + lower + "}^{" + upper + "} " + expression;
            }
        }

        // Format function call as math
        std::stringstream ss;
        ss << func->function_name << "(";
        for (size_t i = 0; i < func->arguments.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << formatExpressionAsMath(*func->arguments[i], evaluator);
        }
        ss << ")";
        return ss.str();
    } else if (const auto* methodCall = dynamic_cast<const MethodCall*>(&expr)) {
        // Handle math.* methods specially
        if (const auto* identifier = dynamic_cast<const Identifier*>(methodCall->object.get())) {
            if (identifier->name == "math") {
                if (methodCall->method_name == "summation" && methodCall->arguments.size() == 4) {
                    // Format as LaTeX summation: \sum_{var=lower}^{upper} expression
                    std::string expression = formatExpressionAsMath(*methodCall->arguments[0], evaluator);
                    std::string variable = formatExpressionAsMath(*methodCall->arguments[1], evaluator);
                    std::string lower = formatExpressionAsMath(*methodCall->arguments[2], evaluator);
                    std::string upper = formatExpressionAsMath(*methodCall->arguments[3], evaluator);

                    return "\\sum_{" + variable + "=" + lower + "}^{" + upper + "} " + expression;
                } else if (methodCall->method_name == "sqrt" && methodCall->arguments.size() == 1) {
                    return "\\sqrt{" + formatExpressionAsMath(*methodCall->arguments[0], evaluator) + "}";
                } else if (methodCall->method_name == "abs" && methodCall->arguments.size() == 1) {
                    return "\\left|" + formatExpressionAsMath(*methodCall->arguments[0], evaluator) + "\\right|";
                } else if (methodCall->method_name == "mod" && methodCall->arguments.size() == 2) {
                    std::string dividend = formatExpressionAsMath(*methodCall->arguments[0], evaluator);
                    std::string divisor = formatExpressionAsMath(*methodCall->arguments[1], evaluator);
                    return dividend + " \\bmod " + divisor;
                } else if (methodCall->method_name == "sin" && methodCall->arguments.size() == 1) {
                    return "\\sin(" + formatExpressionAsMath(*methodCall->arguments[0], evaluator) + ")";
                } else if (methodCall->method_name == "cos" && methodCall->arguments.size() == 1) {
                    return "\\cos(" + formatExpressionAsMath(*methodCall->arguments[0], evaluator) + ")";
                } else if (methodCall->method_name == "tan" && methodCall->arguments.size() == 1) {
                    return "\\tan(" + formatExpressionAsMath(*methodCall->arguments[0], evaluator) + ")";
                } else if (methodCall->method_name == "sum" && methodCall->arguments.size() == 1) {
                    return "\\sum " + formatExpressionAsMath(*methodCall->arguments[0], evaluator);
                } else if (methodCall->method_name == "max" && methodCall->arguments.size() == 1) {
                    return "\\text{max}(" + formatExpressionAsMath(*methodCall->arguments[0], evaluator) + ")";
                } else if (methodCall->method_name == "min" && methodCall->arguments.size() == 1) {
                    return "\\text{min}(" + formatExpressionAsMath(*methodCall->arguments[0], evaluator) + ")";
                } else if (methodCall->method_name == "exp" && methodCall->arguments.size() == 1) {
                    return "e^{" + formatExpressionAsMath(*methodCall->arguments[0], evaluator) + "}";
                } else if (methodCall->method_name == "diff" && methodCall->arguments.size() == 2) {
                    // Format math.diff(expression, variable) as \frac{d}{dx} \left( expression \right)
                    std::string expression = formatExpressionAsMath(*methodCall->arguments[0], evaluator);
                    std::string variable = formatExpressionAsMath(*methodCall->arguments[1], evaluator);
                    return "\\frac{d}{d" + variable + "} \\left( " + expression + " \\right)";
                }
            }
        }

        // Handle matrix method calls with LaTeX formatting
        if (methodCall->method_name == "det") {
            return "\\begin{vmatrix}" + formatExpressionAsMath(*methodCall->object, evaluator) + "\\end{vmatrix}";
        } else if (methodCall->method_name == "tr") {
            return "\\text{tr}(" + formatExpressionAsMath(*methodCall->object, evaluator) + ")";
        } else if (methodCall->method_name == "inv") {
            return formatExpressionAsMath(*methodCall->object, evaluator) + "^{-1}";
        } else if (methodCall->method_name == "T") {
            return formatExpressionAsMath(*methodCall->object, evaluator) + "^\\mathsf{T}";
        }

        // Default method call formatting
        std::stringstream ss;
        ss << formatExpressionAsMath(*methodCall->object, evaluator) << "." << methodCall->method_name << "(";
        for (size_t i = 0; i < methodCall->arguments.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << formatExpressionAsMath(*methodCall->arguments[i], evaluator);
        }
        ss << ")";
        return ss.str();
    } else if (const auto* unary = dynamic_cast<const UnaryExpression*>(&expr)) {
        std::string operand = formatExpressionAsMath(*unary->operand, evaluator);

        // Check if operand is a binary expression that needs parentheses
        bool needsParentheses = dynamic_cast<const BinaryExpression*>(unary->operand.get()) != nullptr;

        if (unary->operator_str == "-") {
            if (needsParentheses) {
                return "-\\left(" + operand + "\\right)";
            }
            return "-" + operand;
        }
        return unary->operator_str + operand;
    } else if (const auto* range = dynamic_cast<const RangeExpression*>(&expr)) {
        return formatExpressionAsMath(*range->start, evaluator) + "\\ldots" + formatExpressionAsMath(*range->end, evaluator);
    } else if (const auto* array = dynamic_cast<const ArrayExpression*>(&expr)) {
        // Format array as LaTeX column vector using bmatrix
        std::stringstream ss;
        ss << "\\begin{bmatrix}";
        const auto& elements = array->elements;
        for (size_t i = 0; i < elements.size(); ++i) {
            if (i > 0) ss << " \\\\ ";
            ss << formatExpressionAsMath(*elements[i], evaluator);
        }
        ss << "\\end{bmatrix}";
        return ss.str();
    } else if (const auto* arrayAccess = dynamic_cast<const ArrayAccess*>(&expr)) {
        // Format array access as arrayName_index in subscript notation
        std::string arrayName = convertToMathJax(arrayAccess->arrayName);
        std::string indexStr = formatExpressionAsMath(*arrayAccess->index, evaluator);
        return arrayName + "_{" + indexStr + "}";
    } else if (const auto* pipeExpr = dynamic_cast<const PipeExpression*>(&expr)) {
        // Format pipe expression as: \left.expression\right|_{var=val, var=val}
        std::string exprStr = formatExpressionAsMath(*pipeExpr->expression, evaluator);
        std::stringstream ss;
        ss << "\\left." << exprStr << "\\right|_{";
        for (size_t i = 0; i < pipeExpr->substitutions.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << pipeExpr->substitutions[i].variable << "=";
            ss << formatExpressionAsMath(*pipeExpr->substitutions[i].value, evaluator);
        }
        ss << "}";
        return ss.str();
    } else {
        // Unknown expression type - debug
        return "[UNKNOWN:" + expr.toString() + "]";
    }

    return "";
}

// Implementation of formatting methods (similar to markdown formatter but for HTML)
std::string HtmlFormatter::formatStatement(const Statement& stmt) {
    return formatStatementWithDepth(stmt, 0);
}

std::string HtmlFormatter::formatStatementWithDepth(const Statement& stmt, int depth) {
    return formatStatementWithDepth(stmt, depth, false);
}

std::string HtmlFormatter::formatStatementWithDepth(const Statement& stmt, int depth, bool inFunction) {
    std::string indent(depth * 4, ' ');

    if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(&stmt)) {
        return indent + formatAssignment(*assignment);
    } else if (const auto* print = dynamic_cast<const PrintStatement*>(&stmt)) {
        return indent + formatPrint(*print);
    } else if (const auto* expr = dynamic_cast<const ExpressionStatement*>(&stmt)) {
        return indent + formatExpressionStatement(*expr);
    } else if (const auto* func = dynamic_cast<const FunctionDeclaration*>(&stmt)) {
        return indent + formatFunctionDeclaration(*func);
    } else if (const auto* forStmt = dynamic_cast<const ForStatement*>(&stmt)) {
        return formatFor(*forStmt);
    } else if (const auto* whileStmt = dynamic_cast<const WhileStatement*>(&stmt)) {
        return formatWhile(*whileStmt);
    } else if (const auto* ifStmt = dynamic_cast<const IfStatement*>(&stmt)) {
        return formatIfWithDepth(*ifStmt, depth, inFunction);
    } else if (const auto* piecewise = dynamic_cast<const PiecewiseFunctionDeclaration*>(&stmt)) {
        return indent + formatPiecewiseFunctionDeclaration(*piecewise);
    } else {
        return indent + "/* Unknown statement */";
    }
}

std::string HtmlFormatter::formatAssignment(const AssignmentStatement& stmt) {
    std::string assignment = formatVariableName(stmt.variable) + " := " + formatExpression(*stmt.expression);
    std::string result;
    if (!stmt.inlineComment.empty()) {
        if (stmt.commentBefore) {
            result = "\\text{" + stmt.inlineComment + "} \\quad " + assignment;
        } else {
            result = assignment + " \\quad \\text{" + stmt.inlineComment + "}";
        }
    } else {
        result = assignment;
    }
    return result + ";";
}

std::string HtmlFormatter::formatPrint(const PrintStatement& stmt) {
    return "print(" + formatExpression(*stmt.expression) + ");";
}

std::string HtmlFormatter::formatExpressionStatement(const ExpressionStatement& stmt) {
    return formatExpression(*stmt.expression) + ";";
}

std::string HtmlFormatter::formatFunctionDeclaration(const FunctionDeclaration& stmt) {
    std::stringstream ss;
    ss << formatVariableName(stmt.name) << "(";

    const auto& params = stmt.parameters;
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << formatVariableName(params[i]);
    }

    ss << ") := {";

    // Format function body (statements)
    for (const auto& bodyStmt : stmt.body) {
        ss << " " << formatStatement(*bodyStmt) << ";";
    }

    ss << " }";

    return ss.str();
}

std::string HtmlFormatter::formatPiecewiseFunctionDeclaration(const PiecewiseFunctionDeclaration& stmt) {
    std::stringstream ss;
    ss << formatVariableName(stmt.name) << "(";

    const auto& params = stmt.parameters;
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << formatVariableName(params[i]);
    }

    ss << ") := /* piecewise function */;";

    return ss.str();
}

std::string HtmlFormatter::formatFor(const ForStatement& stmt) {
    std::stringstream ss;
    ss << "for " << formatVariableName(stmt.variable) << " in " << formatExpression(*stmt.range) << " {\n";

    for (const auto& bodyStmt : stmt.body) {
        ss << formatStatementWithDepth(*bodyStmt, 1) << "\n";
    }

    ss << "}";
    return ss.str();
}

std::string HtmlFormatter::formatWhile(const WhileStatement& stmt) {
    std::stringstream ss;
    ss << "while (" << formatExpression(*stmt.condition) << ") {\n";

    for (const auto& bodyStmt : stmt.body) {
        ss << formatStatementWithDepth(*bodyStmt, 1) << "\n";
    }

    ss << "}";
    return ss.str();
}

std::string HtmlFormatter::formatIf(const IfStatement& stmt) {
    return formatIfWithDepth(stmt, 0, false);
}

std::string HtmlFormatter::formatIfWithDepth(const IfStatement& stmt, int depth, bool inFunction) {
    std::string indent(depth * 4, ' ');
    std::stringstream ss;

    ss << indent << "if (" << formatExpression(*stmt.condition) << ") {\n";

    for (const auto& thenStmt : stmt.then_body) {
        ss << formatStatementWithDepth(*thenStmt, depth + 1, inFunction) << "\n";
    }

    if (!stmt.else_body.empty()) {
        ss << indent << "} else {\n";
        for (const auto& elseStmt : stmt.else_body) {
            ss << formatStatementWithDepth(*elseStmt, depth + 1, inFunction) << "\n";
        }
    }

    ss << indent << "}";
    return ss.str();
}

std::string HtmlFormatter::formatExpression(const Expression& expr) {
    if (const auto* identifier = dynamic_cast<const Identifier*>(&expr)) {
        return formatVariableName(identifier->name);
    } else if (const auto* number = dynamic_cast<const Number*>(&expr)) {
        return formatDouble(number->value);
    } else if (const auto* binary = dynamic_cast<const BinaryExpression*>(&expr)) {
        return formatBinaryExpression(*binary);
    } else if (const auto* unary = dynamic_cast<const UnaryExpression*>(&expr)) {
        return formatUnaryExpression(*unary);
    } else if (const auto* func = dynamic_cast<const FunctionCall*>(&expr)) {
        return formatFunctionCall(*func);
    } else if (const auto* range = dynamic_cast<const RangeExpression*>(&expr)) {
        return formatRangeExpression(*range);
    } else if (const auto* piecewise = dynamic_cast<const PiecewiseExpression*>(&expr)) {
        return formatPiecewiseExpression(*piecewise);
    } else if (const auto* array = dynamic_cast<const ArrayExpression*>(&expr)) {
        return formatArrayExpression(*array);
    } else if (const auto* access = dynamic_cast<const ArrayAccess*>(&expr)) {
        return formatArrayAccess(*access);
    } else if (const auto* unitExpr = dynamic_cast<const UnitExpression*>(&expr)) {
        std::string valueStr = formatExpression(*unitExpr->value);
        return valueStr + " " + formatUnitForLatex(unitExpr->unit);
    } else {
        return "/* Unknown expression */";
    }
}

std::string HtmlFormatter::formatBinaryExpression(const BinaryExpression& expr) {
    std::string left = formatExpression(*expr.left);
    std::string right = formatExpression(*expr.right);
    std::string op = expr.operator_str;

    return "(" + left + " " + op + " " + right + ")";
}

std::string HtmlFormatter::formatUnaryExpression(const UnaryExpression& expr) {
    std::string operand = formatExpression(*expr.operand);
    return expr.operator_str + operand;
}

std::string HtmlFormatter::formatFunctionCall(const FunctionCall& expr) {
    std::stringstream ss;
    ss << formatVariableName(expr.function_name) << "(";

    const auto& args = expr.arguments;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << formatExpression(*args[i]);
    }

    ss << ")";
    return ss.str();
}

std::string HtmlFormatter::formatVariableName(const std::string& varName) {
    return varName;
}

std::string HtmlFormatter::formatRangeExpression(const RangeExpression& expr) {
    return formatExpression(*expr.start) + "..." + formatExpression(*expr.end);
}

std::string HtmlFormatter::formatPiecewiseExpression(const PiecewiseExpression& /* expr */) {
    // For now, return a simple placeholder since we need to check the actual structure
    return "{/* piecewise expression */}";
}

std::string HtmlFormatter::formatArrayExpression(const ArrayExpression& expr) {
    std::stringstream ss;
    ss << "[";

    const auto& elements = expr.elements;
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << formatExpression(*elements[i]);
    }

    ss << "]";
    return ss.str();
}

std::string HtmlFormatter::formatArrayAccess(const ArrayAccess& expr) {
    return expr.arrayName + "[" + formatExpression(*expr.index) + "]";
}

// Placeholder implementations for complex formatting methods
std::string HtmlFormatter::formatExpressionWithValues(const Expression& expr, Evaluator& /* evaluator */) {
    return formatExpression(expr);
}

std::string HtmlFormatter::formatDecoratedStatementContent(const DecoratedStatement& /* stmt */, Evaluator& /* evaluator */) {
    return "/* Decorated statement */";
}

std::string HtmlFormatter::formatStatementForArray(const Statement& stmt, Evaluator& /* evaluator */) {
    return formatStatement(stmt);
}

std::string HtmlFormatter::formatExpressionWithValuesAsMath(const Expression& expr, Evaluator& evaluator) {
    if (const auto* identifier = dynamic_cast<const Identifier*>(&expr)) {
        // Replace identifier with its value from the evaluator's environment
        try {
            Value value = evaluator.getVariableValue(identifier->name);
            return formatValueAsMath(value);
        } catch (...) {
            // If variable not found, keep original identifier
            return identifier->name;
        }
    } else if (const auto* number = dynamic_cast<const Number*>(&expr)) {
        // Numbers stay the same
        return formatExpressionAsMath(*number, evaluator);
    } else if (const auto* binary = dynamic_cast<const BinaryExpression*>(&expr)) {
        // Recursively process binary expressions

        if (binary->operator_str == "/") {
            // Special handling for division: collect numerator and all denominators
            std::string numerator;
            std::vector<std::string> denominators;

            // Check if left side is also a division (nested fraction)
            const BinaryExpression* leftBinary = dynamic_cast<const BinaryExpression*>(binary->left.get());
            if (leftBinary && leftBinary->operator_str == "/") {
                // Extract numerator from nested division
                numerator = formatExpressionWithValuesAsMath(*leftBinary->left, evaluator);
                // Add first denominator
                denominators.push_back(formatExpressionWithValuesAsMath(*leftBinary->right, evaluator));
            } else {
                numerator = formatExpressionWithValuesAsMath(*binary->left, evaluator);
            }

            // Add the current denominator
            denominators.push_back(formatExpressionWithValuesAsMath(*binary->right, evaluator));

            // Build denominator string
            std::string denominator = denominators[0];
            for (size_t i = 1; i < denominators.size(); ++i) {
                denominator += " \\cdot " + denominators[i];
            }

            return "\\frac{" + numerator + "}{" + denominator + "}";
        }

        std::string left = formatExpressionWithValuesAsMath(*binary->left, evaluator);
        std::string right = formatExpressionWithValuesAsMath(*binary->right, evaluator);

        if (binary->operator_str == "^") {
            // Wrap negative numbers in parentheses for exponentiation: (-5)^2 instead of -5^2
            if (!left.empty() && left[0] == '-') {
                return "{(" + left + ")}^{" + right + "}";
            }
            return "{" + left + "}^{" + right + "}";
        } else if (binary->operator_str == "*") {
            return left + " \\cdot " + right;
        } else if (binary->operator_str == "<=") {
            return left + " \\leq " + right;
        } else if (binary->operator_str == ">=") {
            return left + " \\geq " + right;
        } else {
            return left + " " + binary->operator_str + " " + right;
        }
    } else if (const auto* unary = dynamic_cast<const UnaryExpression*>(&expr)) {
        std::string operand = formatExpressionWithValuesAsMath(*unary->operand, evaluator);
        if (unary->operator_str == "-") {
            // If operand starts with -, wrap it in extra parentheses to show (-(-5)) instead of (--5)
            if (!operand.empty() && operand[0] == '-') {
                return "(-(" + operand + "))";
            }
            return "(-" + operand + ")";
        }
        return unary->operator_str + operand;
    } else if (const auto* functionCall = dynamic_cast<const FunctionCall*>(&expr)) {
        // Handle special mathematical functions with LaTeX formatting and value substitution
        if (functionCall->function_name == "sqrt") {
            if (functionCall->arguments.size() == 1) {
                return "\\sqrt{" + formatExpressionWithValuesAsMath(*functionCall->arguments[0], evaluator) + "}";
            }
        } else if (functionCall->function_name == "summation") {
            if (functionCall->arguments.size() == 4) {
                // Format as LaTeX summation: \sum_{var=lower}^{upper} expression
                // For summation, keep the variable literal in the expression too
                std::string expression = formatExpressionAsMath(*functionCall->arguments[0], evaluator);
                std::string variable = formatExpressionAsMath(*functionCall->arguments[1], evaluator); // Keep variable as literal
                std::string lower = formatExpressionWithValuesAsMath(*functionCall->arguments[2], evaluator);
                std::string upper = formatExpressionWithValuesAsMath(*functionCall->arguments[3], evaluator);

                return "\\sum_{" + variable + "=" + lower + "}^{" + upper + "} " + expression;
            }
        }

        // Default function formatting with value substitution
        std::stringstream ss;
        ss << functionCall->function_name << "(";
        for (size_t i = 0; i < functionCall->arguments.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << formatExpressionWithValuesAsMath(*functionCall->arguments[i], evaluator);
        }
        ss << ")";
        return ss.str();
    } else if (const auto* methodCall = dynamic_cast<const MethodCall*>(&expr)) {
        // Handle math.* methods specially
        if (const auto* identifier = dynamic_cast<const Identifier*>(methodCall->object.get())) {
            if (identifier->name == "math") {
                if (methodCall->method_name == "summation" && methodCall->arguments.size() == 4) {
                    // Format as LaTeX summation: \sum_{var=lower}^{upper} expression
                    // For summation, keep the variable literal in the expression too
                    std::string expression = formatExpressionAsMath(*methodCall->arguments[0], evaluator);
                    std::string variable = formatExpressionAsMath(*methodCall->arguments[1], evaluator); // Keep variable as literal
                    std::string lower = formatExpressionWithValuesAsMath(*methodCall->arguments[2], evaluator);
                    std::string upper = formatExpressionWithValuesAsMath(*methodCall->arguments[3], evaluator);

                    return "\\sum_{" + variable + "=" + lower + "}^{" + upper + "} " + expression;
                } else if (methodCall->method_name == "sqrt" && methodCall->arguments.size() == 1) {
                    return "\\sqrt{" + formatExpressionWithValuesAsMath(*methodCall->arguments[0], evaluator) + "}";
                } else if (methodCall->method_name == "abs" && methodCall->arguments.size() == 1) {
                    return "\\left|" + formatExpressionWithValuesAsMath(*methodCall->arguments[0], evaluator) + "\\right|";
                } else if (methodCall->method_name == "mod" && methodCall->arguments.size() == 2) {
                    std::string dividend = formatExpressionWithValuesAsMath(*methodCall->arguments[0], evaluator);
                    std::string divisor = formatExpressionWithValuesAsMath(*methodCall->arguments[1], evaluator);
                    return dividend + " \\bmod " + divisor;
                } else if (methodCall->method_name == "sin" && methodCall->arguments.size() == 1) {
                    return "\\sin(" + formatExpressionWithValuesAsMath(*methodCall->arguments[0], evaluator) + ")";
                } else if (methodCall->method_name == "cos" && methodCall->arguments.size() == 1) {
                    return "\\cos(" + formatExpressionWithValuesAsMath(*methodCall->arguments[0], evaluator) + ")";
                } else if (methodCall->method_name == "tan" && methodCall->arguments.size() == 1) {
                    return "\\tan(" + formatExpressionWithValuesAsMath(*methodCall->arguments[0], evaluator) + ")";
                } else if (methodCall->method_name == "sum" && methodCall->arguments.size() == 1) {
                    return "\\sum " + formatExpressionWithValuesAsMath(*methodCall->arguments[0], evaluator);
                } else if (methodCall->method_name == "max" && methodCall->arguments.size() == 1) {
                    return "\\text{max}(" + formatExpressionWithValuesAsMath(*methodCall->arguments[0], evaluator) + ")";
                } else if (methodCall->method_name == "min" && methodCall->arguments.size() == 1) {
                    return "\\text{min}(" + formatExpressionWithValuesAsMath(*methodCall->arguments[0], evaluator) + ")";
                } else if (methodCall->method_name == "exp" && methodCall->arguments.size() == 1) {
                    return "e^{" + formatExpressionWithValuesAsMath(*methodCall->arguments[0], evaluator) + "}";
                } else if (methodCall->method_name == "diff" && methodCall->arguments.size() == 2) {
                    // Format math.diff(expression, variable) as \frac{d}{dx} \left( expression \right)
                    std::string expression = formatExpressionAsMath(*methodCall->arguments[0], evaluator);
                    std::string variable = formatExpressionAsMath(*methodCall->arguments[1], evaluator);
                    return "\\frac{d}{d" + variable + "} \\left( " + expression + " \\right)";
                }
            }
        }

        // Handle matrix method calls with LaTeX formatting
        if (methodCall->method_name == "det") {
            return "\\begin{vmatrix}" + formatExpressionWithValuesAsMath(*methodCall->object, evaluator) + "\\end{vmatrix}";
        } else if (methodCall->method_name == "tr") {
            return "\\text{tr}(" + formatExpressionWithValuesAsMath(*methodCall->object, evaluator) + ")";
        } else if (methodCall->method_name == "inv") {
            return formatExpressionWithValuesAsMath(*methodCall->object, evaluator) + "^{-1}";
        } else if (methodCall->method_name == "T") {
            return formatExpressionWithValuesAsMath(*methodCall->object, evaluator) + "^\\mathsf{T}";
        }

        // Default method call formatting
        std::stringstream ss;
        ss << formatExpressionWithValuesAsMath(*methodCall->object, evaluator) << "." << methodCall->method_name << "(";
        for (size_t i = 0; i < methodCall->arguments.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << formatExpressionWithValuesAsMath(*methodCall->arguments[i], evaluator);
        }
        ss << ")";
        return ss.str();
    } else if (const auto* array = dynamic_cast<const ArrayExpression*>(&expr)) {
        // Handle array expressions by evaluating them first to get proper matrix/vector formatting
        try {
            Value value = evaluator.evaluateExpression(*array);
            return formatValueAsMath(value);
        } catch (...) {
            // Fallback to regular array formatting if evaluation fails
            return formatExpressionAsMath(expr, evaluator);
        }
    }

    // Fallback to regular formatting
    return formatExpressionAsMath(expr, evaluator);
}

std::string HtmlFormatter::formatValueAsMath(const Value& value) {
    if (std::holds_alternative<double>(value)) {
        return formatDouble(std::get<double>(value));
    } else if (std::holds_alternative<UnitValue>(value)) {
        UnitValue unitVal = std::get<UnitValue>(value);
        return formatDouble(unitVal.value) + " " + formatUnitForLatex(unitVal.unit);
    } else if (std::holds_alternative<ArrayValue>(value)) {
        ArrayValue arrayVal = std::get<ArrayValue>(value);
        if (arrayVal.isMatrix) {
            // Format matrix as LaTeX matrix
            std::stringstream ss;
            ss << "\\begin{bmatrix}\n";
            for (size_t i = 0; i < arrayVal.matrixRows.size(); ++i) {
                for (size_t j = 0; j < arrayVal.matrixRows[i].size(); ++j) {
                    if (j > 0) ss << " & ";
                    double val = arrayVal.matrixRows[i][j];
                    if (val == static_cast<int>(val)) {
                        ss << static_cast<int>(val);
                    } else {
                        ss << val;
                    }
                }
                if (i < arrayVal.matrixRows.size() - 1) ss << " \\\\\n";
            }
            ss << "\n\\end{bmatrix}";
            return ss.str();
        } else {
            // Format vector as LaTeX column or row vector
            std::stringstream ss;
            if (arrayVal.isColumnVector) {
                ss << "\\begin{bmatrix}";
                for (size_t i = 0; i < arrayVal.elements.size(); ++i) {
                    double val = arrayVal.elements[i];
                    if (val == static_cast<int>(val)) {
                        ss << static_cast<int>(val);
                    } else {
                        ss << val;
                    }
                    if (i < arrayVal.elements.size() - 1) ss << " \\\\ ";
                }
                ss << "\\end{bmatrix}";
            } else {
                ss << "\\begin{bmatrix}";
                for (size_t i = 0; i < arrayVal.elements.size(); ++i) {
                    if (i > 0) ss << " & ";
                    double val = arrayVal.elements[i];
                    if (val == static_cast<int>(val)) {
                        ss << static_cast<int>(val);
                    } else {
                        ss << val;
                    }
                }
                ss << "\\end{bmatrix}";
            }
            return ss.str();
        }
    }
    return "UNKNOWN_VALUE";
}

std::string HtmlFormatter::formatPiecewiseExpressionAsMath(const PiecewiseExpression& expr, Evaluator& evaluator) {
    std::stringstream ss;

    ss << "\\begin{cases}\n";

    for (const auto& case_ptr : expr.cases) {
        ss << formatExpressionAsMath(*case_ptr->value, evaluator);

        if (case_ptr->condition == nullptr) {
            // "otherwise" case
            ss << " & \\text{otherwise}";
        } else {
            // Regular condition
            ss << " & " << formatExpressionAsMath(*case_ptr->condition, evaluator);
        }

        ss << " \\\\\n";
    }

    ss << "\\end{cases}";
    return ss.str();
}

} // namespace madola