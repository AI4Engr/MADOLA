#include "evaluator.h"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <map>
#include <algorithm>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace madola {

// Global static map for piecewise functions
static std::map<std::string, const PiecewiseFunctionDeclaration*> piecewiseFunctions;

Value Evaluator::evaluateFunctionCall(const FunctionCall& expr) {
    // Special handling for matrix functions

    // Special handling for type() function
    if (expr.function_name == "type") {
        if (expr.arguments.size() != 1) {
            throw std::runtime_error("Function type() expects 1 argument (variable), got " + std::to_string(expr.arguments.size()));
        }

        // Extract variable name from argument (must be an identifier)
        const auto* varIdentifier = dynamic_cast<const Identifier*>(expr.arguments[0].get());
        if (!varIdentifier) {
            throw std::runtime_error("Argument to type() must be a variable identifier");
        }

        // Check if variable exists
        if (!env.exists(varIdentifier->name)) {
            return std::string("undefined");
        }

        // Get the variable and determine its type
        Value value = env.get(varIdentifier->name);
        if (std::holds_alternative<double>(value)) {
            return std::string("number");
        } else if (std::holds_alternative<std::string>(value)) {
            return std::string("string");
        } else if (std::holds_alternative<UnitValue>(value)) {
            return std::string("unit");
        } else if (std::holds_alternative<ArrayValue>(value)) {
            const ArrayValue& arrayVal = std::get<ArrayValue>(value);
            if (arrayVal.isMatrix) {
                return std::string("matrix");
            } else if (arrayVal.isColumnVector) {
                return std::string("column_vector");
            } else {
                return std::string("array");
            }
        } else {
            return std::string("unknown");
        }
    }

    // Special handling for graph function
    if (expr.function_name == "graph") {
        if (expr.arguments.size() < 2 || expr.arguments.size() > 3) {
            throw std::runtime_error("Function graph expects 2-3 arguments (x_array, y_array, [title]), got " + std::to_string(expr.arguments.size()));
        }

        Value xVal = evaluateExpression(*expr.arguments[0]);
        Value yVal = evaluateExpression(*expr.arguments[1]);

        if (!std::holds_alternative<ArrayValue>(xVal) || !std::holds_alternative<ArrayValue>(yVal)) {
            throw std::runtime_error("graph() arguments must be arrays");
        }

        ArrayValue xArray = std::get<ArrayValue>(xVal);
        ArrayValue yArray = std::get<ArrayValue>(yVal);

        if (xArray.elements.size() != yArray.elements.size()) {
            throw std::runtime_error("graph() x and y arrays must have the same length");
        }

        // Get optional title
        std::string title = "";
        if (expr.arguments.size() == 3) {
            Value titleVal = evaluateExpression(*expr.arguments[2]);
            if (std::holds_alternative<std::string>(titleVal)) {
                title = std::get<std::string>(titleVal);
            } else {
                throw std::runtime_error("graph() title argument must be a string");
            }
        }

        return generateGraph(xArray, yArray, title);
    }

    // Special handling for graph_3d function
    if (expr.function_name == "graph_3d") {
        if (expr.arguments.size() < 1 || expr.arguments.size() > 7) {
            throw std::runtime_error("Function graph_3d expects 1-7 arguments (title, [width, height, depth, hole_width, hole_height, hole_depth]), got " + std::to_string(expr.arguments.size()));
        }

        // Get title (required) - convert any type to string
        Value titleVal = evaluateExpression(*expr.arguments[0]);
        std::string title;
        if (std::holds_alternative<std::string>(titleVal)) {
            title = std::get<std::string>(titleVal);
        } else if (std::holds_alternative<double>(titleVal)) {
            title = "Graph " + std::to_string(std::get<double>(titleVal));
        } else {
            title = "3D Graph";
        }

        // Get optional dimensions
        std::vector<double> dimensions;
        for (size_t i = 1; i < expr.arguments.size(); ++i) {
            Value dimVal = evaluateExpression(*expr.arguments[i]);
            if (!std::holds_alternative<double>(dimVal)) {
                throw std::runtime_error("graph_3d() dimension arguments must be numbers");
            }
            dimensions.push_back(std::get<double>(dimVal));
        }

        return generate3DGraph(title, dimensions);
    }

    // Special handling for table function
    if (expr.function_name == "table") {
        if (expr.arguments.size() < 2) {
            throw std::runtime_error("Function table expects at least 2 arguments (headers_array, column1, ...), got " + std::to_string(expr.arguments.size()));
        }

        // First argument: array of header names (strings or identifiers)
        // For headers, we need to get the original values from the AST
        std::vector<std::string> headers;
        const ArrayExpression* headersArray = dynamic_cast<const ArrayExpression*>(expr.arguments[0].get());
        if (headersArray) {
            for (const auto& elem : headersArray->elements) {
                // Try to get the string value from StringLiteral first
                if (const auto* strLit = dynamic_cast<const StringLiteral*>(elem.get())) {
                    headers.push_back(strLit->value);
                } else if (const auto* identifier = dynamic_cast<const Identifier*>(elem.get())) {
                    // Use variable name as header
                    headers.push_back(identifier->name);
                } else {
                    // Fallback: use the expression's string representation
                    headers.push_back(elem->toString());
                }
            }
        } else {
            throw std::runtime_error("table() first argument must be an array literal of headers");
        }

        // Remaining arguments: column data arrays (numeric or string)
        std::vector<TableData::ColumnData> columns;
        for (size_t i = 1; i < expr.arguments.size(); ++i) {
            // Check if it's an array literal with string elements
            const ArrayExpression* arrayExpr = dynamic_cast<const ArrayExpression*>(expr.arguments[i].get());
            if (arrayExpr && !arrayExpr->elements.empty()) {
                // Check if first element is a string literal
                const StringLiteral* firstStr = dynamic_cast<const StringLiteral*>(arrayExpr->elements[0].get());
                if (firstStr) {
                    // String array - extract strings directly from AST
                    std::vector<std::string> stringCol;
                    for (const auto& elem : arrayExpr->elements) {
                        if (const auto* strLit = dynamic_cast<const StringLiteral*>(elem.get())) {
                            stringCol.push_back(strLit->value);
                        } else {
                            throw std::runtime_error("table() string array column must contain only string literals");
                        }
                    }
                    columns.push_back(stringCol);
                    continue;
                }
            }

            // Numeric array - evaluate normally
            Value colVal = evaluateExpression(*expr.arguments[i]);
            if (!std::holds_alternative<ArrayValue>(colVal)) {
                throw std::runtime_error("table() column argument " + std::to_string(i) + " must be an array");
            }
            const ArrayValue& colArray = std::get<ArrayValue>(colVal);
            columns.push_back(colArray.elements);
        }

        // Verify column count matches header count
        if (columns.size() != headers.size()) {
            throw std::runtime_error("table() number of columns (" + std::to_string(columns.size()) +
                                    ") must match number of headers (" + std::to_string(headers.size()) + ")");
        }

        return generateTable(headers, columns);
    }

    // Regular function call - evaluate arguments
    std::vector<Value> arguments;
    for (const auto& arg : expr.arguments) {
        arguments.push_back(evaluateExpression(*arg));
    }

    return callFunction(expr.function_name, arguments);
}

Value Evaluator::callFunction(const std::string& name, const std::vector<Value>& arguments) {
    // Resolve aliases first
    std::string resolvedName = env.resolveAlias(name);

    // First check if it's a built-in function
    if (resolvedName == "time") {
        if (!arguments.empty()) {
            throw std::runtime_error("Function time expects 0 arguments, got " + std::to_string(arguments.size()));
        }
        // Return current time in milliseconds
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        return static_cast<double>(millis);
    }

    if (resolvedName == "sqrt") {
        if (arguments.size() != 1) {
            throw std::runtime_error("Function sqrt expects 1 argument, got " + std::to_string(arguments.size()));
        }

        double val;
        if (std::holds_alternative<double>(arguments[0])) {
            val = std::get<double>(arguments[0]);
        } else if (std::holds_alternative<UnitValue>(arguments[0])) {
            val = std::get<UnitValue>(arguments[0]).value;
        } else {
            throw std::runtime_error("Function sqrt only supports numeric arguments");
        }

        if (val < 0) {
            throw std::runtime_error("Cannot take square root of negative number: " + std::to_string(val));
        }
        return std::sqrt(val);
    }

    if (resolvedName == "sin") {
        if (arguments.size() != 1) {
            throw std::runtime_error("Function sin expects 1 argument, got " + std::to_string(arguments.size()));
        }

        double val;
        if (std::holds_alternative<double>(arguments[0])) {
            val = std::get<double>(arguments[0]);
        } else if (std::holds_alternative<UnitValue>(arguments[0])) {
            val = std::get<UnitValue>(arguments[0]).value;
        } else {
            throw std::runtime_error("Function sin only supports numeric arguments");
        }

        return std::sin(val);
    }

    if (resolvedName == "cos") {
        if (arguments.size() != 1) {
            throw std::runtime_error("Function cos expects 1 argument, got " + std::to_string(arguments.size()));
        }

        double val;
        if (std::holds_alternative<double>(arguments[0])) {
            val = std::get<double>(arguments[0]);
        } else if (std::holds_alternative<UnitValue>(arguments[0])) {
            val = std::get<UnitValue>(arguments[0]).value;
        } else {
            throw std::runtime_error("Function cos only supports numeric arguments");
        }

        return std::cos(val);
    }

    if (resolvedName == "tan") {
        if (arguments.size() != 1) {
            throw std::runtime_error("Function tan expects 1 argument, got " + std::to_string(arguments.size()));
        }

        double val;
        if (std::holds_alternative<double>(arguments[0])) {
            val = std::get<double>(arguments[0]);
        } else if (std::holds_alternative<UnitValue>(arguments[0])) {
            val = std::get<UnitValue>(arguments[0]).value;
        } else {
            throw std::runtime_error("Function tan only supports numeric arguments");
        }

        return std::tan(val);
    }


    // Check if it's a WASM function (use resolved name for WASM lookup)
    if (wasmLoader.isFunctionAvailable(resolvedName)) {
        return callWasmFunction(resolvedName, arguments);
    }

    // Check if it's a piecewise function
    auto piecewiseIt = piecewiseFunctions.find(resolvedName);
    if (piecewiseIt != piecewiseFunctions.end()) {
        return callPiecewiseFunction(resolvedName, arguments, *piecewiseIt->second);
    }

    // Otherwise, call regular function (use resolved name)
    const FunctionDeclaration* func = env.getFunction(resolvedName);

    if (func->parameters.size() != arguments.size()) {
        throw std::runtime_error("Function " + name + " expects " +
                                std::to_string(func->parameters.size()) +
                                " arguments, got " +
                                std::to_string(arguments.size()));
    }

    // Create new environment for function scope
    Environment oldEnv = env; // Save current environment

    // Copy function definitions to new environment so recursive calls work
    env.copyFunctionsFrom(oldEnv);

    // Bind parameters to arguments
    for (size_t i = 0; i < func->parameters.size(); ++i) {
        env.define(func->parameters[i], arguments[i]);
    }

    // Save and reset control-flow state for this function invocation
    bool previousReturnPending = returnPending;
    Value previousPendingReturn = pendingReturnValue;
    bool previousBreakPending = breakPending;
    int previousLoopDepth = loopDepth;

    functionDepth++;
    returnPending = false;
    pendingReturnValue = 0.0;
    breakPending = false;

    Value returnValue = 0.0; // Default return value

    try {
        for (const auto& stmt : func->body) {
            if (const auto* returnStmt = dynamic_cast<const ReturnStatement*>(stmt.get())) {
                returnValue = executeReturn(*returnStmt);
                break; // Exit function on explicit return
            }

            std::vector<std::string> dummy_outputs; // Functions don't produce direct output
            executeStatement(*stmt, dummy_outputs);

            if (returnPending) {
                returnValue = pendingReturnValue;
                break;
            }

            if (breakPending) {
                breakPending = false;
                throw std::runtime_error("Break statement outside of loop");
            }
        }
    } catch (...) {
        env = oldEnv; // Restore environment before rethrowing
        returnPending = previousReturnPending;
        pendingReturnValue = previousPendingReturn;
        breakPending = previousBreakPending;
        loopDepth = previousLoopDepth;
        functionDepth--;
        throw;
    }

    env = oldEnv; // Restore original environment

    Value finalValue = returnPending ? pendingReturnValue : returnValue;

    // Restore previous control-flow state
    returnPending = previousReturnPending;
    pendingReturnValue = previousPendingReturn;
    breakPending = previousBreakPending;
    loopDepth = previousLoopDepth;
    functionDepth--;

    return finalValue;
}

void Evaluator::executePiecewiseFunction(const PiecewiseFunctionDeclaration& stmt) {
    // For piecewise functions, we need to store them separately since they have different evaluation logic

    // Create a temporary regular function to register the function name
    std::vector<StatementPtr> emptyBody;
    auto tempFunc = std::make_unique<FunctionDeclaration>(stmt.name, stmt.parameters, std::move(emptyBody));

    // Register the function
    env.defineFunction(stmt.name, *tempFunc);

    // Store the piecewise function for later use
    piecewiseFunctions[stmt.name] = &stmt;

    // Store the temp function
    static std::vector<std::unique_ptr<FunctionDeclaration>> tempFunctions;
    tempFunctions.push_back(std::move(tempFunc));
}

Value Evaluator::callPiecewiseFunction(const std::string& name, const std::vector<Value>& arguments, const PiecewiseFunctionDeclaration& func) {
    // Check parameter count
    if (func.parameters.size() != arguments.size()) {
        throw std::runtime_error("Function " + name + " expects " +
                                std::to_string(func.parameters.size()) +
                                " arguments, got " +
                                std::to_string(arguments.size()));
    }

    // Create new environment for function scope
    Environment oldEnv = env; // Save current environment

    // Bind parameters to arguments
    for (size_t i = 0; i < func.parameters.size(); ++i) {
        env.define(func.parameters[i], arguments[i]);
    }

    // Evaluate the piecewise expression
    Value result;
    try {
        result = evaluatePiecewiseExpression(*func.piecewise);
    } catch (const std::exception&) {
        env = oldEnv; // Restore environment on error
        throw;
    }

    // Restore environment
    env = oldEnv;

    return result;
}

Value Evaluator::callWasmFunction(const std::string& name, const std::vector<Value>& arguments) {
    const WasmFunction* wasmFunc = wasmLoader.getFunction(name);
    if (!wasmFunc) {
        throw std::runtime_error("WASM function '" + name + "' not found");
    }

    // Convert arguments to doubles (WASM functions expect double parameters)
    std::vector<double> doubleArgs;
    for (const auto& arg : arguments) {
        if (std::holds_alternative<double>(arg)) {
            doubleArgs.push_back(std::get<double>(arg));
        } else if (std::holds_alternative<UnitValue>(arg)) {
            doubleArgs.push_back(std::get<UnitValue>(arg).value);
        } else if (std::holds_alternative<std::string>(arg)) {
            // Try to convert string to double
            try {
                doubleArgs.push_back(std::stod(std::get<std::string>(arg)));
            } catch (...) {
                throw std::runtime_error("Cannot convert string argument to number for WASM function '" + name + "'");
            }
        }
    }

    // Execute the actual WASM function
    try {
#ifdef __EMSCRIPTEN__
        return executeWasmInBrowser(name, doubleArgs);
#else
        return executeWasmWithNodeJS(name, doubleArgs);
#endif
    } catch (const std::exception& e) {
        throw std::runtime_error("WASM execution failed for '" + name + "': " + e.what());
    }
}

Value Evaluator::executeWasmWithNodeJS(const std::string& name, const std::vector<double>& arguments) {
    const WasmFunction* wasmFunc = wasmLoader.getFunction(name);
    if (!wasmFunc) {
        throw std::runtime_error("WASM function not found: " + name);
    }

    // Build Node.js command to execute the WASM function (single line)
    std::stringstream jsCode;
    
    // Convert backslashes to forward slashes for Node.js compatibility on Windows
    std::string jsPath = wasmFunc->jsWrapperPath;
    std::replace(jsPath.begin(), jsPath.end(), '\\', '/');
    
    // Extract base path (directory containing the JS file)
    std::string basePath = jsPath.substr(0, jsPath.find_last_of('/'));
    
    jsCode << "const " << name << "Addon = require('" << jsPath << "'); ";
    jsCode << "async function run() { ";
    jsCode << "const addon = new " << name << "Addon('" << basePath << "'); ";
    jsCode << "await addon.load(); ";
    jsCode << "const result = addon." << name << "(";

    // Add arguments
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) jsCode << ", ";
        jsCode << arguments[i];
    }

    jsCode << "); ";
    jsCode << "console.log(result); ";
    jsCode << "} ";
    jsCode << "run().catch(console.error);";

    // Execute Node.js with the generated code
    std::string command = "node -e \"" + jsCode.str() + "\"";
    std::string output = executeCommand(command);

    // Parse the numeric result from the output
    try {
        // Remove any trailing whitespace/newlines
        output.erase(output.find_last_not_of(" \t\n\r\f\v") + 1);
        return std::stod(output);
    } catch (...) {
        throw std::runtime_error("Failed to parse numeric result from WASM execution: " + output);
    }
}

std::string Evaluator::executeCommand(const std::string& command) {
    std::string result;

#ifdef _WIN32
    // Windows implementation
    FILE* pipe = _popen(command.c_str(), "r");
#else
    // Unix/Linux implementation
    FILE* pipe = popen(command.c_str(), "r");
#endif

    if (!pipe) {
        throw std::runtime_error("Failed to execute command: " + command);
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }

#ifdef _WIN32
    int returnCode = _pclose(pipe);
#else
    int returnCode = pclose(pipe);
#endif

    if (returnCode != 0) {
        throw std::runtime_error("Command execution failed with code " + std::to_string(returnCode));
    }

    return result;
}

Value Evaluator::executeWasmInBrowser([[maybe_unused]] const std::string& name, [[maybe_unused]] const std::vector<double>& arguments) {
#ifdef __EMSCRIPTEN__
    const WasmFunction* wasmFunc = wasmLoader.getFunction(name);
    if (!wasmFunc) {
        throw std::runtime_error("WASM function not found: " + name);
    }

    // Build JSON arguments array
    std::stringstream argsJson;
    argsJson << "[";
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) argsJson << ",";
        argsJson << arguments[i];
    }
    argsJson << "]";

    // Call JavaScript bridge function via EM_ASM
    double result = EM_ASM_DOUBLE({
        var functionName = UTF8ToString($0);
        var argsJson = UTF8ToString($1);
        var args = JSON.parse(argsJson);

        console.log('[MADOLA WASM] Attempting to call function:', functionName, 'with args:', args);
        console.log('[MADOLA WASM] Bridge exists?', typeof window.madolaWasmBridge !== 'undefined');
        console.log('[MADOLA WASM] Bridge contents:', window.madolaWasmBridge);

        // Check if the bridge has the function loaded
        if (typeof window.madolaWasmBridge === 'undefined' ||
            typeof window.madolaWasmBridge[functionName] === 'undefined') {
            console.error('[MADOLA WASM] Function not available in bridge: ' + functionName);
            console.error('[MADOLA WASM] Available functions:', Object.keys(window.madolaWasmBridge || {}));
            return 0.0;
        }

        // Call the loaded WASM function through the bridge
        try {
            var wasmFunc = window.madolaWasmBridge[functionName];
            var result = wasmFunc.apply(null, args);
            console.log('[MADOLA WASM] Function call successful, result:', result);
            return result;
        } catch (e) {
            console.error('[MADOLA WASM] Error calling function ' + functionName + ':', e);
            return 0.0;
        }
    }, name.c_str(), argsJson.str().c_str());

    return result;
#else
    throw std::runtime_error("executeWasmInBrowser only available in Emscripten builds");
#endif
}

Value Evaluator::generateGraph(const ArrayValue& xArray, const ArrayValue& yArray, const std::string& title) {
    // Create graph data structure
    std::string graphTitle = title.empty() 
        ? "Graph " + std::to_string(collectedGraphs.size() + 1)
        : title;
    GraphData graphData(xArray.elements, yArray.elements, graphTitle);

    // Add to collection
    collectedGraphs.push_back(graphData);

    // Return success message
    return std::string("Graph " + std::to_string(collectedGraphs.size()) + " created with " +
                      std::to_string(xArray.elements.size()) + " data points");
}

Value Evaluator::generate3DGraph(const std::string& title, const std::vector<double>& dimensions) {
    // Default dimensions for brick with hole
    double width = 4.0, height = 2.0, depth = 3.0;
    double hole_width = 2.0, hole_height = 1.0, hole_depth = 1.5;

    // Override with provided dimensions
    if (dimensions.size() >= 1) width = dimensions[0];
    if (dimensions.size() >= 2) height = dimensions[1];
    if (dimensions.size() >= 3) depth = dimensions[2];
    if (dimensions.size() >= 4) hole_width = dimensions[3];
    if (dimensions.size() >= 5) hole_height = dimensions[4];
    if (dimensions.size() >= 6) hole_depth = dimensions[5];

    // Create 3D graph data structure
    Graph3DData graph3DData(title, "brick_with_hole", width, height, depth, hole_width, hole_height, hole_depth);

    // Add to collection
    collected3DGraphs.push_back(graph3DData);

    // Return success message
    return std::string("3D Graph '" + title + "' created with brick (" +
                      std::to_string(width) + "×" + std::to_string(height) + "×" + std::to_string(depth) +
                      ") and hole (" + std::to_string(hole_width) + "×" + std::to_string(hole_height) + "×" + std::to_string(hole_depth) + ")");
}

Value Evaluator::evaluateMethodCall(const MethodCall& expr) {
    // Check if this is a math.* call
    // The object will be an Identifier with name "math"
    if (const auto* identifier = dynamic_cast<const Identifier*>(expr.object.get())) {
        if (identifier->name == "math") {
            // math.summation
            if (expr.method_name == "summation") {
                if (expr.arguments.size() != 4) {
                    throw std::runtime_error("Function math.summation expects 4 arguments (expression, variable, lower_bound, upper_bound), got " + std::to_string(expr.arguments.size()));
                }

                // Extract variable name from second argument (must be an identifier)
                const auto* varIdentifier = dynamic_cast<const Identifier*>(expr.arguments[1].get());
                if (!varIdentifier) {
                    throw std::runtime_error("Second argument to math.summation must be a variable identifier");
                }

                // Create a SummationExpression and evaluate it directly
                const Expression* expressionPtr = expr.arguments[0].get();
                const Expression* lowerBoundPtr = expr.arguments[2].get();
                const Expression* upperBoundPtr = expr.arguments[3].get();

                return evaluateSummation(*expressionPtr, varIdentifier->name, *lowerBoundPtr, *upperBoundPtr);
            }
            // math.sin
            else if (expr.method_name == "sin") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.sin expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);
                double val;
                if (std::holds_alternative<double>(argVal)) {
                    val = std::get<double>(argVal);
                } else if (std::holds_alternative<UnitValue>(argVal)) {
                    val = std::get<UnitValue>(argVal).value;
                } else {
                    throw std::runtime_error("Function math.sin only supports numeric arguments");
                }

                return std::sin(val);
            }
            // math.cos
            else if (expr.method_name == "cos") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.cos expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);
                double val;
                if (std::holds_alternative<double>(argVal)) {
                    val = std::get<double>(argVal);
                } else if (std::holds_alternative<UnitValue>(argVal)) {
                    val = std::get<UnitValue>(argVal).value;
                } else {
                    throw std::runtime_error("Function math.cos only supports numeric arguments");
                }

                return std::cos(val);
            }
            // math.tan
            else if (expr.method_name == "tan") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.tan expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);
                double val;
                if (std::holds_alternative<double>(argVal)) {
                    val = std::get<double>(argVal);
                } else if (std::holds_alternative<UnitValue>(argVal)) {
                    val = std::get<UnitValue>(argVal).value;
                } else {
                    throw std::runtime_error("Function math.tan only supports numeric arguments");
                }

                return std::tan(val);
            }
            // math.sqrt
            else if (expr.method_name == "sqrt") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.sqrt expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);

                // Handle array input (element-wise sqrt)
                if (std::holds_alternative<ArrayValue>(argVal)) {
                    const ArrayValue& arr = std::get<ArrayValue>(argVal);
                    ArrayValue result = arr;  // Copy array structure

                    for (double& elem : result.elements) {
                        if (elem < 0) {
                            throw std::runtime_error("Cannot take square root of negative number: " + std::to_string(elem));
                        }
                        elem = std::sqrt(elem);
                    }
                    return result;
                }

                // Handle scalar input
                double val;
                if (std::holds_alternative<double>(argVal)) {
                    val = std::get<double>(argVal);
                } else if (std::holds_alternative<UnitValue>(argVal)) {
                    val = std::get<UnitValue>(argVal).value;
                } else {
                    throw std::runtime_error("Function math.sqrt only supports numeric or array arguments");
                }

                if (val < 0) {
                    throw std::runtime_error("Cannot take square root of negative number: " + std::to_string(val));
                }
                return std::sqrt(val);
            }
            // math.sqr
            else if (expr.method_name == "sqr") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.sqr expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);

                // Handle array input (element-wise sqr)
                if (std::holds_alternative<ArrayValue>(argVal)) {
                    const ArrayValue& arr = std::get<ArrayValue>(argVal);
                    ArrayValue result = arr;  // Copy array structure

                    for (double& elem : result.elements) {
                        elem = elem * elem;
                    }
                    return result;
                }

                // Handle scalar input
                double val;
                if (std::holds_alternative<double>(argVal)) {
                    val = std::get<double>(argVal);
                } else if (std::holds_alternative<UnitValue>(argVal)) {
                    val = std::get<UnitValue>(argVal).value;
                } else {
                    throw std::runtime_error("Function math.sqr only supports numeric or array arguments");
                }

                return val * val;
            }
            // math.abs
            else if (expr.method_name == "abs") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.abs expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);
                double val;
                if (std::holds_alternative<double>(argVal)) {
                    val = std::get<double>(argVal);
                } else if (std::holds_alternative<UnitValue>(argVal)) {
                    val = std::get<UnitValue>(argVal).value;
                } else {
                    throw std::runtime_error("Function math.abs only supports numeric arguments");
                }

                return std::abs(val);
            }
            // math.mod
            else if (expr.method_name == "mod") {
                if (expr.arguments.size() != 2) {
                    throw std::runtime_error("Function math.mod expects 2 arguments, got " + std::to_string(expr.arguments.size()));
                }

                Value arg1Val = evaluateExpression(*expr.arguments[0]);
                Value arg2Val = evaluateExpression(*expr.arguments[1]);

                double val1, val2;
                if (std::holds_alternative<double>(arg1Val)) {
                    val1 = std::get<double>(arg1Val);
                } else if (std::holds_alternative<UnitValue>(arg1Val)) {
                    val1 = std::get<UnitValue>(arg1Val).value;
                } else {
                    throw std::runtime_error("Function math.mod only supports numeric arguments");
                }

                if (std::holds_alternative<double>(arg2Val)) {
                    val2 = std::get<double>(arg2Val);
                } else if (std::holds_alternative<UnitValue>(arg2Val)) {
                    val2 = std::get<UnitValue>(arg2Val).value;
                } else {
                    throw std::runtime_error("Function math.mod only supports numeric arguments");
                }

                if (val2 == 0) {
                    throw std::runtime_error("Division by zero in math.mod");
                }

                return std::fmod(val1, val2);
            }
            // math.max
            else if (expr.method_name == "max") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.max expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);

                // Handle array input
                if (std::holds_alternative<ArrayValue>(argVal)) {
                    const auto& arr = std::get<ArrayValue>(argVal);
                    if (arr.elements.empty()) {
                        throw std::runtime_error("Function math.max cannot operate on empty array");
                    }

                    double maxVal = std::numeric_limits<double>::lowest();
                    for (const auto& elem : arr.elements) {
                        if (elem > maxVal) {
                            maxVal = elem;
                        }
                    }
                    return maxVal;
                }

                // Handle scalar input
                double val;
                if (std::holds_alternative<double>(argVal)) {
                    val = std::get<double>(argVal);
                } else if (std::holds_alternative<UnitValue>(argVal)) {
                    val = std::get<UnitValue>(argVal).value;
                } else {
                    throw std::runtime_error("Function math.max only supports numeric or array arguments");
                }

                return val;
            }
            // math.min
            else if (expr.method_name == "min") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.min expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);

                // Handle array input
                if (std::holds_alternative<ArrayValue>(argVal)) {
                    const auto& arr = std::get<ArrayValue>(argVal);
                    if (arr.elements.empty()) {
                        throw std::runtime_error("Function math.min cannot operate on empty array");
                    }

                    double minVal = std::numeric_limits<double>::max();
                    for (const auto& elem : arr.elements) {
                        if (elem < minVal) {
                            minVal = elem;
                        }
                    }
                    return minVal;
                }

                // Handle scalar input
                double val;
                if (std::holds_alternative<double>(argVal)) {
                    val = std::get<double>(argVal);
                } else if (std::holds_alternative<UnitValue>(argVal)) {
                    val = std::get<UnitValue>(argVal).value;
                } else {
                    throw std::runtime_error("Function math.min only supports numeric or array arguments");
                }

                return val;
            }
            // math.exp
            else if (expr.method_name == "exp") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.exp expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);

                // Handle array input
                if (std::holds_alternative<ArrayValue>(argVal)) {
                    const auto& arr = std::get<ArrayValue>(argVal);
                    std::vector<double> result;
                    result.reserve(arr.elements.size());

                    for (const auto& elem : arr.elements) {
                        result.push_back(std::exp(elem));
                    }
                    return ArrayValue(result, arr.isColumnVector);
                }

                // Handle scalar input
                double val;
                if (std::holds_alternative<double>(argVal)) {
                    val = std::get<double>(argVal);
                } else if (std::holds_alternative<UnitValue>(argVal)) {
                    val = std::get<UnitValue>(argVal).value;
                } else {
                    throw std::runtime_error("Function math.exp only supports numeric or array arguments");
                }

                return std::exp(val);
            }
            // math.sum
            else if (expr.method_name == "sum") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.sum expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);

                // Handle array input
                if (std::holds_alternative<ArrayValue>(argVal)) {
                    const auto& arr = std::get<ArrayValue>(argVal);
                    if (arr.elements.empty()) {
                        return 0.0; // Sum of empty array is 0
                    }

                    double sum = 0.0;
                    for (const auto& elem : arr.elements) {
                        sum += elem;
                    }
                    return sum;
                }

                // Handle scalar input (sum of a scalar is the scalar itself)
                double val;
                if (std::holds_alternative<double>(argVal)) {
                    val = std::get<double>(argVal);
                } else if (std::holds_alternative<UnitValue>(argVal)) {
                    val = std::get<UnitValue>(argVal).value;
                } else {
                    throw std::runtime_error("Function math.sum only supports numeric or array arguments");
                }

                return val;
            }
        }
    }

    // Evaluate the object expression
    Value objectValue = evaluateExpression(*expr.object);

    // Handle matrix/array methods
    if (std::holds_alternative<ArrayValue>(objectValue)) {
        const ArrayValue& arrayVal = std::get<ArrayValue>(objectValue);

        if (expr.method_name == "det") {
            if (!expr.arguments.empty()) {
                throw std::runtime_error("Method .det() expects no arguments, got " + std::to_string(expr.arguments.size()));
            }
            return matrixDeterminant(arrayVal);
        } else if (expr.method_name == "inv") {
            if (!expr.arguments.empty()) {
                throw std::runtime_error("Method .inv() expects no arguments, got " + std::to_string(expr.arguments.size()));
            }
            return matrixInverse(arrayVal);
        } else if (expr.method_name == "tr") {
            if (!expr.arguments.empty()) {
                throw std::runtime_error("Method .tr() expects no arguments, got " + std::to_string(expr.arguments.size()));
            }
            return matrixTrace(arrayVal);
        } else if (expr.method_name == "T") {
            if (!expr.arguments.empty()) {
                throw std::runtime_error("Method .T() expects no arguments, got " + std::to_string(expr.arguments.size()));
            }
            return matrixTranspose(arrayVal);
        } else if (expr.method_name == "eigenvalues") {
            if (!expr.arguments.empty()) {
                throw std::runtime_error("Method .eigenvalues() expects no arguments, got " + std::to_string(expr.arguments.size()));
            }
            return matrixEigenvalues(arrayVal);
        } else if (expr.method_name == "eigenvectors") {
            if (!expr.arguments.empty()) {
                throw std::runtime_error("Method .eigenvectors() expects no arguments, got " + std::to_string(expr.arguments.size()));
            }
            return matrixEigenvectors(arrayVal);
        } else {
            throw std::runtime_error("Unknown method '" + expr.method_name + "' for matrix/array");
        }
    }

    throw std::runtime_error("Method calls are only supported on matrices/arrays");
}

Value Evaluator::generateTable(const std::vector<std::string>& headers, const std::vector<TableData::ColumnData>& columns) {
    // Create table data structure
    TableData tableData(headers, columns);

    // Add to collection
    collectedTables.push_back(tableData);

    // Find max row count
    size_t maxRows = 0;
    for (const auto& col : columns) {
        if (std::holds_alternative<std::vector<double>>(col)) {
            maxRows = std::max(maxRows, std::get<std::vector<double>>(col).size());
        } else if (std::holds_alternative<std::vector<std::string>>(col)) {
            maxRows = std::max(maxRows, std::get<std::vector<std::string>>(col).size());
        }
    }

    // Return success message
    return std::string("Table " + std::to_string(collectedTables.size()) + " created with " +
                      std::to_string(headers.size()) + " columns and " +
                      std::to_string(maxRows) + " rows");
}

} // namespace madola