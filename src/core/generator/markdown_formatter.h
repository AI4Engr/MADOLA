#pragma once
#include "../ast/ast.h"
#include "../evaluator/evaluator.h"
#include <string>

namespace madola {

class MarkdownFormatter {
public:
    explicit MarkdownFormatter();

    struct FormatResult {
        std::string markdown;
        bool success;
        std::string error;
    };

    FormatResult formatProgram(const Program& program);
    FormatResult formatProgramWithExecution(const Program& program);

private:
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
    std::string formatMethodCall(const MethodCall& expr);
    std::string formatVariableName(const std::string& varName);
    std::string formatRangeExpression(const RangeExpression& expr);
    std::string formatPiecewiseExpression(const PiecewiseExpression& expr);
    std::string formatArrayExpression(const ArrayExpression& expr);
    std::string formatArrayAccess(const ArrayAccess& expr);
    std::string formatIfWithDepth(const IfStatement& stmt, int depth = 0, bool inFunction = false);
    std::string formatExpressionWithValues(const Expression& expr, Evaluator& evaluator);
    std::string formatDecoratedStatementContent(const DecoratedStatement& stmt, Evaluator& evaluator);
    std::string formatStatementForArray(const Statement& stmt, Evaluator& evaluator);
};

using MarkdownFormatterPtr = std::unique_ptr<MarkdownFormatter>;

} // namespace madola