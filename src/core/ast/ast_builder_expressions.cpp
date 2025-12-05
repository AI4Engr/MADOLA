#include "ast_builder.h"
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <cctype>

namespace madola {

ExpressionPtr ASTBuilder::buildExpression(TSNode node, const std::string& source) {
    const char* node_type = ts_node_type(node);

    if (strcmp(node_type, "identifier") == 0) {
        return buildIdentifier(node, source);
    } else if (strcmp(node_type, "string_literal") == 0) {
        return buildStringLiteral(node, source);
    } else if (strcmp(node_type, "latex_frac") == 0) {
        return buildIdentifier(node, source);
    } else if (strcmp(node_type, "greek_identifier") == 0) {
        return buildIdentifier(node, source);
    } else if (strcmp(node_type, "subscripted_identifier") == 0) {
        return buildIdentifier(node, source);
    } else if (strcmp(node_type, "number") == 0) {
        return buildNumber(node, source);
    } else if (strcmp(node_type, "complex_number") == 0) {
        return buildComplexNumber(node, source);
    } else if (strcmp(node_type, "unit_expression") == 0) {
        return buildUnitExpression(node, source);
    } else if (strcmp(node_type, "function_call") == 0) {
        return buildFunctionCall(node, source);
    } else if (strcmp(node_type, "method_call") == 0) {
        return buildMethodCall(node, source);
    } else if (strcmp(node_type, "binary_expression") == 0) {
        return buildBinaryExpression(node, source);
    } else if (strcmp(node_type, "unary_expression") == 0) {
        return buildUnaryExpression(node, source);
    } else if (strcmp(node_type, "range_expression") == 0) {
        return buildRangeExpression(node, source);
    } else if (strcmp(node_type, "pipe_expression") == 0) {
        return buildPipeExpression(node, source);
    } else if (strcmp(node_type, "piecewise_expression") == 0) {
        return buildPiecewiseExpression(node, source);
    } else if (strcmp(node_type, "array_expression") == 0) {
        return buildArrayExpression(node, source);
    } else if (strcmp(node_type, "array_access") == 0) {
        return buildArrayAccess(node, source);
    } else if (strcmp(node_type, "primary_expression") == 0) {
        // Handle primary_expression which can be identifier, number, or (expression)
        uint32_t childCount = ts_node_child_count(node);
        if (childCount == 1) {
            // Simple case: identifier or number
            TSNode child = ts_node_child(node, 0);
            return buildExpression(child, source);
        } else if (childCount == 3) {
            // Parenthesized expression: '(' expression ')'
            TSNode exprChild = ts_node_child(node, 1); // Skip '(' and get expression
            return buildExpression(exprChild, source);
        }
    } else if (strcmp(node_type, "expression") == 0 ||
               strcmp(node_type, "logical_or_expression") == 0 ||
               strcmp(node_type, "logical_and_expression") == 0 ||
               strcmp(node_type, "comparison_expression") == 0 ||
               strcmp(node_type, "additive_expression") == 0 ||
               strcmp(node_type, "multiplicative_expression") == 0 ||
               strcmp(node_type, "primary_expression") == 0 ||
               strcmp(node_type, "postfix_expression") == 0 ||
               strcmp(node_type, "unary_expression") == 0 ||
               strcmp(node_type, "power_expression") == 0) {
        // Handle intermediate expression types by looking at their first child
        uint32_t childCount = ts_node_child_count(node);
        if (childCount == 1) {
            // Simple case: just one child
            TSNode child = ts_node_child(node, 0);
            return buildExpression(child, source);
        } else if (childCount == 2 && strcmp(node_type, "unary_expression") == 0) {
            // Unary expression: operator + operand
            TSNode op = ts_node_child(node, 0);
            TSNode operand = ts_node_child(node, 1);

            std::string operator_str = getNodeText(op, source);
            auto operandExpr = buildExpression(operand, source);

            if (operandExpr) {
                return std::make_unique<UnaryExpression>(operator_str, std::move(operandExpr));
            }
        } else if (childCount > 1) {
            // Multiple children - could be a binary operation
            // Check if it matches a binary operation pattern
            TSNode left = ts_node_child(node, 0);
            TSNode op = ts_node_child(node, 1);
            TSNode right = ts_node_child(node, 2);

            if (childCount >= 3) {
                std::string operator_str = getNodeText(op, source);
                auto leftExpr = buildExpression(left, source);
                auto rightExpr = buildExpression(right, source);

                if (leftExpr && rightExpr) {
                    return std::make_unique<BinaryExpression>(std::move(leftExpr), operator_str, std::move(rightExpr));
                }
            }

            // Fall back to first child
            TSNode child = ts_node_child(node, 0);
            return buildExpression(child, source);
        } else {
            // No children, try to parse as identifier or number directly
            std::string text = getNodeText(node, source);
            // Try to parse as number first
            try {
                double value = std::stod(text);
                return std::make_unique<Number>(value);
            } catch (...) {
                // If not a number, treat as identifier
                return std::make_unique<Identifier>(text);
            }
        }
    }

    return nullptr;
}

ExpressionPtr ASTBuilder::buildIdentifier(TSNode node, const std::string& source) {
    std::string name = getNodeText(node, source);
    SourceLocation start_pos = getNodeStartPosition(node);
    SourceLocation end_pos = getNodeEndPosition(node);
    return std::make_unique<Identifier>(name, start_pos, end_pos);
}

ExpressionPtr ASTBuilder::buildStringLiteral(TSNode node, const std::string& source) {
    std::string text = getNodeText(node, source);
    // Remove surrounding quotes
    if (text.length() >= 2 && text.front() == '"' && text.back() == '"') {
        text = text.substr(1, text.length() - 2);
    }
    SourceLocation start_pos = getNodeStartPosition(node);
    SourceLocation end_pos = getNodeEndPosition(node);
    return std::make_unique<StringLiteral>(text, start_pos, end_pos);
}

ExpressionPtr ASTBuilder::buildNumber(TSNode node, const std::string& source) {
    std::string value_str = getNodeText(node, source);
    double value = std::stod(value_str);
    SourceLocation start_pos = getNodeStartPosition(node);
    SourceLocation end_pos = getNodeEndPosition(node);
    return std::make_unique<Number>(value, start_pos, end_pos);
}

ExpressionPtr ASTBuilder::buildComplexNumber(TSNode node, const std::string& source) {
    // complex_number is tokenized, so we parse the full text
    std::string fullText = getNodeText(node, source);

    double real = 0.0;
    double imaginary = 0.0;

    // Remove spaces
    std::string text = fullText;
    text.erase(std::remove(text.begin(), text.end(), ' '), text.end());

    // Check if it's just "i"
    if (text == "i") {
        imaginary = 1.0;
    }
    // Check for pure imaginary like "2i" or "-3i"
    else if (text.back() == 'i' && text.find('+') == std::string::npos && text.find('-') == std::string::npos) {
        std::string imagStr = text.substr(0, text.length() - 1);
        if (imagStr.empty()) {
            imaginary = 1.0;
        } else {
            imaginary = std::stod(imagStr);
        }
    }
    // Complex number with real and imaginary parts like "1+2i" or "3-4i"
    else if (text.back() == 'i') {
        size_t plusPos = text.find('+');
        size_t minusPos = text.rfind('-'); // Use rfind to get the operator, not negative sign

        if (plusPos != std::string::npos && plusPos > 0) {
            // Format: real + imag i
            std::string realStr = text.substr(0, plusPos);
            std::string imagStr = text.substr(plusPos + 1, text.length() - plusPos - 2); // -2 for 'i'
            real = std::stod(realStr);
            if (imagStr.empty() || imagStr == "+") {
                imaginary = 1.0;
            } else {
                imaginary = std::stod(imagStr);
            }
        } else if (minusPos != std::string::npos && minusPos > 0) {
            // Format: real - imag i
            std::string realStr = text.substr(0, minusPos);
            std::string imagStr = text.substr(minusPos + 1, text.length() - minusPos - 2); // -2 for 'i'
            real = std::stod(realStr);
            if (imagStr.empty() || imagStr == "-") {
                imaginary = -1.0;
            } else {
                imaginary = -std::stod(imagStr);
            }
        }
    }

    SourceLocation start_pos = getNodeStartPosition(node);
    SourceLocation end_pos = getNodeEndPosition(node);
    return std::make_unique<ComplexNumber>(real, imaginary, start_pos, end_pos);
}

ExpressionPtr ASTBuilder::buildUnitExpression(TSNode node, const std::string& source) {
    // unit_expression: number unit_identifier optional(^ number)
    TSNode numberNode = ts_node_child(node, 0);
    TSNode unitNode = ts_node_child(node, 1);

    auto valueExpr = buildNumber(numberNode, source);
    std::string unitStr = getNodeText(unitNode, source);

    // Check if there's an exponent (^2, ^3, etc.)
    uint32_t childCount = ts_node_child_count(node);
    if (childCount >= 4) {
        // Children: number, unit_identifier, ^, number
        TSNode exponentNode = ts_node_child(node, 3);
        std::string exponentStr = getNodeText(exponentNode, source);
        unitStr += exponentStr;  // Append exponent to unit (e.g., "in" + "3" = "in3")
    }

    return std::make_unique<UnitExpression>(std::move(valueExpr), unitStr);
}

ExpressionPtr ASTBuilder::buildFunctionCall(TSNode node, const std::string& source) {
    // function_call: qualified_identifier "(" optional(argument_list) ")"
    std::string funcName = getNodeText(ts_node_child(node, 0), source);
    std::vector<ExpressionPtr> arguments;

    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; i++) {
        TSNode child = ts_node_child(node, i);
        const char* childType = ts_node_type(child);

        if (strcmp(childType, "argument_list") == 0) {
            uint32_t argCount = ts_node_child_count(child);
            for (uint32_t j = 0; j < argCount; j++) {
                TSNode argNode = ts_node_child(child, j);
                if (strcmp(ts_node_type(argNode), "expression") == 0) {
                    auto expr = buildExpression(argNode, source);
                    if (expr) {
                        arguments.push_back(std::move(expr));
                    }
                }
                // Skip comma tokens
            }
        }
    }

    return std::make_unique<FunctionCall>(funcName, std::move(arguments));
}

ExpressionPtr ASTBuilder::buildMethodCall(TSNode node, const std::string& source) {
    // method_call: primary_expression "." identifier "(" optional(argument_list) ")"
    uint32_t childCount = ts_node_child_count(node);

    ExpressionPtr object = nullptr;
    std::string methodName;
    std::vector<ExpressionPtr> arguments;

    for (uint32_t i = 0; i < childCount; i++) {
        TSNode child = ts_node_child(node, i);
        const char* childType = ts_node_type(child);

        if (strcmp(childType, "primary_expression") == 0) {
            object = buildExpression(child, source);
        } else if (strcmp(childType, "identifier") == 0) {
            methodName = getNodeText(child, source);
        } else if (strcmp(childType, "argument_list") == 0) {
            uint32_t argCount = ts_node_child_count(child);
            for (uint32_t j = 0; j < argCount; j++) {
                TSNode argNode = ts_node_child(child, j);
                if (strcmp(ts_node_type(argNode), "expression") == 0) {
                    auto expr = buildExpression(argNode, source);
                    if (expr) {
                        arguments.push_back(std::move(expr));
                    }
                }
            }
        }
    }

    return std::make_unique<MethodCall>(std::move(object), methodName, std::move(arguments));
}

ExpressionPtr ASTBuilder::buildBinaryExpression(TSNode node, const std::string& source) {
    // binary_expression: primary_expression operator primary_expression
    uint32_t childCount = ts_node_child_count(node);

    if (childCount == 2) {
        // Tree-sitter might not include the operator as a separate node.
        // Determine operator by using byte ranges between left and right children.
        TSNode leftNode = ts_node_child(node, 0);
        TSNode rightNode = ts_node_child(node, 1);

        auto left = buildExpression(leftNode, source);
        auto right = buildExpression(rightNode, source);

        if (!left || !right) {
            return nullptr;
        }

        uint32_t leftEndByte = ts_node_end_byte(leftNode);
        uint32_t rightStartByte = ts_node_start_byte(rightNode);

        std::string op;
        if (rightStartByte >= leftEndByte && rightStartByte <= source.size()) {
            // Extract operator directly from source between the two child nodes
            op = source.substr(leftEndByte, static_cast<size_t>(rightStartByte - leftEndByte));
            // Trim whitespace
            op.erase(op.begin(), std::find_if(op.begin(), op.end(), [](int ch) { return !std::isspace(ch); }));
            op.erase(std::find_if(op.rbegin(), op.rend(), [](int ch) { return !std::isspace(ch); }).base(), op.end());
        }

        if (op.empty()) {
            // Fallback: try to read operator as middle child if present
            if (ts_node_child_count(node) >= 3) {
                op = getNodeText(ts_node_child(node, 1), source);
            } else {
                // As a last resort, treat it as addition (should not happen)
                op = "+";
            }
        }

        return std::make_unique<BinaryExpression>(std::move(left), op, std::move(right));
    } else if (childCount == 3) {
        // Traditional case with separate operator node
        auto left = buildExpression(ts_node_child(node, 0), source);
        std::string op = getNodeText(ts_node_child(node, 1), source);
        auto right = buildExpression(ts_node_child(node, 2), source);

        // Check if both expressions were successfully built
        if (!left || !right) {
            return nullptr;
        }

        return std::make_unique<BinaryExpression>(std::move(left), op, std::move(right));
    }

    return nullptr;
}

ExpressionPtr ASTBuilder::buildUnaryExpression(TSNode node, const std::string& source) {
    // unary_expression: ('+' | '-') primary_expression
    std::string op = getNodeText(ts_node_child(node, 0), source);
    auto operand = buildExpression(ts_node_child(node, 1), source);

    return std::make_unique<UnaryExpression>(op, std::move(operand));
}

ExpressionPtr ASTBuilder::buildRangeExpression(TSNode node, const std::string& source) {
    // range_expression: primary_expression "..." primary_expression
    auto start = buildExpression(ts_node_child(node, 0), source);
    auto end = buildExpression(ts_node_child(node, 2), source); // Skip "..."

    return std::make_unique<RangeExpression>(std::move(start), std::move(end));
}

ExpressionPtr ASTBuilder::buildPipeExpression(TSNode node, const std::string& source) {
    // pipe_expression: logical_or_expression "|" substitution_list
    // substitution_list: substitution_pair ("," substitution_pair)*
    // substitution_pair: identifier ":" logical_or_expression

    uint32_t nodeChildCount = ts_node_child_count(node);

    // If there's only 1 child, it's just a logical_or_expression without substitution
    if (nodeChildCount == 1) {
        return buildExpression(ts_node_child(node, 0), source);
    }

    auto expression = buildExpression(ts_node_child(node, 0), source);

    // Child 1 is the "|" operator, child 2 is the substitution_list
    TSNode substitutionListNode = ts_node_child(node, 2);

    std::vector<SubstitutionPair> substitutions;
    uint32_t childCount = ts_node_child_count(substitutionListNode);

    for (uint32_t i = 0; i < childCount; ++i) {
        TSNode child = ts_node_child(substitutionListNode, i);
        const char* childType = ts_node_type(child);

        if (strcmp(childType, "substitution_pair") == 0) {
            // substitution_pair: identifier ":" logical_or_expression
            TSNode identifierNode = ts_node_child(child, 0);
            TSNode valueNode = ts_node_child(child, 2); // Skip ":"

            std::string variable = getNodeText(identifierNode, source);
            auto value = buildExpression(valueNode, source);

            substitutions.push_back(SubstitutionPair(variable, std::move(value)));
        }
    }

    return std::make_unique<PipeExpression>(std::move(expression), std::move(substitutions));
}

ExpressionPtr ASTBuilder::buildPiecewiseExpression(TSNode node, const std::string& source) {
    // piecewise_expression: 'piecewise' '{' repeat(seq(piecewise_case, ',')) optional(piecewise_case) '}'

    std::vector<std::unique_ptr<PiecewiseCase>> cases;

    uint32_t childCount = ts_node_child_count(node);

    // Skip 'piecewise' token and '{' - start from child 2
    for (uint32_t i = 2; i < childCount - 1; ++i) { // -1 to skip closing '}'
        TSNode child = ts_node_child(node, i);
        const char* childType = ts_node_type(child);

        if (strcmp(childType, "piecewise_case") == 0) {
            // piecewise_case: '(' expression ',' (expression | 'otherwise') ')'
            TSNode valueNode = ts_node_child(child, 1);   // Skip '('
            TSNode condNode = ts_node_child(child, 3);    // Skip expression, ','

            auto valueExpr = buildExpression(valueNode, source);

            ExpressionPtr condExpr = nullptr;
            const char* condType = ts_node_type(condNode);
            if (strcmp(condType, "otherwise") != 0) {
                condExpr = buildExpression(condNode, source);
            }
            // If it's "otherwise", condExpr remains nullptr

            cases.push_back(std::make_unique<PiecewiseCase>(std::move(valueExpr), std::move(condExpr)));
        }
        // Skip commas and other punctuation
    }

    return std::make_unique<PiecewiseExpression>(std::move(cases));
}

ExpressionPtr ASTBuilder::buildArrayExpression(TSNode node, const std::string& source) {
    // array_expression: '[' optional(array_elements) ']'
    std::vector<ExpressionPtr> elements;
    bool isColumnVector = false;

    uint32_t childCount = ts_node_child_count(node);
    if (childCount < 2) {
        // Empty array: []
        return std::make_unique<ArrayExpression>(std::move(elements), false);
    }

    // Look for array_elements child
    for (uint32_t i = 1; i < childCount - 1; ++i) { // Skip '[' and ']'
        TSNode child = ts_node_child(node, i);
        const char* childType = ts_node_type(child);

        if (strcmp(childType, "array_elements") == 0) {
            // Check if it's row_vector_elements or column_vector_elements
            TSNode elementsChild = ts_node_child(child, 0);
            const char* elementsType = ts_node_type(elementsChild);

            if (strcmp(elementsType, "column_vector_elements") == 0) {
                isColumnVector = true;
                // Parse column vector elements separated by ';'
                uint32_t elemCount = ts_node_child_count(elementsChild);
                for (uint32_t j = 0; j < elemCount; j++) {
                    TSNode elemNode = ts_node_child(elementsChild, j);
                    const char* nodeType = ts_node_type(elemNode);
                    // Skip separators (';')
                    if (strcmp(nodeType, ";") != 0) {
                        auto expr = buildExpression(elemNode, source);
                        if (expr) {
                            elements.push_back(std::move(expr));
                        }
                    }
                }
            } else if (strcmp(elementsType, "matrix_elements") == 0) {
                // Parse matrix elements - multiple rows separated by ';'
                std::vector<std::vector<ExpressionPtr>> matrixRows;
                uint32_t elemCount = ts_node_child_count(elementsChild);

                for (uint32_t j = 0; j < elemCount; j++) {
                    TSNode elemNode = ts_node_child(elementsChild, j);
                    const char* nodeType = ts_node_type(elemNode);

                    if (strcmp(nodeType, "matrix_row") == 0) {
                        std::vector<ExpressionPtr> rowElements;
                        uint32_t rowCount = ts_node_child_count(elemNode);

                        for (uint32_t k = 0; k < rowCount; k++) {
                            TSNode rowElemNode = ts_node_child(elemNode, k);
                            const char* rowNodeType = ts_node_type(rowElemNode);
                            // Skip separators (',')
                            if (strcmp(rowNodeType, ",") != 0) {
                                auto expr = buildExpression(rowElemNode, source);
                                if (expr) {
                                    rowElements.push_back(std::move(expr));
                                }
                            }
                        }
                        matrixRows.push_back(std::move(rowElements));
                    }
                }
                return std::make_unique<ArrayExpression>(std::move(matrixRows));
            } else if (strcmp(elementsType, "row_vector_elements") == 0) {
                isColumnVector = false;
                // Parse row vector elements separated by ','
                uint32_t elemCount = ts_node_child_count(elementsChild);
                for (uint32_t j = 0; j < elemCount; j++) {
                    TSNode elemNode = ts_node_child(elementsChild, j);
                    const char* nodeType = ts_node_type(elemNode);
                    // Skip separators (',')
                    if (strcmp(nodeType, ",") != 0) {
                        auto expr = buildExpression(elemNode, source);
                        if (expr) {
                            elements.push_back(std::move(expr));
                        }
                    }
                }
            }
            break;
        }
    }

    return std::make_unique<ArrayExpression>(std::move(elements), isColumnVector);
}

ExpressionPtr ASTBuilder::buildArrayAccess(TSNode node, const std::string& source) {
    // array_access: identifier '[' expression ']'
    TSNode identifierNode = ts_node_child(node, 0);
    TSNode indexNode = ts_node_child(node, 2); // Skip identifier and '['

    std::string arrayName = getNodeText(identifierNode, source);
    auto indexExpr = buildExpression(indexNode, source);

    if (!indexExpr) {
        throw std::runtime_error("Failed to build index expression for array access");
    }

    return std::make_unique<ArrayAccess>(arrayName, std::move(indexExpr));
}

} // namespace madola
