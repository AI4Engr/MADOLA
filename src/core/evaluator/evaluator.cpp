#include "evaluator.h"
#include "../ast/ast_builder.h"
#include "../utils/paths.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <climits> 

namespace madola {

Evaluator::Evaluator() {
    // Define unit identifiers as constants with value 1
    // This allows compound units like: velocity := 25 * m/s

    // Length units
    env.define("mm", UnitValue{1.0, "mm"});
    env.define("cm", UnitValue{1.0, "cm"});
    env.define("m", UnitValue{1.0, "m"});
    env.define("km", UnitValue{1.0, "km"});
    env.define("in", UnitValue{1.0, "in"});
    env.define("ft", UnitValue{1.0, "ft"});
    env.define("yd", UnitValue{1.0, "yd"});
    env.define("mi", UnitValue{1.0, "mi"});

    // Mass units
    env.define("kg", UnitValue{1.0, "kg"});
    env.define("g", UnitValue{1.0, "g"});
    env.define("mg", UnitValue{1.0, "mg"});
    env.define("lb", UnitValue{1.0, "lb"});
    env.define("oz", UnitValue{1.0, "oz"});
    env.define("ton", UnitValue{1.0, "ton"});

    // Force units
    env.define("N", UnitValue{1.0, "N"});
    env.define("kN", UnitValue{1.0, "kN"});
    env.define("lbf", UnitValue{1.0, "lbf"});
    env.define("kip", UnitValue{1.0, "kip"});

    // Pressure/Stress units
    env.define("Pa", UnitValue{1.0, "Pa"});
    env.define("kPa", UnitValue{1.0, "kPa"});
    env.define("MPa", UnitValue{1.0, "MPa"});
    env.define("GPa", UnitValue{1.0, "GPa"});
    env.define("psi", UnitValue{1.0, "psi"});
    env.define("ksi", UnitValue{1.0, "ksi"});

    // Time units
    env.define("s", UnitValue{1.0, "s"});
    env.define("ms", UnitValue{1.0, "ms"});
    env.define("min", UnitValue{1.0, "min"});
    env.define("h", UnitValue{1.0, "h"});

    // Temperature units
    env.define("K", UnitValue{1.0, "K"});
    env.define("C", UnitValue{1.0, "C"});
    env.define("F", UnitValue{1.0, "F"});

    // Volume units
    env.define("L", UnitValue{1.0, "L"});
    env.define("gal", UnitValue{1.0, "gal"});
}

Evaluator::EvaluationResult Evaluator::evaluate(const Program& program, const std::string& mdaFileName) {
    EvaluationResult result;
    result.success = true;

    // Reset control flow state for a new evaluation run
    returnPending = false;
    pendingReturnValue = 0.0;
    breakPending = false;
    functionDepth = 0;
    loopDepth = 0;

    try {
        // Check for @version directive at the start of the program
        bool hasVersion = false;
        if (!program.statements.empty()) {
            if (auto* versionStmt = dynamic_cast<const VersionStatement*>(program.statements[0].get())) {
                hasVersion = true;
                if (versionStmt->version != "0.01") {
                    throw std::runtime_error("Unsupported version: " + versionStmt->version + ". Expected version 0.01");
                }
            }
        }

        if (!hasVersion) {
            throw std::runtime_error("Missing @version directive. Every MADOLA file must start with '@version' to specify the language version.");
        }

        // First pass: execute all statements
        for (const auto& stmt : program.statements) {
            executeStatement(*stmt, result.outputs);
        }

        // Code generation is disabled in WASM build to avoid unsupported I/O and memory pressure
        #ifndef __EMSCRIPTEN__
        // Second pass: generate C++ files for functions with @gen_cpp decorator
        result.cppFiles = cppGen.generateCppFiles(program);

        // Third pass: generate WASM files for functions with @gen_addon decorator
        result.wasmFiles = wasmGen.generateWasmFiles(program, mdaFileName);

        // Add C++ files from @gen_addon to cppFiles array (for display in web app)
        for (const auto& wasmFile : result.wasmFiles) {
            if (!wasmFile.cppContent.empty()) {
                // Extract filename from path
                std::string filename = wasmFile.functionName + ".cpp";

                CppGenerator::GeneratedFile cppFile;
                cppFile.filename = filename;
                cppFile.content = wasmFile.cppContent;
                result.cppFiles.push_back(cppFile);
            }
        }
        #endif

        // Fourth pass: transfer collected graphs to result
        result.graphs = std::move(collectedGraphs);
        result.graphs3d = std::move(collected3DGraphs);
        result.tables = std::move(collectedTables);

    } catch (const std::exception& e) {
        result.success = false;
        result.error = e.what();
    }

    return result;
}

void Evaluator::executeStatement(const Statement& stmt, std::vector<std::string>& outputs) {
    // Use dynamic_cast to determine statement type
    if (const auto* assignment = dynamic_cast<const AssignmentStatement*>(&stmt)) {
        executeAssignment(*assignment);
    } else if (const auto* arrayAssignment = dynamic_cast<const ArrayAssignmentStatement*>(&stmt)) {
        executeArrayAssignment(*arrayAssignment);
    } else if (const auto* print = dynamic_cast<const PrintStatement*>(&stmt)) {
        executePrint(*print, outputs);
    } else if (const auto* exprStmt = dynamic_cast<const ExpressionStatement*>(&stmt)) {
        executeExpressionStatement(*exprStmt, outputs);
    } else if (dynamic_cast<const CommentStatement*>(&stmt)) {
        // Comments don't generate output or modify state
        // They are purely for documentation
    } else if (dynamic_cast<const SkipStatement*>(&stmt)) {
        // Skip statements don't generate output or modify state in the evaluator
        // They only affect markdown formatting
    } else if (dynamic_cast<const HeadingStatement*>(&stmt)) {
        // Heading statements don't generate output or modify state in the evaluator
        // They are purely for HTML report formatting
    } else if (dynamic_cast<const VersionStatement*>(&stmt)) {
        // Version statements are metadata only - don't generate output or modify state
    } else if (dynamic_cast<const ParagraphStatement*>(&stmt)) {
        // Paragraph statements don't generate output or modify state in the evaluator
        // They are purely for HTML report formatting
    } else if (const auto* functionDecl = dynamic_cast<const FunctionDeclaration*>(&stmt)) {
        executeFunction(*functionDecl);
    } else if (const auto* piecewiseFuncDecl = dynamic_cast<const PiecewiseFunctionDeclaration*>(&stmt)) {
        executePiecewiseFunction(*piecewiseFuncDecl);
    } else if (const auto* importStmt = dynamic_cast<const ImportStatement*>(&stmt)) {
        executeImport(*importStmt);
    } else if (const auto* returnStmt = dynamic_cast<const ReturnStatement*>(&stmt)) {
        if (functionDepth == 0) {
            throw std::runtime_error("Return statement outside of function");
        }
        executeReturn(*returnStmt);
    } else if (dynamic_cast<const BreakStatement*>(&stmt)) {
        if (loopDepth == 0) {
            throw std::runtime_error("Break statement outside of loop");
        }
        breakPending = true;
    } else if (const auto* forStmt = dynamic_cast<const ForStatement*>(&stmt)) {
        executeFor(*forStmt, outputs);
    } else if (const auto* whileStmt = dynamic_cast<const WhileStatement*>(&stmt)) {
        executeWhile(*whileStmt, outputs);
    } else if (const auto* ifStmt = dynamic_cast<const IfStatement*>(&stmt)) {
        executeIf(*ifStmt, outputs);
    } else if (const auto* decoratedStmt = dynamic_cast<const DecoratedStatement*>(&stmt)) {
        // Check if this has @gen_addon or @gen_cpp decorator
        if (decoratedStmt->hasDecorator("gen_addon") || decoratedStmt->hasDecorator("gen_cpp")) {
            // Skip execution - these are for code generation only
            // But still register the function if it's a function declaration
            if (dynamic_cast<const FunctionDeclaration*>(decoratedStmt->statement.get())) {
                // Don't execute the function body, but we could register it if needed
            }
            return;
        }
        // Execute the underlying statement for other decorators
        executeStatement(*decoratedStmt->statement, outputs);
    } else {
        throw std::runtime_error("Unknown statement type");
    }
}

void Evaluator::executeAssignment(const AssignmentStatement& stmt) {
    Value value = evaluateExpression(*stmt.expression);
    env.define(stmt.variable, value);
}

void Evaluator::executeArrayAssignment(const ArrayAssignmentStatement& stmt) {
    // Evaluate the index and value
    Value indexValue = evaluateExpression(*stmt.index);
    Value assignValue = evaluateExpression(*stmt.expression);

    // Index must be a number
    if (!std::holds_alternative<double>(indexValue)) {
        throw std::runtime_error("Array index must be a number");
    }

    double indexDouble = std::get<double>(indexValue);

    // Check for precision loss when converting to integer
    if (std::floor(indexDouble) != indexDouble) {
        throw std::runtime_error("Array index must be an integer, got: " + std::to_string(indexDouble));
    }

    // Check for overflow when converting to int
    if (indexDouble > static_cast<double>(INT_MAX) || indexDouble < static_cast<double>(INT_MIN)) {
        throw std::runtime_error("Array index out of valid range: " + std::to_string(indexDouble));
    }

    int index = static_cast<int>(indexDouble);

    // Check for negative index
    if (index < 0) {
        throw std::runtime_error("Array index must be non-negative, got: " + std::to_string(index));
    }

    // Check if array exists, if not create it
    // Default to column vector (true) if creating a new array, as this is more common in mathematical contexts
    ArrayValue arrayVal(std::vector<double>(), stmt.isColumnVector ? stmt.isColumnVector : true);
    if (env.exists(stmt.arrayName)) {
        Value existingVal = env.get(stmt.arrayName);
        if (std::holds_alternative<ArrayValue>(existingVal)) {
            arrayVal = std::get<ArrayValue>(existingVal);
        } else {
            throw std::runtime_error("Variable '" + stmt.arrayName + "' is not an array");
        }
    }

    // Ensure the array is large enough
    if (static_cast<size_t>(index) >= arrayVal.elements.size()) {
        arrayVal.elements.resize(index + 1, 0.0); // Fill with zeros
    }

    // Set the value - support both double and UnitValue
    if (std::holds_alternative<double>(assignValue)) {
        arrayVal.elements[index] = std::get<double>(assignValue);
    } else if (std::holds_alternative<UnitValue>(assignValue)) {
        // Extract the numeric value from UnitValue
        const UnitValue& unitVal = std::get<UnitValue>(assignValue);
        arrayVal.elements[index] = unitVal.value;
    } else if (std::holds_alternative<std::string>(assignValue)) {
        throw std::runtime_error("Array assignment currently only supports numeric values (got string: '" + std::get<std::string>(assignValue) + "')");
    } else if (std::holds_alternative<ArrayValue>(assignValue)) {
        throw std::runtime_error("Array assignment currently only supports numeric values (got array value)");
    } else {
        throw std::runtime_error("Array assignment: unknown value type");
    }

    // Store the updated array
    env.define(stmt.arrayName, arrayVal);
}

void Evaluator::executePrint(const PrintStatement& stmt, std::vector<std::string>& outputs) {
    Value value = evaluateExpression(*stmt.expression);
    outputs.push_back(valueToString(value));
}

void Evaluator::executeExpressionStatement(const ExpressionStatement& stmt, std::vector<std::string>& outputs) {
    Value value = evaluateExpression(*stmt.expression);
    // For expression statements, only add to output if it returns a meaningful value
    if (std::holds_alternative<std::string>(value)) {
        std::string result = std::get<std::string>(value);
        if (!result.empty()) {
            outputs.push_back(result);
        }
    }
}

void Evaluator::executeFunction(const FunctionDeclaration& stmt) {
    env.defineFunction(stmt.name, stmt);
}

void Evaluator::executeImport(const ImportStatement& stmt) {
    std::string errorMsg;
    std::string moduleName = stmt.moduleName;
    std::string modulePath = moduleName;

#ifdef __EMSCRIPTEN__
    // In browser/WASM environment, skip filesystem checks and go straight to WASM import
    // The web app preloads WASM modules into window.madolaWasmBridge
    std::vector<std::string> functionNames;
    for (const auto& item : stmt.importItems) {
        functionNames.push_back(item->originalName);
    }

    if (!wasmLoader.loadModule(stmt.moduleName, functionNames, errorMsg)) {
        throw std::runtime_error("WASM import failed for module '" + stmt.moduleName + "': " + errorMsg);
    }

    // Register imported WASM functions with their aliases
    for (const auto& item : stmt.importItems) {
        if (wasmLoader.isFunctionAvailable(item->originalName)) {
            std::string effectiveName = item->getEffectiveName();

            // Register alias if present
            if (!item->aliasName.empty()) {
                env.defineAlias(effectiveName, item->originalName);
            }

            std::cout << "Imported WASM function: " << item->originalName;
            if (!item->aliasName.empty()) {
                std::cout << " as " << effectiveName;
            }
            std::cout << " from " << stmt.moduleName << std::endl;
        } else {
            throw std::runtime_error("Failed to import function '" + item->originalName + "' from module '" + stmt.moduleName + "'");
        }
    }
    return;
#else
    // Native environment: Try .mda files first, then WASM

    // Try to resolve module path - add .mda extension if not present
    if (modulePath.find('.') == std::string::npos) {
        modulePath += ".mda";
    }

    // Check if this is a .mda file import
    bool isMdaFile = (modulePath.size() > 4 && modulePath.substr(modulePath.size() - 4) == ".mda");

    if (isMdaFile) {
        // Search for .mda file in multiple locations
        std::vector<std::filesystem::path> searchPaths = {
            modulePath,                                                      // Current directory
            std::filesystem::current_path() / modulePath,                    // Explicit current path
            std::filesystem::path(utils::getTroveDirectory()) / modulePath   // ~/.madola/trove/
        };

        std::string resolvedPath;
        bool found = false;
        for (const auto& path : searchPaths) {
            if (std::filesystem::exists(path)) {
                resolvedPath = path.string();
                found = true;
                break;
            }
        }

        if (!found) {
            // .mda file not found - try WASM import instead
            // Extract function names for WASM loader (original names, not aliases)
            std::vector<std::string> functionNames;
            for (const auto& item : stmt.importItems) {
                functionNames.push_back(item->originalName);
            }

            if (!wasmLoader.loadModule(stmt.moduleName, functionNames, errorMsg)) {
                throw std::runtime_error("Cannot find module '" + modulePath + "' in current directory or ~/.madola/trove/, and WASM import failed: " + errorMsg);
            }

            // Register imported WASM functions with their aliases
            for (const auto& item : stmt.importItems) {
                if (wasmLoader.isFunctionAvailable(item->originalName)) {
                    std::string effectiveName = item->getEffectiveName();

                    // Register alias if present
                    if (!item->aliasName.empty()) {
                        env.defineAlias(effectiveName, item->originalName);
                    }

                    std::cout << "Imported WASM function: " << item->originalName;
                    if (!item->aliasName.empty()) {
                        std::cout << " as " << effectiveName;
                    }
                    std::cout << " from " << stmt.moduleName << std::endl;
                } else {
                    throw std::runtime_error("Failed to import function '" + item->originalName + "' from module '" + stmt.moduleName + "'");
                }
            }
            return;
        }

        // Import from MADOLA file
        // Parse and evaluate the imported file
        std::ifstream file(resolvedPath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file for import: " + resolvedPath);
        }

        std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        // Parse the imported file using ASTBuilder
        ASTBuilder builder;
        ProgramPtr program = builder.buildProgram(source);
        if (!program) {
            throw std::runtime_error("Failed to parse imported file '" + modulePath + "'");
        }

        // Create a temporary evaluator to extract function definitions
        Evaluator tempEvaluator;
        auto evalResult = tempEvaluator.evaluate(*program, modulePath);

        if (!evalResult.success) {
            throw std::runtime_error("Failed to evaluate imported file '" + modulePath + "': " + evalResult.error);
        }

        // Import requested functions from the temporary evaluator
        for (const auto& item : stmt.importItems) {
            const FunctionDeclaration* func = tempEvaluator.env.getFunction(item->originalName);
            if (func == nullptr) {
                throw std::runtime_error("Function '" + item->originalName + "' not found in module '" + modulePath + "'");
            }

            // Register function in current environment
            // Store the imported program to keep function pointers valid
            env.defineFunction(item->originalName, *func);

            // Register alias if present
            std::string effectiveName = item->getEffectiveName();
            if (!item->aliasName.empty()) {
                env.defineAlias(effectiveName, item->originalName);
            }

            std::cout << "Imported function: " << item->originalName;
            if (!item->aliasName.empty()) {
                std::cout << " as " << effectiveName;
            }
            std::cout << " from " << modulePath << std::endl;
        }

        // Keep the imported program alive so function pointers remain valid
        importedPrograms.push_back(std::move(program));
    } else {
        // Try to import as WASM module
        // Extract function names for WASM loader (original names, not aliases)
        std::vector<std::string> functionNames;
        for (const auto& item : stmt.importItems) {
            functionNames.push_back(item->originalName);
        }

        if (!wasmLoader.loadModule(stmt.moduleName, functionNames, errorMsg)) {
            throw std::runtime_error("Import failed: " + errorMsg);
        }

        // Register imported WASM functions with their aliases
        for (const auto& item : stmt.importItems) {
            if (wasmLoader.isFunctionAvailable(item->originalName)) {
                std::string effectiveName = item->getEffectiveName();

                // Register alias if present
                if (!item->aliasName.empty()) {
                    env.defineAlias(effectiveName, item->originalName);
                }

                std::cout << "Imported WASM function: " << item->originalName;
                if (!item->aliasName.empty()) {
                    std::cout << " as " << effectiveName;
                }
                std::cout << " from " << stmt.moduleName << std::endl;
            } else {
                throw std::runtime_error("Failed to import function '" + item->originalName + "' from module '" + stmt.moduleName + "'");
            }
        }
    }
#endif
}

Value Evaluator::executeReturn(const ReturnStatement& stmt) {
    Value value = evaluateExpression(*stmt.expression);
    returnPending = true;
    pendingReturnValue = value;
    return value;
}

void Evaluator::executeFor(const ForStatement& stmt, std::vector<std::string>& outputs) {
    // Evaluate the range expression
    if (const auto* rangeExpr = dynamic_cast<const RangeExpression*>(stmt.range.get())) {
        Value startVal = evaluateExpression(*rangeExpr->start);
        Value endVal = evaluateExpression(*rangeExpr->end);

        if (!std::holds_alternative<double>(startVal) || !std::holds_alternative<double>(endVal)) {
            throw std::runtime_error("Range values must be numbers");
        }

        double start = std::get<double>(startVal);
        double end = std::get<double>(endVal);

        // Check for precision loss when converting to integer
        if (std::floor(start) != start) {
            throw std::runtime_error("For loop start value must be an integer, got: " + std::to_string(start));
        }
        if (std::floor(end) != end) {
            throw std::runtime_error("For loop end value must be an integer, got: " + std::to_string(end));
        }

        // Check for overflow when converting to int
        if (start > static_cast<double>(INT_MAX) || start < static_cast<double>(INT_MIN)) {
            throw std::runtime_error("For loop start value out of valid range: " + std::to_string(start));
        }
        if (end > static_cast<double>(INT_MAX) || end < static_cast<double>(INT_MIN)) {
            throw std::runtime_error("For loop end value out of valid range: " + std::to_string(end));
        }

        // For simplicity, we'll only support integer ranges for now
        int startInt = static_cast<int>(start);
        int endInt = static_cast<int>(end);

        // Save the current value of the loop variable if it exists
        bool varExisted = env.exists(stmt.variable);
        Value oldValue;
        if (varExisted) {
            oldValue = env.get(stmt.variable);
        }

        // Execute the loop
        loopDepth++;
        bool exitLoop = false;
        for (int i = startInt; i <= endInt; i++) {
            env.define(stmt.variable, static_cast<double>(i));

            // Execute loop body
            for (const auto& bodyStmt : stmt.body) {
                executeStatement(*bodyStmt, outputs);

                if (returnPending) {
                    exitLoop = true;
                    break;
                }

                if (breakPending) {
                    breakPending = false;
                    exitLoop = true;
                    break;
                }
            }

            if (exitLoop) {
                break;
            }
        }
        loopDepth--;

        // Restore the original value of the loop variable
        if (varExisted) {
            env.define(stmt.variable, oldValue);
        }
    } else {
        throw std::runtime_error("For loop requires a range expression");
    }
}

void Evaluator::executeWhile(const WhileStatement& stmt, std::vector<std::string>& outputs) {
    // Execute the loop while condition is true
    loopDepth++;
    while (true) {
        // Evaluate the condition
        Value conditionValue = evaluateExpression(*stmt.condition);

        // Convert to boolean
        bool condition = false;
        if (std::holds_alternative<double>(conditionValue)) {
            condition = std::get<double>(conditionValue) != 0.0;
        } else if (std::holds_alternative<std::string>(conditionValue)) {
            condition = !std::get<std::string>(conditionValue).empty();
        } else if (std::holds_alternative<UnitValue>(conditionValue)) {
            condition = std::get<UnitValue>(conditionValue).value != 0.0;
        } else {
            throw std::runtime_error("While condition must evaluate to a number or compatible type");
        }

        // Exit loop if condition is false
        if (!condition) {
            break;
        }

        // Execute loop body with break support
        bool exitLoop = false;
        for (const auto& bodyStmt : stmt.body) {
            executeStatement(*bodyStmt, outputs);

            if (returnPending) {
                exitLoop = true;
                break;
            }

            if (breakPending) {
                breakPending = false;
                exitLoop = true;
                break;
            }
        }

        if (exitLoop) {
            break;
        }
    }
    loopDepth--;
}

void Evaluator::executeIf(const IfStatement& stmt, std::vector<std::string>& outputs) {
    // Evaluate the condition
    Value conditionValue = evaluateExpression(*stmt.condition);

    // Convert to boolean
    bool condition = false;
    if (std::holds_alternative<double>(conditionValue)) {
        condition = std::get<double>(conditionValue) != 0.0;
    } else if (std::holds_alternative<std::string>(conditionValue)) {
        condition = !std::get<std::string>(conditionValue).empty();
    } else if (std::holds_alternative<UnitValue>(conditionValue)) {
        condition = std::get<UnitValue>(conditionValue).value != 0.0;
    }

    // Execute appropriate branch
    const auto& branch = condition ? stmt.then_body : stmt.else_body;
    for (const auto& branchStmt : branch) {
        executeStatement(*branchStmt, outputs);

        if (returnPending || breakPending) {
            break;
        }
    }
}

Value Evaluator::getVariableValue(const std::string& name) const {
    return env.get(name);
}

std::string Evaluator::valueToString(const Value& value) {
    std::ostringstream oss;

    if (std::holds_alternative<double>(value)) {
        double val = std::get<double>(value);

        // Check if the value is effectively an integer
        if (std::floor(val) == val && std::abs(val) < 1e15) {
            oss << static_cast<long long>(val);
        } else {
            oss << std::fixed << std::setprecision(3) << val;
            std::string result = oss.str();
            // Remove trailing zeros
            result.erase(result.find_last_not_of('0') + 1, std::string::npos);
            if (result.back() == '.') result.pop_back();
            return result;
        }
    } else if (std::holds_alternative<ComplexValue>(value)) {
        const ComplexValue& complex = std::get<ComplexValue>(value);

        if (complex.real == 0 && complex.imaginary == 0) {
            oss << "0";
        } else {
            bool needsReal = (complex.real != 0);
            bool needsImag = (complex.imaginary != 0);
            double imag = complex.imaginary;

            if (needsReal) {
                if (std::floor(complex.real) == complex.real && std::abs(complex.real) < 1e15) {
                    oss << static_cast<long long>(complex.real);
                } else {
                    oss << complex.real;
                }
            }

            if (needsImag) {
                if (needsReal) {
                    if (complex.imaginary > 0) {
                        oss << " + ";
                    } else {
                        oss << " - ";
                        imag = -complex.imaginary;
                    }
                }

                if (imag == 1.0) {
                    oss << "i";
                } else if (imag == -1.0 && !needsReal) {
                    oss << "-i";
                } else if (std::floor(imag) == imag && std::abs(imag) < 1e15) {
                    oss << static_cast<long long>(imag) << "i";
                } else {
                    oss << imag << "i";
                }
            }
        }
    } else if (std::holds_alternative<std::string>(value)) {
        // Process escape sequences in strings
        std::string str = std::get<std::string>(value);
        std::string processed;
        processed.reserve(str.length());
        
        for (size_t i = 0; i < str.length(); ++i) {
            if (str[i] == '\\' && i + 1 < str.length()) {
                // Handle escape sequences
                switch (str[i + 1]) {
                    case 'n':
                        processed += '\n';
                        ++i;
                        break;
                    case 't':
                        processed += '\t';
                        ++i;
                        break;
                    case 'r':
                        processed += '\r';
                        ++i;
                        break;
                    case '\\':
                        processed += '\\';
                        ++i;
                        break;
                    case '"':
                        processed += '"';
                        ++i;
                        break;
                    default:
                        // Unknown escape sequence, keep as is
                        processed += str[i];
                        break;
                }
            } else {
                processed += str[i];
            }
        }
        oss << processed;
    } else if (std::holds_alternative<UnitValue>(value)) {
        const UnitValue& unitVal = std::get<UnitValue>(value);

        // Check if the numeric part is effectively an integer
        if (std::floor(unitVal.value) == unitVal.value && std::abs(unitVal.value) < 1e15) {
            oss << static_cast<long long>(unitVal.value);
        } else {
            std::ostringstream tempSs;
            tempSs << std::fixed << std::setprecision(3) << unitVal.value;
            std::string numStr = tempSs.str();
            // Remove trailing zeros
            numStr.erase(numStr.find_last_not_of('0') + 1, std::string::npos);
            if (numStr.back() == '.') numStr.pop_back();
            oss << numStr;
        }

        if (!unitVal.unit.empty()) {
            oss << " " << unitVal.unit;
        }
    } else if (std::holds_alternative<ArrayValue>(value)) {
        const ArrayValue& arr = std::get<ArrayValue>(value);

        if (arr.isMatrix) {
            oss << "[";
            for (size_t i = 0; i < arr.matrixRows.size(); ++i) {
                if (i > 0) oss << "; ";
                oss << "[";
                for (size_t j = 0; j < arr.matrixRows[i].size(); ++j) {
                    if (j > 0) oss << ", ";

                    double val = arr.matrixRows[i][j];
                    if (std::floor(val) == val && std::abs(val) < 1e15) {
                        oss << static_cast<long long>(val);
                    } else {
                        oss << val;
                    }
                }
                oss << "]";
            }
            oss << "]";
        } else {
            if (arr.isColumnVector) {
                oss << "[";
                for (size_t i = 0; i < arr.elements.size(); ++i) {
                    if (i > 0) oss << "; ";

                    double val = arr.elements[i];
                    if (std::floor(val) == val && std::abs(val) < 1e15) {
                        oss << static_cast<long long>(val);
                    } else {
                        oss << val;
                    }
                }
                oss << "]";
            } else {
                oss << "[";
                for (size_t i = 0; i < arr.elements.size(); ++i) {
                    if (i > 0) oss << ", ";

                    double val = arr.elements[i];
                    if (std::floor(val) == val && std::abs(val) < 1e15) {
                        oss << static_cast<long long>(val);
                    } else {
                        oss << val;
                    }
                }
                oss << "]";
            }
        }
    }

    return oss.str();
}

} // namespace madola