#pragma once
#include "ast.h"
#include "../tree_sitter_madola.h"
#include <memory>

namespace madola {

// Helper function to convert Tree-sitter position to SourceLocation
inline SourceLocation tsPointToSourceLocation(TSPoint point, uint32_t offset = 0) {
    return SourceLocation(point.row + 1, point.column + 1, offset); // Convert to 1-based indexing
}

class ASTBuilder {
public:
    explicit ASTBuilder();
    ProgramPtr buildProgram(const std::string& source);

private:
    TSParser* parser;

    StatementPtr buildStatement(TSNode node, const std::string& source);
    StatementPtr buildAssignmentStatement(TSNode node, const std::string& source);
    StatementPtr buildPrintStatement(TSNode node, const std::string& source);
    StatementPtr buildExpressionStatement(TSNode node, const std::string& source);
    StatementPtr buildCommentStatement(TSNode node, const std::string& source);
    StatementPtr buildFunctionDeclaration(TSNode node, const std::string& source);
    StatementPtr buildDecoratedFunctionDeclaration(TSNode node, const std::string& source);
    StatementPtr buildReturnStatement(TSNode node, const std::string& source);
    StatementPtr buildBreakStatement(TSNode node, const std::string& source);
    StatementPtr buildForStatement(TSNode node, const std::string& source);
    StatementPtr buildWhileStatement(TSNode node, const std::string& source);
    StatementPtr buildIfStatement(TSNode node, const std::string& source);
    StatementPtr buildDecoratedStatement(TSNode node, const std::string& source);
    StatementPtr buildSkipStatement(TSNode node, const std::string& source);
    StatementPtr buildImportStatement(TSNode node, const std::string& source);
    StatementPtr buildPiecewiseFunctionDeclaration(TSNode node, const std::string& source);
    StatementPtr buildHeadingStatement(TSNode node, const std::string& source);
    StatementPtr buildVersionStatement(TSNode node, const std::string& source);
    StatementPtr buildParagraphStatement(TSNode node, const std::string& source);
    ExpressionPtr buildExpression(TSNode node, const std::string& source);
    ExpressionPtr buildIdentifier(TSNode node, const std::string& source);
    ExpressionPtr buildStringLiteral(TSNode node, const std::string& source);
    ExpressionPtr buildNumber(TSNode node, const std::string& source);
    ExpressionPtr buildComplexNumber(TSNode node, const std::string& source);
    ExpressionPtr buildUnitExpression(TSNode node, const std::string& source);
    ExpressionPtr buildFunctionCall(TSNode node, const std::string& source);
    ExpressionPtr buildMethodCall(TSNode node, const std::string& source);
    ExpressionPtr buildBinaryExpression(TSNode node, const std::string& source);
    ExpressionPtr buildUnaryExpression(TSNode node, const std::string& source);
    ExpressionPtr buildRangeExpression(TSNode node, const std::string& source);
    ExpressionPtr buildPiecewiseExpression(TSNode node, const std::string& source);
    ExpressionPtr buildArrayExpression(TSNode node, const std::string& source);
    ExpressionPtr buildArrayAccess(TSNode node, const std::string& source);

    std::string getNodeText(TSNode node, const std::string& source);

    // Helper methods for position tracking
    SourceLocation getNodeStartPosition(TSNode node) const;
    SourceLocation getNodeEndPosition(TSNode node) const;
};

using ASTBuilderPtr = std::unique_ptr<ASTBuilder>;

} // namespace madola