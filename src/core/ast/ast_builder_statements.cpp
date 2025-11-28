#include "ast_builder.h"
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <cctype>
#include <optional>

namespace madola {

ASTBuilder::ASTBuilder() {
    parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_madola());
}

std::string ASTBuilder::getNodeText(TSNode node, const std::string& source) {
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);

    // Clamp to source bounds to avoid invalid substr ranges
    if (start > end) {
        std::swap(start, end);
    }
    if (end > source.size()) {
        end = static_cast<uint32_t>(source.size());
    }
    if (start > source.size()) {
        start = static_cast<uint32_t>(source.size());
    }

    return source.substr(start, end - start);
}

SourceLocation ASTBuilder::getNodeStartPosition(TSNode node) const {
    TSPoint start_point = ts_node_start_point(node);
    uint32_t start_byte = ts_node_start_byte(node);
    return tsPointToSourceLocation(start_point, start_byte);
}

SourceLocation ASTBuilder::getNodeEndPosition(TSNode node) const {
    TSPoint end_point = ts_node_end_point(node);
    uint32_t end_byte = ts_node_end_byte(node);
    return tsPointToSourceLocation(end_point, end_byte);
}

ProgramPtr ASTBuilder::buildProgram(const std::string& source) {
    auto program = std::make_unique<Program>();

    TSTree* tree = ts_parser_parse_string(parser, nullptr, source.c_str(), source.length());
    if (!tree) {
        throw std::runtime_error("Failed to parse source with Tree-sitter");
    }

    TSNode root_node = ts_tree_root_node(tree);
    // const char* root_type = ts_node_type(root_node);  // Unused for now

    // Check for parse errors
    if (ts_node_has_error(root_node)) {
        std::string error_msg = "Parse error detected in source";
        ts_tree_delete(tree);
        throw std::runtime_error(error_msg);
    }

    uint32_t child_count = ts_node_child_count(root_node);
    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_child(root_node, i);
        // const char* child_type = ts_node_type(child);  // Unused for now
        // For debugging: print the node type we're trying to parse
        std::string debug_text = getNodeText(child, source);

        auto stmt = buildStatement(child, source);
        if (stmt) {
            program->addStatement(std::move(stmt));
        }
    }

    ts_tree_delete(tree);
    return program;
}

StatementPtr ASTBuilder::buildStatement(TSNode node, const std::string& source) {
    const char* node_type = ts_node_type(node);

    if (strcmp(node_type, "assignment_statement") == 0) {
        return buildAssignmentStatement(node, source);
    } else if (strcmp(node_type, "print_statement") == 0) {
        return buildPrintStatement(node, source);
    } else if (strcmp(node_type, "expression_statement") == 0) {
        return buildExpressionStatement(node, source);
    } else if (strcmp(node_type, "comment_statement") == 0) {
        return buildCommentStatement(node, source);
    } else if (strcmp(node_type, "single_line_comment") == 0) {
        return buildCommentStatement(node, source);
    } else if (strcmp(node_type, "multi_line_comment") == 0) {
        return buildCommentStatement(node, source);
    } else if (strcmp(node_type, "decorated_function_declaration") == 0) {
        return buildDecoratedFunctionDeclaration(node, source);
    } else if (strcmp(node_type, "function_declaration") == 0) {
        return buildFunctionDeclaration(node, source);
    } else if (strcmp(node_type, "piecewise_function_declaration") == 0) {
        return buildPiecewiseFunctionDeclaration(node, source);
    } else if (strcmp(node_type, "return_statement") == 0) {
        return buildReturnStatement(node, source);
    } else if (strcmp(node_type, "break_statement") == 0) {
        return buildBreakStatement(node, source);
    } else if (strcmp(node_type, "for_statement") == 0) {
        return buildForStatement(node, source);
    } else if (strcmp(node_type, "while_statement") == 0) {
        return buildWhileStatement(node, source);
    } else if (strcmp(node_type, "if_statement") == 0) {
        return buildIfStatement(node, source);
    } else if (strcmp(node_type, "decorated_statement") == 0) {
        return buildDecoratedStatement(node, source);
    } else if (strcmp(node_type, "skip_statement") == 0) {
        return buildSkipStatement(node, source);
    } else if (strcmp(node_type, "import_statement") == 0) {
        return buildImportStatement(node, source);
    } else if (strcmp(node_type, "heading_statement") == 0) {
        return buildHeadingStatement(node, source);
    } else if (strcmp(node_type, "version_statement") == 0) {
        return buildVersionStatement(node, source);
    } else if (strcmp(node_type, "paragraph_statement") == 0) {
        return buildParagraphStatement(node, source);
    } else if (strcmp(node_type, "statement") == 0) {
        // If we get a generic "statement" node, look at its first child
        if (ts_node_child_count(node) > 0) {
            TSNode child = ts_node_child(node, 0);
            return buildStatement(child, source);
        }
    }

    // If we get here, we have an unrecognized node type
    return nullptr;
}

StatementPtr ASTBuilder::buildAssignmentStatement(TSNode node, const std::string& source) {
    // assignment_statement: (identifier | array_access) ":=" optional("|-" before_comment) expression optional("-|" after_comment) ";"
    TSNode target_node = ts_node_child(node, 0);
    const char* targetType = ts_node_type(target_node);

    // Find expression node and inline comments by scanning all children
    std::string inlineComment = "";
    bool commentBefore = false;
    TSNode expression_node;
    bool foundExpression = false;

    uint32_t childCount = ts_node_child_count(node);

    for (uint32_t i = 0; i < childCount; i++) {
        TSNode child = ts_node_child(node, i);
        const char* childType = ts_node_type(child);

        if (strcmp(childType, "before_comment") == 0) {
            // |- in source = comment displays before in output
            inlineComment = getNodeText(child, source);
            commentBefore = true;
        } else if (strcmp(childType, "after_comment") == 0) {
            // -| in source = comment displays after in output
            inlineComment = getNodeText(child, source);
            commentBefore = false;
        } else if (strcmp(childType, "expression") == 0 ||
                   strcmp(childType, "comparison_expression") == 0 ||
                   strcmp(childType, "additive_expression") == 0 ||
                   strcmp(childType, "multiplicative_expression") == 0 ||
                   strcmp(childType, "power_expression") == 0 ||
                   strcmp(childType, "primary_expression") == 0) {
            expression_node = child;
            foundExpression = true;
        }
    }

    if (!foundExpression) {
        throw std::runtime_error("Failed to find expression in assignment statement");
    }

    auto expr = buildExpression(expression_node, source);
    if (!expr) {
        std::string debugText = getNodeText(expression_node, source);
        const char* nodeType = ts_node_type(expression_node);
        throw std::runtime_error("Failed to build expression for assignment. Node type: " + std::string(nodeType) + ", Text: " + debugText);
    }

    // Check if it's an array access assignment
    if (strcmp(targetType, "array_access") == 0) {
        // array_access: identifier "[" expression optional(";") "]"
        TSNode array_name_node = ts_node_child(target_node, 0);
        TSNode index_node = ts_node_child(target_node, 2);

        std::string arrayName = getNodeText(array_name_node, source);
        auto indexExpr = buildExpression(index_node, source);

        if (!indexExpr) {
            throw std::runtime_error("Failed to build index expression for array assignment");
        }

        // Check for optional semicolon (column vector indicator)
        bool isColumnVector = false;
        uint32_t childCount = ts_node_child_count(target_node);
        for (uint32_t i = 0; i < childCount; ++i) {
            TSNode child = ts_node_child(target_node, i);
            const char* childType = ts_node_type(child);
            if (strcmp(childType, ";") == 0) {
                isColumnVector = true;
                break;
            }
        }

        return std::make_unique<ArrayAssignmentStatement>(arrayName, std::move(indexExpr), std::move(expr), inlineComment, isColumnVector);
    } else {
        // Regular identifier assignment
        std::string varName = getNodeText(target_node, source);
        return std::make_unique<AssignmentStatement>(varName, std::move(expr), inlineComment, commentBefore);
    }
}

StatementPtr ASTBuilder::buildPrintStatement(TSNode node, const std::string& source) {
    // print_statement: "print" "(" expression ")" ";"
    TSNode expression_node = ts_node_child(node, 2);
    auto expr = buildExpression(expression_node, source);

    return std::make_unique<PrintStatement>(std::move(expr));
}

StatementPtr ASTBuilder::buildExpressionStatement(TSNode node, const std::string& source) {
    // expression_statement: expression ";"
    TSNode expression_node = ts_node_child(node, 0);
    auto expr = buildExpression(expression_node, source);

    return std::make_unique<ExpressionStatement>(std::move(expr));
}

StatementPtr ASTBuilder::buildCommentStatement(TSNode node, const std::string& source) {
    // comment_statement: single_line_comment | multi_line_comment
    // For a token node, we get the text directly from the node
    std::string commentText = getNodeText(node, source);

    // Handle single-line comments
    if (commentText.length() >= 2 && commentText.substr(0, 2) == "//") {
        commentText = commentText.substr(2);
    }
    // Handle multi-line comments
    else if (commentText.length() >= 4 && commentText.substr(0, 2) == "/*" &&
             commentText.substr(commentText.length() - 2) == "*/") {
        commentText = commentText.substr(2, commentText.length() - 4);
    }

    return std::make_unique<CommentStatement>(commentText);
}

StatementPtr ASTBuilder::buildFunctionDeclaration(TSNode node, const std::string& source) {
    // function_declaration: "fn" identifier "(" optional(parameter_list) ")" "{" repeat(statement) "}"
    std::string funcName = getNodeText(ts_node_child(node, 1), source); // Skip "fn"

    std::vector<std::string> parameters;
    std::vector<StatementPtr> body;

    // Parse parameters if they exist
    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; i++) {
        TSNode child = ts_node_child(node, i);
        const char* childType = ts_node_type(child);

        if (strcmp(childType, "parameter_list") == 0) {
            uint32_t paramCount = ts_node_child_count(child);
            for (uint32_t j = 0; j < paramCount; j++) {
                TSNode paramNode = ts_node_child(child, j);
                if (strcmp(ts_node_type(paramNode), "identifier") == 0) {
                    parameters.push_back(getNodeText(paramNode, source));
                }
                // Skip comma tokens
            }
        } else if (strcmp(childType, "statement") == 0) {
            auto stmt = buildStatement(child, source);
            if (stmt) {
                body.push_back(std::move(stmt));
            }
        }
    }

    return std::make_unique<FunctionDeclaration>(funcName, std::move(parameters), std::move(body));
}

StatementPtr ASTBuilder::buildDecoratedFunctionDeclaration(TSNode node, const std::string& source) {
    // decorated_function_declaration: repeat1(decorator) function_declaration
    std::vector<std::string> decorators;
    TSNode functionDeclNode;

    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; i++) {
        TSNode child = ts_node_child(node, i);
        const char* childType = ts_node_type(child);

        if (strcmp(childType, "decorator") == 0) {
            // decorator: '@' identifier
            TSNode identifierNode = ts_node_child(child, 1); // Skip '@'
            std::string decoratorName = getNodeText(identifierNode, source);
            decorators.push_back(decoratorName);
        } else if (strcmp(childType, "function_declaration") == 0) {
            functionDeclNode = child;
        }
    }

    // Parse the function declaration part
    std::string funcName = getNodeText(ts_node_child(functionDeclNode, 1), source); // Skip "fn"

    std::vector<std::string> parameters;
    std::vector<StatementPtr> body;

    // Parse parameters and body from function declaration
    uint32_t funcChildCount = ts_node_child_count(functionDeclNode);
    for (uint32_t i = 0; i < funcChildCount; i++) {
        TSNode child = ts_node_child(functionDeclNode, i);
        const char* childType = ts_node_type(child);

        if (strcmp(childType, "parameter_list") == 0) {
            uint32_t paramCount = ts_node_child_count(child);
            for (uint32_t j = 0; j < paramCount; j++) {
                TSNode paramNode = ts_node_child(child, j);
                if (strcmp(ts_node_type(paramNode), "identifier") == 0) {
                    parameters.push_back(getNodeText(paramNode, source));
                }
            }
        } else if (strcmp(childType, "statement") == 0) {
            auto stmt = buildStatement(child, source);
            if (stmt) {
                body.push_back(std::move(stmt));
            }
        }
    }

    return std::make_unique<FunctionDeclaration>(funcName, std::move(parameters), std::move(body), std::move(decorators));
}

StatementPtr ASTBuilder::buildReturnStatement(TSNode node, const std::string& source) {
    // return_statement: "return" expression ";"
    TSNode expression_node = ts_node_child(node, 1); // Skip "return"
    auto expr = buildExpression(expression_node, source);
    return std::make_unique<ReturnStatement>(std::move(expr));
}

StatementPtr ASTBuilder::buildBreakStatement(TSNode /* node */, const std::string& /* source */) {
    // break_statement: "break" ";"
    return std::make_unique<BreakStatement>();
}

StatementPtr ASTBuilder::buildForStatement(TSNode node, const std::string& source) {
    // for_statement: "for" identifier "in" range_expression "{" repeat(statement) "}"
    std::string varName = getNodeText(ts_node_child(node, 1), source); // Skip "for"
    auto rangeExpr = buildExpression(ts_node_child(node, 3), source); // Skip "for", identifier, "in"

    std::vector<StatementPtr> body;

    // Parse body statements
    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 5; i < childCount - 1; i++) { // Skip "for", identifier, "in", range, "{", and final "}"
        TSNode child = ts_node_child(node, i);
        const char* childType = ts_node_type(child);

        if (strcmp(childType, "statement") == 0) {
            auto stmt = buildStatement(child, source);
            if (stmt) {
                body.push_back(std::move(stmt));
            }
        }
    }

    return std::make_unique<ForStatement>(varName, std::move(rangeExpr), std::move(body));
}

StatementPtr ASTBuilder::buildWhileStatement(TSNode node, const std::string& source) {
    // while_statement: "while" "(" expression ")" (block | statement)
    // Get condition expression (child 2: skip "while" and "(")
    auto condition = buildExpression(ts_node_child(node, 2), source);

    std::vector<StatementPtr> body;
    uint32_t childCount = ts_node_child_count(node);

    // Check if it's a block or single statement
    TSNode bodyNode = ts_node_child(node, 4); // After "while", "(", expression, ")"
    const char* bodyType = ts_node_type(bodyNode);

    if (strcmp(bodyType, "{") == 0) {
        // Block form: { statements }
        // Parse all statements between { and }
        for (uint32_t i = 5; i < childCount - 1; i++) {
            TSNode child = ts_node_child(node, i);
            const char* childType = ts_node_type(child);

            if (strcmp(childType, "statement") == 0) {
                auto stmt = buildStatement(child, source);
                if (stmt) {
                    body.push_back(std::move(stmt));
                }
            }
        }
    } else {
        // Single statement form
        auto stmt = buildStatement(bodyNode, source);
        if (stmt) {
            body.push_back(std::move(stmt));
        }
    }

    return std::make_unique<WhileStatement>(std::move(condition), std::move(body));
}

StatementPtr ASTBuilder::buildImportStatement(TSNode node, const std::string& source) {
    uint32_t childCount = ts_node_child_count(node);

    // Check if it's "import identifier;" (simple form)
    if (childCount == 3) {
        // import_statement: "import" identifier ";"
        std::string functionName = getNodeText(ts_node_child(node, 1), source); // Skip "import"

        // For simple imports, assume the module name is the same as the function name
        std::vector<std::unique_ptr<ImportItem>> items;
        items.push_back(std::make_unique<ImportItem>(functionName));
        return std::make_unique<ImportStatement>(functionName, std::move(items));
    }
    // Otherwise it's "from module import import_item_list;" (complex form)
    else {
        // import_statement: "from" identifier "import" import_item_list ";"
        std::string moduleName = getNodeText(ts_node_child(node, 1), source); // Skip "from"
        TSNode importItemListNode = ts_node_child(node, 3); // Skip "from", identifier, "import"

        std::vector<std::unique_ptr<ImportItem>> items;

        // Parse import_item_list: import_item repeat(seq(',', import_item))
        uint32_t listChildCount = ts_node_child_count(importItemListNode);
        for (uint32_t i = 0; i < listChildCount; i++) {
            TSNode child = ts_node_child(importItemListNode, i);
            const char* childType = ts_node_type(child);

            if (strcmp(childType, "import_item") == 0) {
                // Parse import_item: identifier optional(seq('as', identifier))
                uint32_t itemChildCount = ts_node_child_count(child);

                std::string originalName = getNodeText(ts_node_child(child, 0), source);
                std::string aliasName = "";

                // Check for alias
                if (itemChildCount >= 3) { // identifier, 'as', identifier
                    // The 'as' token is at index 1, alias identifier at index 2
                    aliasName = getNodeText(ts_node_child(child, 2), source);
                }

                items.push_back(std::make_unique<ImportItem>(originalName, aliasName));
            }
            // Skip comma nodes
        }

        return std::make_unique<ImportStatement>(moduleName, std::move(items));
    }
}

StatementPtr ASTBuilder::buildIfStatement(TSNode node, const std::string& source) {
    // New grammar: if_statement: "if" "(" expression ")" choice(block|statement) optional(seq("else" choice(block|statement)))
    TSNode conditionNode = ts_node_child(node, 2); // Skip "if" and "("
    auto condition = buildExpression(conditionNode, source);

    std::vector<StatementPtr> thenBody;
    std::vector<StatementPtr> elseBody;

    // The then body is at index 4 (after "if", "(", expression, ")")
    TSNode thenBodyNode = ts_node_child(node, 4);
    const char* thenType = ts_node_type(thenBodyNode);

    if (strcmp(thenType, "{") == 0) {
        // Block form: find the matching closing brace and parse statements between
        uint32_t childCount = ts_node_child_count(node);
        for (uint32_t i = 5; i < childCount; i++) {
            TSNode child = ts_node_child(node, i);
            const char* childType = ts_node_type(child);

            if (strcmp(childType, "}") == 0) {
                break; // End of then block
            } else if (strcmp(childType, "statement") == 0) {
                auto stmt = buildStatement(child, source);
                if (stmt) {
                    thenBody.push_back(std::move(stmt));
                }
            }
        }
    } else {
        // Single statement form
        auto stmt = buildStatement(thenBodyNode, source);
        if (stmt) {
            thenBody.push_back(std::move(stmt));
        }
    }

    // Check for else clause
    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 5; i < childCount; i++) {
        TSNode child = ts_node_child(node, i);
        const char* childType = ts_node_type(child);

        if (strcmp(childType, "else") == 0) {
            // Found else, next node is the else body
            if (i + 1 < childCount) {
                TSNode elseBodyNode = ts_node_child(node, i + 1);
                const char* elseType = ts_node_type(elseBodyNode);

                if (strcmp(elseType, "{") == 0) {
                    // Block form: parse statements until closing brace
                    for (uint32_t j = i + 2; j < childCount; j++) {
                        TSNode elseChild = ts_node_child(node, j);
                        const char* elseChildType = ts_node_type(elseChild);

                        if (strcmp(elseChildType, "}") == 0) {
                            break; // End of else block
                        } else if (strcmp(elseChildType, "statement") == 0) {
                            auto stmt = buildStatement(elseChild, source);
                            if (stmt) {
                                elseBody.push_back(std::move(stmt));
                            }
                        }
                    }
                } else {
                    // Single statement form
                    auto stmt = buildStatement(elseBodyNode, source);
                    if (stmt) {
                        elseBody.push_back(std::move(stmt));
                    }
                }
            }
            break;
        }
    }

    return std::make_unique<IfStatement>(std::move(condition), std::move(thenBody), std::move(elseBody));
}

StatementPtr ASTBuilder::buildDecoratedStatement(TSNode node, const std::string& source) {
    // decorated_statement: repeat1(decorator) (assignment_statement | expression_statement | print_statement | comment_statement)
    std::vector<Decorator> decorators;
    TSNode statementNode;

    uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; i++) {
        TSNode child = ts_node_child(node, i);
        const char* childType = ts_node_type(child);

        if (strcmp(childType, "decorator") == 0) {
            uint32_t decoratorChildCount = ts_node_child_count(child);

            auto getIdentifierText = [&](uint32_t nodeIndex) -> std::string {
                if (decoratorChildCount > nodeIndex) {
                    TSNode identNode = ts_node_child(child, nodeIndex);
                    return getNodeText(identNode, source);
                }
                return "";
            };

            auto isLayoutIdentifier = [](const std::string& text) {
                return text == "layout" || text == "array"; // Keep legacy "array" keyword for backward compatibility
            };

            auto tryParseLayoutToken = [](const std::string& tokenText) -> std::optional<std::pair<int, int>> {
                size_t prefixLen = std::string::npos;
                if (tokenText.rfind("layout", 0) == 0) {
                    prefixLen = 6;
                } else if (tokenText.rfind("array", 0) == 0) {
                    prefixLen = 5;
                } else {
                    return std::nullopt;
                }

                size_t xPos = tokenText.find('x', prefixLen);
                if (xPos != std::string::npos && xPos > prefixLen) {
                    std::string rowsStr = tokenText.substr(prefixLen, xPos - prefixLen);
                    std::string colsStr = tokenText.substr(xPos + 1);
                    bool validRows = !rowsStr.empty() && std::all_of(rowsStr.begin(), rowsStr.end(), ::isdigit);
                    bool validCols = !colsStr.empty() && std::all_of(colsStr.begin(), colsStr.end(), ::isdigit);
                    if (validRows && validCols) {
                        return std::make_pair(std::stoi(rowsStr), std::stoi(colsStr));
                    }
                }
                return std::nullopt;
            };

            if (decoratorChildCount >= 5) {
                // Spaced format: @layout 2 x 2 (still accept legacy "array" keyword)
                std::string identifierText = getIdentifierText(1);
                if (isLayoutIdentifier(identifierText)) {
                    TSNode rowsNode = ts_node_child(child, 2);
                    TSNode colsNode = ts_node_child(child, 4);

                    std::string rowsText = getNodeText(rowsNode, source);
                    std::string colsText = getNodeText(colsNode, source);

                    int rows = std::stoi(rowsText);
                    int cols = std::stoi(colsText);

                    decorators.emplace_back(rows, cols);
                    continue;
                }
                // If identifier is not layout/array, fall through to simple handling below
            }

            if (decoratorChildCount >= 2) {
                // @identifier or @identifier[style] or compact @layout2x2 format
                TSNode tokenNode = ts_node_child(child, 1); // Skip '@'
                std::string tokenText = getNodeText(tokenNode, source);

                // Check if there's a decorator_style node (for @merge2[center])
                std::string decoratorStyle = "";
                for (uint32_t childIdx = 2; childIdx < decoratorChildCount; ++childIdx) {
                    TSNode styleCandidate = ts_node_child(child, childIdx);
                    const char* styleType = ts_node_type(styleCandidate);
                    if (strcmp(styleType, "decorator_style") == 0) {
                        decoratorStyle = getNodeText(styleCandidate, source);
                        break;
                    }
                }

                // Check if it matches layout pattern: layout<digits>x<digits> (accept legacy array)
                if (auto layoutDims = tryParseLayoutToken(tokenText)) {
                    decorators.emplace_back(layoutDims->first, layoutDims->second);
                } else {
                    // Check if it matches parameterized pattern: identifier<digits> (e.g., merge2, col3)
                    // But exclude h1-h4, p which are special heading/paragraph markers
                    bool isSpecialMarker = (tokenText == "h1" || tokenText == "h2" ||
                                           tokenText == "h3" || tokenText == "h4" ||
                                           tokenText == "p");

                    if (!isSpecialMarker) {
                        size_t firstDigitPos = std::string::npos;
                        for (size_t pos = 0; pos < tokenText.length(); ++pos) {
                            if (std::isdigit(static_cast<unsigned char>(tokenText[pos]))) {
                                firstDigitPos = pos;
                                break;
                            }
                        }

                        if (firstDigitPos != std::string::npos && firstDigitPos > 0) {
                            std::string decoratorName = tokenText.substr(0, firstDigitPos);
                            std::string paramStr = tokenText.substr(firstDigitPos);

                            // Verify paramStr is all digits
                            bool validParam = !paramStr.empty() && std::all_of(paramStr.begin(), paramStr.end(), ::isdigit);

                            if (validParam) {
                                int param = std::stoi(paramStr);

                                // Use the decorator style if found
                                if (!decoratorStyle.empty()) {
                                    decorators.emplace_back(decoratorName, param, decoratorStyle);
                                } else {
                                    decorators.emplace_back(decoratorName, param);
                                }
                            } else {
                                // Not a valid parameterized decorator, treat as simple
                                decorators.emplace_back(tokenText);
                            }
                        } else {
                            // Simple decorator (no digits)
                            // But still check if there's a style
                            if (!decoratorStyle.empty()) {
                                decorators.emplace_back(tokenText);
                            } else {
                                decorators.emplace_back(tokenText);
                            }
                        }
                    } else {
                        // Special marker, treat as simple decorator
                        decorators.emplace_back(tokenText);
                    }
                }
            } else {
                // Default: treat as simple decorator
                TSNode identifierNode = ts_node_child(child, 1);
                std::string decoratorName = getNodeText(identifierNode, source);
                decorators.emplace_back(decoratorName);
            }
        } else if (strcmp(childType, "assignment_statement") == 0 ||
                   strcmp(childType, "expression_statement") == 0 ||
                   strcmp(childType, "print_statement") == 0 ||
                   strcmp(childType, "comment_statement") == 0) {
            statementNode = child;
        }
    }

    // Build the underlying statement
    auto stmt = buildStatement(statementNode, source);

    return std::make_unique<DecoratedStatement>(std::move(decorators), std::move(stmt));
}

StatementPtr ASTBuilder::buildSkipStatement(TSNode node, const std::string& source) {
    // skip_statement: '@skip'

    // Check if there's an empty line after @skip (which would make it have no effect)
    uint32_t end = ts_node_end_byte(node);
    bool hasEmptyLineAfter = false;

    // Look at the characters after @skip
    if (end < source.length()) {
        size_t pos = end;

        // Skip any spaces/tabs after @skip
        while (pos < source.length() && (source[pos] == ' ' || source[pos] == '\t')) {
            pos++;
        }

        // If we find a newline, continue to check if there's an empty line
        if (pos < source.length() && (source[pos] == '\n' || source[pos] == '\r')) {
            pos++; // Skip the newline character

            // Handle \r\n case
            if (pos < source.length() && source[pos-1] == '\r' && source[pos] == '\n') {
                pos++;
            }

            // Now check if the next line is empty or contains only whitespace
            size_t lineStart = pos;
            while (pos < source.length() && (source[pos] == ' ' || source[pos] == '\t')) {
                pos++;
            }

            // If we find another newline, that means there was an empty line
            if (pos < source.length() && (source[pos] == '\n' || source[pos] == '\r')) {
                hasEmptyLineAfter = true;
            }
            // If we reached end of file with only whitespace, also consider it empty
            else if (pos >= source.length() && lineStart < source.length()) {
                hasEmptyLineAfter = true;
            }
        }
    }

    // If there's an empty line after @skip, it should not skip the next statement
    return std::make_unique<SkipStatement>(!hasEmptyLineAfter);
}

StatementPtr ASTBuilder::buildPiecewiseFunctionDeclaration(TSNode node, const std::string& source) {
    // piecewise_function_declaration: identifier '(' optional(parameter_list) ')' ':=' piecewise_expression ';'

    // Get function name (first child)
    TSNode nameNode = ts_node_child(node, 0);
    std::string funcName = getNodeText(nameNode, source);

    // Get parameters (third child if present)
    std::vector<std::string> parameters;
    TSNode paramListNode = ts_node_child(node, 2); // Inside parentheses
    const char* paramListType = ts_node_type(paramListNode);

    if (strcmp(paramListType, "parameter_list") == 0) {
        uint32_t paramCount = ts_node_child_count(paramListNode);
        for (uint32_t i = 0; i < paramCount; i += 2) { // Skip commas
            TSNode paramNode = ts_node_child(paramListNode, i);
            std::string paramName = getNodeText(paramNode, source);
            parameters.push_back(paramName);
        }
    }

    // Get piecewise expression (fifth child - after identifier, '(', params, ')', ':=')
    TSNode piecewiseNode = ts_node_child(node, 5);
    auto piecewiseExpr = std::unique_ptr<PiecewiseExpression>(
        dynamic_cast<PiecewiseExpression*>(buildPiecewiseExpression(piecewiseNode, source).release())
    );

    return std::make_unique<PiecewiseFunctionDeclaration>(funcName, std::move(parameters), std::move(piecewiseExpr));
}

StatementPtr ASTBuilder::buildHeadingStatement(TSNode node, const std::string& source) {
    // heading_statement: choice(@h1/@h2/@h3/@h4) optional([heading_style]) repeat1(text_paragraph)
    uint32_t childCount = ts_node_child_count(node);

    // Parse the heading tag (@h1, @h2, etc.)
    TSNode headingTag = ts_node_child(node, 0);
    std::string tagText = getNodeText(headingTag, source);

    int level = 1;
    std::string style = "";

    // Determine heading level from tag
    if (tagText == "@h1") level = 1;
    else if (tagText == "@h2") level = 2;
    else if (tagText == "@h3") level = 3;
    else if (tagText == "@h4") level = 4;

    // Check for optional style attribute [style]
    uint32_t currentIndex = 1;
    if (childCount > currentIndex) {
        TSNode nextChild = ts_node_child(node, currentIndex);
        const char* nextType = ts_node_type(nextChild);

        if (strcmp(nextType, "[") == 0) {
            // Style attribute present: [, heading_style, ]
            currentIndex++;
            if (currentIndex < childCount) {
                TSNode styleNode = ts_node_child(node, currentIndex);
                const char* styleNodeType = ts_node_type(styleNode);
                if (strcmp(styleNodeType, "heading_style") == 0) {
                    style = getNodeText(styleNode, source);
                }
                currentIndex++; // Skip heading_style
            }
            if (currentIndex < childCount) {
                currentIndex++; // Skip ]
            }
        }
    }

    // Collect text content
    std::vector<std::string> paragraphs;
    for (uint32_t i = currentIndex; i < childCount; i++) {
        TSNode child = ts_node_child(node, i);
        const char* childType = ts_node_type(child);

        if (strcmp(childType, "heading_content") == 0) {
            std::string content = getNodeText(child, source);
            paragraphs.push_back(content);
        }
    }

    return std::make_unique<HeadingStatement>(level, style, std::move(paragraphs));
}

StatementPtr ASTBuilder::buildVersionStatement(TSNode node, const std::string& source) {
    // version_statement: '@version' version_number
    uint32_t childCount = ts_node_child_count(node);

    if (childCount < 2) {
        return std::make_unique<VersionStatement>("0.0");
    }

    TSNode versionNode = ts_node_child(node, 1);
    std::string version = getNodeText(versionNode, source);

    return std::make_unique<VersionStatement>(version);
}

StatementPtr ASTBuilder::buildParagraphStatement(TSNode node, const std::string& source) {
    // paragraph_statement: '@p' optional('[' paragraph_style ']') '{' paragraph_content '}'
    uint32_t childCount = ts_node_child_count(node);

    std::string style = "";

    // Collect text content and style (skip '@p', '[', ']', '{', and '}' tokens)
    std::vector<std::string> lines;
    for (uint32_t i = 0; i < childCount; i++) {
        TSNode child = ts_node_child(node, i);
        const char* childType = ts_node_type(child);

        if (strcmp(childType, "paragraph_style") == 0) {
            style = getNodeText(child, source);
        } else if (strcmp(childType, "paragraph_content") == 0) {
            std::string content = getNodeText(child, source);
            lines.push_back(content);
        }
    }

    return std::make_unique<ParagraphStatement>(std::move(lines), style);
}

} // namespace madola
