#include "evaluator.h"
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <climits>  // Add this line for INT_MAX

namespace madola {

Value Evaluator::evaluateExpression(const Expression& expr) {
    // Use dynamic_cast to determine expression type
    if (const auto* identifier = dynamic_cast<const Identifier*>(&expr)) {
        return env.get(identifier->name);
    } else if (const auto* number = dynamic_cast<const Number*>(&expr)) {
        return number->value;
    } else if (const auto* stringLit = dynamic_cast<const StringLiteral*>(&expr)) {
        return stringLit->value;
    } else if (const auto* complexNum = dynamic_cast<const ComplexNumber*>(&expr)) {
        return ComplexValue(complexNum->real, complexNum->imaginary);
    } else if (const auto* binary = dynamic_cast<const BinaryExpression*>(&expr)) {
        return evaluateBinaryExpression(*binary);
    } else if (const auto* unary = dynamic_cast<const UnaryExpression*>(&expr)) {
        return evaluateUnaryExpression(*unary);
    } else if (const auto* functionCall = dynamic_cast<const FunctionCall*>(&expr)) {
        return evaluateFunctionCall(*functionCall);
    } else if (const auto* methodCall = dynamic_cast<const MethodCall*>(&expr)) {
        return evaluateMethodCall(*methodCall);
    } else if (dynamic_cast<const RangeExpression*>(&expr)) {
        // Range expressions are evaluated by for loops, not standalone
        throw std::runtime_error("Range expressions can only be used in for loops");
    } else if (const auto* pipeExpr = dynamic_cast<const PipeExpression*>(&expr)) {
        return evaluatePipeExpression(*pipeExpr);
    } else if (const auto* unitExpr = dynamic_cast<const UnitExpression*>(&expr)) {
        return evaluateUnitExpression(*unitExpr);
    } else if (const auto* piecewiseExpr = dynamic_cast<const PiecewiseExpression*>(&expr)) {
        return evaluatePiecewiseExpression(*piecewiseExpr);
    } else if (const auto* summationExpr = dynamic_cast<const SummationExpression*>(&expr)) {
        return evaluateSummationExpression(*summationExpr);
    } else if (const auto* arrayExpr = dynamic_cast<const ArrayExpression*>(&expr)) {
        return evaluateArrayExpression(*arrayExpr);
    } else if (const auto* arrayAccess = dynamic_cast<const ArrayAccess*>(&expr)) {
        return evaluateArrayAccess(*arrayAccess);
    } else {
        throw std::runtime_error("Unknown expression type");
    }
}

Value Evaluator::evaluateUnitExpression(const UnitExpression& expr) {
    Value val = evaluateExpression(*expr.value);

    if (std::holds_alternative<double>(val)) {
        double numericValue = std::get<double>(val);
        return UnitValue{numericValue, expr.unit};
    } else {
        throw std::runtime_error("Unit expressions require numeric values");
    }
}

Value Evaluator::evaluateUnaryExpression(const UnaryExpression& expr) {
    Value operand = evaluateExpression(*expr.operand);

    if (std::holds_alternative<double>(operand)) {
        double val = std::get<double>(operand);

        if (expr.operator_str == "-") {
            return -val;
        } else if (expr.operator_str == "+") {
            return val;
        } else if (expr.operator_str == "!") {
            // Logical NOT: 0 is false, non-zero is true
            return (val == 0.0) ? 1.0 : 0.0;
        } else {
            throw std::runtime_error("Unknown unary operator: " + expr.operator_str);
        }
    } else if (std::holds_alternative<ArrayValue>(operand)) {
        const ArrayValue& arr = std::get<ArrayValue>(operand);
        ArrayValue result = arr;  // Copy array structure

        if (expr.operator_str == "-") {
            for (double& elem : result.elements) {
                elem = -elem;
            }
            return result;
        } else if (expr.operator_str == "+") {
            return result;
        } else {
            throw std::runtime_error("Unknown unary operator for arrays: " + expr.operator_str);
        }
    } else {
        throw std::runtime_error("Unary operations only supported on numbers and arrays");
    }
}

Value Evaluator::evaluatePipeExpression(const PipeExpression& expr) {
    // Create a temporary environment with the substituted values
    // Save the current values of the variables being substituted
    std::vector<std::pair<std::string, Value>> savedValues;
    std::vector<std::string> newVariables;

    for (const auto& sub : expr.substitutions) {
        try {
            // Try to get the current value (if it exists)
            Value currentValue = env.get(sub.variable);
            savedValues.push_back({sub.variable, currentValue});
        } catch (...) {
            // Variable doesn't exist yet, mark it as new
            newVariables.push_back(sub.variable);
        }

        // Evaluate the substitution value and set it in the environment
        Value subValue = evaluateExpression(*sub.value);
        env.define(sub.variable, subValue);
    }

    // Evaluate the expression with the substituted values
    Value result = evaluateExpression(*expr.expression);

    // If the result is a string (symbolic expression), try to evaluate it numerically
    #ifdef WITH_SYMENGINE
    if (std::holds_alternative<std::string>(result)) {
        try {
            result = evaluateSymbolicExpression(std::get<std::string>(result));
        } catch (...) {
            // If evaluation fails, keep the string result
        }
    }
    #endif

    // Restore the original values
    for (const auto& [var, val] : savedValues) {
        env.define(var, val);
    }

    // Remove newly created variables
    for (const auto& var : newVariables) {
        env.remove(var);
    }

    return result;
}

Value Evaluator::evaluateBinaryExpression(const BinaryExpression& expr) {
    Value left = evaluateExpression(*expr.left);
    Value right = evaluateExpression(*expr.right);

    // Handle string concatenation with + operator (must be before other type checks)
    if (expr.operator_str == "+") {
        // String + String
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right)) {
            return std::get<std::string>(left) + std::get<std::string>(right);
        }
        // Number + String (convert number to string)
        if (std::holds_alternative<double>(left) && std::holds_alternative<std::string>(right)) {
            return valueToString(left) + std::get<std::string>(right);
        }
        // String + Number (convert number to string)
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<double>(right)) {
            return std::get<std::string>(left) + valueToString(right);
        }
        // UnitValue + String
        if (std::holds_alternative<UnitValue>(left) && std::holds_alternative<std::string>(right)) {
            return valueToString(left) + std::get<std::string>(right);
        }
        // String + UnitValue
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<UnitValue>(right)) {
            return std::get<std::string>(left) + valueToString(right);
        }
        // ComplexValue + String
        if (std::holds_alternative<ComplexValue>(left) && std::holds_alternative<std::string>(right)) {
            return valueToString(left) + std::get<std::string>(right);
        }
        // String + ComplexValue
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<ComplexValue>(right)) {
            return std::get<std::string>(left) + valueToString(right);
        }
        // ArrayValue + String
        if (std::holds_alternative<ArrayValue>(left) && std::holds_alternative<std::string>(right)) {
            return valueToString(left) + std::get<std::string>(right);
        }
        // String + ArrayValue
        if (std::holds_alternative<std::string>(left) && std::holds_alternative<ArrayValue>(right)) {
            return std::get<std::string>(left) + valueToString(right);
        }
    }

    // Handle complex number operations
    if (std::holds_alternative<ComplexValue>(left) || std::holds_alternative<ComplexValue>(right)) {
        ComplexValue leftComplex = std::holds_alternative<ComplexValue>(left) ?
            std::get<ComplexValue>(left) : ComplexValue(std::get<double>(left), 0.0);
        ComplexValue rightComplex = std::holds_alternative<ComplexValue>(right) ?
            std::get<ComplexValue>(right) : ComplexValue(std::get<double>(right), 0.0);

        if (expr.operator_str == "+") {
            return leftComplex + rightComplex;
        } else if (expr.operator_str == "-") {
            return leftComplex - rightComplex;
        } else if (expr.operator_str == "*") {
            return leftComplex * rightComplex;
        } else if (expr.operator_str == "/") {
            return leftComplex / rightComplex;
        } else {
            throw std::runtime_error("Operator " + expr.operator_str + " not supported for complex numbers");
        }
    }

    // Handle array-scalar operations (element-wise)
    if (std::holds_alternative<ArrayValue>(left) && std::holds_alternative<double>(right)) {
        const ArrayValue& arr = std::get<ArrayValue>(left);
        double scalar = std::get<double>(right);

        ArrayValue result = arr;  // Copy array structure

        if (expr.operator_str == "+") {
            for (double& elem : result.elements) {
                elem += scalar;
            }
            return result;
        } else if (expr.operator_str == "-") {
            for (double& elem : result.elements) {
                elem -= scalar;
            }
            return result;
        } else if (expr.operator_str == "*") {
            for (double& elem : result.elements) {
                elem *= scalar;
            }
            return result;
        } else if (expr.operator_str == "/") {
            if (scalar == 0.0) {
                throw std::runtime_error("Division by zero in array-scalar operation");
            }
            for (double& elem : result.elements) {
                elem /= scalar;
            }
            return result;
        } else if (expr.operator_str == "^") {
            for (double& elem : result.elements) {
                elem = std::pow(elem, scalar);
            }
            return result;
        }
    }

    // Handle scalar-array operations (element-wise)
    if (std::holds_alternative<double>(left) && std::holds_alternative<ArrayValue>(right)) {
        double scalar = std::get<double>(left);
        const ArrayValue& arr = std::get<ArrayValue>(right);

        ArrayValue result = arr;  // Copy array structure

        if (expr.operator_str == "+") {
            for (double& elem : result.elements) {
                elem = scalar + elem;
            }
            return result;
        } else if (expr.operator_str == "-") {
            for (double& elem : result.elements) {
                elem = scalar - elem;
            }
            return result;
        } else if (expr.operator_str == "*") {
            for (double& elem : result.elements) {
                elem = scalar * elem;
            }
            return result;
        } else if (expr.operator_str == "/") {
            for (double& elem : result.elements) {
                if (elem == 0.0) {
                    throw std::runtime_error("Division by zero in scalar-array operation");
                }
                elem = scalar / elem;
            }
            return result;
        } else if (expr.operator_str == "^") {
            for (double& elem : result.elements) {
                elem = std::pow(scalar, elem);
            }
            return result;
        }
    }

    // Handle element-wise array-array operations
    if (std::holds_alternative<ArrayValue>(left) && std::holds_alternative<ArrayValue>(right)) {
        const ArrayValue& leftArr = std::get<ArrayValue>(left);
        const ArrayValue& rightArr = std::get<ArrayValue>(right);

        // For element-wise operations (not matrix multiplication), arrays must have same size
        if (expr.operator_str != "*" || (!leftArr.isMatrix && !rightArr.isMatrix)) {
            if (leftArr.elements.size() != rightArr.elements.size()) {
                throw std::runtime_error("Array size mismatch for element-wise operation: " +
                                       std::to_string(leftArr.elements.size()) + " vs " +
                                       std::to_string(rightArr.elements.size()));
            }

            ArrayValue result = leftArr;  // Copy array structure

            if (expr.operator_str == "+") {
                for (size_t i = 0; i < result.elements.size(); ++i) {
                    result.elements[i] = leftArr.elements[i] + rightArr.elements[i];
                }
                return result;
            } else if (expr.operator_str == "-") {
                for (size_t i = 0; i < result.elements.size(); ++i) {
                    result.elements[i] = leftArr.elements[i] - rightArr.elements[i];
                }
                return result;
            } else if (expr.operator_str == "/") {
                for (size_t i = 0; i < result.elements.size(); ++i) {
                    if (rightArr.elements[i] == 0.0) {
                        throw std::runtime_error("Division by zero in element-wise array operation");
                    }
                    result.elements[i] = leftArr.elements[i] / rightArr.elements[i];
                }
                return result;
            } else if (expr.operator_str == "^") {
                for (size_t i = 0; i < result.elements.size(); ++i) {
                    result.elements[i] = std::pow(leftArr.elements[i], rightArr.elements[i]);
                }
                return result;
            }
        }
    }

    // Handle matrix-vector operations
    if (expr.operator_str == "*") {
        // Check for matrix * vector multiplication
        if (std::holds_alternative<ArrayValue>(left) && std::holds_alternative<ArrayValue>(right)) {
            const ArrayValue& leftArray = std::get<ArrayValue>(left);
            const ArrayValue& rightArray = std::get<ArrayValue>(right);

            // Matrix * Matrix multiplication
            if (leftArray.isMatrix && rightArray.isMatrix) {
                return multiplyMatrixMatrix(leftArray, rightArray);
            }
            // Matrix * Column Vector
            else if (leftArray.isMatrix && rightArray.isColumnVector) {
                return multiplyMatrixVector(leftArray, rightArray);
            }
            // Row Vector * Matrix
            else if (!leftArray.isMatrix && !leftArray.isColumnVector && rightArray.isMatrix) {
                return multiplyVectorMatrix(leftArray, rightArray);
            }
            // Row Vector * Column Vector (dot product)
            else if (!leftArray.isMatrix && !leftArray.isColumnVector &&
                     !rightArray.isMatrix && rightArray.isColumnVector) {
                return dotProduct(leftArray, rightArray);
            }
            // Element-wise multiplication for non-matrix arrays
            else if (!leftArray.isMatrix && !rightArray.isMatrix) {
                if (leftArray.elements.size() != rightArray.elements.size()) {
                    throw std::runtime_error("Array size mismatch for element-wise multiplication: " +
                                           std::to_string(leftArray.elements.size()) + " vs " +
                                           std::to_string(rightArray.elements.size()));
                }
                ArrayValue result = leftArray;
                for (size_t i = 0; i < result.elements.size(); ++i) {
                    result.elements[i] = leftArray.elements[i] * rightArray.elements[i];
                }
                return result;
            }
        }
    }

    // Fast path: if both are plain numbers, do direct numeric operations
    if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
        double a = std::get<double>(left);
        double b = std::get<double>(right);
        if (expr.operator_str == "+") {
            return a + b;
        } else if (expr.operator_str == "-") {
            return a - b;
        } else if (expr.operator_str == "*") {
            return a * b;
        } else if (expr.operator_str == "/") {
            if (b == 0.0) {
                std::ostringstream oss;
                oss << "Division by zero: " << a << " / " << b;
                throw std::runtime_error(oss.str());
            }
            return a / b;
        } else if (expr.operator_str == "^") {
            return std::pow(a, b);
        } else if (expr.operator_str == ">") {
            return (a > b) ? 1.0 : 0.0;
        } else if (expr.operator_str == "<") {
            return (a < b) ? 1.0 : 0.0;
        } else if (expr.operator_str == ">=") {
            return (a >= b) ? 1.0 : 0.0;
        } else if (expr.operator_str == "<=") {
            return (a <= b) ? 1.0 : 0.0;
        } else if (expr.operator_str == "==") {
            return (a == b) ? 1.0 : 0.0;
        } else if (expr.operator_str == "!=") {
            return (a != b) ? 1.0 : 0.0;
        } else if (expr.operator_str == "&&") {
            return (a != 0.0 && b != 0.0) ? 1.0 : 0.0;
        } else if (expr.operator_str == "||") {
            return (a != 0.0 || b != 0.0) ? 1.0 : 0.0;
        } else if (expr.operator_str == "%") {
            if (b == 0.0) {
                throw std::runtime_error("Modulo by zero");
            }
            return std::fmod(a, b);
        }
        // Fallthrough to unit-aware logic for any other operator
    }

    // Convert pure numbers to UnitValue for consistent handling
    UnitValue leftUnit = std::holds_alternative<UnitValue>(left) ?
        std::get<UnitValue>(left) : UnitValue(std::get<double>(left));
    UnitValue rightUnit = std::holds_alternative<UnitValue>(right) ?
        std::get<UnitValue>(right) : UnitValue(std::get<double>(right));

    try {
        if (expr.operator_str == "+") {
            return leftUnit + rightUnit;
        } else if (expr.operator_str == "-") {
            return leftUnit - rightUnit;
        } else if (expr.operator_str == "*") {
            return leftUnit * rightUnit;
        } else if (expr.operator_str == "/") {
            return leftUnit / rightUnit;
        } else if (expr.operator_str == "^") {
            return leftUnit ^ rightUnit;
        } else if (expr.operator_str == ">") {
            return (leftUnit.value > rightUnit.value) ? 1.0 : 0.0;
        } else if (expr.operator_str == "<") {
            return (leftUnit.value < rightUnit.value) ? 1.0 : 0.0;
        } else if (expr.operator_str == ">=") {
            return (leftUnit.value >= rightUnit.value) ? 1.0 : 0.0;
        } else if (expr.operator_str == "<=") {
            return (leftUnit.value <= rightUnit.value) ? 1.0 : 0.0;
        } else if (expr.operator_str == "==") {
            return (leftUnit.value == rightUnit.value) ? 1.0 : 0.0;
        } else if (expr.operator_str == "!=") {
            return (leftUnit.value != rightUnit.value) ? 1.0 : 0.0;
        } else if (expr.operator_str == "&&") {
            return (leftUnit.value != 0.0 && rightUnit.value != 0.0) ? 1.0 : 0.0;
        } else if (expr.operator_str == "||") {
            return (leftUnit.value != 0.0 || rightUnit.value != 0.0) ? 1.0 : 0.0;
        } else {
            throw std::runtime_error("Unknown binary operator: " + expr.operator_str);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Binary operation error: " + std::string(e.what()));
    }
}

Value Evaluator::evaluatePiecewiseExpression(const PiecewiseExpression& expr) {
    // Evaluate each case in order
    for (const auto& case_ptr : expr.cases) {
        if (case_ptr->condition == nullptr) {
            // This is the "otherwise" case - always return its value
            return evaluateExpression(*case_ptr->value);
        } else {
            // Evaluate the condition
            Value condResult = evaluateExpression(*case_ptr->condition);

            // Check if condition is true
            // Convert to boolean: non-zero numbers are true, zero is false
            bool isTrue = false;
            if (std::holds_alternative<double>(condResult)) {
                isTrue = std::get<double>(condResult) != 0.0;
            } else if (std::holds_alternative<UnitValue>(condResult)) {
                isTrue = std::get<UnitValue>(condResult).value != 0.0;
            }
            // String values are considered false for now

            if (isTrue) {
                return evaluateExpression(*case_ptr->value);
            }
        }
    }

    // If no condition was true and there's no "otherwise" case, return 0
    return 0.0;
}

Value Evaluator::evaluateSummationExpression(const SummationExpression& expr) {
    return evaluateSummation(*expr.expression, expr.variable, *expr.lowerBound, *expr.upperBound);
}

Value Evaluator::evaluateSummation(const Expression& expr, const std::string& variable, const Expression& lowerBound, const Expression& upperBound) {
    // Evaluate bounds
    Value lowerVal = evaluateExpression(lowerBound);
    Value upperVal = evaluateExpression(upperBound);


    // Convert bounds to integers
    int lower, upper;
    double lowerDouble, upperDouble;

    if (std::holds_alternative<double>(lowerVal)) {
        lowerDouble = std::get<double>(lowerVal);
    } else if (std::holds_alternative<UnitValue>(lowerVal)) {
        lowerDouble = std::get<UnitValue>(lowerVal).value;
    } else {
        throw std::runtime_error("Summation lower bound must be numeric");
    }

    if (std::holds_alternative<double>(upperVal)) {
        upperDouble = std::get<double>(upperVal);
    } else if (std::holds_alternative<UnitValue>(upperVal)) {
        upperDouble = std::get<UnitValue>(upperVal).value;
    } else {
        throw std::runtime_error("Summation upper bound must be numeric");
    }

    // Check for precision loss when converting to integer
    if (std::floor(lowerDouble) != lowerDouble) {
        throw std::runtime_error("Summation lower bound must be an integer, got: " + std::to_string(lowerDouble));
    }
    if (std::floor(upperDouble) != upperDouble) {
        throw std::runtime_error("Summation upper bound must be an integer, got: " + std::to_string(upperDouble));
    }

    // Check for overflow when converting to int
    if (lowerDouble > static_cast<double>(INT_MAX) || lowerDouble < static_cast<double>(INT_MIN)) {
        throw std::runtime_error("Summation lower bound out of valid range: " + std::to_string(lowerDouble));
    }
    if (upperDouble > static_cast<double>(INT_MAX) || upperDouble < static_cast<double>(INT_MIN)) {
        throw std::runtime_error("Summation upper bound out of valid range: " + std::to_string(upperDouble));
    }

    lower = static_cast<int>(lowerDouble);
    upper = static_cast<int>(upperDouble);

    // Save the current value of the summation variable if it exists
    Value savedValue;
    bool variableExisted = env.exists(variable);
    if (variableExisted) {
        savedValue = env.get(variable);
    }

    // Perform summation
    double sum = 0.0;
    try {
        for (int i = lower; i <= upper; ++i) {
            // Set the summation variable to current index
            env.define(variable, static_cast<double>(i));

            // Evaluate the expression for this iteration
            Value result = evaluateExpression(expr);

            // Add to sum
            if (std::holds_alternative<double>(result)) {
                sum += std::get<double>(result);
            } else if (std::holds_alternative<UnitValue>(result)) {
                // For unit values, just add the numeric part
                sum += std::get<UnitValue>(result).value;
            } else {
                throw std::runtime_error("Summation expression must evaluate to numeric values");
            }
        }
    } catch (const std::exception&) {
        // Restore the original variable value on error
        if (variableExisted) {
            env.define(variable, savedValue);
        }
        throw;
    }

    // Restore the original variable value
    if (variableExisted) {
        env.define(variable, savedValue);
    }
    return sum;
}

Value Evaluator::evaluateArrayExpression(const ArrayExpression& expr) {
    if (expr.isMatrix) {
        // Handle matrix
        std::vector<std::vector<double>> matrixRows;

        for (const auto& row : expr.matrixRows) {
            std::vector<double> rowElements;
            for (const auto& elementExpr : row) {
                Value val = evaluateExpression(*elementExpr);

                if (std::holds_alternative<double>(val)) {
                    rowElements.push_back(std::get<double>(val));
                } else if (std::holds_alternative<std::string>(val)) {
                    throw std::runtime_error("Matrix elements must be numeric (got string: '" + std::get<std::string>(val) + "')");
                } else if (std::holds_alternative<UnitValue>(val)) {
                    throw std::runtime_error("Matrix elements must be numeric (got unit value)");
                } else if (std::holds_alternative<ArrayValue>(val)) {
                    throw std::runtime_error("Matrix elements must be numeric (got nested array)");
                } else {
                    throw std::runtime_error("Matrix elements must be numeric (got unknown type)");
                }
            }
            matrixRows.push_back(std::move(rowElements));
        }

        return ArrayValue(std::move(matrixRows));
    } else {
        // Handle vector
        std::vector<double> elements;

        for (const auto& elementExpr : expr.elements) {
            Value val = evaluateExpression(*elementExpr);

            if (std::holds_alternative<double>(val)) {
                elements.push_back(std::get<double>(val));
            } else if (std::holds_alternative<std::string>(val)) {
                throw std::runtime_error("Array elements must be numeric (got string: '" + std::get<std::string>(val) + "')");
            } else if (std::holds_alternative<UnitValue>(val)) {
                throw std::runtime_error("Array elements must be numeric (got unit value)");
            } else if (std::holds_alternative<ArrayValue>(val)) {
                throw std::runtime_error("Array elements must be numeric (got nested array)");
            } else {
                throw std::runtime_error("Array elements must be numeric (got unknown type)");
            }
        }

        return ArrayValue(std::move(elements), expr.isColumnVector);
    }
}

Value Evaluator::evaluateArrayAccess(const ArrayAccess& expr) {
    // Get the array from the environment
    Value arrayValue = env.get(expr.arrayName);

    if (!std::holds_alternative<ArrayValue>(arrayValue)) {
        throw std::runtime_error("Variable '" + expr.arrayName + "' is not an array");
    }

    const ArrayValue& array = std::get<ArrayValue>(arrayValue);

    // Evaluate the index expression
    Value indexValue = evaluateExpression(*expr.index);

    if (!std::holds_alternative<double>(indexValue)) {
        throw std::runtime_error("Array index must be a number");
    }

    double indexDouble = std::get<double>(indexValue);

    // Check for precision loss when converting to integer
    if (std::floor(indexDouble) != indexDouble) {
        throw std::runtime_error("Array index must be an integer, got: " + std::to_string(indexDouble));
    }

    // Check for overflow when converting to int
    if (indexDouble > static_cast<double>(INT_MAX) || indexDouble < static_cast<double>(INT_MIN)) {
        throw std::runtime_error("Array index out of valid range: " + std::to_string(indexDouble));
    }

    int index = static_cast<int>(indexDouble);

    // Check bounds (negative and upper bound)
    if (index < 0) {
        throw std::runtime_error("Array index must be non-negative, got: " + std::to_string(index) +
                                " for array '" + expr.arrayName + "'");
    }

    if (index >= static_cast<int>(array.elements.size())) {
        throw std::runtime_error("Array index " + std::to_string(index) + " out of bounds for array '" +
                                expr.arrayName + "' (size: " + std::to_string(array.elements.size()) + ")");
    }

    return array.elements[index];
}

} // namespace madola