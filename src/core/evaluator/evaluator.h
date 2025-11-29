#pragma once
#include "../ast/ast.h"
#include "../generator/cpp_generator.h"
#include "../generator/wasm_generator.h"
#include "../generator/wasm_addon_loader.h"
#include "../generator/unit_system.h"
#include <unordered_map>
#include <string>
#include <variant>
#include <memory>
#include <stdexcept>

namespace madola {

struct ComplexValue {
    double real;
    double imaginary;

    ComplexValue(double r = 0.0, double i = 0.0)
        : real(r), imaginary(i) {}

    ComplexValue operator+(const ComplexValue& other) const {
        return ComplexValue(real + other.real, imaginary + other.imaginary);
    }

    ComplexValue operator-(const ComplexValue& other) const {
        return ComplexValue(real - other.real, imaginary - other.imaginary);
    }

    ComplexValue operator*(const ComplexValue& other) const {
        // (a + bi)(c + di) = (ac - bd) + (ad + bc)i
        return ComplexValue(
            real * other.real - imaginary * other.imaginary,
            real * other.imaginary + imaginary * other.real
        );
    }

    ComplexValue operator/(const ComplexValue& other) const {
        // (a + bi)/(c + di) = ((ac + bd) + (bc - ad)i) / (c^2 + d^2)
        double denominator = other.real * other.real + other.imaginary * other.imaginary;
        if (denominator == 0.0) {
            throw std::runtime_error("Division by zero in complex division");
        }
        return ComplexValue(
            (real * other.real + imaginary * other.imaginary) / denominator,
            (imaginary * other.real - real * other.imaginary) / denominator
        );
    }

    ComplexValue operator-() const {
        return ComplexValue(-real, -imaginary);
    }
};

struct ArrayValue {
    std::vector<double> elements;
    bool isColumnVector;
    bool isMatrix;
    std::vector<std::vector<double>> matrixRows;

    ArrayValue(std::vector<double> elems, bool isColumn = false)
        : elements(std::move(elems)), isColumnVector(isColumn), isMatrix(false) {}

    ArrayValue(std::vector<std::vector<double>> rows)
        : isColumnVector(false), isMatrix(true), matrixRows(std::move(rows)) {}
};

using Value = std::variant<double, std::string, ComplexValue, UnitValue, ArrayValue>;

struct GraphData {
    std::vector<double> x_values;
    std::vector<double> y_values;
    std::string title;

    GraphData(const std::vector<double>& x, const std::vector<double>& y, const std::string& t = "")
        : x_values(x), y_values(y), title(t) {}
};

struct Graph3DData {
    std::string type;  // e.g., "brick_with_hole"
    std::string title;
    double width, height, depth;  // brick dimensions
    double hole_width, hole_height, hole_depth;  // hole dimensions

    Graph3DData(const std::string& t = "", const std::string& type_name = "brick_with_hole",
                double w = 4.0, double h = 2.0, double d = 3.0,
                double hw = 2.0, double hh = 1.0, double hd = 1.5)
        : type(type_name), title(t), width(w), height(h), depth(d),
          hole_width(hw), hole_height(hh), hole_depth(hd) {}
};

struct TableData {
    using ColumnData = std::variant<std::vector<double>, std::vector<std::string>>;

    std::vector<std::string> headers;  // Column titles
    std::vector<ColumnData> columns;   // Column data arrays (numeric or string)

    TableData() = default;
    TableData(const std::vector<std::string>& h, const std::vector<ColumnData>& c)
        : headers(h), columns(c) {}

    // Legacy constructor for backward compatibility
    TableData(const std::vector<std::string>& h, const std::vector<std::vector<double>>& c)
        : headers(h) {
        for (const auto& col : c) {
            columns.push_back(col);
        }
    }
};

// Forward declarations
class FunctionDeclaration;

struct Function {
    std::string name;
    std::vector<std::string> parameters;
    std::vector<StatementPtr> body;

    Function(const std::string& n,
             const std::vector<std::string>& params,
             const std::vector<StatementPtr>& /* statements */)
        : name(n), parameters(params) {
        // Deep copy the statements - handled in evaluator for now
    }
};

class Environment {
public:
    void define(const std::string& name, const Value& value);
    Value get(const std::string& name) const;
    bool exists(const std::string& name) const;

    void defineFunction(const std::string& name, const FunctionDeclaration& func);
    const FunctionDeclaration* getFunction(const std::string& name) const;
    bool functionExists(const std::string& name) const;
    void copyFunctionsFrom(const Environment& other);

    // For handling import aliases
    void defineAlias(const std::string& aliasName, const std::string& originalName);
    std::string resolveAlias(const std::string& name) const;

private:
    std::unordered_map<std::string, Value> variables;
    std::unordered_map<std::string, const FunctionDeclaration*> functions;
    std::unordered_map<std::string, std::string> aliases; // alias -> original name
};

class Evaluator {
public:
    explicit Evaluator();

    struct EvaluationResult {
        std::vector<std::string> outputs;
        std::vector<CppGenerator::GeneratedFile> cppFiles;
        std::vector<WasmGenerator::GeneratedWasm> wasmFiles;
        std::vector<GraphData> graphs;
        std::vector<Graph3DData> graphs3d;
        std::vector<TableData> tables;
        bool success;
        std::string error;
    };

    EvaluationResult evaluate(const Program& program, const std::string& mdaFileName = "default");
    Value evaluateExpression(const Expression& expr);
    Value getVariableValue(const std::string& name) const;
    void executeStatement(const Statement& stmt, std::vector<std::string>& outputs);

#ifdef WITH_SYMENGINE
    // Public symbolic computation functions
    Value evaluateSymbolicExpression(const std::string& symbolicExpr);
#endif

private:
    Environment env;
    CppGenerator cppGen;
    WasmGenerator wasmGen;
    WasmAddonLoader wasmLoader;
    std::vector<GraphData> collectedGraphs;
    std::vector<Graph3DData> collected3DGraphs;
    std::vector<TableData> collectedTables;
    std::vector<ProgramPtr> importedPrograms;  // Keep imported programs alive

    void executeAssignment(const AssignmentStatement& stmt);
    void executeArrayAssignment(const ArrayAssignmentStatement& stmt);
    void executePrint(const PrintStatement& stmt, std::vector<std::string>& outputs);
    void executeExpressionStatement(const ExpressionStatement& stmt, std::vector<std::string>& outputs);
    void executeFunction(const FunctionDeclaration& stmt);
    void executePiecewiseFunction(const PiecewiseFunctionDeclaration& stmt);
    void executeImport(const ImportStatement& stmt);
    void executeFor(const ForStatement& stmt, std::vector<std::string>& outputs);
    void executeWhile(const WhileStatement& stmt, std::vector<std::string>& outputs);
    void executeIf(const IfStatement& stmt, std::vector<std::string>& outputs);
    Value executeReturn(const ReturnStatement& stmt);
    Value evaluateBinaryExpression(const BinaryExpression& expr);
    Value evaluateUnaryExpression(const UnaryExpression& expr);
    Value evaluatePiecewiseExpression(const PiecewiseExpression& expr);
    Value evaluateSummationExpression(const SummationExpression& expr);
    Value evaluateSummation(const Expression& expr, const std::string& variable, const Expression& lowerBound, const Expression& upperBound);
    Value evaluateFunctionCall(const FunctionCall& expr);
    Value evaluateMethodCall(const MethodCall& expr);
    Value evaluateUnitExpression(const UnitExpression& expr);
    Value evaluateArrayExpression(const ArrayExpression& expr);
    Value evaluateArrayAccess(const ArrayAccess& expr);
    Value multiplyMatrixVector(const ArrayValue& matrix, const ArrayValue& vector);
    Value multiplyVectorMatrix(const ArrayValue& vector, const ArrayValue& matrix);
    Value multiplyMatrixMatrix(const ArrayValue& matrix1, const ArrayValue& matrix2);
    Value dotProduct(const ArrayValue& vector1, const ArrayValue& vector2);
    Value matrixDeterminant(const ArrayValue& matrix);
    Value matrixInverse(const ArrayValue& matrix);
    Value matrixTrace(const ArrayValue& matrix);
    Value matrixTranspose(const ArrayValue& matrix);
    Value matrixEigenvalues(const ArrayValue& matrix);
    Value matrixEigenvectors(const ArrayValue& matrix);
    std::string valueToString(const Value& value);

#ifdef WITH_SYMENGINE
    // Symbolic computation functions (private)
    Value evaluateSymbolicDiff(const Expression& expr, const std::string& variable);
    Value evaluateSymbolicMatrixDiff(const ArrayValue& matrix, const std::string& variable);
#endif
    Value generateGraph(const ArrayValue& xArray, const ArrayValue& yArray, const std::string& title = "");
    Value generate3DGraph(const std::string& title, const std::vector<double>& dimensions);
    Value generateTable(const std::vector<std::string>& headers, const std::vector<TableData::ColumnData>& columns);

    // For function call execution
    Value callFunction(const std::string& name, const std::vector<Value>& arguments);
    Value callPiecewiseFunction(const std::string& name, const std::vector<Value>& arguments, const PiecewiseFunctionDeclaration& func);
    Value callWasmFunction(const std::string& name, const std::vector<Value>& arguments);
    Value executeWasmWithNodeJS(const std::string& name, const std::vector<double>& arguments);
    Value executeWasmInBrowser(const std::string& name, const std::vector<double>& arguments);
    std::string executeCommand(const std::string& command);

    // Control flow state (used instead of C++ exceptions for WASM compatibility)
    bool returnPending = false;
    Value pendingReturnValue = 0.0;
    bool breakPending = false;
    int functionDepth = 0;
    int loopDepth = 0;
};

using EvaluatorPtr = std::unique_ptr<Evaluator>;

} // namespace madola