#include "html_formatter.h"
#include "embedded_css.h"
#include <sstream>
#include <algorithm>
#include <regex>

namespace madola {

HtmlFormatter::HtmlFormatter() = default;

std::string HtmlFormatter::loadCssFile() {
    // Return embedded CSS from generated header
    return getEmbeddedCss();
}

std::string HtmlFormatter::generateHtmlHeader(const std::string& title) {
    std::string cssContent = loadCssFile();
    return R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>)" + title + R"(</title>
    <script>
        window.MathJax = {
            tex: {
                displayMath: [['$$', '$$'], ['\\[', '\\]']],
                inlineMath: [['$', '$'], ['\\(', '\\)']],
                processEscapes: true,
                processEnvironments: true,
                packages: {'[+]': ['ams', 'newcommand', 'configmacros']}
            },
            chtml: {
                scale: 1.0,
                displayAlign: 'left'
            },
            options: {
                ignoreHtmlClass: 'tex2jax_ignore',
                processHtmlClass: 'tex2jax_process'
            }
        };
    </script>
    <script id="MathJax-script" async src="https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-mml-chtml.js"></script>
    <script src="https://d3js.org/d3.v7.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/three@0.160.0/build/three.min.js"></script>
    <style>
)" + cssContent + R"(
    </style>
</head>
<body>
    <div class="container">
)";
}

std::string HtmlFormatter::generateHtmlFooter() {
    return R"(    </div>
</body>
</html>)";
}

std::string HtmlFormatter::escapeHtml(const std::string& text) {
    std::string result = text;
    std::regex lt("<");
    std::regex gt(">");
    std::regex amp("&");
    std::regex quot("\"");

    result = std::regex_replace(result, amp, "&amp;");
    result = std::regex_replace(result, lt, "&lt;");
    result = std::regex_replace(result, gt, "&gt;");
    result = std::regex_replace(result, quot, "&quot;");

    return result;
}

std::string HtmlFormatter::parseMarkdownFormatting(const std::string& text) {
    // First escape HTML to prevent XSS
    std::string result = escapeHtml(text);

    // Parse markdown formatting: ***bold-italic***, **bold**, *italic*
    // Process in order: bold-italic first, then bold, then italic

    // ***bold-italic*** → <strong><em>text</em></strong>
    std::regex boldItalicPattern(R"(\*\*\*([^*]+)\*\*\*)");
    result = std::regex_replace(result, boldItalicPattern, "<strong><em>$1</em></strong>");

    // **bold** → <strong>text</strong>
    std::regex boldPattern(R"(\*\*([^*]+)\*\*)");
    result = std::regex_replace(result, boldPattern, "<strong>$1</strong>");

    // *italic* → <em>text</em>
    std::regex italicPattern(R"(\*([^*]+)\*)");
    result = std::regex_replace(result, italicPattern, "<em>$1</em>");

    return result;
}

std::string HtmlFormatter::convertToMathJax(const std::string& text) {
    // Convert LaTeX variables to MathJax format
    std::string result = text;

    // Convert simple variable names with subscripts/superscripts
    std::regex greekPattern(R"(\\(alpha|beta|gamma|delta|epsilon|zeta|eta|theta|iota|kappa|lambda|mu|nu|xi|omicron|pi|rho|sigma|tau|upsilon|phi|chi|psi|omega))");
    result = std::regex_replace(result, greekPattern, "\\$1");

    // Convert simple math expressions
    std::regex mathPattern(R"(\b([a-zA-Z]+)_\{([^}]+)\}\b)");
    result = std::regex_replace(result, mathPattern, "$1_{$2}");

    std::regex superPattern(R"(\b([a-zA-Z]+)\^\{([^}]+)\}\b)");
    result = std::regex_replace(result, superPattern, "$1^{$2}");

    return result;
}

HtmlFormatter::FormatResult HtmlFormatter::formatProgram(const Program& program) {
    FormatResult result;
    result.success = true;

    std::stringstream html;
    html << generateHtmlHeader("MADOLA Program");
    html << "<h1>MADOLA Program</h1>\n";

    html << "<div class=\"madola-code\">\n";
    html << "<h3>Source Code</h3>\n";

    for (const auto& stmt : program.statements) {
        html << "<div>" << escapeHtml(formatStatement(*stmt)) << "</div>\n";
    }

    html << "</div>\n";
    html << generateHtmlFooter();

    result.html = html.str();
    return result;
}

HtmlFormatter::FormatResult HtmlFormatter::formatProgramWithExecution(const Program& program) {
    FormatResult result;

    try {
        Evaluator evaluator;
        auto evalResult = evaluator.evaluate(program, "html_output");

        std::stringstream html;
        html << generateHtmlHeader("MADOLA Output");

        if (evalResult.success) {
            // Process statements in order, interleaving math content and graphs
            html << generateOrderedContent(program, evaluator, evalResult);

            // Show execution output if any (excluding graph creation messages)
            if (!evalResult.outputs.empty()) {
                std::vector<std::string> filteredOutputs;
                for (const auto& output : evalResult.outputs) {
                    // Skip graph creation messages
                    if (output.find("Graph") == 0 && output.find("created with") != std::string::npos &&
                        output.find("data points") != std::string::npos) {
                        continue; // Skip this output
                    }
                    // Skip table creation messages
                    if (output.find("Table") == 0 && output.find("created with") != std::string::npos &&
                        output.find("columns") != std::string::npos) {
                        continue; // Skip this output
                    }
                    filteredOutputs.push_back(output);
                }

                // Only show execution results if there are non-graph outputs
                if (!filteredOutputs.empty()) {
                    html << "<div class=\"execution-result\">\n";
                    for (const auto& output : filteredOutputs) {
                        // First escape HTML special characters
                        std::string processedOutput = escapeHtml(output);
                        // Then convert newlines to <br> tags for HTML display
                        size_t pos = 0;
                        while ((pos = processedOutput.find('\n', pos)) != std::string::npos) {
                            processedOutput.replace(pos, 1, "<br>");
                            pos += 4; // Length of "<br>"
                        }
                        html << convertToMathJax(processedOutput) << "\n";
                    }
                    html << "</div>\n";
                }
            }
        } else {
            html << "<h2>Execution Error</h2>\n";
            html << "<div class=\"execution-result\" style=\"background: #f8d7da; border-color: #dc3545;\">\n";
            html << "<strong>Error:</strong> " << escapeHtml(evalResult.error) << "\n";
            html << "</div>\n";
        }

        html << generateHtmlFooter();

        result.html = html.str();
        result.success = true;
    } catch (const std::exception& e) {
        result.success = false;
        result.error = e.what();
    }

    return result;
}

std::string HtmlFormatter::generateOrderedContent(const Program& program, Evaluator& evaluator, const Evaluator::EvaluationResult& evalResult) {
    std::stringstream html;
    size_t graphIndex = 0;
    size_t graph3DIndex = 0;
    size_t tableIndex = 0;

    // Store program reference for @eval statement handling
    currentProgram = &program;

    // Track @skip decorator to skip the next statement
    bool skipNextStatement = false;

    // Process each statement in order
    for (size_t i = 0; i < program.statements.size(); ++i) {
        const auto& stmt = program.statements[i];
        currentStatement = stmt.get();

        // Handle @skip statement - only set flag to skip if there's no line break after
        if (const auto* skipStmt = dynamic_cast<const SkipStatement*>(stmt.get())) {
            if (skipStmt->shouldSkipNext) {
                skipNextStatement = true;
            }
            // If shouldSkipNext is false (line break after @skip), do nothing
            continue;
        }

        // Skip this statement if the previous one was @skip
        if (skipNextStatement) {
            skipNextStatement = false;
            continue;
        }

        // Skip comments - don't show in HTML report
        if (dynamic_cast<const CommentStatement*>(stmt.get())) {
            continue;
        }

        // Handle heading statements - output as HTML headings
        if (const auto* headingStmt = dynamic_cast<const HeadingStatement*>(stmt.get())) {
            std::string tag = "h" + std::to_string(headingStmt->level);

            // Build style attribute if present
            std::string styleAttr = "";
            if (!headingStmt->style.empty()) {
                if (headingStmt->style == "center") {
                    styleAttr = " style=\"text-align:center\"";
                } else if (headingStmt->style == "left") {
                    styleAttr = " style=\"text-align:left\"";
                } else if (headingStmt->style == "right") {
                    styleAttr = " style=\"text-align:right\"";
                } else {
                    styleAttr = " style=\"" + headingStmt->style + "\"";
                }
            }

            // Output each paragraph as a separate heading with proper line breaks
            for (size_t p = 0; p < headingStmt->textParagraphs.size(); ++p) {
                html << "<" << tag;
                if (p == 0 && !styleAttr.empty()) {
                    // First paragraph gets the style
                    html << styleAttr;
                }
                html << ">" << escapeHtml(headingStmt->textParagraphs[p]) << "</" << tag << ">\n";
            }
            continue;
        }

        // Skip version statements - don't show in HTML report
        if (dynamic_cast<const VersionStatement*>(stmt.get())) {
            continue;
        }

        // Handle paragraph statements - output as HTML paragraphs with markdown formatting
        if (const auto* paragraphStmt = dynamic_cast<const ParagraphStatement*>(stmt.get())) {
            // Build style attribute - always include white-space: pre-wrap
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
                html << "<p" << styleAttr << ">" << parseMarkdownFormatting(processedLine) << "</p>\n";
            }
            continue;
        }

        // Check for parameterized decorators (like @table2, @pair2) that affect multiple statements
        if (const auto* decoratedStmt = dynamic_cast<const DecoratedStatement*>(stmt.get())) {
            // Check for any parameterized decorator
            const Decorator* paramDec = nullptr;
            for (const auto& dec : decoratedStmt->decorators) {
                if (dec.isParameterized()) {
                    paramDec = &dec;
                    break;
                }
            }

            if (paramDec != nullptr) {
                int n = paramDec->parameter;

                // Scan forward to find n expressions/functions INCLUDING this decorated statement
                // Each expression can have at most one @h1-@h4 or @p before it
                std::vector<std::pair<std::string, std::string>> tableRows; // (rightText, leftMath)

                size_t stmtIdx = i; // Start with the decorated statement itself
                int expressionsFound = 0;
                std::string pendingRightText = "";

                while (stmtIdx < program.statements.size() && expressionsFound < n) {
                    const auto& currentStmt = program.statements[stmtIdx];

                    // Check if this is a heading or paragraph - save as pending right text
                    if (const auto* heading = dynamic_cast<const HeadingStatement*>(currentStmt.get())) {
                        std::string tag = "h" + std::to_string(heading->level);
                        std::string styleAttr = "";
                        if (!heading->style.empty()) {
                            if (heading->style == "center") styleAttr = " style=\"text-align:center\"";
                            else if (heading->style == "left") styleAttr = " style=\"text-align:left\"";
                            else if (heading->style == "right") styleAttr = " style=\"text-align:right\"";
                            else styleAttr = " style=\"" + heading->style + "\"";
                        }
                        // Only take the first paragraph
                        if (!heading->textParagraphs.empty()) {
                            pendingRightText = "<" + tag + styleAttr + ">" + escapeHtml(heading->textParagraphs[0]) + "</" + tag + ">";
                        }
                        stmtIdx++;
                        continue;
                    } else if (const auto* para = dynamic_cast<const ParagraphStatement*>(currentStmt.get())) {
                        std::string styleAttr = " style=\"white-space: pre-wrap";
                        if (!para->style.empty()) {
                            if (para->style == "center") styleAttr += "; text-align:center";
                            else if (para->style == "left") styleAttr += "; text-align:left";
                            else if (para->style == "right") styleAttr += "; text-align:right";
                            else styleAttr += "; " + para->style;
                        }
                        styleAttr += "\"";
                        // Only take the first line
                        if (!para->textLines.empty()) {
                            pendingRightText = "<p" + styleAttr + ">" + parseMarkdownFormatting(para->textLines[0]) + "</p>";
                        }
                        stmtIdx++;
                        continue;
                    }

                    // Check if this is an expression/function for the left column
                    std::string leftMath = "";
                    std::string inlineCommentText = "";

                    if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(currentStmt.get())) {
                        // Format assignment WITHOUT inline comment in the math expression
                        leftMath = "$$" + convertToMathJax(assignment->variable) + " = " +
                                   formatExpressionAsMath(*assignment->expression, evaluator) + "$$";
                        // Extract inline comment separately for right column
                        if (!assignment->inlineComment.empty()) {
                            inlineCommentText = escapeHtml(assignment->inlineComment);
                        }
                    } else if (const auto* funcDecl = dynamic_cast<const FunctionDeclaration*>(currentStmt.get())) {
                        leftMath = formatStatementAsMath(*funcDecl, evaluator);
                    } else if (const auto* decoratedStmt = dynamic_cast<const DecoratedStatement*>(currentStmt.get())) {
                        // Check if the decorated statement is an assignment with inline comment
                        if (const auto* innerAssignment = dynamic_cast<const AssignmentStatement*>(decoratedStmt->statement.get())) {
                            // Format WITHOUT inline comment
                            leftMath = "$$" + convertToMathJax(innerAssignment->variable) + " = " +
                                       formatExpressionAsMath(*innerAssignment->expression, evaluator) + "$$";
                            if (!innerAssignment->inlineComment.empty()) {
                                inlineCommentText = escapeHtml(innerAssignment->inlineComment);
                            }
                        } else {
                            leftMath = formatStatementAsMath(*decoratedStmt->statement, evaluator);
                        }
                    }

                    if (!leftMath.empty()) {
                        // Use inline comment if present, otherwise use pending text from @p/@h
                        std::string rightText = !inlineCommentText.empty() ? inlineCommentText : pendingRightText;
                        tableRows.push_back({rightText, leftMath});
                        pendingRightText = ""; // Reset for next row
                        expressionsFound++;
                    }

                    stmtIdx++;
                }

                // Generate borderless table (fit content, with optional centering)
                // Use table-layout: auto to let browser calculate column widths based on content
                // This ensures the left column is wide enough for all rows
                std::string tableStyle = "border: none; border-collapse: collapse; table-layout: auto;";
                if (!paramDec->style.empty() && paramDec->style == "center") {
                    tableStyle += " margin-left: auto; margin-right: auto;";
                }
                html << "<table class=\"madola-pair-table\" style=\"" << tableStyle << "\">\n";

                for (const auto& row : tableRows) {
                    html << "<tr style=\"border: none;\">\n";

                    // Left column: math expression or function (no inline comments)
                    // Use vertical-align: middle for better alignment
                    // No vertical padding, only horizontal: 0px top/bottom, 10px left/right
                    // Set white-space: nowrap to prevent wrapping and ensure consistent width
                    html << "<td style=\"border: none; vertical-align: middle; padding: 0px 10px; white-space: nowrap;\">\n";
                    html << row.second; // leftMath
                    html << "\n</td>\n";

                    // Right column: inline comment or text (blank if none)
                    // Use vertical-align: middle for better alignment
                    // No vertical padding, only horizontal: 0px top/bottom, 10px left/right
                    // Add text-align: left to align comments properly
                    html << "<td style=\"border: none; vertical-align: middle; padding: 0px 10px; text-align: left;\">\n";
                    if (!row.first.empty()) {
                        html << row.first << "\n";
                    }
                    html << "</td>\n";
                    html << "</tr>\n";
                }

                html << "</table>\n";

                // Skip all processed statements
                i = stmtIdx - 1; // -1 because loop will increment
                continue;
            } else if (decoratedStmt->hasLayoutDecorator()) {
                // Find the layout decorator
                for (const auto& dec : decoratedStmt->decorators) {
                    if (dec.isLayout()) {
                        int rows = dec.rows;
                        int cols = dec.cols;
                        int totalElements = rows * cols;

                        if (i + totalElements - 1 < program.statements.size()) {
                            // Handle layout decorator
                            html << "<div class=\"math-expression align-expression\">\n";
                            html << "$$\n\\begin{array}{";

                            // Generate column specification based on dimensions
                            for (int col = 0; col < cols; ++col) {
                                html << "c";
                            }
                            html << "}\n";

                            // Process array elements
                            for (int row = 0; row < rows; ++row) {
                                for (int col = 0; col < cols; ++col) {
                                    int elementIndex = row * cols + col;
                                    if (i + elementIndex < program.statements.size()) {
                                        const auto& arrayStmt = program.statements[i + elementIndex];

                                        if (col > 0) html << " & \\quad \\quad ";

                                        if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(arrayStmt.get())) {
                                            std::string result = convertToMathJax(assignment->variable) + " = " + formatExpressionAsMath(*assignment->expression, evaluator);
                                            if (!assignment->inlineComment.empty()) {
                                                if (assignment->commentBefore) {
                                                    result = "\\text{" + assignment->inlineComment + "} \\quad " + result;
                                                } else {
                                                    result = result + " \\quad \\text{" + assignment->inlineComment + "}";
                                                }
                                            }
                                            html << result;
                                        } else if (const auto* decoratedArrayStmt = dynamic_cast<const DecoratedStatement*>(arrayStmt.get())) {
                                            if (decoratedArrayStmt->hasDecorator("eval")) {
                                                // Handle @eval decorated statements in arrays
                                                if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(decoratedArrayStmt->statement.get())) {
                                                    std::string result = convertToMathJax(assignment->variable) + " = " + formatExpressionAsMath(*assignment->expression, evaluator);
                                                    result += " = ";
                                                    Value evalResult = evaluator.evaluateExpression(*assignment->expression);
                                                    result += formatValueAsMath(evalResult);
                                                    html << result;
                                                } else if (const auto* exprStmt = dynamic_cast<const ExpressionStatement*>(decoratedArrayStmt->statement.get())) {
                                                    std::string result = formatExpressionAsMath(*exprStmt->expression, evaluator);
                                                    result += " = ";
                                                    Value evalResult = evaluator.evaluateExpression(*exprStmt->expression);
                                                    result += formatValueAsMath(evalResult);
                                                    html << result;
                                                }
                                            } else if (decoratedArrayStmt->hasDecorator("resolve")) {
                                                // Handle @resolve decorated statements in arrays
                                                if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(decoratedArrayStmt->statement.get())) {
                                                    std::string result = convertToMathJax(assignment->variable) + " = " + formatExpressionAsMath(*assignment->expression, evaluator);
                                                    result += " = ";
                                                    result += formatExpressionWithValuesAsMath(*assignment->expression, evaluator);
                                                    result += " = ";
                                                    Value evalResult = evaluator.evaluateExpression(*assignment->expression);
                                                    result += formatValueAsMath(evalResult);
                                                    html << result;
                                                }
                                            } else if (decoratedArrayStmt->hasDecorator("resolveAlign")) {
                                                // Handle @resolveAlign decorated statements in arrays
                                                if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(decoratedArrayStmt->statement.get())) {
                                                    html << "\\begin{array}{l}\n";
                                                    html << convertToMathJax(assignment->variable) << " = " << formatExpressionAsMath(*assignment->expression, evaluator);
                                                    html << " \\\\\n";
                                                    html << "\\quad = " << formatExpressionWithValuesAsMath(*assignment->expression, evaluator);
                                                    html << " \\\\\n";
                                                    html << "\\quad = ";
                                                    Value result = evaluator.evaluateExpression(*assignment->expression);
                                                    html << formatValueAsMath(result);
                                                    html << "\n\\end{array}";
                                                }
                                            } else if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(decoratedArrayStmt->statement.get())) {
                                                std::string result = convertToMathJax(assignment->variable) + " = " + formatExpressionAsMath(*assignment->expression, evaluator);
                                                if (!assignment->inlineComment.empty()) {
                                                    if (assignment->commentBefore) {
                                                        result = "\\text{" + assignment->inlineComment + "} \\quad " + result;
                                                    } else {
                                                        result = result + " \\quad \\text{" + assignment->inlineComment + "}";
                                                    }
                                                }
                                                html << result;
                                            }
                                        }
                                    }
                                }
                                html << " \\\\\n";
                            }

                            html << "\\end{array}\n$$\n";
                            html << "</div>\n";

                            // Skip the processed statements
                            i += totalElements - 1;
                            break; // Exit the decorator loop
                        }
                    }
                }
                continue; // Skip regular processing for this statement
            }
        }

        // Generate mathematical content for individual statements (not part of an array)
        std::string mathExpr = formatStatementAsMath(*stmt, evaluator);
        if (!mathExpr.empty()) {
            // All math expressions should have align-expression class
            std::string cssClass = "math-expression align-expression";
            html << "<div class=\"" << cssClass << "\">\n";
            html << "$$" << mathExpr << "$$\n";
            html << "</div>\n";
        }

        // Check if this statement creates a graph and insert it immediately
        bool isGraphCall = false;
        bool is3DGraphCall = false;
        bool isTableCall = false;

        // Check direct function call statements
        if (const auto* func = dynamic_cast<const FunctionCall*>(stmt.get())) {
            if (func->function_name == "graph") {
                isGraphCall = true;
            } else if (func->function_name == "graph_3d") {
                is3DGraphCall = true;
            } else if (func->function_name == "table") {
                isTableCall = true;
            }
        }

        // Check expression statements that contain function calls
        if (const auto* exprStmt = dynamic_cast<const ExpressionStatement*>(stmt.get())) {
            if (const auto* func = dynamic_cast<const FunctionCall*>(exprStmt->expression.get())) {
                if (func->function_name == "graph") {
                    isGraphCall = true;
                } else if (func->function_name == "graph_3d") {
                    is3DGraphCall = true;
                } else if (func->function_name == "table") {
                    isTableCall = true;
                }
            }
        }

        // Check print statements that contain function calls
        if (const auto* printStmt = dynamic_cast<const PrintStatement*>(stmt.get())) {
            if (const auto* func = dynamic_cast<const FunctionCall*>(printStmt->expression.get())) {
                if (func->function_name == "graph") {
                    isGraphCall = true;
                } else if (func->function_name == "graph_3d") {
                    is3DGraphCall = true;
                } else if (func->function_name == "table") {
                    isTableCall = true;
                }
            }
        }

        if (isGraphCall && graphIndex < evalResult.graphs.size()) {
            // Generate graph HTML directly
            html << generateSingleGraphHtml(evalResult.graphs[graphIndex], graphIndex);
            graphIndex++;
        }

        if (is3DGraphCall && graph3DIndex < evalResult.graphs3d.size()) {
            // Generate 3D graph HTML directly
            html << generate3DGraphHtml(evalResult.graphs3d[graph3DIndex], graph3DIndex);
            graph3DIndex++;
        }

        if (isTableCall && tableIndex < evalResult.tables.size()) {
            // Generate table HTML directly
            html << generateSingleTableHtml(evalResult.tables[tableIndex], tableIndex);
            tableIndex++;
        }
    }

    return html.str();
}

std::string HtmlFormatter::generateSingleGraphHtml(const GraphData& graph, size_t index) {
    std::stringstream html;

    html << "<div class=\"graph-container\">\n";
    html << "<div class=\"graph-title\">" << escapeHtml(graph.title) << "</div>\n";
    html << "<div id=\"graph" << index << "\"></div>\n";
    html << "<script>\n";
    html << "(function() {\n";
    html << "    const data" << index << " = [";

    // Write data points
    for (size_t j = 0; j < graph.x_values.size(); ++j) {
        if (j > 0) html << ",";
        html << "{x:" << graph.x_values[j] << ",y:" << graph.y_values[j] << "}";
    }

    html << "];\n\n";
    html << "    const margin = {top: 20, right: 30, bottom: 40, left: 50};\n";
    html << "    const container = document.getElementById(\"graph" << index << "\");\n";
    html << "    const containerWidth = container.offsetWidth || 800;\n";
    html << "    const width = containerWidth - margin.left - margin.right;\n";
    html << "    const height = Math.min(300, containerWidth * 0.5) - margin.top - margin.bottom;\n\n";

    html << "    const svg = d3.select(\"#graph" << index << "\")\n";
    html << "        .append(\"svg\")\n";
    html << "        .attr(\"width\", \"100%\")\n";
    html << "        .attr(\"height\", height + margin.top + margin.bottom)\n";
    html << "        .attr(\"viewBox\", `0 0 ${containerWidth} ${height + margin.top + margin.bottom}`)\n";
    html << "        .attr(\"preserveAspectRatio\", \"xMidYMid meet\");\n\n";

    html << "    const g = svg.append(\"g\")\n";
    html << "        .attr(\"transform\", `translate(${margin.left},${margin.top})`);\n\n";

    html << "    const xScale = d3.scaleLinear()\n";
    html << "        .domain(d3.extent(data" << index << ", d => d.x))\n";
    html << "        .range([0, width]);\n\n";

    html << "    const yScale = d3.scaleLinear()\n";
    html << "        .domain(d3.extent(data" << index << ", d => d.y))\n";
    html << "        .range([height, 0]);\n\n";

    html << "    const line = d3.line()\n";
    html << "        .x(d => xScale(d.x))\n";
    html << "        .y(d => yScale(d.y));\n\n";

    // Add axes
    html << "    g.append(\"g\")\n";
    html << "        .attr(\"class\", \"axis\")\n";
    html << "        .attr(\"transform\", `translate(0,${height})`)\n";
    html << "        .call(d3.axisBottom(xScale));\n\n";

    html << "    g.append(\"g\")\n";
    html << "        .attr(\"class\", \"axis\")\n";
    html << "        .call(d3.axisLeft(yScale));\n\n";

    // Add line
    html << "    g.append(\"path\")\n";
    html << "        .datum(data" << index << ")\n";
    html << "        .attr(\"class\", \"line\")\n";
    html << "        .attr(\"d\", line);\n\n";

    // Add dots
    html << "    g.selectAll(\".dot\")\n";
    html << "        .data(data" << index << ")\n";
    html << "        .enter().append(\"circle\")\n";
    html << "        .attr(\"class\", \"dot\")\n";
    html << "        .attr(\"cx\", d => xScale(d.x))\n";
    html << "        .attr(\"cy\", d => yScale(d.y))\n";
    html << "        .attr(\"r\", 3);\n";

    html << "})();\n";
    html << "</script>\n";
    html << "</div>\n";

    return html.str();
}

} // namespace madola
