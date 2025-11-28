#include "core/ast/ast.h"
#include "core/evaluator/evaluator.h"
#include "core/generator/markdown_formatter.h"
#include "core/generator/html_formatter.h"
#include "core/utils/paths.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

#ifdef WITH_TREE_SITTER
#include "core/ast/ast_builder.h"
#endif

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#endif

using namespace madola;

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void writeCppFiles(const std::vector<madola::CppGenerator::GeneratedFile>& cppFiles) {
    if (cppFiles.empty()) {
        return;
    }

    // Create ~/.madola/gen_cpp directory if it doesn't exist
    std::string genCppDir = madola::utils::getGenCppDirectory();

    for (const auto& file : cppFiles) {
        std::string filePath = genCppDir + "/" + file.filename;
        std::ofstream outFile(filePath);
        if (outFile.is_open()) {
            outFile << file.content;
            outFile.close();
            std::cout << "Generated C++ file: " << filePath << std::endl;
        } else {
            std::cerr << "Failed to write C++ file: " << filePath << std::endl;
        }
    }
}

void reportWasmFiles(const std::vector<madola::WasmGenerator::GeneratedWasm>& wasmFiles) {
    if (wasmFiles.empty()) {
        return;
    }

    for (const auto& file : wasmFiles) {
        if (file.compilationSuccess) {
            std::cout << "Generated WASM addon: " << file.wasmPath << std::endl;
            std::cout << "Generated JS wrapper: " << file.jsWrapperPath << std::endl;
        } else {
            std::cerr << "WASM compilation failed for " << file.functionName << ": " << file.errorMessage << std::endl;
        }
    }
}

void writeHtmlOutput(const std::string& html, const std::string& baseName) {
    // Create regression/results/html directory if it doesn't exist
    std::filesystem::create_directories("regression/results/html");

    std::string htmlFilePath = "regression/results/html/" + baseName + ".html";
    std::ofstream htmlFile(htmlFilePath);

    if (!htmlFile.is_open()) {
        std::cerr << "Failed to create HTML file: " << htmlFilePath << std::endl;
        return;
    }

    htmlFile << html;
    htmlFile.close();
    std::cout << "Generated HTML output: " << htmlFilePath << std::endl;
}

void showUsage(const char* programName) {
    std::cout << "Usage: " << programName << " <file.mda> [--html]" << std::endl;
    std::cout << "  <file.mda>  - MADOLA source file to process" << std::endl;
    std::cout << "  --html      - Output HTML format with integrated graphs and math" << std::endl;
}

bool isRunningInNewWindow() {
#ifdef _WIN32
    // On Windows, check if we're running in a console that was created for this process
    // This is a heuristic - if stdin is not a console, we're likely in a script/pipe
    return _isatty(_fileno(stdin)) && _isatty(_fileno(stdout));
#else
    // On Unix-like systems, check if we have a controlling terminal
    return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
#endif
}

void pauseIfNeeded() {
    // Disabled pause for command line usage
    // No longer pause regardless of execution context
}

int main(int argc, char* argv[]) {
    // Check for --help flag
    if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        showUsage(argv[0]);
        return 0;
    }

    if (argc < 2 || argc > 3) {
        showUsage(argv[0]);
        pauseIfNeeded();
        return 1;
    }

    bool htmlMode = false;
    if (argc == 3) {
        if (std::string(argv[2]) == "--html") {
            htmlMode = true;
        } else {
            std::cerr << "Error: Unknown option '" << argv[2] << "'" << std::endl;
            showUsage(argv[0]);
            pauseIfNeeded();
            return 1;
        }
    }

    try {
        // Read MADOLA source file
        std::string source = readFile(argv[1]);

        std::unique_ptr<Program> program;

#ifdef WITH_TREE_SITTER
        // Use Tree-sitter parser to build AST from source
        ASTBuilder builder;
        program = builder.buildProgram(source);
#else
        std::cout << "WARNING: Tree-sitter NOT enabled - using mock parser" << std::endl;
        // For now, create a mock AST since Tree-sitter integration is pending
        // Parse basic import statements from the source
        program = std::make_unique<Program>();

        // Simple parser for import statements
        std::istringstream stream(source);
        std::string line;
        while (std::getline(stream, line)) {
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            if (line.empty() || (line.length() >= 2 && line.substr(0, 2) == "//")) {
                continue; // Skip empty lines and comments
            }

            // Parse import statement: "from test_cube import cube"
            if (line.length() >= 5 && line.substr(0, 5) == "from " && line.find(" import ") != std::string::npos) {
                size_t fromPos = 5; // After "from "
                size_t importPos = line.find(" import ");
                if (importPos != std::string::npos) {
                    std::string moduleName = line.substr(fromPos, importPos - fromPos);
                    std::string functionsStr = line.substr(importPos + 8); // After " import "

                    // Remove semicolon if present
                    if (!functionsStr.empty() && functionsStr.back() == ';') {
                        functionsStr.pop_back();
                    }

                    // Parse function names (split by comma)
                    std::vector<std::string> functionNames;
                    std::istringstream funcStream(functionsStr);
                    std::string func;
                    while (std::getline(funcStream, func, ',')) {
                        func.erase(0, func.find_first_not_of(" \t"));
                        func.erase(func.find_last_not_of(" \t") + 1);
                        if (!func.empty()) {
                            functionNames.push_back(func);
                        }
                    }

                    auto importStmt = std::make_unique<ImportStatement>(moduleName, functionNames);
                    program->addStatement(std::move(importStmt));
                    continue;
                }
            }

            // Parse other statements: x1 := cube(10); print(x1);
            if (line.find(":=") != std::string::npos) {
                // Assignment
                size_t assignPos = line.find(":=");
                std::string varName = line.substr(0, assignPos);
                varName.erase(0, varName.find_first_not_of(" \t"));
                varName.erase(varName.find_last_not_of(" \t") + 1);

                std::string exprStr = line.substr(assignPos + 2);
                exprStr.erase(0, exprStr.find_first_not_of(" \t"));
                if (!exprStr.empty() && exprStr.back() == ';') {
                    exprStr.pop_back();
                }
                exprStr.erase(exprStr.find_last_not_of(" \t") + 1);

                // Parse function call: cube(10)
                if (exprStr.find('(') != std::string::npos && exprStr.find(')') != std::string::npos) {
                    size_t parenPos = exprStr.find('(');
                    std::string funcName = exprStr.substr(0, parenPos);
                    std::string argsStr = exprStr.substr(parenPos + 1, exprStr.find(')') - parenPos - 1);

                    std::vector<ExpressionPtr> args;
                    if (!argsStr.empty()) {
                        // Parse comma-separated arguments
                        std::stringstream ss(argsStr);
                        std::string token;
                        while (std::getline(ss, token, ',')) {
                            // Trim whitespace
                            token.erase(0, token.find_first_not_of(" \t"));
                            token.erase(token.find_last_not_of(" \t") + 1);

                            if (!token.empty()) {
                                try {
                                    double argValue = std::stod(token);
                                    args.push_back(std::make_unique<Number>(argValue));
                                } catch (...) {
                                    // If not a number, treat as identifier
                                    args.push_back(std::make_unique<Identifier>(token));
                                }
                            }
                        }
                    }

                    auto funcCall = std::make_unique<FunctionCall>(funcName, std::move(args));
                    auto assignment = std::make_unique<AssignmentStatement>(varName, std::move(funcCall));
                    program->addStatement(std::move(assignment));
                }
            } else if (line.length() >= 6 && line.substr(0, 6) == "print(") {
                // Print statement
                size_t startPos = 6; // After "print("
                size_t endPos = line.find(')', startPos);
                if (endPos != std::string::npos) {
                    std::string varName = line.substr(startPos, endPos - startPos);
                    varName.erase(0, varName.find_first_not_of(" \t"));
                    varName.erase(varName.find_last_not_of(" \t") + 1);

                    auto identifier = std::make_unique<Identifier>(varName);
                    auto printStmt = std::make_unique<PrintStatement>(std::move(identifier));
                    program->addStatement(std::move(printStmt));
                }
            } else if (line.find('(') != std::string::npos && line.find(')') != std::string::npos && line.find(":=") == std::string::npos) {
                // Standalone function call (not an assignment)
                std::cout << "DEBUG: Found standalone function call: " << line << std::endl;
                size_t parenPos = line.find('(');
                std::string funcName = line.substr(0, parenPos);
                funcName.erase(0, funcName.find_first_not_of(" \t"));
                funcName.erase(funcName.find_last_not_of(" \t") + 1);
                std::cout << "DEBUG: Function name: '" << funcName << "'" << std::endl;

                std::string argsStr = line.substr(parenPos + 1, line.find(')') - parenPos - 1);
                if (!argsStr.empty() && argsStr.back() == ';') {
                    // Remove semicolon if present in args
                    size_t semicolonInLine = line.find(';');
                    if (semicolonInLine > line.find(')')) {
                        // Semicolon is after closing paren, not in args
                    } else {
                        argsStr.pop_back();
                    }
                }

                std::vector<ExpressionPtr> args;
                if (!argsStr.empty()) {
                    // Parse comma-separated arguments
                    std::stringstream ss(argsStr);
                    std::string token;
                    while (std::getline(ss, token, ',')) {
                        // Trim whitespace
                        token.erase(0, token.find_first_not_of(" \t"));
                        token.erase(token.find_last_not_of(" \t") + 1);

                        if (!token.empty()) {
                            try {
                                double argValue = std::stod(token);
                                args.push_back(std::make_unique<Number>(argValue));
                            } catch (...) {
                                // If not a number, treat as identifier
                                args.push_back(std::make_unique<Identifier>(token));
                            }
                        }
                    }
                }

                auto funcCall = std::make_unique<FunctionCall>(funcName, std::move(args));

                // For standalone function calls, we need to wrap them in a print statement
                // or create a special statement type. For now, let's print the result.
                auto printStmt = std::make_unique<PrintStatement>(std::move(funcCall));
                program->addStatement(std::move(printStmt));
            }
        }
#endif

        // Extract filename without extension for output generation
        std::string filename = argv[1];
        std::filesystem::path filePath(filename);
        std::string baseName = filePath.stem().string();

        if (htmlMode) {
            // HTML formatting mode - output to stdout for redirection
            HtmlFormatter formatter;
            auto result = formatter.formatProgramWithExecution(*program);
            if (result.success) {
                std::cout << result.html;
            } else {
                std::cerr << "HTML formatting failed: " << result.error << std::endl;
                return 1;
            }
        } else {
            // Direct evaluation mode
            Evaluator evaluator;
            auto result = evaluator.evaluate(*program, baseName);

            if (result.success) {
                std::cout << "Execution completed successfully" << std::endl;
                if (!result.outputs.empty()) {
                    std::cout << "Output:" << std::endl;
                    for (const auto& output : result.outputs) {
                        std::cout << "  " << output << std::endl;
                    }
                }

                // Write generated C++ files
                writeCppFiles(result.cppFiles);

                // Report WASM files
                reportWasmFiles(result.wasmFiles);

                std::cout.flush(); // Ensure immediate output
                pauseIfNeeded(); // Pause to see results
            } else {
                std::cerr << "❌ Execution failed: " << result.error << std::endl;
                std::cerr.flush(); // Ensure immediate output
                pauseIfNeeded();
                return 1;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        pauseIfNeeded();
        return 1;
    }

    return 0;
}