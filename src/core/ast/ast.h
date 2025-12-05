#pragma once
#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <cstdint>

// AST Node definitions for MADOLA language

namespace madola {

struct SourceLocation {
    uint32_t line;
    uint32_t column;
    uint32_t offset;

    SourceLocation(uint32_t l = 0, uint32_t c = 0, uint32_t o = 0)
        : line(l), column(c), offset(o) {}

    bool isValid() const { return line > 0 || column > 0 || offset > 0; }
};

class ASTNode {
public:
    SourceLocation start_pos;
    SourceLocation end_pos;

    ASTNode(const SourceLocation& start = SourceLocation(),
            const SourceLocation& end = SourceLocation())
        : start_pos(start), end_pos(end) {}

    virtual ~ASTNode() = default;
    virtual std::string toString() const = 0;

    void setSourceLocation(const SourceLocation& start, const SourceLocation& end) {
        start_pos = start;
        end_pos = end;
    }

    SourceLocation getStartPosition() const { return start_pos; }
    SourceLocation getEndPosition() const { return end_pos; }
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

class Expression : public ASTNode {
public:
    Expression(const SourceLocation& start = SourceLocation(),
               const SourceLocation& end = SourceLocation())
        : ASTNode(start, end) {}
    virtual ~Expression() = default;
};

using ExpressionPtr = std::unique_ptr<Expression>;

class Statement : public ASTNode {
public:
    Statement(const SourceLocation& start = SourceLocation(),
              const SourceLocation& end = SourceLocation())
        : ASTNode(start, end) {}
    virtual ~Statement() = default;
};

using StatementPtr = std::unique_ptr<Statement>;

// Expression implementations
class Identifier : public Expression {
public:
    std::string name;

    explicit Identifier(const std::string& n,
                       const SourceLocation& start = SourceLocation(),
                       const SourceLocation& end = SourceLocation())
        : Expression(start, end), name(n) {}
    std::string toString() const override { return name; }
};

class Number : public Expression {
public:
    double value;

    explicit Number(double v,
                   const SourceLocation& start = SourceLocation(),
                   const SourceLocation& end = SourceLocation())
        : Expression(start, end), value(v) {}
    std::string toString() const override {
        // Format as integer if it's a whole number, otherwise as double
        if (value == static_cast<int>(value)) {
            return std::to_string(static_cast<int>(value));
        } else {
            return std::to_string(value);
        }
    }
};

class StringLiteral : public Expression {
public:
    std::string value;

    explicit StringLiteral(const std::string& v,
                          const SourceLocation& start = SourceLocation(),
                          const SourceLocation& end = SourceLocation())
        : Expression(start, end), value(v) {}
    std::string toString() const override {
        return "\"" + value + "\"";
    }
};

class ComplexNumber : public Expression {
public:
    double real;
    double imaginary;

    explicit ComplexNumber(double r, double i,
                          const SourceLocation& start = SourceLocation(),
                          const SourceLocation& end = SourceLocation())
        : Expression(start, end), real(r), imaginary(i) {}

    std::string toString() const override {
        if (real == 0 && imaginary == 0) {
            return "0";
        }

        std::string result;
        bool needsReal = (real != 0);
        bool needsImag = (imaginary != 0);
        double imag = imaginary; // Use local copy for manipulation

        if (needsReal) {
            if (real == static_cast<int>(real)) {
                result += std::to_string(static_cast<int>(real));
            } else {
                result += std::to_string(real);
            }
        }

        if (needsImag) {
            if (needsReal && imaginary > 0) {
                result += " + ";
            } else if (needsReal && imaginary < 0) {
                result += " - ";
                imag = -imaginary;
            }

            if (imag == 1 && !needsReal) {
                result += "i";
            } else if (imag == -1 && needsReal) {
                result += "i";
            } else if (imaginary == -1 && !needsReal) {
                result += "-i";
            } else if (imag == static_cast<int>(imag)) {
                result += std::to_string(static_cast<int>(imag)) + "i";
            } else {
                result += std::to_string(imag) + "i";
            }
        }

        if (!needsReal && !needsImag) {
            result = "0";
        }

        return result;
    }
};

class BinaryExpression : public Expression {
public:
    ExpressionPtr left;
    std::string operator_str;
    ExpressionPtr right;

    BinaryExpression(ExpressionPtr l, const std::string& op, ExpressionPtr r)
        : left(std::move(l)), operator_str(op), right(std::move(r)) {}

    std::string toString() const override {
        return "(" + left->toString() + " " + operator_str + " " + right->toString() + ")";
    }
};

class UnaryExpression : public Expression {
public:
    std::string operator_str;
    ExpressionPtr operand;

    UnaryExpression(const std::string& op, ExpressionPtr expr)
        : operator_str(op), operand(std::move(expr)) {}

    std::string toString() const override {
        return operator_str + operand->toString();
    }
};

class FunctionCall : public Expression {
public:
    std::string function_name;
    std::vector<ExpressionPtr> arguments;

    FunctionCall(const std::string& name, std::vector<ExpressionPtr> args)
        : function_name(name), arguments(std::move(args)) {}

    std::string toString() const override {
        std::string result = function_name + "(";
        for (size_t i = 0; i < arguments.size(); ++i) {
            if (i > 0) result += ", ";
            result += arguments[i]->toString();
        }
        result += ")";
        return result;
    }
};

class MethodCall : public Expression {
public:
    ExpressionPtr object;
    std::string method_name;
    std::vector<ExpressionPtr> arguments;

    MethodCall(ExpressionPtr obj, const std::string& name, std::vector<ExpressionPtr> args)
        : object(std::move(obj)), method_name(name), arguments(std::move(args)) {}

    std::string toString() const override {
        std::string result = object->toString() + "." + method_name + "(";
        for (size_t i = 0; i < arguments.size(); ++i) {
            if (i > 0) result += ", ";
            result += arguments[i]->toString();
        }
        result += ")";
        return result;
    }
};

class RangeExpression : public Expression {
public:
    ExpressionPtr start;
    ExpressionPtr end;

    RangeExpression(ExpressionPtr s, ExpressionPtr e)
        : start(std::move(s)), end(std::move(e)) {}

    std::string toString() const override {
        return start->toString() + "..." + end->toString();
    }
};

// Pipe substitution expression for variable substitution like "x^2 + y | x:2, y:3"
struct SubstitutionPair {
    std::string variable;
    ExpressionPtr value;

    SubstitutionPair(const std::string& var, ExpressionPtr val)
        : variable(var), value(std::move(val)) {}

    // Move constructor
    SubstitutionPair(SubstitutionPair&& other) noexcept
        : variable(std::move(other.variable)), value(std::move(other.value)) {}

    // Move assignment operator
    SubstitutionPair& operator=(SubstitutionPair&& other) noexcept {
        if (this != &other) {
            variable = std::move(other.variable);
            value = std::move(other.value);
        }
        return *this;
    }

    // Delete copy constructor and copy assignment
    SubstitutionPair(const SubstitutionPair&) = delete;
    SubstitutionPair& operator=(const SubstitutionPair&) = delete;
};

class PipeExpression : public Expression {
public:
    ExpressionPtr expression;
    std::vector<SubstitutionPair> substitutions;

    PipeExpression(ExpressionPtr expr, std::vector<SubstitutionPair> subs)
        : expression(std::move(expr)), substitutions(std::move(subs)) {}

    std::string toString() const override {
        std::string result = expression->toString() + " | ";
        for (size_t i = 0; i < substitutions.size(); ++i) {
            if (i > 0) result += ", ";
            result += substitutions[i].variable + ":" + substitutions[i].value->toString();
        }
        return result;
    }
};

// Summation expression for mathematical summations like sum(i^2, i, 1, n)
class SummationExpression : public Expression {
public:
    ExpressionPtr expression;   // The expression to sum (e.g., i^2)
    std::string variable;       // The summation variable (e.g., i)
    ExpressionPtr lowerBound;   // Lower bound (e.g., 1)
    ExpressionPtr upperBound;   // Upper bound (e.g., n)

    SummationExpression(ExpressionPtr expr, const std::string& var, 
                       ExpressionPtr lower, ExpressionPtr upper)
        : expression(std::move(expr)), variable(var), 
          lowerBound(std::move(lower)), upperBound(std::move(upper)) {}

    std::string toString() const override {
        return "summation(" + expression->toString() + ", " + variable + 
               ", " + lowerBound->toString() + ", " + upperBound->toString() + ")";
    }
};

// Unit expression for values with units like "5 mm" or "3.14 kg"
class UnitExpression : public Expression {
public:
    ExpressionPtr value;    // The numeric value
    std::string unit;       // The unit string (e.g., "mm", "kg", "m/s")

    UnitExpression(ExpressionPtr val, const std::string& unit_str)
        : value(std::move(val)), unit(unit_str) {}

    std::string toString() const override {
        return value->toString() + " " + unit;
    }
};

// Array expression for vectors like [1, 2, 3] (row) or [1; 2; 3] (column) or matrices
class ArrayExpression : public Expression {
public:
    std::vector<ExpressionPtr> elements;
    bool isColumnVector;  // true for [1;2;3], false for [1,2,3]
    bool isMatrix;        // true for multi-row matrices
    std::vector<std::vector<ExpressionPtr>> matrixRows; // For matrix storage

    ArrayExpression(std::vector<ExpressionPtr> elems, bool isColumn = false)
        : elements(std::move(elems)), isColumnVector(isColumn), isMatrix(false) {}

    ArrayExpression(std::vector<std::vector<ExpressionPtr>> rows)
        : isColumnVector(false), isMatrix(true), matrixRows(std::move(rows)) {}

    std::string toString() const override {
        if (isMatrix) {
            std::string result = "[";
            for (size_t i = 0; i < matrixRows.size(); ++i) {
                if (i > 0) result += "; ";
                for (size_t j = 0; j < matrixRows[i].size(); ++j) {
                    if (j > 0) result += ", ";
                    result += matrixRows[i][j]->toString();
                }
            }
            result += "]";
            return result;
        } else {
            std::string result = "[";
            const char* separator = isColumnVector ? "; " : ", ";
            for (size_t i = 0; i < elements.size(); ++i) {
                if (i > 0) result += separator;
                result += elements[i]->toString();
            }
            result += "]";
            return result;
        }
    }
};

// Array access expression like a[0]
class ArrayAccess : public Expression {
public:
    std::string arrayName;
    ExpressionPtr index;

    ArrayAccess(const std::string& name, ExpressionPtr idx)
        : arrayName(name), index(std::move(idx)) {}

    std::string toString() const override {
        return arrayName + "[" + index->toString() + "]";
    }
};

// Piecewise case: (value, condition)
class PiecewiseCase : public ASTNode {
public:
    ExpressionPtr value;      // The value to return if condition is true
    ExpressionPtr condition;  // The condition to check (nullptr for "otherwise")

    PiecewiseCase(ExpressionPtr val, ExpressionPtr cond)
        : value(std::move(val)), condition(std::move(cond)) {}

    std::string toString() const override {
        if (condition) {
            return "(" + value->toString() + ", " + condition->toString() + ")";
        } else {
            return "(" + value->toString() + ", otherwise)";
        }
    }
};

class PiecewiseExpression : public Expression {
public:
    std::vector<std::unique_ptr<PiecewiseCase>> cases;

    explicit PiecewiseExpression(std::vector<std::unique_ptr<PiecewiseCase>> case_list)
        : cases(std::move(case_list)) {}

    std::string toString() const override {
        std::string result = "piecewise {\n";
        for (const auto& case_ptr : cases) {
            result += "    " + case_ptr->toString() + ",\n";
        }
        result += "}";
        return result;
    }
};

// Statement implementations
class AssignmentStatement : public Statement {
public:
    std::string variable;
    ExpressionPtr expression;
    std::string inlineComment;
    bool commentBefore; // true for |-, false for -|

    AssignmentStatement(const std::string& var, ExpressionPtr expr, const std::string& comment = "", bool before = false)
        : variable(var), expression(std::move(expr)), inlineComment(comment), commentBefore(before) {}

    std::string toString() const override {
        std::string result = variable + " := ";
        if (!inlineComment.empty() && commentBefore) {
            result += "|-" + inlineComment + " ";
        }
        result += expression->toString();
        if (!inlineComment.empty() && !commentBefore) {
            result += " -|" + inlineComment;
        }
        result += ";";
        return result;
    }
};

class ArrayAssignmentStatement : public Statement {
public:
    std::string arrayName;
    ExpressionPtr index;
    ExpressionPtr expression;
    std::string inlineComment;
    bool isColumnVector;  // true for v[i;], false for v[i]

    ArrayAssignmentStatement(const std::string& name, ExpressionPtr idx, ExpressionPtr expr, const std::string& comment = "", bool isColumn = false)
        : arrayName(name), index(std::move(idx)), expression(std::move(expr)), inlineComment(comment), isColumnVector(isColumn) {}

    std::string toString() const override {
        std::string result = arrayName + "[" + index->toString() + "] := " + expression->toString();
        if (!inlineComment.empty()) {
            result += " --" + inlineComment;
        }
        result += ";";
        return result;
    }
};

class PrintStatement : public Statement {
public:
    ExpressionPtr expression;

    explicit PrintStatement(ExpressionPtr expr)
        : expression(std::move(expr)) {}

    std::string toString() const override {
        return "print(" + expression->toString() + ");";
    }
};

class ExpressionStatement : public Statement {
public:
    ExpressionPtr expression;

    explicit ExpressionStatement(ExpressionPtr expr)
        : expression(std::move(expr)) {}

    std::string toString() const override {
        return expression->toString() + ";";
    }
};

class CommentStatement : public Statement {
public:
    std::string content;

    explicit CommentStatement(const std::string& comment)
        : content(comment) {}

    std::string toString() const override {
        return "//" + content;
    }
};

class SkipStatement : public Statement {
public:
    bool shouldSkipNext; // False if there's a line break after @skip

    explicit SkipStatement(bool skipNext = true) : shouldSkipNext(skipNext) {}

    std::string toString() const override {
        return "@skip";
    }
};

class LayoutDirective : public Statement {
public:
    int rows;
    int cols;

    LayoutDirective(int r, int c) : rows(r), cols(c) {}

    std::string toString() const override {
        return "@layout" + std::to_string(rows) + "x" + std::to_string(cols);
    }
};

// Import item for individual functions with optional aliases
class ImportItem : public ASTNode {
public:
    std::string originalName;
    std::string aliasName;  // Empty if no alias

    ImportItem(const std::string& original, const std::string& alias = "")
        : originalName(original), aliasName(alias) {}

    std::string toString() const override {
        if (!aliasName.empty()) {
            return originalName + " as " + aliasName;
        }
        return originalName;
    }

    std::string getEffectiveName() const {
        return aliasName.empty() ? originalName : aliasName;
    }
};

class ImportStatement : public Statement {
public:
    std::string moduleName;
    std::vector<std::unique_ptr<ImportItem>> importItems;

    ImportStatement(const std::string& module, std::vector<std::unique_ptr<ImportItem>> items)
        : moduleName(module), importItems(std::move(items)) {}

    std::string toString() const override {
        std::string result = "from " + moduleName + " import ";
        for (size_t i = 0; i < importItems.size(); ++i) {
            if (i > 0) result += ", ";
            result += importItems[i]->toString();
        }
        return result;
    }
};

class ReturnStatement : public Statement {
public:
    ExpressionPtr expression;

    explicit ReturnStatement(ExpressionPtr expr)
        : expression(std::move(expr)) {}

    std::string toString() const override {
        return "return " + expression->toString() + ";";
    }
};

class BreakStatement : public Statement {
public:
    BreakStatement() = default;

    std::string toString() const override {
        return "break;";
    }
};

class FunctionDeclaration : public Statement {
public:
    std::string name;
    std::vector<std::string> parameters;
    std::vector<StatementPtr> body;
    std::vector<std::string> decorators;  // Added support for decorators

    FunctionDeclaration(const std::string& func_name,
                       std::vector<std::string> params,
                       std::vector<StatementPtr> statements,
                       std::vector<std::string> decors = {})
        : name(func_name), parameters(std::move(params)), body(std::move(statements)), decorators(std::move(decors)) {}

    bool hasDecorator(const std::string& decorator) const {
        for (const auto& dec : decorators) {
            if (dec == decorator) return true;
        }
        return false;
    }

    std::string toString() const override {
        std::string result;
        for (const auto& decorator : decorators) {
            result += "@" + decorator + "\n";
        }
        result += "fn " + name + "(";
        for (size_t i = 0; i < parameters.size(); ++i) {
            if (i > 0) result += ", ";
            result += parameters[i];
        }
        result += ") {\n";
        for (const auto& stmt : body) {
            result += "    " + stmt->toString() + "\n";
        }
        result += "}";
        return result;
    }
};

class PiecewiseFunctionDeclaration : public Statement {
public:
    std::string name;
    std::vector<std::string> parameters;
    std::unique_ptr<PiecewiseExpression> piecewise;

    PiecewiseFunctionDeclaration(const std::string& func_name,
                                std::vector<std::string> params,
                                std::unique_ptr<PiecewiseExpression> pw_expr)
        : name(func_name), parameters(std::move(params)), piecewise(std::move(pw_expr)) {}

    std::string toString() const override {
        std::string result = name + "(";
        for (size_t i = 0; i < parameters.size(); ++i) {
            if (i > 0) result += ", ";
            result += parameters[i];
        }
        result += ") := " + piecewise->toString();
        return result;
    }
};

class ForStatement : public Statement {
public:
    std::string variable;
    ExpressionPtr range;
    std::vector<StatementPtr> body;

    ForStatement(const std::string& var, ExpressionPtr range_expr, std::vector<StatementPtr> statements)
        : variable(var), range(std::move(range_expr)), body(std::move(statements)) {}

    std::string toString() const override {
        std::string result = "for " + variable + " in " + range->toString() + " {\n";
        for (const auto& stmt : body) {
            result += "    " + stmt->toString() + "\n";
        }
        result += "}";
        return result;
    }
};

class WhileStatement : public Statement {
public:
    ExpressionPtr condition;
    std::vector<StatementPtr> body;

    WhileStatement(ExpressionPtr cond, std::vector<StatementPtr> statements)
        : condition(std::move(cond)), body(std::move(statements)) {}

    std::string toString() const override {
        std::string result = "while (" + condition->toString() + ") {\n";
        for (const auto& stmt : body) {
            result += "    " + stmt->toString() + "\n";
        }
        result += "}";
        return result;
    }
};

class IfStatement : public Statement {
public:
    ExpressionPtr condition;
    std::vector<StatementPtr> then_body;
    std::vector<StatementPtr> else_body;

    IfStatement(ExpressionPtr cond, std::vector<StatementPtr> then_stmts, std::vector<StatementPtr> else_stmts = {})
        : condition(std::move(cond)), then_body(std::move(then_stmts)), else_body(std::move(else_stmts)) {}

    std::string toString() const override {
        std::string result = "if (" + condition->toString() + ") {\n";
        for (const auto& stmt : then_body) {
            result += "    " + stmt->toString() + "\n";
        }
        result += "}";
        if (!else_body.empty()) {
            result += " else {\n";
            for (const auto& stmt : else_body) {
                result += "    " + stmt->toString() + "\n";
            }
            result += "}";
        }
        return result;
    }
};

// Represents a decorator that can be simple (@eval) or layout (@layout2x3)
class Decorator {
public:
    enum Type { Simple, Layout, Parameterized };
    Type type;
    std::string name;  // For simple and parameterized decorators
    int rows, cols;    // For layout decorators
    int parameter;     // For parameterized decorators like @merge2
    std::string style; // For decorators with style like @merge2[center]

    Decorator(const std::string& decoratorName)
        : type(Simple), name(decoratorName), rows(0), cols(0), parameter(0), style("") {}

    Decorator(int r, int c)
        : type(Layout), name("layout"), rows(r), cols(c), parameter(0), style("") {}

    Decorator(const std::string& decoratorName, int param)
        : type(Parameterized), name(decoratorName), rows(0), cols(0), parameter(param), style("") {}

    Decorator(const std::string& decoratorName, int param, const std::string& decoratorStyle)
        : type(Parameterized), name(decoratorName), rows(0), cols(0), parameter(param), style(decoratorStyle) {}

    bool isLayout() const { return type == Layout; }
    bool isSimple() const { return type == Simple; }
    bool isParameterized() const { return type == Parameterized; }

    std::string toString() const {
        if (type == Layout) {
            return "@layout" + std::to_string(rows) + "x" + std::to_string(cols);
        }
        if (type == Parameterized) {
            std::string result = "@" + name + std::to_string(parameter);
            if (!style.empty()) {
                result += "[" + style + "]";
            }
            return result;
        }
        return "@" + name;
    }
};

class DecoratedStatement : public Statement {
public:
    std::vector<Decorator> decorators;
    StatementPtr statement;

    DecoratedStatement(std::vector<Decorator> decors, StatementPtr stmt)
        : decorators(std::move(decors)), statement(std::move(stmt)) {}

    bool hasDecorator(const std::string& decorator) const {
        for (const auto& dec : decorators) {
            if (dec.isSimple() && dec.name == decorator) return true;
        }
        return false;
    }

    bool hasLayoutDecorator() const {
        for (const auto& dec : decorators) {
            if (dec.isLayout()) return true;
        }
        return false;
    }

    const Decorator* getLayoutDecorator() const {
        for (const auto& dec : decorators) {
            if (dec.isLayout()) return &dec;
        }
        return nullptr;
    }

    std::string toString() const override {
        std::string result;
        for (const auto& decorator : decorators) {
            result += decorator.toString() + "\n";
        }
        result += statement->toString();
        return result;
    }
};

class HeadingStatement : public Statement {
public:
    int level;  // 1-4 for h1-h4
    std::string style;  // Optional style attribute (e.g., "center")
    std::vector<std::string> textParagraphs;

    HeadingStatement(int lvl, const std::string& styleAttr, std::vector<std::string> paragraphs)
        : level(lvl), style(styleAttr), textParagraphs(std::move(paragraphs)) {}

    std::string toString() const override {
        std::string result = "@h" + std::to_string(level);
        if (!style.empty()) {
            result += "[" + style + "]";
        }
        result += "\n";
        for (const auto& para : textParagraphs) {
            result += para + "\n";
        }
        return result;
    }
};

class VersionStatement : public Statement {
public:
    std::string version;

    explicit VersionStatement(const std::string& ver)
        : version(ver) {}

    std::string toString() const override {
        return "@version " + version;
    }
};

class ParagraphStatement : public Statement {
public:
    std::string style;
    std::vector<std::string> textLines;

    explicit ParagraphStatement(std::vector<std::string> lines, std::string paragraphStyle = "")
        : style(std::move(paragraphStyle)), textLines(std::move(lines)) {}

    std::string toString() const override {
        std::string result = "@p";
        if (!style.empty()) {
            result += "[" + style + "]";
        }
        result += "\n";
        for (const auto& line : textLines) {
            result += line + "\n";
        }
        return result;
    }
};

class Program : public ASTNode {
public:
    std::vector<StatementPtr> statements;

    void addStatement(StatementPtr stmt) {
        statements.push_back(std::move(stmt));
    }

    std::string toString() const override {
        std::string result;
        for (const auto& stmt : statements) {
            result += stmt->toString() + "\n";
        }
        return result;
    }
};

using ProgramPtr = std::unique_ptr<Program>;

} // namespace madola