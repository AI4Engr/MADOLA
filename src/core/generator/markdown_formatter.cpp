#include "markdown_formatter.h"
#include <sstream>
#include <regex>
#include <iostream>

namespace madola {

static int globalGraphCounter = 0;

MarkdownFormatter::MarkdownFormatter() = default;

MarkdownFormatter::FormatResult MarkdownFormatter::formatProgram(const Program& program) {
    FormatResult result;
    result.success = true;

    try {
        std::stringstream ss;

        // Add code block wrapper
        ss << "```madola\n";

        // Format each statement
        for (const auto& stmt : program.statements) {
            ss << formatStatement(*stmt) << "\n";
        }

        ss << "```\n";

        result.markdown = ss.str();
    } catch (const std::exception& e) {
        result.success = false;
        result.error = e.what();
    }

    return result;
}


MarkdownFormatter::FormatResult MarkdownFormatter::formatProgramWithExecution(const Program& program) {
    FormatResult result;
    result.success = true;

    // Reset global graph counter for each formatting operation
    globalGraphCounter = 0;

    try {
        std::stringstream ss;

        // Pre-evaluate the program to build up environment for @resolve decorators
        Evaluator evaluator;
        auto evalResult = evaluator.evaluate(program);

        // Track @skip decorator to skip the next statement
        bool skipNextStatement = false;

        // Format each statement
        for (size_t i = 0; i < program.statements.size(); ++i) {
            const auto& stmt = program.statements[i];


            // Special case: Check for @layout decorator applied to comment
            if (const auto* decoratedStmt = dynamic_cast<const DecoratedStatement*>(stmt.get())) {
                if (decoratedStmt->hasLayoutDecorator() &&
                    dynamic_cast<const CommentStatement*>(decoratedStmt->statement.get())) {

                    // Found @layoutrxc applied to comment - handle specially
                    const Decorator* arrayDecorator = decoratedStmt->getLayoutDecorator();
                    const auto* comment = dynamic_cast<const CommentStatement*>(decoratedStmt->statement.get());

                    int rows = arrayDecorator->rows;
                    int cols = arrayDecorator->cols;
                    std::string commentText = comment->content;

                    // Collect the next rows*cols math expressions starting from i+1
                    std::vector<std::string> cellContents;
                    size_t collected = 0;
                    size_t needed = rows * cols;

                    for (size_t j = i + 1; j < program.statements.size() && collected < needed; ++j) {
                        const auto& mathStmt = program.statements[j];

                        if (dynamic_cast<const AssignmentStatement*>(mathStmt.get()) ||
                            dynamic_cast<const DecoratedStatement*>(mathStmt.get())) {

                            std::string cellContent = formatStatementForArray(*mathStmt, evaluator);

                            // Add comment to the first cell
                            if (collected == 0) {
                                cellContent = commentText + " \\quad " + cellContent;
                            }

                            cellContents.push_back(cellContent);
                            collected++;
                        }
                    }

                    if (!cellContents.empty()) {
                        // Generate the array
                        ss << "$$\n";
                        ss << "\\begin{array}{" << std::string(cols, 'c') << "}\n";

                        for (int row = 0; row < rows; ++row) {
                            for (int col = 0; col < cols; ++col) {
                                size_t idx = row * cols + col;
                                if (idx < cellContents.size()) {
                                    ss << cellContents[idx];
                                }

                                if (col < cols - 1) {
                                    ss << " & \\quad \\quad ";
                                }
                            }

                            if (row < rows - 1) {
                                ss << " \\\\\n";
                            } else {
                                ss << " \\\\\n";
                            }
                        }

                        ss << "\\end{array}\n";
                        ss << "$$\n\n";
                    }

                    // Skip the processed statements
                    i = i + collected; // Skip the decorated comment and all math statements
                    continue;
                }
            }

            // Skip this statement if the previous one was @skip
            if (skipNextStatement) {
                skipNextStatement = false;
                continue;
            }

            if (const auto* functionDecl = dynamic_cast<const FunctionDeclaration*>(stmt.get())) {
            // Format function declaration in LaTeX
            ss << "$$\n";
            ss << "\\begin{array}{l}\n";
            ss << "\\textbf{function } " << functionDecl->name << "(";
            for (size_t i = 0; i < functionDecl->parameters.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << functionDecl->parameters[i];
            }
            ss << ") \\\\\n";
            ss << "\\left|\n";
            ss << "\\begin{array}{l}\n";

            for (const auto& bodyStmt : functionDecl->body) {
                std::string formatted = formatStatementWithDepth(*bodyStmt, 0, true); // In function context
                if (dynamic_cast<const AssignmentStatement*>(bodyStmt.get())) {
                    ss << "\\quad " << formatted << " \\\\\n";
                } else if (dynamic_cast<const ForStatement*>(bodyStmt.get())) {
                    ss << "\\quad " << formatted << "\\\\\n";
                } else {
                    ss << "\\quad " << formatted << "\n";
                }
            }

            ss << "\\end{array}\n";
            ss << "\\right.\n";
            ss << "\\end{array}\n";
            ss << "$$\n\n";

        } else if (const auto* piecewiseFuncDecl = dynamic_cast<const PiecewiseFunctionDeclaration*>(stmt.get())) {
            // Format piecewise function declaration in LaTeX
            ss << "$$\n";
            ss << piecewiseFuncDecl->name << "(";
            for (size_t i = 0; i < piecewiseFuncDecl->parameters.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << piecewiseFuncDecl->parameters[i];
            }
            ss << ") = ";
            ss << formatPiecewiseExpression(*piecewiseFuncDecl->piecewise);
            ss << "\n$$\n\n";

        } else if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(stmt.get())) {
            ss << "$$\n";
            if (!assignment->inlineComment.empty()) {
                ss << "\\text{" << assignment->inlineComment << "} \\quad ";
            }
            ss << formatVariableName(assignment->variable) << " = " << formatExpression(*assignment->expression);
            ss << "\n$$\n\n";

        } else if (dynamic_cast<const PrintStatement*>(stmt.get())) {
            // Skip formatting print statements in the LaTeX section
            // The output will be shown separately below

        } else if (const auto* exprStmt = dynamic_cast<const ExpressionStatement*>(stmt.get())) {
            // Handle expression statements (like graph() calls)
            if (const auto* functionCall = dynamic_cast<const FunctionCall*>(exprStmt->expression.get())) {
                if (functionCall->function_name == "graph") {
                    // Insert graph placeholder using global counter
                    ss << "<!-- GRAPH_PLACEHOLDER_" << globalGraphCounter++ << " -->\n\n";
                }
            }

        } else if (dynamic_cast<const CommentStatement*>(stmt.get())) {
            // Skip comments - don't show in HTML report
        } else if (const auto* headingStmt = dynamic_cast<const HeadingStatement*>(stmt.get())) {
            // Format heading statement as HTML
            std::string tag = "h" + std::to_string(headingStmt->level);

            // Build style attribute if present
            std::string styleAttr = "";
            if (!headingStmt->style.empty()) {
                if (headingStmt->style == "center") {
                    styleAttr = " style=\"text-align:center\"";
                } else {
                    // Generic style pass-through
                    styleAttr = " style=\"" + headingStmt->style + "\"";
                }
            }

            // Output each paragraph as a separate heading (or with <br> for continuation lines)
            if (!headingStmt->textParagraphs.empty()) {
                // First paragraph gets the full heading with style
                ss << "<" << tag << styleAttr << ">";
                ss << headingStmt->textParagraphs[0];
                ss << "</" << tag << ">\n\n";

                // Additional paragraphs as separate headings without style (continuation)
                for (size_t p = 1; p < headingStmt->textParagraphs.size(); ++p) {
                    ss << "<" << tag << ">";
                    ss << headingStmt->textParagraphs[p];
                    ss << "</" << tag << "><br>\n";
                }
            }
        } else if (const auto* skipStmt = dynamic_cast<const SkipStatement*>(stmt.get())) {
            // Handle @skip statement - only set flag to skip if there's no line break after
            if (skipStmt->shouldSkipNext) {
                skipNextStatement = true;
            }
            // If shouldSkipNext is false (line break after @skip), do nothing
        } else if (dynamic_cast<const VersionStatement*>(stmt.get())) {
            // Skip version statements - don't show in markdown/HTML report
        } else if (const auto* paragraphStmt = dynamic_cast<const ParagraphStatement*>(stmt.get())) {
            // Format paragraph statement as HTML with pre-wrap
            std::string styleAttr = " style=\"white-space: pre-wrap";
            if (!paragraphStmt->style.empty()) {
                if (paragraphStmt->style == "center") {
                    styleAttr += "; text-align:center";
                } else if (paragraphStmt->style == "left") {
                    styleAttr += "; text-align:left";
                } else if (paragraphStmt->style == "right") {
                    styleAttr += "; text-align:right";
                } else {
                    styleAttr += "; " + paragraphStmt->style;
                }
            }
            styleAttr += "\"";

            for (const auto& line : paragraphStmt->textLines) {
                // Convert tabs to \t for display
                std::string processedLine = line;
                size_t pos = 0;
                while ((pos = processedLine.find('\t', pos)) != std::string::npos) {
                    processedLine.replace(pos, 1, "\\t");
                    pos += 2;
                }
                ss << "<p" << styleAttr << ">" << processedLine << "</p>\n";
            }
            ss << "\n";
        } else if (const auto* forStmt = dynamic_cast<const ForStatement*>(stmt.get())) {
            // Format for loop in LaTeX
            ss << "$$\n";
            ss << "\\begin{array}{l}\n";
            ss << "\\textbf{for } " << formatVariableName(forStmt->variable) << " \\textbf{ in } ";

            // Format the range properly
            if (const auto* rangeExpr = dynamic_cast<const RangeExpression*>(forStmt->range.get())) {
                if (rangeExpr->start && rangeExpr->end) {
                    ss << formatExpression(*rangeExpr->start) << "..." << formatExpression(*rangeExpr->end);
                } else {
                    ss << "INVALID_RANGE";
                }
            } else if (forStmt->range) {
                ss << formatExpression(*forStmt->range);
            } else {
                ss << "NULL_RANGE";
            }

            ss << " \\\\\n";
            ss << "\\left|\n";
            ss << "\\begin{array}{l}\n";

            for (size_t j = 0; j < forStmt->body.size(); ++j) {
                ss << formatStatementWithDepth(*forStmt->body[j], 1, false);
                if (j < forStmt->body.size() - 1) {
                    ss << " \\\\\n";
                } else {
                    ss << "\n";
                }
            }

            ss << "\\end{array}\n";
            ss << "\\right.\n";
            ss << "\\end{array}\n";
            ss << "$$\n\n";
        } else if (const auto* ifStmt = dynamic_cast<const IfStatement*>(stmt.get())) {
            // Format if statement in LaTeX
            ss << "$$\n";
            ss << "\\begin{array}{l}\n";
            ss << "\\textbf{if } " << formatExpression(*ifStmt->condition) << " \\textbf{ then} \\\\\n";
            ss << "\\left|\n";
            ss << "\\begin{array}{l}\n";

            for (const auto& bodyStmt : ifStmt->then_body) {
                ss << formatStatementWithDepth(*bodyStmt, 1, false) << "\n";
            }

            ss << "\\end{array}\n";
            ss << "\\right.\n";

            // Handle else clause if present
            if (!ifStmt->else_body.empty()) {
                ss << "\\\\\n";
                ss << "\\textbf{else} \\\\\n";
                ss << "\\left|\n";
                ss << "\\begin{array}{l}\n";

                for (const auto& bodyStmt : ifStmt->else_body) {
                    ss << formatStatementWithDepth(*bodyStmt, 1, false) << "\n";
                }

                ss << "\\end{array}\n";
                ss << "\\right.\n";
            }

            ss << "\\end{array}\n";
            ss << "$$\n\n";
        } else if (const auto* decoratedStmt = dynamic_cast<const DecoratedStatement*>(stmt.get())) {
            // Handle @layoutrxc decorator - collect next r*c expressions and format as array
            if (decoratedStmt->hasLayoutDecorator()) {
                const Decorator* arrayDecorator = decoratedStmt->getLayoutDecorator();
                if (!arrayDecorator) {
                    // Fallback: treat as regular decorated statement
                    continue;
                }

                int rows = arrayDecorator->rows;
                int cols = arrayDecorator->cols;

                // Validate array dimensions
                if (rows <= 0 || cols <= 0 || rows > 10 || cols > 10) {
                    // Invalid dimensions, treat as regular statement
                    continue;
                }

                std::vector<std::string> cellContents;
                size_t needed = rows * cols;
                size_t collected = 0;
                size_t nextIndex = i + 1;

                // Check if the decorated statement is a comment
                if (const auto* comment = dynamic_cast<const CommentStatement*>(decoratedStmt->statement.get())) {
                    // Array decorator applied to comment: combine comment with first math expression
                    std::string commentText = comment->content;

                    // According to the specification, skip the first expression and combine comment with the second expression
                    size_t expressionCount = 0;
                    for (size_t j = nextIndex; j < program.statements.size() && expressionCount < 2; ++j) {
                        const auto& stmt = program.statements[j];
                        if (dynamic_cast<const AssignmentStatement*>(stmt.get()) ||
                            dynamic_cast<const DecoratedStatement*>(stmt.get())) {
                            expressionCount++;
                            if (expressionCount == 2) {
                                // This is the second expression - combine it with the comment
                                std::string mathContent = formatStatementForArray(*stmt, evaluator);
                                cellContents.push_back(commentText + " \\quad " + mathContent);
                                collected++;
                                nextIndex = j + 1; // Continue from after this expression
                                break;
                            }
                        }
                    }
                } else {
                    // Array decorator applied to math expression: use it as first cell
                    cellContents.push_back(formatDecoratedStatementContent(*decoratedStmt, evaluator));
                    collected++;
                }

                // Collect remaining math expressions
                for (size_t j = nextIndex; j < program.statements.size() && collected < needed; ++j) {
                    const auto& stmt = program.statements[j];

                    // Only collect assignment statements and decorated statements
                    if (dynamic_cast<const AssignmentStatement*>(stmt.get()) ||
                        dynamic_cast<const DecoratedStatement*>(stmt.get())) {
                        cellContents.push_back(formatStatementForArray(*stmt, evaluator));
                        collected++;
                    }
                }

                // Generate array if we have content
                if (!cellContents.empty()) {
                    // Start the LaTeX array environment
                    ss << "$$\n";
                    ss << "\\begin{array}{" << std::string(cols, 'c') << "}\n";

                    // Format the array
                    for (int row = 0; row < rows; ++row) {
                        for (int col = 0; col < cols; ++col) {
                            size_t idx = row * cols + col;
                            if (idx < cellContents.size()) {
                                ss << cellContents[idx];
                            }

                            if (col < cols - 1) {
                                ss << " & \\quad \\quad ";
                            }
                        }

                        if (row < rows - 1) {
                            ss << " \\\\\n";
                        } else {
                            ss << " \\\\\n";
                        }
                    }

                    ss << "\\end{array}\n";
                    ss << "$$\n\n";
                }

                // Skip processed statements in the main loop
                i = nextIndex + (collected - 1) - 1; // -1 because the main loop will increment
            }
            // Handle decorated comments (for non-layout decorators, skip the comment)
            else if (dynamic_cast<const CommentStatement*>(decoratedStmt->statement.get())) {
                // Skip comments - don't show in HTML report
            }
            // Handle @resolve decorator - variable =expression = substituted = result
            else if (decoratedStmt->hasDecorator("resolve")) {
                if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(decoratedStmt->statement.get())) {
                    ss << "$$\n";
                    ss << formatVariableName(assignment->variable) << " = " << formatExpression(*assignment->expression);

                    // Add = and show variable values substituted
                    ss << " = ";
                    ss << formatExpressionWithValues(*assignment->expression, evaluator);

                    // Add = and show final result
                    ss << " = ";
                    Value result = evaluator.evaluateExpression(*assignment->expression);
                    if (std::holds_alternative<double>(result)) {
                        double val = std::get<double>(result);
                        if (val == static_cast<int>(val)) {
                            ss << static_cast<int>(val);
                        } else {
                            ss << val;
                        }
                    } else if (std::holds_alternative<ComplexValue>(result)) {
                        ComplexValue complex = std::get<ComplexValue>(result);
                        if (complex.real == 0 && complex.imaginary == 0) {
                            ss << "0";
                        } else {
                            bool needsReal = (complex.real != 0);
                            bool needsImag = (complex.imaginary != 0);

                            if (needsReal) {
                                if (complex.real == static_cast<int>(complex.real)) {
                                    ss << static_cast<int>(complex.real);
                                } else {
                                    ss << complex.real;
                                }
                            }

                            if (needsImag) {
                                double imag = complex.imaginary;
                                if (needsReal) {
                                    if (imag > 0) {
                                        ss << " + ";
                                    } else {
                                        ss << " - ";
                                        imag = -imag;
                                    }
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
                        }
                    } else if (std::holds_alternative<UnitValue>(result)) {
                        UnitValue unitVal = std::get<UnitValue>(result);
                        if (unitVal.value == static_cast<int>(unitVal.value)) {
                            ss << static_cast<int>(unitVal.value);
                        } else {
                            ss << unitVal.value;
                        }
                    } else if (std::holds_alternative<ArrayValue>(result)) {
                        ArrayValue arrayVal = std::get<ArrayValue>(result);
                        if (arrayVal.isMatrix) {
                            // Format matrix as LaTeX matrix
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
                        } else {
                            // Format vector as LaTeX column or row vector
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
                        }
                    }

                    ss << "\n$$\n\n";
                }
            } else if (decoratedStmt->hasDecorator("eval")) {
                // Handle @eval decorator - show variable =expression = result
                if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(decoratedStmt->statement.get())) {
                    ss << "$$\n";
                    ss << formatVariableName(assignment->variable) << " = " << formatExpression(*assignment->expression);

                    // Add = and show final result directly
                    ss << " = ";
                    Value result = evaluator.evaluateExpression(*assignment->expression);
                    if (std::holds_alternative<double>(result)) {
                        double val = std::get<double>(result);
                        if (val == static_cast<int>(val)) {
                            ss << static_cast<int>(val);
                        } else {
                            ss << val;
                        }
                    } else if (std::holds_alternative<UnitValue>(result)) {
                        UnitValue unitVal = std::get<UnitValue>(result);
                        if (unitVal.value == static_cast<int>(unitVal.value)) {
                            ss << static_cast<int>(unitVal.value);
                        } else {
                            ss << unitVal.value;
                        }
                    } else if (std::holds_alternative<ArrayValue>(result)) {
                        ArrayValue arrayVal = std::get<ArrayValue>(result);
                        if (arrayVal.isMatrix) {
                            // Format matrix as LaTeX matrix
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
                        } else {
                            // Format vector as LaTeX column or row vector
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
                        }
                    }

                    ss << "\n$$\n\n";
                } else if (const auto* exprStmt = dynamic_cast<const ExpressionStatement*>(decoratedStmt->statement.get())) {
                    // Handle @eval on expression statements like "a;"
                    ss << "$$\n";
                    ss << formatExpression(*exprStmt->expression);
                    ss << " = ";
                    Value result = evaluator.evaluateExpression(*exprStmt->expression);
                    if (std::holds_alternative<double>(result)) {
                        double val = std::get<double>(result);
                        if (val == static_cast<int>(val)) {
                            ss << static_cast<int>(val);
                        } else {
                            ss << val;
                        }
                    } else if (std::holds_alternative<ArrayValue>(result)) {
                        ArrayValue arrayVal = std::get<ArrayValue>(result);
                        if (arrayVal.isMatrix) {
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
                        } else {
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
                        }
                    }
                    ss << "\n$$\n\n";
                }
            } else if (decoratedStmt->hasDecorator("resolveAlign")) {
                // Handle @resolveAlign decorator - align assignment and equations
                if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(decoratedStmt->statement.get())) {
                    ss << "$$\n";
                    ss << "\\begin{align}\n";
                    ss << formatVariableName(assignment->variable) << " &= " << formatExpression(*assignment->expression);

                    // Add = and show variable values substituted (aligned)
                    ss << " \\\\\n";
                    ss << "&= ";
                    ss << formatExpressionWithValues(*assignment->expression, evaluator);

                    // Add = and show final result (aligned)
                    ss << " \\\\\n";
                    ss << "&= ";
                    Value result = evaluator.evaluateExpression(*assignment->expression);
                    if (std::holds_alternative<double>(result)) {
                        double val = std::get<double>(result);
                        if (val == static_cast<int>(val)) {
                            ss << static_cast<int>(val);
                        } else {
                            ss << val;
                        }
                    } else if (std::holds_alternative<UnitValue>(result)) {
                        UnitValue unitVal = std::get<UnitValue>(result);
                        if (unitVal.value == static_cast<int>(unitVal.value)) {
                            ss << static_cast<int>(unitVal.value);
                        } else {
                            ss << unitVal.value;
                        }
                    } else if (std::holds_alternative<ArrayValue>(result)) {
                        ArrayValue arrayVal = std::get<ArrayValue>(result);
                        if (arrayVal.isMatrix) {
                            // Format matrix as LaTeX matrix
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
                        } else {
                            // Format vector as LaTeX column or row vector
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
                        }
                    }

                    ss << "\n\\end{align}\n";
                    ss << "$$\n\n";
                }
            } else {
                // For other decorators, just format the underlying statement normally
                if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(decoratedStmt->statement.get())) {
                    ss << "$$\n";
                    ss << formatVariableName(assignment->variable) << " = " << formatExpression(*assignment->expression) << "\n";
                    ss << "$$\n\n";
                }
            }
        }
        }

        if (evalResult.success && !evalResult.outputs.empty()) {
            for (const auto& output : evalResult.outputs) {
                // Convert unit output to LaTeX format if it contains units
                std::string latexOutput = output;

                // Simple heuristic: if the output contains space followed by unit characters, format as LaTeX
                std::regex unitPattern(R"(\s+([a-zA-Z]+(?:\*[a-zA-Z]+)*)\s*$)");
                std::smatch match;

                if (std::regex_search(latexOutput, match, unitPattern)) {
                    std::string unitPart = match[1].str();
                    std::string valuePart = latexOutput.substr(0, match.position());

                    // Format compound units for LaTeX
                    if (unitPart.find('*') != std::string::npos) {
                        // Handle compound units like "mm*cm"
                        std::stringstream unitSs;
                        std::stringstream currentUnit;

                        for (char c : unitPart) {
                            if (c == '*') {
                                if (currentUnit.tellp() > 0) {
                                    unitSs << "\\text{ " << currentUnit.str() << "} \\cdot ";
                                    currentUnit.str("");
                                    currentUnit.clear();
                                }
                            } else {
                                currentUnit << c;
                            }
                        }

                        // Add the last unit
                        if (currentUnit.tellp() > 0) {
                            unitSs << "\\text{ " << currentUnit.str() << "}";
                        }

                        latexOutput = valuePart + " " + unitSs.str();
                    } else {
                        // Simple unit
                        latexOutput = valuePart + " \\text{ " + unitPart + "}";
                    }
                }

                ss << latexOutput << "\n\n";
            }
        }

        result.markdown = ss.str();
    } catch (const std::exception& e) {
        // If an exception occurs, put the error message in the markdown output
        result.markdown = "ERROR: " + std::string(e.what());
    }
    
    return result;
}

std::string MarkdownFormatter::formatStatement(const Statement& stmt) {
    if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(&stmt)) {
        return formatAssignment(*assignment);
    } else if (const auto* print = dynamic_cast<const PrintStatement*>(&stmt)) {
        return formatPrint(*print);
    } else if (const auto* exprStmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
        return formatExpressionStatement(*exprStmt);
    } else if (const auto* functionDecl = dynamic_cast<const FunctionDeclaration*>(&stmt)) {
        return formatFunctionDeclaration(*functionDecl);
    } else if (const auto* piecewiseFuncDecl = dynamic_cast<const PiecewiseFunctionDeclaration*>(&stmt)) {
        return formatPiecewiseFunctionDeclaration(*piecewiseFuncDecl);
    } else if (const auto* forStmt = dynamic_cast<const ForStatement*>(&stmt)) {
        return formatFor(*forStmt);
    } else if (const auto* whileStmt = dynamic_cast<const WhileStatement*>(&stmt)) {
        return formatWhile(*whileStmt);
    } else if (const auto* ifStmt = dynamic_cast<const IfStatement*>(&stmt)) {
        return formatIf(*ifStmt);
    }
    return "Unknown statement";
}

std::string MarkdownFormatter::formatAssignment(const AssignmentStatement& stmt) {
    std::string assignment = formatVariableName(stmt.variable) + " =" + formatExpression(*stmt.expression);
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

std::string MarkdownFormatter::formatPrint(const PrintStatement& stmt) {
    return "print(" + formatExpression(*stmt.expression) + ");";
}


std::string MarkdownFormatter::formatExpressionStatement(const ExpressionStatement& stmt) {
    // Check if this is a graph function call
    if (const auto* functionCall = dynamic_cast<const FunctionCall*>(stmt.expression.get())) {
        if (functionCall->function_name == "graph") {
            // Instead of showing the function call, insert a placeholder for the web app
            return "<!-- GRAPH_PLACEHOLDER_" + std::to_string(globalGraphCounter++) + " -->";
        }
    }

    // For other expression statements, format normally
    return formatExpression(*stmt.expression) + ";";
}

std::string MarkdownFormatter::formatFunctionDeclaration(const FunctionDeclaration& stmt) {
    std::stringstream ss;
    ss << "fn " << stmt.name << "(";
    for (size_t i = 0; i < stmt.parameters.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << stmt.parameters[i];
    }
    ss << ") { ... }";
    return ss.str();
}


std::string MarkdownFormatter::formatVariableName(const std::string& varName) {
    // Variable names containing subscripts like A_{inv} should be preserved as-is
    // since they are already in proper LaTeX subscript format
    return varName;
}

std::string MarkdownFormatter::formatExpression(const Expression& expr) {
    if (const auto* identifier = dynamic_cast<const Identifier*>(&expr)) {
        return formatVariableName(identifier->name);
    } else if (const auto* number = dynamic_cast<const Number*>(&expr)) {
        // Format as integer if it's a whole number, otherwise as double with trailing zeros removed
        if (number->value == static_cast<int>(number->value)) {
            return std::to_string(static_cast<int>(number->value));
        } else {
            // Format with trailing zeros removed
            std::string result = std::to_string(number->value);
            // Remove trailing zeros and decimal point if not needed
            result.erase(result.find_last_not_of('0') + 1, std::string::npos);
            result.erase(result.find_last_not_of('.') + 1, std::string::npos);
            return result;
        }
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
        std::string valueStr = formatExpression(*unitExpr->value);
        std::string unit = unitExpr->unit;

        // Check if unit ends with a digit (like in3, mm2, ft3)
        if (unit.length() > 1 && std::isdigit(static_cast<unsigned char>(unit[unit.length() - 1]))) {
            // Find where the number starts
            size_t numStart = unit.length() - 1;
            while (numStart > 0 && std::isdigit(static_cast<unsigned char>(unit[numStart - 1]))) {
                numStart--;
            }

            std::string baseUnit = unit.substr(0, numStart);
            std::string exponent = unit.substr(numStart);
            return valueStr + " \\text{ " + baseUnit + "}^{" + exponent + "}";
        }

        return valueStr + " \\text{ " + unit + "}";
    } else if (const auto* binary = dynamic_cast<const BinaryExpression*>(&expr)) {
        return formatBinaryExpression(*binary);
    } else if (const auto* unary = dynamic_cast<const UnaryExpression*>(&expr)) {
        return formatUnaryExpression(*unary);
    } else if (const auto* functionCall = dynamic_cast<const FunctionCall*>(&expr)) {
        return formatFunctionCall(*functionCall);
    } else if (const auto* methodCall = dynamic_cast<const MethodCall*>(&expr)) {
        return formatMethodCall(*methodCall);
    } else if (const auto* rangeExpr = dynamic_cast<const RangeExpression*>(&expr)) {
        return formatRangeExpression(*rangeExpr);
    } else if (const auto* piecewiseExpr = dynamic_cast<const PiecewiseExpression*>(&expr)) {
        return formatPiecewiseExpression(*piecewiseExpr);
    } else if (const auto* arrayExpr = dynamic_cast<const ArrayExpression*>(&expr)) {
        return formatArrayExpression(*arrayExpr);
    } else if (const auto* arrayAccess = dynamic_cast<const ArrayAccess*>(&expr)) {
        return formatArrayAccess(*arrayAccess);
    }

    // Return a safe default instead of throwing
    return "UNKNOWN_EXPR";
}

std::string MarkdownFormatter::formatBinaryExpression(const BinaryExpression& expr) {
    if (!expr.left || !expr.right) {
        return "INVALID_BINARY_EXPR";
    }

    std::string left = formatExpression(*expr.left);
    std::string right = formatExpression(*expr.right);

    if (expr.operator_str == "/") {
        return "\\frac{" + left + "}{" + right + "}";
    } else if (expr.operator_str == "^") {
        return "{" + left + "}^{" + right + "}";
    } else if (expr.operator_str == "*") {
        return left + " \\cdot " + right;
    } else {
        return left + " " + expr.operator_str + " " + right;
    }
}

std::string MarkdownFormatter::formatFunctionCall(const FunctionCall& expr) {
    // Handle special mathematical functions with LaTeX formatting
    if (expr.function_name == "sqrt") {
        if (expr.arguments.size() == 1) {
            return "\\sqrt{" + formatExpression(*expr.arguments[0]) + "}";
        }
    }

    if (expr.function_name == "summation" && expr.arguments.size() == 4) {
        // Format as: \sum_{variable=lowerBound}^{upperBound} expression
        std::string variable;
        if (const auto* varIdentifier = dynamic_cast<const Identifier*>(expr.arguments[1].get())) {
            variable = varIdentifier->name;
        } else {
            variable = "i"; // fallback
        }

        return "\\sum_{" + variable + "=" + formatExpression(*expr.arguments[2]) + "}^{" +
               formatExpression(*expr.arguments[3]) + "} " + formatExpression(*expr.arguments[0]);
    }

    // Default function call formatting
    std::stringstream ss;
    ss << expr.function_name << "(";
    for (size_t i = 0; i < expr.arguments.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << formatExpression(*expr.arguments[i]);
    }
    ss << ")";
    return ss.str();
}

std::string MarkdownFormatter::formatMethodCall(const MethodCall& expr) {
    // Check if this is a math.* method call
    if (const auto* identifier = dynamic_cast<const Identifier*>(expr.object.get())) {
        if (identifier->name == "math") {
            // Handle math.sqrt with LaTeX formatting
            if (expr.method_name == "sqrt") {
                if (expr.arguments.size() == 1) {
                    return "\\sqrt{" + formatExpression(*expr.arguments[0]) + "}";
                }
            }
        }
    }

    // Handle matrix methods with LaTeX formatting
    if (expr.method_name == "det") {
        return "\\begin{vmatrix}" + formatExpression(*expr.object) + "\\end{vmatrix}";
    }

    if (expr.method_name == "tr") {
        return "\\text{tr}(" + formatExpression(*expr.object) + ")";
    }

    if (expr.method_name == "inv") {
        return formatExpression(*expr.object) + "^{-1}";
    }

    if (expr.method_name == "T") {
        return formatExpression(*expr.object) + "^\\mathsf{T}";
    }

    // Default formatting: object.method(args)
    std::stringstream ss;
    ss << formatExpression(*expr.object) << "." << expr.method_name << "(";
    for (size_t i = 0; i < expr.arguments.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << formatExpression(*expr.arguments[i]);
    }
    ss << ")";
    return ss.str();
}

std::string MarkdownFormatter::formatFor(const ForStatement& stmt) {
    std::stringstream ss;
    ss << "for " << stmt.variable << " in " << formatExpression(*stmt.range) << " { ... }";
    return ss.str();
}

std::string MarkdownFormatter::formatWhile(const WhileStatement& stmt) {
    std::stringstream ss;
    ss << "while (" << formatExpression(*stmt.condition) << ") { ... }";
    return ss.str();
}

std::string MarkdownFormatter::formatIf(const IfStatement& stmt) {
    std::stringstream ss;
    ss << "if (" << formatExpression(*stmt.condition) << ") { ... }";
    if (!stmt.else_body.empty()) {
        ss << " else { ... }";
    }
    return ss.str();
}

std::string MarkdownFormatter::formatRangeExpression(const RangeExpression& expr) {
    return formatExpression(*expr.start) + "..." + formatExpression(*expr.end);
}

std::string MarkdownFormatter::formatUnaryExpression(const UnaryExpression& expr) {
    if (!expr.operand) {
        return "INVALID_UNARY_EXPR";
    }

    // Check if operand is a binary expression that needs grouping
    bool needsGrouping = dynamic_cast<const BinaryExpression*>(expr.operand.get()) != nullptr;

    std::string operand = formatExpression(*expr.operand);

    if (expr.operator_str == "-") {
        if (needsGrouping) {
            return "-(" + operand + ")";
        }
        return "-" + operand;
    }

    if (needsGrouping) {
        return expr.operator_str + "(" + operand + ")";
    }
    return expr.operator_str + operand;
}

std::string MarkdownFormatter::formatStatementWithDepth(const Statement& stmt, int depth) {
    std::string indent;
    for (int i = 0; i < depth; ++i) {
        indent += "\\quad ";
    }

    if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(&stmt)) {
        return indent + formatVariableName(assignment->variable) + " =" + formatExpression(*assignment->expression);
    } else if (const auto* arrayAssignment = dynamic_cast<const ArrayAssignmentStatement*>(&stmt)) {
        std::string result = indent + formatVariableName(arrayAssignment->arrayName) + "[" + formatExpression(*arrayAssignment->index);
        if (arrayAssignment->isColumnVector) {
            result += ";";
        }
        result += "] =" + formatExpression(*arrayAssignment->expression);
        return result;
    } else if (const auto* returnStmt = dynamic_cast<const ReturnStatement*>(&stmt)) {
        return indent + "\\text{return } " + formatExpression(*returnStmt->expression);
    } else if (dynamic_cast<const BreakStatement*>(&stmt)) {
        return indent + "\\textbf{break}";
    } else if (const auto* forStmt = dynamic_cast<const ForStatement*>(&stmt)) {
        std::stringstream ss;
        ss << indent << "\\textbf{foreach}\\; " << formatVariableName(forStmt->variable) << "\\in\\{";

        if (const auto* rangeExpr = dynamic_cast<const RangeExpression*>(forStmt->range.get())) {
            if (rangeExpr->start && rangeExpr->end) {
                ss << formatExpression(*rangeExpr->start) << ",\\dots," << formatExpression(*rangeExpr->end);
            }
        }

        ss << "\\}\\; \\textbf{do}\\\\\n";
        ss << indent << "\\left|\n";
        ss << "\\begin{array}{l}\n";

        for (size_t i = 0; i < forStmt->body.size(); ++i) {
            ss << formatStatementWithDepth(*forStmt->body[i], depth + 1, false);
            if (i < forStmt->body.size() - 1) {
                ss << " \\\\\n";
            } else {
                ss << "\n";
            }
        }

        ss << "\\end{array}\n";
        ss << "\\right.";
        return ss.str();
    } else if (const auto* whileStmt = dynamic_cast<const WhileStatement*>(&stmt)) {
        std::stringstream ss;
        ss << indent << "\\textbf{while } " << formatExpression(*whileStmt->condition) << " \\textbf{ do}\\\\\n";
        ss << indent << "\\left|\n";
        ss << "\\begin{array}{l}\n";

        for (size_t i = 0; i < whileStmt->body.size(); ++i) {
            ss << formatStatementWithDepth(*whileStmt->body[i], depth + 1, false);
            if (i < whileStmt->body.size() - 1) {
                ss << " \\\\\n";
            } else {
                ss << "\n";
            }
        }

        ss << "\\end{array}\n";
        ss << "\\right.";
        return ss.str();
    } else if (const auto* ifStmt = dynamic_cast<const IfStatement*>(&stmt)) {
        return formatIfWithDepth(*ifStmt, depth, false);
    }

    return indent + "UNKNOWN_STATEMENT";
}

std::string MarkdownFormatter::formatStatementWithDepth(const Statement& stmt, int depth, bool inFunction) {
    std::string indent;
    for (int i = 0; i < depth; ++i) {
        indent += "\\quad ";
    }

    if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(&stmt)) {
        return indent + formatVariableName(assignment->variable) + " =" + formatExpression(*assignment->expression);
    } else if (const auto* arrayAssignment = dynamic_cast<const ArrayAssignmentStatement*>(&stmt)) {
        std::string result = indent + formatVariableName(arrayAssignment->arrayName) + "[" + formatExpression(*arrayAssignment->index);
        if (arrayAssignment->isColumnVector) {
            result += ";";
        }
        result += "] =" + formatExpression(*arrayAssignment->expression);
        return result;
    } else if (const auto* returnStmt = dynamic_cast<const ReturnStatement*>(&stmt)) {
        return indent + "\\text{return } " + formatExpression(*returnStmt->expression);
    } else if (dynamic_cast<const BreakStatement*>(&stmt)) {
        return indent + "\\textbf{break}";
    } else if (const auto* forStmt = dynamic_cast<const ForStatement*>(&stmt)) {
        std::stringstream ss;
        ss << indent << "\\textbf{foreach}\\; " << formatVariableName(forStmt->variable) << "\\in\\{";

        if (const auto* rangeExpr = dynamic_cast<const RangeExpression*>(forStmt->range.get())) {
            if (rangeExpr->start && rangeExpr->end) {
                ss << formatExpression(*rangeExpr->start) << ",\\dots," << formatExpression(*rangeExpr->end);
            }
        }

        ss << "\\}\\; \\textbf{do}\\\\\n";
        // Add \quad for \left| only if we're in a function context
        ss << (inFunction ? "\\quad " : "") << indent << "\\left|\n";
        ss << "\\begin{array}{l}\n";

        for (size_t i = 0; i < forStmt->body.size(); ++i) {
            ss << formatStatementWithDepth(*forStmt->body[i], depth + 1, inFunction);
            if (i < forStmt->body.size() - 1) {
                ss << " \\\\\n";
            } else {
                ss << "\n";
            }
        }

        ss << "\\end{array}\n";
        ss << "\\right.";
        return ss.str();
    } else if (const auto* whileStmt = dynamic_cast<const WhileStatement*>(&stmt)) {
        std::stringstream ss;
        ss << indent << "\\textbf{while } " << formatExpression(*whileStmt->condition) << " \\textbf{ do}\\\\\n";
        // Add \quad for \left| only if we're in a function context
        ss << (inFunction ? "\\quad " : "") << indent << "\\left|\n";
        ss << "\\begin{array}{l}\n";

        for (size_t i = 0; i < whileStmt->body.size(); ++i) {
            ss << formatStatementWithDepth(*whileStmt->body[i], depth + 1, inFunction);
            if (i < whileStmt->body.size() - 1) {
                ss << " \\\\\n";
            } else {
                ss << "\n";
            }
        }

        ss << "\\end{array}\n";
        ss << "\\right.";
        return ss.str();
    } else if (const auto* ifStmt = dynamic_cast<const IfStatement*>(&stmt)) {
        return formatIfWithDepth(*ifStmt, depth, inFunction);
    }

    return indent + "UNKNOWN_STATEMENT";
}

std::string MarkdownFormatter::formatIfWithDepth(const IfStatement& stmt, int depth, bool inFunction) {
    std::string indent;
    for (int i = 0; i < depth; ++i) {
        indent += "\\quad ";
    }

    std::stringstream ss;
    ss << indent << "\\textbf{if } " << formatExpression(*stmt.condition) << " \\textbf{ then} \\\\\n";
    // Add \quad for \left| only if we're in a function context
    ss << (inFunction ? "\\quad " : "") << indent << "\\left|\n";
    ss << "\\begin{array}{l}\n";

    for (const auto& bodyStmt : stmt.then_body) {
        ss << formatStatementWithDepth(*bodyStmt, depth + 1, inFunction) << "\n";
    }

    ss << "\\end{array}\n";
    ss << "\\right.";

    // Handle else clause if present
    if (!stmt.else_body.empty()) {
        ss << "\\\\\n";
        ss << indent << "\\quad \\textbf{else} \\\\\n";
        ss << (inFunction ? "\\quad " : "") << indent << "\\left|\n";
        ss << "\\begin{array}{l}\n";

        for (const auto& bodyStmt : stmt.else_body) {
            ss << formatStatementWithDepth(*bodyStmt, depth + 1, inFunction) << "\n";
        }

        ss << "\\end{array}\n";
        ss << "\\right.";
    }

    return ss.str();
}

std::string MarkdownFormatter::formatExpressionWithValues(const Expression& expr, Evaluator& evaluator) {
    if (const auto* identifier = dynamic_cast<const Identifier*>(&expr)) {
        // Replace identifier with its value from the evaluator's environment
        try {
            Value value = evaluator.getVariableValue(identifier->name);
            if (std::holds_alternative<double>(value)) {
                double val = std::get<double>(value);
                std::string result;
                if (val == static_cast<int>(val)) {
                    result = std::to_string(static_cast<int>(val));
                } else {
                    // Format with trailing zeros removed
                    result = std::to_string(val);
                    result.erase(result.find_last_not_of('0') + 1, std::string::npos);
                    result.erase(result.find_last_not_of('.') + 1, std::string::npos);
                }
                // Wrap negative numbers in parentheses
                if (val < 0) {
                    result = "(" + result + ")";
                }
                return result;
            } else if (std::holds_alternative<ComplexValue>(value)) {
                ComplexValue complex = std::get<ComplexValue>(value);
                std::stringstream ss;

                if (complex.real == 0 && complex.imaginary == 0) {
                    return "0";
                }

                bool needsReal = (complex.real != 0);
                bool needsImag = (complex.imaginary != 0);

                if (needsReal) {
                    if (complex.real == static_cast<int>(complex.real)) {
                        ss << static_cast<int>(complex.real);
                    } else {
                        ss << complex.real;
                    }
                }

                if (needsImag) {
                    double imag = complex.imaginary;
                    if (needsReal) {
                        if (imag > 0) {
                            ss << " + ";
                        } else {
                            ss << " - ";
                            imag = -imag;
                        }
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
        } catch (...) {
            // If variable not found, keep original identifier
            return identifier->name;
        }
        return identifier->name;
    } else if (const auto* number = dynamic_cast<const Number*>(&expr)) {
        // Numbers stay the same
        return formatExpression(*number);
    } else if (const auto* unitExpr = dynamic_cast<const UnitExpression*>(&expr)) {
        // Format unit expressions with their values
        return formatExpressionWithValues(*unitExpr->value, evaluator) + " \\text{ " + unitExpr->unit + "}";
    } else if (const auto* binary = dynamic_cast<const BinaryExpression*>(&expr)) {
        // Recursively process binary expressions
        std::string left = formatExpressionWithValues(*binary->left, evaluator);
        std::string right = formatExpressionWithValues(*binary->right, evaluator);

        if (binary->operator_str == "/") {
            return "\\frac{" + left + "}{" + right + "}";
        } else if (binary->operator_str == "^") {
            return "{" + left + "}^{" + right + "}";
        } else if (binary->operator_str == "*") {
            return left + " \\cdot " + right;
        } else {
            return left + " " + binary->operator_str + " " + right;
        }
    } else if (const auto* unary = dynamic_cast<const UnaryExpression*>(&expr)) {
        std::string operand = formatExpressionWithValues(*unary->operand, evaluator);
        if (unary->operator_str == "-") {
            return "(-" + operand + ")";
        }
        return unary->operator_str + operand;
    } else if (const auto* functionCall = dynamic_cast<const FunctionCall*>(&expr)) {
        if (functionCall->function_name == "sqrt") {
            if (functionCall->arguments.size() == 1) {
                return "\\sqrt{" + formatExpressionWithValues(*functionCall->arguments[0], evaluator) + "}";
            }
        }

        // Handle matrix functions with LaTeX formatting and value substitution
        if (functionCall->function_name == "T" && functionCall->arguments.size() == 1) {
            return formatExpressionWithValues(*functionCall->arguments[0], evaluator) + "^\\mathsf{T}";
        }

        if (functionCall->function_name == "Det" && functionCall->arguments.size() == 1) {
            // For determinants, we want to show the matrix elements directly inside vmatrix
            // without the bmatrix wrapper
            if (const auto* identifier = dynamic_cast<const Identifier*>(functionCall->arguments[0].get())) {
                try {
                    Value value = evaluator.getVariableValue(identifier->name);
                    if (std::holds_alternative<ArrayValue>(value)) {
                        ArrayValue arrayVal = std::get<ArrayValue>(value);
                        if (arrayVal.isMatrix) {
                            // Format matrix elements directly inside vmatrix
                            std::stringstream ss;
                            ss << "\\begin{vmatrix}\n";
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
                            ss << "\n\\end{vmatrix}";
                            return ss.str();
                        }
                    }
                } catch (...) {
                    // If variable not found, fall back to regular formatting
                }
            }
            // Default case: use regular substitution
            return "\\begin{vmatrix}" + formatExpressionWithValues(*functionCall->arguments[0], evaluator) + "\\end{vmatrix}";
        }

        if (functionCall->function_name == "Tr" && functionCall->arguments.size() == 1) {
            return "\\text{tr}(" + formatExpressionWithValues(*functionCall->arguments[0], evaluator) + ")";
        }

        if (functionCall->function_name == "Inv" && functionCall->arguments.size() == 1) {
            return formatExpressionWithValues(*functionCall->arguments[0], evaluator) + "^{-1}";
        }

        // Default function formatting with value substitution
        std::stringstream ss;
        ss << functionCall->function_name << "(";
        for (size_t i = 0; i < functionCall->arguments.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << formatExpressionWithValues(*functionCall->arguments[i], evaluator);
        }
        ss << ")";
        return ss.str();
    } else if (const auto* methodCall = dynamic_cast<const MethodCall*>(&expr)) {
        // Check if this is a math.* method call
        if (const auto* identifier = dynamic_cast<const Identifier*>(methodCall->object.get())) {
            if (identifier->name == "math") {
                // Handle math.sqrt with LaTeX formatting and value substitution
                if (methodCall->method_name == "sqrt") {
                    if (methodCall->arguments.size() == 1) {
                        return "\\sqrt{" + formatExpressionWithValues(*methodCall->arguments[0], evaluator) + "}";
                    }
                }
            }
        }

        // For other method calls, fall back to regular formatting
        std::stringstream ss;
        ss << formatExpressionWithValues(*methodCall->object, evaluator) << "." << methodCall->method_name << "(";
        for (size_t i = 0; i < methodCall->arguments.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << formatExpressionWithValues(*methodCall->arguments[i], evaluator);
        }
        ss << ")";
        return ss.str();
    }

    // Fallback to regular formatting
    return formatExpression(expr);
}

std::string MarkdownFormatter::formatPiecewiseExpression(const PiecewiseExpression& expr) {
    std::stringstream ss;

    ss << "\\begin{cases}\n";

    for (const auto& case_ptr : expr.cases) {
        ss << formatExpression(*case_ptr->value);

        if (case_ptr->condition == nullptr) {
            // "otherwise" case
            ss << " & \\text{otherwise}";
        } else {
            // Regular condition
            ss << " & " << formatExpression(*case_ptr->condition);
        }

        ss << " \\\\\n";
    }

    ss << "\\end{cases}";
    return ss.str();
}

std::string MarkdownFormatter::formatPiecewiseFunctionDeclaration(const PiecewiseFunctionDeclaration& stmt) {
    std::stringstream ss;

    // Function name with parameters
    ss << stmt.name << "(";
    for (size_t i = 0; i < stmt.parameters.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << stmt.parameters[i];
    }
    ss << ") = ";

    // Format the piecewise expression
    ss << formatPiecewiseExpression(*stmt.piecewise);

    return ss.str();
}

std::string MarkdownFormatter::formatDecoratedStatementContent(const DecoratedStatement& stmt, Evaluator& evaluator) {
    // Special handling for array-decorated comments
    if (stmt.hasLayoutDecorator()) {
        if (const auto* comment = dynamic_cast<const CommentStatement*>(stmt.statement.get())) {
            // For layout decorator on a comment, just return the comment content
            // The comment will be attached to the next math expression in the array processing logic
            return comment->content;
        }
    }

    // Handle decorated comments for non-layout decorators
    if (const auto* comment = dynamic_cast<const CommentStatement*>(stmt.statement.get())) {
        // For non-layout decorators applied to comments, just return the comment content
        // (The decorator effect doesn't apply to comments)
        return comment->content;
    }

    // Handle different decorator types within array elements
    if (stmt.hasDecorator("resolve")) {
        if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(stmt.statement.get())) {
            std::stringstream ss;
            ss << formatVariableName(assignment->variable) << " = " << formatExpression(*assignment->expression);
            ss << " = " << formatExpressionWithValues(*assignment->expression, evaluator);
            ss << " = ";

            Value result = evaluator.evaluateExpression(*assignment->expression);
            if (std::holds_alternative<double>(result)) {
                double val = std::get<double>(result);
                if (val == static_cast<int>(val)) {
                    ss << static_cast<int>(val);
                } else {
                    ss << val;
                }
            }
            return ss.str();
        }
    } else if (stmt.hasDecorator("eval")) {
        if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(stmt.statement.get())) {
            std::stringstream ss;
            ss << formatVariableName(assignment->variable) << " = " << formatExpression(*assignment->expression);
            ss << " = ";

            Value result = evaluator.evaluateExpression(*assignment->expression);
            if (std::holds_alternative<double>(result)) {
                double val = std::get<double>(result);
                if (val == static_cast<int>(val)) {
                    ss << static_cast<int>(val);
                } else {
                    ss << val;
                }
            }
            return ss.str();
        }
    } else if (stmt.hasDecorator("resolveAlign")) {
        // For array context, format as stacked equations without the align environment
        if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(stmt.statement.get())) {
            std::stringstream ss;
            ss << "\\begin{array}{l}\n";
            ss << formatVariableName(assignment->variable) << " = " << formatExpression(*assignment->expression);
            ss << " \\\\\n";
            ss << "\\quad = " << formatExpressionWithValues(*assignment->expression, evaluator);
            ss << " \\\\\n";
            ss << "\\quad = ";

            Value result = evaluator.evaluateExpression(*assignment->expression);
            if (std::holds_alternative<double>(result)) {
                double val = std::get<double>(result);
                if (val == static_cast<int>(val)) {
                    ss << static_cast<int>(val);
                } else {
                    ss << val;
                }
            } else if (std::holds_alternative<UnitValue>(result)) {
                UnitValue unitVal = std::get<UnitValue>(result);
                if (unitVal.value == static_cast<int>(unitVal.value)) {
                    ss << static_cast<int>(unitVal.value);
                } else {
                    ss << unitVal.value;
                }
            }

            ss << "\n\\end{array}";
            return ss.str();
        }
    }

    // Default: format as regular assignment
    return formatStatementForArray(*stmt.statement, evaluator);
}

std::string MarkdownFormatter::formatStatementForArray(const Statement& stmt, Evaluator& evaluator) {
    if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(&stmt)) {
        std::stringstream ss;
        if (!assignment->inlineComment.empty()) {
            ss << "\\text{" << assignment->inlineComment << "} \\quad ";
        }
        ss << formatVariableName(assignment->variable) << " = " << formatExpression(*assignment->expression);
        return ss.str();
    } else if (const auto* decoratedStmt = dynamic_cast<const DecoratedStatement*>(&stmt)) {
        return formatDecoratedStatementContent(*decoratedStmt, evaluator);
    }

    // For other statement types, return empty (shouldn't be in arrays)
    return "";
}

std::string MarkdownFormatter::formatArrayExpression(const ArrayExpression& expr) {
    if (expr.isMatrix) {
        // Format matrix as LaTeX matrix with square brackets
        std::stringstream ss;
        ss << "\\begin{bmatrix}\n";
        for (size_t i = 0; i < expr.matrixRows.size(); ++i) {
            for (size_t j = 0; j < expr.matrixRows[i].size(); ++j) {
                ss << formatExpression(*expr.matrixRows[i][j]);
                if (j < expr.matrixRows[i].size() - 1) {
                    ss << " & ";
                }
            }
            if (i < expr.matrixRows.size() - 1) {
                ss << " \\\\\n";
            }
        }
        ss << "\n\\end{bmatrix}";
        return ss.str();
    } else if (expr.isColumnVector) {
        // Format column vector as LaTeX column matrix with square brackets
        std::stringstream ss;
        ss << "\\begin{bmatrix}\n";
        for (size_t i = 0; i < expr.elements.size(); ++i) {
            ss << formatExpression(*expr.elements[i]);
            if (i < expr.elements.size() - 1) {
                ss << " \\\\\n";
            }
        }
        ss << "\n\\end{bmatrix}";
        return ss.str();
    } else {
        // Format row vector as LaTeX row matrix with square brackets
        std::stringstream ss;
        ss << "\\begin{bmatrix}\n";
        for (size_t i = 0; i < expr.elements.size(); ++i) {
            ss << formatExpression(*expr.elements[i]);
            if (i < expr.elements.size() - 1) {
                ss << " & ";
            }
        }
        ss << "\n\\end{bmatrix}";
        return ss.str();
    }
}

std::string MarkdownFormatter::formatArrayAccess(const ArrayAccess& expr) {
    return formatVariableName(expr.arrayName) + "_{" + formatExpression(*expr.index) + "}";
}

} // namespace madola