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
            json += "\"" + result.outputs[i] + "\"";
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
            json += "\"errorMessage\":\"" + result.wasmFiles[i].errorMessage + "\"}";
        }

        json += "],\"error\":\"" + result.error + "\"}";

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

// Free memory allocated by WASM functions
EMSCRIPTEN_KEEPALIVE
void free_result(char* ptr) {
    delete[] ptr;
}

} // extern "C"