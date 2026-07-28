#pragma once
#include "../ast/ast.h"
#include "../evaluator/evaluator.h"
#include <string>
#include <set>
#include <vector>
#include <sstream>

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

    // Neutral structured export for the private Typst report generator (app/js/typst).
    // Evaluates the program and walks statements in source order, emitting one JSON
    // record per heading/paragraph/formula/svg() call. Contains no layout/style
    // decisions (no fonts, colors, page setup) - that know-how lives in the private
    // mda-to-typ.js converter. Returns a JSON array as a string; on parse/eval failure
    // returns a JSON object {"success":false,"error":"..."}.
    std::string generateTypstPayload(const Program& program);

    // Format a value as inline LaTeX, identical to how @result / assignment blocks
    // render it initially. Exposed so the interactive slider path (wasm_interface's
    // svg_eval_curve) can produce a live-updated value in the SAME format as the
    // first render — avoiding a display mismatch between load and drag.
    static std::string formatValueAsMathPublic(const Value& value);

private:
    std::string generateHtmlHeader(const std::string& title = "MADOLA Output");
    std::string generateHtmlFooter();
    std::string formatStatement(const Statement& stmt);
    std::string formatStatementWithDepth(const Statement& stmt, int depth = 0);
    std::string formatStatementWithDepth(const Statement& stmt, int depth, bool inFunction);
    std::string formatAssignment(const AssignmentStatement& stmt);
    std::string formatRecordFieldAssignment(const RecordFieldAssignmentStatement& stmt);
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
    std::string generateSvgHtml(const SvgData& svg, size_t index);
    std::string generateOrderedContent(const Program& program, Evaluator& evaluator, const Evaluator::EvaluationResult& evalResult);
    std::string generateMathContent(const Program& program, Evaluator& evaluator);
    std::string formatStatementAsMath(const Statement& stmt, Evaluator& evaluator);
    // Builds one {"type":"formula",...} JSON record for `assignment`, using `renderStmt`
    // (the statement passed to formatStatementAsMath - may be a still-decorated wrapper
    // so @hidden/@result/@eval/@resolve render correctly) for the latex. Returns an
    // empty string if the rendered latex is empty (the convention formatStatementAsMath
    // uses to signal "@hidden, suppress this record") - callers must check for empty
    // before writing a preceding comma, since a suppressed record contributes nothing
    // to the JSON array. `groupSuffix` is an optional already-escaped JSON tail (e.g.
    // `,"group":{...}`) spliced in before the closing brace, used to tag
    // @table2/@layout-grouped formulas.
    std::string buildTypstFormulaRecord(const Statement& renderStmt, const AssignmentStatement& assignment,
                                         Evaluator& evaluator, const std::string& groupSuffix = "");
    std::string formatStatementAsMathInFunction(const Statement& stmt, Evaluator& evaluator, int depth = 0);
    std::string formatExpressionAsMath(const Expression& expr, Evaluator& evaluator);
    std::string formatExpressionWithValuesAsMath(const Expression& expr, Evaluator& evaluator);
    std::string formatValueAsMath(const Value& value);
    std::string formatPiecewiseExpressionAsMath(const PiecewiseExpression& expr, Evaluator& evaluator);
    std::string escapeHtml(const std::string& text);
    std::string parseMarkdownFormatting(const std::string& text);
    std::string convertToMathJax(const std::string& text);

    // Collect every name the user introduces (assignment targets, array names,
    // loop variables, function names/parameters, imports) so the math formatter
    // can tell a genuine variable from a bare unit identifier that merely shares
    // a unit's name. Populated once per program.
    void collectUserDefinedNames(const Program& program);
    void collectNamesInStatements(const std::vector<StatementPtr>& statements);
    bool isUnitIdentifier(const std::string& name) const;

    // Context for @eval statement formatting
    const Program* currentProgram = nullptr;
    const Statement* currentStatement = nullptr;

    // Names the user defined anywhere in the current program (see above).
    std::set<std::string> userDefinedNames;
};

using HtmlFormatterPtr = std::unique_ptr<HtmlFormatter>;

} // namespace madola