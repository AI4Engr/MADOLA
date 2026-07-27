#include "html_formatter.h"
#include <sstream>

namespace madola {

// Escape a string for embedding inside a JSON string literal, matching the escaping
// used by wasm_interface.cpp's evaluate_madola so both entry points behave consistently.
static std::string escapeJsonForPayload(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (char c : input) {
        switch (c) {
            case '\\': result += "\\\\"; break;
            case '"':  result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:   result += c; break;
        }
    }
    return result;
}

// Render a Value's numeric magnitude and unit as separate fields, matching the
// {value, unit} shape formula records expose in the payload. Dimensionless doubles
// get an empty unit string. Arrays/matrices/records are not broken into value+unit
// here - the record's "latex" field (via formatStatementAsMath) is the fallback for those.
static void writeValueAndUnit(std::stringstream& json, const Value& value) {
    if (std::holds_alternative<double>(value)) {
        json << "\"value\":" << std::get<double>(value) << ",\"unit\":\"\"";
    } else if (std::holds_alternative<UnitValue>(value)) {
        const UnitValue& uv = std::get<UnitValue>(value);
        json << "\"value\":" << uv.value << ",\"unit\":\"" << escapeJsonForPayload(uv.unit) << "\"";
    } else {
        // Arrays/matrices/records/other: no single scalar value; consumers should rely on "latex".
        json << "\"value\":null,\"unit\":\"\"";
    }
}

std::string HtmlFormatter::buildTypstFormulaRecord(const Statement& renderStmt, const AssignmentStatement& assignment,
                                                    Evaluator& evaluator, const std::string& groupSuffix) {
    std::string latex = formatStatementAsMath(renderStmt, evaluator);
    if (latex.empty()) {
        // @hidden (or any decorator whose renderer intentionally returns "") - the
        // statement must be completely absent from the report, not emitted with
        // blank latex.
        return "";
    }
    std::stringstream record;
    record << "{\"type\":\"formula\",\"name\":\"" << escapeJsonForPayload(assignment.variable)
           << "\",\"latex\":\"" << escapeJsonForPayload(latex) << "\",";
    try {
        Value val = evaluator.getVariableValue(assignment.variable);
        writeValueAndUnit(record, val);
    } catch (const std::exception&) {
        // Not a plain variable lookup - the "latex" field above still carries the
        // full rendered formula.
        record << "\"value\":null,\"unit\":\"\"";
    }
    record << groupSuffix;
    record << "}";
    return record.str();
}

std::string HtmlFormatter::generateTypstPayload(const Program& program) {
    std::stringstream json;

    try {
        Evaluator evaluator;
        auto evalResult = evaluator.evaluate(program, "typst_payload");

        if (!evalResult.success) {
            json << "{\"success\":false,\"error\":\"" << escapeJsonForPayload(evalResult.error) << "\"}";
            return json.str();
        }

        collectUserDefinedNames(program);
        currentProgram = &program;

        json << "{\"success\":true,\"records\":[";
        bool first = true;
        size_t svgIndex = 0;
        bool skipNextStatement = false;

        for (size_t i = 0; i < program.statements.size(); ++i) {
            const Statement* stmt = program.statements[i].get();
            currentStatement = stmt;

            if (const auto* skipStmt = dynamic_cast<const SkipStatement*>(stmt)) {
                if (skipStmt->shouldSkipNext) skipNextStatement = true;
                continue;
            }
            if (skipNextStatement) {
                skipNextStatement = false;
                continue;
            }
            if (dynamic_cast<const CommentStatement*>(stmt)) continue;
            if (dynamic_cast<const VersionStatement*>(stmt)) continue;

            if (const auto* headingStmt = dynamic_cast<const HeadingStatement*>(stmt)) {
                for (const auto& para : headingStmt->textParagraphs) {
                    if (!first) json << ",";
                    first = false;
                    json << "{\"type\":\"heading\",\"level\":" << headingStmt->level
                         << ",\"text\":\"" << escapeJsonForPayload(para) << "\"}";
                }
                continue;
            }

            if (const auto* paragraphStmt = dynamic_cast<const ParagraphStatement*>(stmt)) {
                for (const auto& line : paragraphStmt->textLines) {
                    if (!first) json << ",";
                    first = false;
                    json << "{\"type\":\"paragraph\",\"text\":\"" << escapeJsonForPayload(line) << "\"}";
                }
                continue;
            }

            // Parameterized decorators (@table2/@pair2/...) and @layout NxM grids group
            // this statement with the next N-1 (parameterized) or rows*cols-1 (layout)
            // statements. Mirror generateOrderedContent's scan-forward-and-skip so the
            // grouped statements aren't ALSO picked off individually by the plain
            // AssignmentStatement branch below (which would flatten/duplicate them) -
            // emit each as its own formula record, tagged with group metadata so the
            // private typst converter can still lay them out as a table/grid.
            if (const auto* decoratedStmt = dynamic_cast<const DecoratedStatement*>(stmt)) {
                const Decorator* paramDec = nullptr;
                for (const auto& dec : decoratedStmt->decorators) {
                    if (dec.isParameterized()) { paramDec = &dec; break; }
                }

                if (paramDec != nullptr) {
                    int n = paramDec->parameter;
                    size_t stmtIdx = i;
                    int found = 0;
                    int groupIndex = 0;
                    while (stmtIdx < program.statements.size() && found < n) {
                        const Statement* groupStmt = program.statements[stmtIdx].get();
                        const Statement* groupInner = groupStmt;
                        if (const auto* groupDecorated = dynamic_cast<const DecoratedStatement*>(groupStmt)) {
                            if (groupDecorated->statement) groupInner = groupDecorated->statement.get();
                        }
                        if (const auto* groupAssignment = dynamic_cast<const AssignmentStatement*>(groupInner)) {
                            std::string suffix = ",\"group\":{\"type\":\"" + paramDec->name + "\",\"index\":" +
                                                  std::to_string(groupIndex) + ",\"size\":" + std::to_string(n) + "}";
                            std::string record = buildTypstFormulaRecord(*groupStmt, *groupAssignment, evaluator, suffix);
                            if (!record.empty()) {
                                if (!first) json << ",";
                                json << record;
                                first = false;
                            }
                            // @hidden inside a group still counts toward the group's N
                            // members (it was consumed from the source), it just emits
                            // no record.
                            found++; groupIndex++;
                        }
                        stmtIdx++;
                    }
                    i = stmtIdx - 1; // loop increment will move past the consumed group
                    continue;
                }

                if (decoratedStmt->hasLayoutDecorator()) {
                    const Decorator* layoutDec = decoratedStmt->getLayoutDecorator();
                    int totalElements = layoutDec->rows * layoutDec->cols;
                    if (i + totalElements - 1 < program.statements.size()) {
                        for (int elementIndex = 0; elementIndex < totalElements; ++elementIndex) {
                            const Statement* cellStmt = program.statements[i + elementIndex].get();
                            const Statement* cellInner = cellStmt;
                            if (const auto* cellDecorated = dynamic_cast<const DecoratedStatement*>(cellStmt)) {
                                if (cellDecorated->statement) cellInner = cellDecorated->statement.get();
                            }
                            if (const auto* cellAssignment = dynamic_cast<const AssignmentStatement*>(cellInner)) {
                                std::string suffix = ",\"group\":{\"type\":\"layout\",\"rows\":" +
                                                      std::to_string(layoutDec->rows) + ",\"cols\":" +
                                                      std::to_string(layoutDec->cols) + ",\"index\":" +
                                                      std::to_string(elementIndex) + "}";
                                std::string record = buildTypstFormulaRecord(*cellStmt, *cellAssignment, evaluator, suffix);
                                if (!record.empty()) {
                                    if (!first) json << ",";
                                    json << record;
                                    first = false;
                                }
                            }
                        }
                        i += totalElements - 1;
                        continue;
                    }
                }
            }

            // Unwrap only far enough to test the statement's shape (AssignmentStatement
            // vs svg() call) - the DECORATED statement (not the unwrapped inner) is what
            // gets passed to formatStatementAsMath below, so @hidden/@result/@eval/@resolve
            // render exactly as they do in the HTML path (formatStatementAsMath's own
            // decorator dispatch requires its argument to BE the DecoratedStatement).
            const Statement* inner = stmt;
            if (const auto* decorated = dynamic_cast<const DecoratedStatement*>(stmt)) {
                if (decorated->statement) inner = decorated->statement.get();
            }

            if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(inner)) {
                std::string record = buildTypstFormulaRecord(*stmt, *assignment, evaluator);
                if (!record.empty()) {
                    if (!first) json << ",";
                    json << record;
                    first = false;
                }
                continue;
            }

            if (const auto* arrayAssignment = dynamic_cast<const ArrayAssignmentStatement*>(inner)) {
                // arr[i] := expr - a distinct AST node from AssignmentStatement (not a
                // subclass), so it needs its own branch or it silently vanishes from the
                // payload. formatStatementAsMath has no ArrayAssignmentStatement case, so
                // render the "name[index] = expr" form directly here; a single array
                // element doesn't map to a single Value via getVariableValue (that returns
                // the whole array), so value/unit are left null - "latex" is authoritative.
                if (!first) json << ",";
                first = false;
                std::string indexLatex = formatExpressionAsMath(*arrayAssignment->index, evaluator);
                std::string exprLatex = formatExpressionAsMath(*arrayAssignment->expression, evaluator);
                std::string name = arrayAssignment->arrayName + "[" + indexLatex + "]";
                std::string latex = convertToMathJax(arrayAssignment->arrayName) + "_{" + indexLatex + "} = " + exprLatex;
                json << "{\"type\":\"formula\",\"name\":\"" << escapeJsonForPayload(name)
                     << "\",\"latex\":\"" << escapeJsonForPayload(latex) << "\",\"value\":null,\"unit\":\"\"}";
                continue;
            }

            // svg() calls, either as a bare expression statement or wrapped in print().
            bool isSvgCall = false;
            if (const auto* exprStmt = dynamic_cast<const ExpressionStatement*>(inner)) {
                if (const auto* func = dynamic_cast<const FunctionCall*>(exprStmt->expression.get())) {
                    isSvgCall = (func->function_name == "svg");
                }
            } else if (const auto* printStmt = dynamic_cast<const PrintStatement*>(inner)) {
                if (const auto* func = dynamic_cast<const FunctionCall*>(printStmt->expression.get())) {
                    isSvgCall = (func->function_name == "svg");
                }
            }

            if (isSvgCall && svgIndex < evalResult.svgs.size()) {
                if (!first) json << ",";
                first = false;
                std::string svgHtml = generateSvgHtml(evalResult.svgs[svgIndex], svgIndex);
                json << "{\"type\":\"svg\",\"title\":\"" << escapeJsonForPayload(evalResult.svgs[svgIndex].title)
                     << "\",\"svg\":\"" << escapeJsonForPayload(svgHtml) << "\"}";
                svgIndex++;
                continue;
            }

            // Everything else (graph/table/loops/functions/etc.) is left for a later
            // payload iteration - not needed for the first formula/heading/paragraph/svg pass.
        }

        json << "]}";
        return json.str();
    } catch (const std::exception& e) {
        std::stringstream errJson;
        errJson << "{\"success\":false,\"error\":\"" << escapeJsonForPayload(e.what()) << "\"}";
        return errJson.str();
    }
}

} // namespace madola
