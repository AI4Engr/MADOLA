#include "core/ast/ast.h"
#include "core/ast/ast_builder.h"
#include "core/evaluator/evaluator.h"
#include "core/generator/markdown_formatter.h"
#include "core/generator/html_formatter.h"
#include <emscripten.h>
#include <string>
#include <memory>

using namespace madola;

// Global instances for WASM interface
static std::unique_ptr<MarkdownFormatter> g_markdown_formatter;
static std::unique_ptr<HtmlFormatter> g_html_formatter;
static std::unique_ptr<ASTBuilder> g_ast_builder;

// Escape a string for embedding inside a JSON string literal. Needed anywhere
// user-controlled text (a curve id from .mda source, an exception message) is
// interpolated into a hand-built JSON response - without this, a value containing
// a quote or backslash produces malformed/injectable JSON.
static std::string escapeJsonString(const std::string& input) {
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

// Minimal hand-rolled JSON parser, deliberately narrow: it only understands the
// exact shape register_lookup_table's caller sends — a flat object of flat objects,
// e.g. {"W16X59": {"Ix": 954, "size": "W16X59"}, "W16X67": {...}, ...}. No arrays,
// no nesting beyond two levels, no unicode escapes. Not a general JSON parser — the
// input shape is fully controlled by the (private, app-side) caller, so this narrow
// scope avoids vendoring a full JSON library into the WASM binary for one call site.
namespace {

void skipJsonWhitespace(const std::string& s, size_t& pos) {
    while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) pos++;
}

std::string parseJsonStringLiteral(const std::string& s, size_t& pos) {
    if (pos >= s.size() || s[pos] != '"') {
        throw std::runtime_error("Expected '\"' at position " + std::to_string(pos));
    }
    pos++; // skip opening quote
    std::string result;
    while (pos < s.size() && s[pos] != '"') {
        if (s[pos] == '\\' && pos + 1 < s.size()) {
            char next = s[pos + 1];
            switch (next) {
                case '"':  result += '"';  break;
                case '\\': result += '\\'; break;
                case '/':  result += '/';  break;
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                default:   result += next; break;
            }
            pos += 2;
        } else {
            result += s[pos];
            pos++;
        }
    }
    if (pos >= s.size()) {
        throw std::runtime_error("Unterminated string literal");
    }
    pos++; // skip closing quote
    return result;
}

// Parses a JSON value that is either a string, or a number (returned as a double
// wrapped in madola::Value), for use as a single field's value inside a row object.
madola::Value parseJsonScalarValue(const std::string& s, size_t& pos) {
    skipJsonWhitespace(s, pos);
    if (pos < s.size() && s[pos] == '"') {
        return madola::Value(parseJsonStringLiteral(s, pos));
    }
    // Number: read a contiguous run of digits/sign/decimal-point/exponent chars.
    size_t start = pos;
    while (pos < s.size() && (std::isdigit(static_cast<unsigned char>(s[pos])) ||
           s[pos] == '-' || s[pos] == '+' || s[pos] == '.' || s[pos] == 'e' || s[pos] == 'E')) {
        pos++;
    }
    if (pos == start) {
        throw std::runtime_error("Expected string or number at position " + std::to_string(pos));
    }
    return madola::Value(std::stod(s.substr(start, pos - start)));
}

// Parses one row object: {"field": value, "field2": value2, ...}
madola::RecordValue parseJsonRowObject(const std::string& s, size_t& pos) {
    skipJsonWhitespace(s, pos);
    if (pos >= s.size() || s[pos] != '{') {
        throw std::runtime_error("Expected '{' at position " + std::to_string(pos));
    }
    pos++; // skip '{'
    madola::RecordValue row;
    skipJsonWhitespace(s, pos);
    if (pos < s.size() && s[pos] == '}') {
        pos++;
        return row;
    }
    while (true) {
        skipJsonWhitespace(s, pos);
        std::string fieldName = parseJsonStringLiteral(s, pos);
        skipJsonWhitespace(s, pos);
        if (pos >= s.size() || s[pos] != ':') {
            throw std::runtime_error("Expected ':' at position " + std::to_string(pos));
        }
        pos++; // skip ':'
        madola::Value fieldValue = parseJsonScalarValue(s, pos);
        (*row.fields)[fieldName] = fieldValue;

        skipJsonWhitespace(s, pos);
        if (pos < s.size() && s[pos] == ',') {
            pos++;
            continue;
        }
        break;
    }
    skipJsonWhitespace(s, pos);
    if (pos >= s.size() || s[pos] != '}') {
        throw std::runtime_error("Expected '}' at position " + std::to_string(pos));
    }
    pos++; // skip '}'
    return row;
}

// Parses the top-level table object: {"ROWKEY": {row...}, "ROWKEY2": {row...}, ...}
std::map<std::string, madola::RecordValue> parseLookupTableJson(const std::string& s) {
    size_t pos = 0;
    skipJsonWhitespace(s, pos);
    if (pos >= s.size() || s[pos] != '{') {
        throw std::runtime_error("Expected '{' at position " + std::to_string(pos));
    }
    pos++; // skip '{'
    std::map<std::string, madola::RecordValue> rows;
    skipJsonWhitespace(s, pos);
    if (pos < s.size() && s[pos] == '}') {
        return rows;
    }
    while (true) {
        skipJsonWhitespace(s, pos);
        std::string rowKey = parseJsonStringLiteral(s, pos);
        skipJsonWhitespace(s, pos);
        if (pos >= s.size() || s[pos] != ':') {
            throw std::runtime_error("Expected ':' at position " + std::to_string(pos));
        }
        pos++; // skip ':'
        madola::RecordValue row = parseJsonRowObject(s, pos);
        row.displayLabel = rowKey;
        rows[rowKey] = row;

        skipJsonWhitespace(s, pos);
        if (pos < s.size() && s[pos] == ',') {
            pos++;
            continue;
        }
        break;
    }
    skipJsonWhitespace(s, pos);
    if (pos >= s.size() || s[pos] != '}') {
        throw std::runtime_error("Expected '}' at position " + std::to_string(pos));
    }
    return rows;
}

} // namespace

extern "C" {

// Initialize MADOLA WASM module
EMSCRIPTEN_KEEPALIVE
int init_madola() {
    try {
        g_markdown_formatter = std::make_unique<MarkdownFormatter>();
        g_html_formatter = std::make_unique<HtmlFormatter>();
        g_ast_builder = std::make_unique<ASTBuilder>();
        return 1; // Success
    } catch (...) {
        return 0; // Failure
    }
}

// Parse MADOLA source code and return AST (as string)
EMSCRIPTEN_KEEPALIVE
char* parse_madola(const char* source) {
    try {
        if (!g_ast_builder) {
            init_madola();
        }

        // Use Tree-sitter parser to build AST from source
        auto program = g_ast_builder->buildProgram(std::string(source));
        if (!program) {
            return nullptr;
        }

        std::string result = program->toString();

        // Allocate memory for result (caller must free)
        size_t len = result.length();
        char* output = new char[len + 1];
        memcpy(output, result.c_str(), len);
        output[len] = '\0';
        return output;
    } catch (...) {
        return nullptr;
    }
}

// Evaluate MADOLA program and return JSON result
EMSCRIPTEN_KEEPALIVE
char* evaluate_madola(const char* source) {
    try {
        if (!g_ast_builder) {
            init_madola();
        }

        // Use Tree-sitter parser to build AST from source
        auto program = g_ast_builder->buildProgram(std::string(source));
        if (!program) {
            std::string error_json = "{\"success\":false,\"outputs\":[],\"cppFiles\":[],\"graphs\":[],\"graphs3d\":[],\"wasmFiles\":[],\"error\":\"Failed to parse source code\"}";
            size_t len = error_json.length();
            char* output = new char[len + 1];
            memcpy(output, error_json.c_str(), len);
            output[len] = '\0';
            return output;
        }

        Evaluator evaluator;
        auto result = evaluator.evaluate(*program);

        // Create JSON response
        std::string json = "{\"success\":" + std::string(result.success ? "true" : "false");
        json += ",\"outputs\":[";

        for (size_t i = 0; i < result.outputs.size(); ++i) {
            if (i > 0) json += ",";
            json += "\"" + escapeJsonString(result.outputs[i]) + "\"";
        }

        json += "],\"cppFiles\":[";

        for (size_t i = 0; i < result.cppFiles.size(); ++i) {
            if (i > 0) json += ",";
            json += "{\"filename\":\"" + result.cppFiles[i].filename + "\",";

            // Escape content for JSON
            std::string escapedContent = result.cppFiles[i].content;
            // Replace backslashes and quotes for JSON
            size_t pos = 0;
            while ((pos = escapedContent.find("\\", pos)) != std::string::npos) {
                escapedContent.replace(pos, 1, "\\\\");
                pos += 2;
            }
            pos = 0;
            while ((pos = escapedContent.find("\"", pos)) != std::string::npos) {
                escapedContent.replace(pos, 1, "\\\"");
                pos += 2;
            }
            pos = 0;
            while ((pos = escapedContent.find("\n", pos)) != std::string::npos) {
                escapedContent.replace(pos, 1, "\\n");
                pos += 2;
            }

            json += "\"content\":\"" + escapedContent + "\"}";
        }

        json += "],\"graphs\":[";

        for (size_t i = 0; i < result.graphs.size(); ++i) {
            if (i > 0) json += ",";
            json += "{\"title\":\"" + result.graphs[i].title + "\",";
            json += "\"x_values\":[";

            for (size_t j = 0; j < result.graphs[i].x_values.size(); ++j) {
                if (j > 0) json += ",";
                json += std::to_string(result.graphs[i].x_values[j]);
            }

            json += "],\"y_values\":[";

            for (size_t j = 0; j < result.graphs[i].y_values.size(); ++j) {
                if (j > 0) json += ",";
                json += std::to_string(result.graphs[i].y_values[j]);
            }

            json += "]}";
        }

        json += "],\"graphs3d\":[";

        for (size_t i = 0; i < result.graphs3d.size(); ++i) {
            if (i > 0) json += ",";
            json += "{\"title\":\"" + result.graphs3d[i].title + "\",";
            json += "\"type\":\"" + result.graphs3d[i].type + "\",";
            json += "\"width\":" + std::to_string(result.graphs3d[i].width) + ",";
            json += "\"height\":" + std::to_string(result.graphs3d[i].height) + ",";
            json += "\"depth\":" + std::to_string(result.graphs3d[i].depth) + ",";
            json += "\"hole_width\":" + std::to_string(result.graphs3d[i].hole_width) + ",";
            json += "\"hole_height\":" + std::to_string(result.graphs3d[i].hole_height) + ",";
            json += "\"hole_depth\":" + std::to_string(result.graphs3d[i].hole_depth) + "}";
        }

        json += "],\"wasmFiles\":[";

        for (size_t i = 0; i < result.wasmFiles.size(); ++i) {
            if (i > 0) json += ",";
            json += "{\"functionName\":\"" + result.wasmFiles[i].functionName + "\",";
            json += "\"cppSourcePath\":\"" + result.wasmFiles[i].cppSourcePath + "\",";
            json += "\"wasmPath\":\"" + result.wasmFiles[i].wasmPath + "\",";
            json += "\"jsWrapperPath\":\"" + result.wasmFiles[i].jsWrapperPath + "\",";
            json += "\"compilationSuccess\":" + std::string(result.wasmFiles[i].compilationSuccess ? "true" : "false") + ",";

            // Escape cppContent for JSON
            std::string escapedCppContent = result.wasmFiles[i].cppContent;
            size_t pos = 0;
            while ((pos = escapedCppContent.find("\\", pos)) != std::string::npos) {
                escapedCppContent.replace(pos, 1, "\\\\");
                pos += 2;
            }
            pos = 0;
            while ((pos = escapedCppContent.find("\"", pos)) != std::string::npos) {
                escapedCppContent.replace(pos, 1, "\\\"");
                pos += 2;
            }
            pos = 0;
            while ((pos = escapedCppContent.find("\n", pos)) != std::string::npos) {
                escapedCppContent.replace(pos, 1, "\\n");
                pos += 2;
            }

            json += "\"cppContent\":\"" + escapedCppContent + "\",";
            json += "\"errorMessage\":\"" + escapeJsonString(result.wasmFiles[i].errorMessage) + "\"}";
        }

        json += "],\"error\":\"" + escapeJsonString(result.error) + "\"}";

        size_t len = json.length();
        char* output = new char[len + 1];
        memcpy(output, json.c_str(), len);
        output[len] = '\0';
        return output;
    } catch (const std::exception& e) {
        // Catch standard exceptions and return error JSON
        std::string error_msg = e.what();
        // Escape quotes in error message
        size_t pos = 0;
        while ((pos = error_msg.find("\"", pos)) != std::string::npos) {
            error_msg.replace(pos, 1, "\\\"");
            pos += 2;
        }
        std::string error_json = "{\"success\":false,\"outputs\":[],\"cppFiles\":[],\"graphs\":[],\"graphs3d\":[],\"wasmFiles\":[],\"error\":\"Exception: " + error_msg + "\"}";
        size_t len = error_json.length();
        char* output = new char[len + 1];
        memcpy(output, error_json.c_str(), len);
        output[len] = '\0';
        return output;
    } catch (...) {
        // Catch all other exceptions
        std::string error_json = "{\"success\":false,\"outputs\":[],\"cppFiles\":[],\"graphs\":[],\"graphs3d\":[],\"wasmFiles\":[],\"error\":\"Unknown exception occurred\"}";
        size_t len = error_json.length();
        char* output = new char[len + 1];
        memcpy(output, error_json.c_str(), len);
        output[len] = '\0';
        return output;
    }
}

// Format MADOLA program as markdown
EMSCRIPTEN_KEEPALIVE
char* format_madola(const char* source, int with_execution) {
    try {
        if (!g_markdown_formatter || !g_ast_builder) {
            init_madola();
        }

        // Use Tree-sitter parser to build AST from source
        auto program = g_ast_builder->buildProgram(std::string(source));
        if (!program) {
            return nullptr;
        }

        MarkdownFormatter::FormatResult result;
        if (with_execution) {
            result = g_markdown_formatter->formatProgramWithExecution(*program);
        } else {
            result = g_markdown_formatter->formatProgram(*program);
        }

        if (result.success) {
            size_t len = result.markdown.length();
            char* output = new char[len + 1];
            memcpy(output, result.markdown.c_str(), len);
            output[len] = '\0';
            return output;
        } else {
            return nullptr;
        }
    } catch (...) {
        return nullptr;
    }
}

// Format MADOLA program as HTML
EMSCRIPTEN_KEEPALIVE
char* format_madola_html(const char* source, int with_execution) {
    try {
        if (!g_html_formatter || !g_ast_builder) {
            init_madola();
        }

        // Use Tree-sitter parser to build AST from source
        auto program = g_ast_builder->buildProgram(std::string(source));
        if (!program) {
            return nullptr;
        }

        HtmlFormatter::FormatResult result;
        if (with_execution) {
            result = g_html_formatter->formatProgramWithExecution(*program);
        } else {
            result = g_html_formatter->formatProgram(*program);
        }

        if (result.success) {
            size_t len = result.html.length();
            char* output = new char[len + 1];
            memcpy(output, result.html.c_str(), len);
            output[len] = '\0';
            return output;
        } else {
            return nullptr;
        }
    } catch (...) {
        return nullptr;
    }
}

// Export a neutral structured JSON payload (heading/paragraph/formula/svg records) for
// the private app-side Typst report generator to consume. Contains no page/report
// layout decisions - see HtmlFormatter::generateTypstPayload for the exact contract.
EMSCRIPTEN_KEEPALIVE
char* format_madola_typst_payload(const char* source) {
    try {
        if (!g_html_formatter || !g_ast_builder) {
            init_madola();
        }

        auto program = g_ast_builder->buildProgram(std::string(source));
        if (!program) {
            std::string error_json = "{\"success\":false,\"error\":\"Failed to parse source code\"}";
            size_t len = error_json.length();
            char* output = new char[len + 1];
            memcpy(output, error_json.c_str(), len);
            output[len] = '\0';
            return output;
        }

        std::string json = g_html_formatter->generateTypstPayload(*program);
        size_t len = json.length();
        char* output = new char[len + 1];
        memcpy(output, json.c_str(), len);
        output[len] = '\0';
        return output;
    } catch (...) {
        return nullptr;
    }
}

// Recompute a single interactive svg() curve after rebinding one variable (e.g. a
// dragged @input value), without re-running/re-rendering the whole program. Re-parses
// source (fresh AST for this call only), evaluates once to populate bindings and
// collect svg data, then resamples just the named curve. Returns a small JSON object;
// caller must free_result() it.
//
// result_var (optional, may be empty): a displayed variable name whose fresh value
// should be returned alongside the curve. Because the caller has already rebound the
// dragged @input's literal in `source`, the single evaluate() pass above already holds
// that variable's updated value — so we read it from the SAME evaluator instead of
// re-parsing/re-evaluating the source a second time. The value is formatted with the
// same LaTeX routine the initial render uses, so a dragged value matches the loaded one.
EMSCRIPTEN_KEEPALIVE
char* svg_eval_curve(const char* source, const char* curve_id,
                     const char* param_name, double param_value,
                     const char* result_var) {
    std::string json;
    try {
        if (!g_ast_builder) {
            init_madola();
        }

        auto program = g_ast_builder->buildProgram(std::string(source));
        if (!program) {
            json = "{\"success\":false,\"error\":\"Failed to parse source code\"}";
        } else {
            Evaluator evaluator;
            auto result = evaluator.evaluate(*program);

            auto d = evaluator.resampleSvgCurve(*program, result.svgs,
                                                 std::string(curve_id),
                                                 std::string(param_name), param_value);
            if (d) {
                json = "{\"success\":true,\"curveId\":\"" + escapeJsonString(curve_id) +
                       "\",\"d\":\"" + escapeJsonString(*d) + "\"";

                // Optionally include a displayed variable's fresh value, formatted the
                // same way the initial render does, from this same evaluate() pass.
                std::string resultVarName = result_var ? std::string(result_var) : std::string();
                if (!resultVarName.empty()) {
                    try {
                        Value v = evaluator.getVariableValue(resultVarName);
                        std::string latex = HtmlFormatter::formatValueAsMathPublic(v);
                        json += ",\"resultVar\":\"" + escapeJsonString(resultVarName) +
                                "\",\"resultValue\":\"" + escapeJsonString(latex) + "\"";
                    } catch (const std::exception&) {
                        // Variable missing/uncomputable: omit it, curve update still succeeds.
                    }
                }

                json += "}";
            } else {
                json = "{\"success\":false,\"error\":\"curve not found: " +
                       escapeJsonString(curve_id) + "\"}";
            }
        }
    } catch (const std::exception& e) {
        json = "{\"success\":false,\"error\":\"" + escapeJsonString(e.what()) + "\"}";
    } catch (...) {
        json = "{\"success\":false,\"error\":\"unknown error\"}";
    }

    size_t len = json.length();
    char* output = new char[len + 1];
    memcpy(output, json.c_str(), len);
    output[len] = '\0';
    return output;
}

// Recompute a single named variable's final value after a full evaluate() pass over
// source (the caller is expected to have already rebound the dragged @input's literal
// in that source string, e.g. via a simple text replace). Used to keep a `@result`
// block (or any other displayed value) in sync with a slider drag without re-rendering
// the whole HTML document. Returns a small JSON object; caller must free_result() it.
EMSCRIPTEN_KEEPALIVE
char* eval_named_value(const char* source, const char* var_name) {
    std::string json;
    try {
        if (!g_ast_builder) {
            init_madola();
        }

        auto program = g_ast_builder->buildProgram(std::string(source));
        if (!program) {
            json = "{\"success\":false,\"error\":\"Failed to parse source code\"}";
        } else {
            Evaluator evaluator;
            evaluator.evaluate(*program);

            Value value = evaluator.getVariableValue(std::string(var_name));
            // Format as LaTeX identical to the initial render, so a value swapped in
            // during a slider drag matches how it first appeared.
            std::string valueStr = HtmlFormatter::formatValueAsMathPublic(value);
            json = "{\"success\":true,\"name\":\"" + escapeJsonString(var_name) +
                   "\",\"value\":\"" + escapeJsonString(valueStr) + "\"}";
        }
    } catch (const std::exception& e) {
        json = "{\"success\":false,\"error\":\"" + escapeJsonString(e.what()) + "\"}";
    } catch (...) {
        json = "{\"success\":false,\"error\":\"unknown error\"}";
    }

    size_t len = json.length();
    char* output = new char[len + 1];
    memcpy(output, json.c_str(), len);
    output[len] = '\0';
    return output;
}

// Register (or replace) a persistent named lookup table, e.g. "section", that
// section()/Section()/s.field=... assignment can query at runtime — without any
// domain-specific data (e.g. AISC steel shapes) compiled into this binary. Called
// once by app-private JS at startup (see app/js/data/section-registry.js), after
// loading its own private JSON data file. This function and Evaluator::s_lookupTables
// contain zero hardcoded row data — only the generic storage/lookup mechanism.
//
// json_table_data shape: {"W16X59": {"Ix": 954, "Zx": 108, ...}, "W16X67": {...}, ...}
// Values must be JSON strings or numbers. Returns 1 on success, 0 on malformed input.
EMSCRIPTEN_KEEPALIVE
int register_lookup_table(const char* table_name, const char* json_table_data) {
    try {
        auto rows = parseLookupTableJson(std::string(json_table_data));
        Evaluator::registerLookupTable(std::string(table_name), rows);
        return 1;
    } catch (...) {
        return 0;
    }
}

// Free memory allocated by WASM functions
EMSCRIPTEN_KEEPALIVE
void free_result(char* ptr) {
    delete[] ptr;
}

} // extern "C"