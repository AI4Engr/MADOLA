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

    // Special handling for eval() - explicitly evaluate and surface the value.
    if (expr.function_name == "eval") {
        if (expr.arguments.size() != 1) {
            throw std::runtime_error("Function eval() expects 1 argument, got " + std::to_string(expr.arguments.size()));
        }
        return evaluateExpression(*expr.arguments[0]);
    }

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

    // Special handling for svg function
    if (expr.function_name == "svg") {
        return collectSvg(expr);
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

    if (resolvedName == "testrecord") {
        // Minimal proof-of-concept record constructor: builds a 2-field record
        // to prove the RecordValue/member-access mechanism end-to-end (fields
        // store full Value, so units survive; displayLabel drives rendering).
        // Not a real feature — a placeholder for a future table-backed
        // constructor (e.g. a steel-section lookup) built on this mechanism.
        if (arguments.size() != 2) {
            throw std::runtime_error("Function testrecord expects 2 arguments, got " + std::to_string(arguments.size()));
        }

        RecordValue rec;
        rec.displayLabel = "test";
        (*rec.fields)["x"] = arguments[0];
        (*rec.fields)["y"] = arguments[1];
        return rec;
    }

    if (resolvedName == "section") {
        // Eager construction: look up a shape by name immediately. Contains no
        // steel/domain data itself — queries the "section" table registered at
        // runtime via Evaluator::registerLookupTable (e.g. by app/'s private JS
        // through the WASM-exported register_lookup_table).
        if (arguments.size() != 1) {
            throw std::runtime_error("Function section expects 1 argument (shape name), got " + std::to_string(arguments.size()));
        }
        if (!std::holds_alternative<std::string>(arguments[0])) {
            throw std::runtime_error("Function section expects a string shape name, e.g. section(\"W16X59\")");
        }
        const std::string& shapeName = std::get<std::string>(arguments[0]);
        Value rec = lookupRecordFromTable("section", shapeName);
        RecordValue& recordValue = std::get<RecordValue>(rec);
        (*recordValue.fields)["size"] = shapeName;
        recordValue.displayLabel = shapeName;
        return rec;
    }

    if (resolvedName == "Section") {
        // Lazy construction: an empty placeholder record. Assigning to its
        // "size" field later (s.size = "W16X59";) triggers the same lookup
        // this eager form performs immediately — see
        // Evaluator::executeRecordFieldAssignment.
        if (!arguments.empty()) {
            throw std::runtime_error("Function Section expects 0 arguments, got " + std::to_string(arguments.size()));
        }
        RecordValue rec;
        rec.displayLabel = "";
        (*rec.fields)["size"] = std::string("");
        return rec;
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
            // math.arcsin
            else if (expr.method_name == "arcsin") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.arcsin expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);
                double val;
                if (std::holds_alternative<double>(argVal)) {
                    val = std::get<double>(argVal);
                } else if (std::holds_alternative<UnitValue>(argVal)) {
                    val = std::get<UnitValue>(argVal).value;
                } else {
                    throw std::runtime_error("Function math.arcsin only supports numeric arguments");
                }

                if (val < -1.0 || val > 1.0) {
                    throw std::runtime_error("math.arcsin argument must be in range [-1, 1], got " + std::to_string(val));
                }
                return std::asin(val);
            }
            // math.arccos
            else if (expr.method_name == "arccos") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.arccos expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);
                double val;
                if (std::holds_alternative<double>(argVal)) {
                    val = std::get<double>(argVal);
                } else if (std::holds_alternative<UnitValue>(argVal)) {
                    val = std::get<UnitValue>(argVal).value;
                } else {
                    throw std::runtime_error("Function math.arccos only supports numeric arguments");
                }

                if (val < -1.0 || val > 1.0) {
                    throw std::runtime_error("math.arccos argument must be in range [-1, 1], got " + std::to_string(val));
                }
                return std::acos(val);
            }
            // math.arctan
            else if (expr.method_name == "arctan") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.arctan expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);
                double val;
                if (std::holds_alternative<double>(argVal)) {
                    val = std::get<double>(argVal);
                } else if (std::holds_alternative<UnitValue>(argVal)) {
                    val = std::get<UnitValue>(argVal).value;
                } else {
                    throw std::runtime_error("Function math.arctan only supports numeric arguments");
                }

                return std::atan(val);
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
            // math.ln (natural log)
            else if (expr.method_name == "ln") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.ln expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);
                double val;
                if (std::holds_alternative<double>(argVal)) {
                    val = std::get<double>(argVal);
                } else if (std::holds_alternative<UnitValue>(argVal)) {
                    val = std::get<UnitValue>(argVal).value;
                } else {
                    throw std::runtime_error("Function math.ln only supports numeric arguments");
                }

                if (val <= 0) {
                    throw std::runtime_error("math.ln argument must be positive, got " + std::to_string(val));
                }
                return std::log(val);
            }
            // math.log (base-10 log, or base-N log with a second argument)
            else if (expr.method_name == "log") {
                if (expr.arguments.size() != 1 && expr.arguments.size() != 2) {
                    throw std::runtime_error("Function math.log expects 1 or 2 arguments, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);
                double val;
                if (std::holds_alternative<double>(argVal)) {
                    val = std::get<double>(argVal);
                } else if (std::holds_alternative<UnitValue>(argVal)) {
                    val = std::get<UnitValue>(argVal).value;
                } else {
                    throw std::runtime_error("Function math.log only supports numeric arguments");
                }

                if (val <= 0) {
                    throw std::runtime_error("math.log argument must be positive, got " + std::to_string(val));
                }

                if (expr.arguments.size() == 1) {
                    return std::log10(val);
                }

                Value baseVal = evaluateExpression(*expr.arguments[1]);
                double base;
                if (std::holds_alternative<double>(baseVal)) {
                    base = std::get<double>(baseVal);
                } else if (std::holds_alternative<UnitValue>(baseVal)) {
                    base = std::get<UnitValue>(baseVal).value;
                } else {
                    throw std::runtime_error("Function math.log base only supports numeric arguments");
                }

                if (base <= 0 || base == 1.0) {
                    throw std::runtime_error("math.log base must be positive and not equal to 1, got " + std::to_string(base));
                }
                return std::log(val) / std::log(base);
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
            // math.mean
            else if (expr.method_name == "mean") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.mean expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);
                if (!std::holds_alternative<ArrayValue>(argVal)) {
                    throw std::runtime_error("Function math.mean only supports array arguments");
                }

                const auto& arr = std::get<ArrayValue>(argVal);
                if (arr.elements.empty()) {
                    throw std::runtime_error("Function math.mean cannot operate on empty array");
                }

                double sum = 0.0;
                for (const auto& elem : arr.elements) {
                    sum += elem;
                }
                return sum / static_cast<double>(arr.elements.size());
            }
            // math.variance (population variance)
            else if (expr.method_name == "variance") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.variance expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);
                if (!std::holds_alternative<ArrayValue>(argVal)) {
                    throw std::runtime_error("Function math.variance only supports array arguments");
                }

                const auto& arr = std::get<ArrayValue>(argVal);
                if (arr.elements.empty()) {
                    throw std::runtime_error("Function math.variance cannot operate on empty array");
                }

                double sum = 0.0;
                for (const auto& elem : arr.elements) {
                    sum += elem;
                }
                double mean = sum / static_cast<double>(arr.elements.size());

                double sqDiffSum = 0.0;
                for (const auto& elem : arr.elements) {
                    double diff = elem - mean;
                    sqDiffSum += diff * diff;
                }
                return sqDiffSum / static_cast<double>(arr.elements.size());
            }
            // math.std (population standard deviation)
            else if (expr.method_name == "std") {
                if (expr.arguments.size() != 1) {
                    throw std::runtime_error("Function math.std expects 1 argument, got " + std::to_string(expr.arguments.size()));
                }

                Value argVal = evaluateExpression(*expr.arguments[0]);
                if (!std::holds_alternative<ArrayValue>(argVal)) {
                    throw std::runtime_error("Function math.std only supports array arguments");
                }

                const auto& arr = std::get<ArrayValue>(argVal);
                if (arr.elements.empty()) {
                    throw std::runtime_error("Function math.std cannot operate on empty array");
                }

                double sum = 0.0;
                for (const auto& elem : arr.elements) {
                    sum += elem;
                }
                double mean = sum / static_cast<double>(arr.elements.size());

                double sqDiffSum = 0.0;
                for (const auto& elem : arr.elements) {
                    double diff = elem - mean;
                    sqDiffSum += diff * diff;
                }
                return std::sqrt(sqDiffSum / static_cast<double>(arr.elements.size()));
            }
            // math.pi (constant)
            else if (expr.method_name == "pi") {
                if (!expr.arguments.empty()) {
                    throw std::runtime_error("Constant math.pi takes no arguments, got " + std::to_string(expr.arguments.size()));
                }
                return std::acos(-1.0);
            }
            // math.e (constant, Euler's number)
            else if (expr.method_name == "e") {
                if (!expr.arguments.empty()) {
                    throw std::runtime_error("Constant math.e takes no arguments, got " + std::to_string(expr.arguments.size()));
                }
                return std::exp(1.0);
            }
            // math.phi (constant, golden ratio)
            else if (expr.method_name == "phi") {
                if (!expr.arguments.empty()) {
                    throw std::runtime_error("Constant math.phi takes no arguments, got " + std::to_string(expr.arguments.size()));
                }
                return (1.0 + std::sqrt(5.0)) / 2.0;
            }
#ifdef WITH_SYMENGINE
            // math.diff
            else if (expr.method_name == "diff") {
                if (expr.arguments.size() != 2) {
                    throw std::runtime_error("Function math.diff() expects 2 arguments (expression, variable), got " + std::to_string(expr.arguments.size()));
                }

                // Second argument must be an identifier (the variable to differentiate with respect to)
                const auto* varIdentifier = dynamic_cast<const Identifier*>(expr.arguments[1].get());
                if (!varIdentifier) {
                    throw std::runtime_error("Second argument to math.diff() must be a variable identifier");
                }

                // First argument is the expression to differentiate
                // Don't evaluate it - pass it directly to symbolic differentiation
                return evaluateSymbolicDiff(*expr.arguments[0], varIdentifier->name);
            }
            // math.intg
            else if (expr.method_name == "intg") {
                if (expr.arguments.size() != 2) {
                    throw std::runtime_error("Function math.intg() expects 2 arguments (expression, variable), got " + std::to_string(expr.arguments.size()));
                }

                // Second argument must be an identifier (the variable to integrate with respect to)
                const auto* varIdentifier = dynamic_cast<const Identifier*>(expr.arguments[1].get());
                if (!varIdentifier) {
                    throw std::runtime_error("Second argument to math.intg() must be a variable identifier");
                }

                // First argument is the expression to integrate
                // Don't evaluate it - pass it directly to symbolic integration
                return evaluateSymbolicIntegral(*expr.arguments[0], varIdentifier->name);
            }
#endif
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

Value Evaluator::evaluateMemberAccess(const MemberAccess& expr) {
    Value objectValue = evaluateExpression(*expr.object);

    if (std::holds_alternative<RecordValue>(objectValue)) {
        const RecordValue& record = std::get<RecordValue>(objectValue);
        if (!record.has(expr.member_name)) {
            throw std::runtime_error("Undefined field: " + expr.member_name);
        }
        return record.at(expr.member_name);
    }

    throw std::runtime_error("Member access ('.') is only supported on records");
}

Value Evaluator::lookupRecordFromTable(const std::string& tableName, const std::string& key) {
    auto tableIt = s_lookupTables.find(tableName);
    if (tableIt == s_lookupTables.end()) {
        throw std::runtime_error("Lookup table '" + tableName + "' is not registered "
            "(app must register it at startup before using " + tableName + "(...))");
    }
    auto rowIt = tableIt->second.find(key);
    if (rowIt == tableIt->second.end()) {
        throw std::runtime_error("Unknown " + tableName + " entry: '" + key + "'");
    }

    // Deep-copy fields so the caller's RecordValue never aliases (and can never
    // corrupt via a later field assignment) the canonical stored template row.
    RecordValue result;
    result.displayLabel = rowIt->second.displayLabel;
    result.fields = std::make_shared<std::map<std::string, Value>>(*rowIt->second.fields);
    return result;
}

namespace {

double requireNumber(Evaluator& evaluator, const Expression& expr, const char* context) {
    Value v = evaluator.evaluateExpression(expr);
    double value;
    if (std::holds_alternative<double>(v)) {
        value = std::get<double>(v);
    } else if (std::holds_alternative<UnitValue>(v) &&
               std::get<UnitValue>(v).isDimensionless()) {
        // A ratio whose units cancel (e.g. deflection/allowable, or a length
        // divided by a length) stays wrapped in UnitValue with an empty unit
        // string — UnitValue::operator/ never collapses back to a plain double.
        // Such a value IS a number, so accept it rather than making callers
        // hunt for a unit-stripping idiom the language doesn't have. Anything
        // still carrying a real unit is still rejected: passing "5 mm" as an
        // SVG coordinate is a genuine mistake worth reporting.
        value = std::get<UnitValue>(v).value;
    } else {
        throw std::runtime_error(std::string(context) + " expects a number argument");
    }
    // Reject inf/nan here rather than letting them leak into SVG attributes (produces
    // invalid markup like cx="inf") or data-* attrs (breaks JS parseFloat on recompute).
    // Callers that sample a curve expression already catch this and treat it as a gap.
    if (!std::isfinite(value)) {
        throw std::runtime_error(std::string(context) + " produced a non-finite value (inf/nan)");
    }
    return value;
}

std::string requireStringLiteral(const Expression& expr, const char* context) {
    const auto* strLit = dynamic_cast<const StringLiteral*>(&expr);
    if (!strLit) {
        throw std::runtime_error(std::string(context) + " expects a string literal argument");
    }
    return strLit->value;
}

} // namespace

Value Evaluator::collectSvg(const FunctionCall& expr) {
    if (expr.arguments.size() < 2) {
        throw std::runtime_error("Function svg expects at least 2 arguments (width, height, [shapes...]), got " +
                                  std::to_string(expr.arguments.size()));
    }

    SvgData svgData;
    svgData.width = requireNumber(*this, *expr.arguments[0], "svg() width");
    svgData.height = requireNumber(*this, *expr.arguments[1], "svg() height");

    for (size_t i = 2; i < expr.arguments.size(); ++i) {
        const auto* shapeCall = dynamic_cast<const FunctionCall*>(expr.arguments[i].get());
        if (!shapeCall) {
            throw std::runtime_error("svg() arguments after width/height must be shape calls "
                                      "(line/circle/arrow/text/rect/curve)");
        }

        const std::string& shapeName = shapeCall->function_name;
        const auto& args = shapeCall->arguments;
        SvgShape shape;

        if (shapeName == "line" || shapeName == "arrow") {
            if (args.size() != 4) {
                throw std::runtime_error(shapeName + "() expects 4 arguments (x1,y1,x2,y2), got " +
                                          std::to_string(args.size()));
            }
            shape.kind = (shapeName == "line") ? SvgShape::Kind::Line : SvgShape::Kind::Arrow;
            for (const auto& arg : args) {
                shape.nums.push_back(requireNumber(*this, *arg, shapeName.c_str()));
            }
        } else if (shapeName == "circle") {
            if (args.size() != 3) {
                throw std::runtime_error("circle() expects 3 arguments (cx,cy,r), got " +
                                          std::to_string(args.size()));
            }
            shape.kind = SvgShape::Kind::Circle;
            for (const auto& arg : args) {
                shape.nums.push_back(requireNumber(*this, *arg, "circle()"));
            }
        } else if (shapeName == "rect") {
            if (args.size() != 4) {
                throw std::runtime_error("rect() expects 4 arguments (x,y,w,h), got " +
                                          std::to_string(args.size()));
            }
            shape.kind = SvgShape::Kind::Rect;
            for (const auto& arg : args) {
                shape.nums.push_back(requireNumber(*this, *arg, "rect()"));
            }
        } else if (shapeName == "text") {
            if (args.size() != 3) {
                throw std::runtime_error("text() expects 3 arguments (x,y,\"label\"), got " +
                                          std::to_string(args.size()));
            }
            shape.kind = SvgShape::Kind::Text;
            shape.nums.push_back(requireNumber(*this, *args[0], "text()"));
            shape.nums.push_back(requireNumber(*this, *args[1], "text()"));
            shape.text = requireStringLiteral(*args[2], "text()");
        } else if (shapeName == "curve") {
            if (args.size() < 5 || args.size() > 7) {
                throw std::runtime_error("curve() expects 5-7 arguments (expr, sampleVar, start, end, samples, "
                                          "[\"curveId\"], [maxAbsY]), got " + std::to_string(args.size()));
            }
            shape.kind = SvgShape::Kind::Curve;

            const auto* sampleVarIdent = dynamic_cast<const Identifier*>(args[1].get());
            if (!sampleVarIdent) {
                throw std::runtime_error("curve() second argument must be an identifier naming the sample variable");
            }
            shape.sampleVar = sampleVarIdent->name;
            shape.start = requireNumber(*this, *args[2], "curve()");
            shape.end = requireNumber(*this, *args[3], "curve()");
            double samplesVal = requireNumber(*this, *args[4], "curve()");
            if (samplesVal < 2 || std::floor(samplesVal) != samplesVal) {
                throw std::runtime_error("curve() samples argument must be an integer >= 2");
            }
            shape.samples = static_cast<int>(samplesVal);
            shape.curveId = (args.size() >= 6)
                ? requireStringLiteral(*args[5], "curve()")
                : ("curve" + std::to_string(svgData.shapes.size()));
            if (args.size() == 7) {
                shape.maxAbsYHint = std::abs(requireNumber(*this, *args[6], "curve() maxAbsY"));
            }

            // Sample the curve by rebinding sampleVar over [start,end], reusing the same
            // save/rebind/restore idiom as executeFor's loop-variable rebinding.
            bool varExisted = env.exists(shape.sampleVar);
            Value oldValue;
            if (varExisted) {
                oldValue = env.get(shape.sampleVar);
            }

            shape.xs.reserve(shape.samples);
            shape.ys.reserve(shape.samples);
            for (int s = 0; s < shape.samples; ++s) {
                double t = (shape.samples == 1)
                    ? shape.start
                    : shape.start + (shape.end - shape.start) * s / (shape.samples - 1);
                env.define(shape.sampleVar, t);
                double yv;
                try {
                    yv = requireNumber(*this, *args[0], "curve() expression");
                } catch (const std::exception&) {
                    yv = std::nan("");
                }
                shape.xs.push_back(t);
                shape.ys.push_back(yv);
            }

            if (varExisted) {
                env.define(shape.sampleVar, oldValue);
            } else {
                env.remove(shape.sampleVar);
            }
        } else {
            throw std::runtime_error("Unknown svg shape '" + shapeName +
                                      "' - expected line/circle/arrow/text/rect/curve");
        }

        svgData.shapes.push_back(std::move(shape));
    }

    collectedSvgs.push_back(std::move(svgData));
    return std::string("SVG " + std::to_string(collectedSvgs.size()) + " created with " +
                        std::to_string(collectedSvgs.back().shapes.size()) + " shapes");
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

namespace {

// Find the FunctionCall AST node for a top-level svg(...) statement, unwrapping the
// handful of statement shapes that can carry an expression (mirrors the isSvgCall
// detection in html_formatter_helpers.cpp::generateOrderedContent).
const FunctionCall* asTopLevelCall(const Statement* stmt, const char* name) {
    const FunctionCall* func = dynamic_cast<const FunctionCall*>(stmt);
    if (!func) {
        if (const auto* exprStmt = dynamic_cast<const ExpressionStatement*>(stmt)) {
            func = dynamic_cast<const FunctionCall*>(exprStmt->expression.get());
        } else if (const auto* printStmt = dynamic_cast<const PrintStatement*>(stmt)) {
            func = dynamic_cast<const FunctionCall*>(printStmt->expression.get());
        }
    }
    if (func && func->function_name == name) {
        return func;
    }
    return nullptr;
}

// Recompute the exact same data-space bounds and beam-axis pixel anchor generateSvgHtml
// derives from an SvgData's curves and line() shapes, so the pixel transform used on
// recompute matches the initial render exactly.
void computeCurveBounds(const SvgData& svg, double& xmin, double& xmax, double& maxAbsY) {
    bool have = false;
    xmin = xmax = 0.0;
    maxAbsY = 0.0;
    for (const auto& shape : svg.shapes) {
        if (shape.kind != SvgShape::Kind::Curve) continue;
        if (shape.maxAbsYHint > 0.0) {
            maxAbsY = std::max(maxAbsY, shape.maxAbsYHint);
        }
        for (size_t j = 0; j < shape.xs.size(); ++j) {
            double x = shape.xs[j];
            double y = shape.ys[j];
            if (std::isnan(x) || std::isnan(y)) continue;
            if (!have) {
                xmin = xmax = x;
                have = true;
            } else {
                xmin = std::min(xmin, x);
                xmax = std::max(xmax, x);
            }
            if (shape.maxAbsYHint <= 0.0) {
                maxAbsY = std::max(maxAbsY, std::abs(y));
            }
        }
    }
    if (!have || xmax == xmin) { xmin = 0.0; xmax = 1.0; }
    if (maxAbsY == 0.0) { maxAbsY = 1.0; }
}

// Find the beam axis (longest horizontal line() in this svg()) and its pixel span, the
// same way generateSvgHtml anchors a curve's zero-deflection point to the drawn support.
void computeBeamAxis(const SvgData& svg, double margin, double& baselinePy,
                      double& beamPxStart, double& beamPxEnd) {
    baselinePy = margin + (svg.height - 2 * margin) / 2.0;
    beamPxStart = margin;
    beamPxEnd = svg.width - margin;
    double bestLen = -1.0;
    for (const auto& shape : svg.shapes) {
        if (shape.kind != SvgShape::Kind::Line) continue;
        double x1 = shape.nums[0], y1 = shape.nums[1];
        double x2 = shape.nums[2], y2 = shape.nums[3];
        if (std::abs(y1 - y2) > 0.5) continue;  // not horizontal
        double len = std::abs(x2 - x1);
        if (len > bestLen) {
            bestLen = len;
            baselinePy = y1;
            beamPxStart = std::min(x1, x2);
            beamPxEnd = std::max(x1, x2);
        }
    }
}

} // namespace

std::optional<std::string> Evaluator::resampleSvgCurve(const Program& program,
                                                         const std::vector<SvgData>& svgs,
                                                         const std::string& curveId,
                                                         const std::string& paramName,
                                                         double paramValue) {
    // Locate the curve's SvgData (for width/height and the fixed pixel transform) and
    // the matching AST curve() call (for its still-unevaluated expression), walking both
    // in the same declaration order collectSvg used originally.
    const SvgData* ownerSvg = nullptr;
    const SvgShape* ownerShape = nullptr;
    const FunctionCall* curveCall = nullptr;

    size_t svgIndex = 0;
    for (const auto& stmt : program.statements) {
        const FunctionCall* svgFunc = asTopLevelCall(stmt.get(), "svg");
        if (!svgFunc) continue;
        if (svgIndex >= svgs.size()) break;
        const SvgData& svgData = svgs[svgIndex];

        for (const auto& shape : svgData.shapes) {
            if (shape.kind == SvgShape::Kind::Curve && shape.curveId == curveId) {
                ownerSvg = &svgData;
                ownerShape = &shape;
                break;
            }
        }

        if (ownerShape) {
            // Re-locate the matching curve(...) argument in the fresh AST by position:
            // shapes and svg() arguments (after width/height) are in the same order.
            size_t shapeIdx = static_cast<size_t>(ownerShape - svgData.shapes.data());
            size_t argIdx = 2 + shapeIdx;
            if (argIdx < svgFunc->arguments.size()) {
                curveCall = dynamic_cast<const FunctionCall*>(svgFunc->arguments[argIdx].get());
            }
            break;
        }

        svgIndex++;
    }

    if (!ownerSvg || !ownerShape || !curveCall || curveCall->arguments.empty()) {
        return std::nullopt;
    }

    // Rebind the dragged parameter, resample over the curve's original domain.
    bool paramExisted = env.exists(paramName);
    Value oldParamValue;
    if (paramExisted) {
        oldParamValue = env.get(paramName);
    }
    env.define(paramName, paramValue);

    bool varExisted = env.exists(ownerShape->sampleVar);
    Value oldVarValue;
    if (varExisted) {
        oldVarValue = env.get(ownerShape->sampleVar);
    }

    std::vector<double> xs, ys;
    xs.reserve(ownerShape->samples);
    ys.reserve(ownerShape->samples);
    for (int s = 0; s < ownerShape->samples; ++s) {
        double t = (ownerShape->samples == 1)
            ? ownerShape->start
            : ownerShape->start + (ownerShape->end - ownerShape->start) * s / (ownerShape->samples - 1);
        env.define(ownerShape->sampleVar, t);
        double yv;
        try {
            yv = requireNumber(*this, *curveCall->arguments[0], "curve() expression");
        } catch (const std::exception&) {
            yv = std::nan("");
        }
        xs.push_back(t);
        ys.push_back(yv);
    }

    if (varExisted) {
        env.define(ownerShape->sampleVar, oldVarValue);
    } else {
        env.remove(ownerShape->sampleVar);
    }
    if (paramExisted) {
        env.define(paramName, oldParamValue);
    } else {
        env.remove(paramName);
    }

    // Reproduce generateSvgHtml's exact transform (fixed to the *original* svg's bounds
    // and beam axis, not the resampled points, so the axis/scale don't shift mid-drag).
    double xmin, xmax, maxAbsY;
    computeCurveBounds(*ownerSvg, xmin, xmax, maxAbsY);
    const double margin = 10.0;
    double baselinePy, beamPxStart, beamPxEnd;
    computeBeamAxis(*ownerSvg, margin, baselinePy, beamPxStart, beamPxEnd);
    double availAbove = baselinePy - margin;
    double availBelow = (ownerSvg->height - margin) - baselinePy;
    double ampPx = std::max(1.0, std::min(availAbove, availBelow));
    double yScale = ampPx / maxAbsY;

    std::stringstream d;
    bool started = false;
    for (size_t j = 0; j < xs.size(); ++j) {
        if (std::isnan(xs[j]) || std::isnan(ys[j])) {
            started = false;
            continue;
        }
        double px = beamPxStart + (xs[j] - xmin) / (xmax - xmin) * (beamPxEnd - beamPxStart);
        double py = baselinePy - ys[j] * yScale;
        d << (started ? " L " : "M ") << px << " " << py;
        started = true;
    }

    return d.str();
}

} // namespace madola
