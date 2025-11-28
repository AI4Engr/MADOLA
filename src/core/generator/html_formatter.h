#pragma once
#include "../ast/ast.h"
#include "../evaluator/evaluator.h"
#include <string>

namespace madola {

class HtmlFormatter {
public:
    explicit HtmlFormatter();

    struct FormatResult {
        std::string html;
        bool success;
        std::string error;
    };

    FormatResult formatProgram(const Program& program);
    FormatResult formatProgramWithExecution(const Program& program);

private:
    std::string loadCssFile();
    std::string generateHtmlHeader(const std::string& title = "MADOLA Output");
    std::string generateHtmlFooter();
    std::string formatStatement(const Statement& stmt);
    std::string formatStatementWithDepth(const Statement& stmt, int depth = 0);
    std::string formatStatementWithDepth(const Statement& stmt, int depth, bool inFunction);
    std::string formatAssignment(const AssignmentStatement& stmt);
    std::string formatPrint(const PrintStatement& stmt);
    std::string formatExpressionStatement(const ExpressionStatement& stmt);
    std::string formatFunctionDeclaration(const FunctionDeclaration& stmt);
    std::string formatPiecewiseFunctionDeclaration(const PiecewiseFunctionDeclaration& stmt);
    std::string formatFor(const ForStatement& stmt);
    std::string formatWhile(const WhileStatement& stmt);
    std::string formatIf(const IfStatement& stmt);
    std::string formatExpression(const Expression& expr);
    std::string formatBinaryExpression(const BinaryExpression& expr);
    std::string formatUnaryExpression(const UnaryExpression& expr);
    std::string formatFunctionCall(const FunctionCall& expr);
    std::string formatVariableName(const std::string& varName);
    std::string formatRangeExpression(const RangeExpression& expr);
    std::string formatPiecewiseExpression(const PiecewiseExpression& expr);
    std::string formatArrayExpression(const ArrayExpression& expr);
    std::string formatArrayAccess(const ArrayAccess& expr);
    std::string formatIfWithDepth(const IfStatement& stmt, int depth = 0, bool inFunction = false);
    std::string formatExpressionWithValues(const Expression& expr, Evaluator& evaluator);
    std::string formatDecoratedStatementContent(const DecoratedStatement& stmt, Evaluator& evaluator);
    std::string formatStatementForArray(const Statement& stmt, Evaluator& evaluator);
    std::string generateGraphHtml(const std::vector<GraphData>& graphs);
    std::string generateSingleGraphHtml(const GraphData& graph, size_t index);
    std::string generate3DGraphHtml(const Graph3DData& graph, size_t index);
    std::string generateTableHtml(const std::vector<TableData>& tables);
    std::string generateSingleTableHtml(const TableData& table, size_t index);
    std::string generateOrderedContent(const Program& program, Evaluator& evaluator, const Evaluator::EvaluationResult& evalResult);
    std::string generateMathContent(const Program& program, Evaluator& evaluator);
    std::string formatStatementAsMath(const Statement& stmt, Evaluator& evaluator);
    std::string formatStatementAsMathInFunction(const Statement& stmt, Evaluator& evaluator, int depth = 0);
    std::string formatExpressionAsMath(const Expression& expr, Evaluator& evaluator);
    std::string formatExpressionWithValuesAsMath(const Expression& expr, Evaluator& evaluator);
    std::string formatValueAsMath(const Value& value);
    std::string formatPiecewiseExpressionAsMath(const PiecewiseExpression& expr, Evaluator& evaluator);
    std::string escapeHtml(const std::string& text);
    std::string parseMarkdownFormatting(const std::string& text);
    std::string convertToMathJax(const std::string& text);

    // Context for @eval statement formatting
    const Program* currentProgram = nullptr;
    const Statement* currentStatement = nullptr;
};

using HtmlFormatterPtr = std::unique_ptr<HtmlFormatter>;

} // namespace madola