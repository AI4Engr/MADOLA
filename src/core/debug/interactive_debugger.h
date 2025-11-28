#pragma once
#include "../ast/ast.h"
#include "../debug/source_map.h"
#include "../debug/error_reporting.h"
#include <memory>
#include <set>
#include <map>
#include <functional>
#include <vector>
#include <string>

namespace madola {
namespace debug {

// Breakpoint types
enum class BreakpointType {
    Line,           // Break at specific line
    Function,       // Break when entering function
    Variable,       // Break when variable changes
    Exception       // Break on exceptions
};

// Breakpoint definition
struct Breakpoint {
    uint32_t id;
    BreakpointType type;
    SourceLocation location;
    std::string condition;  // Optional condition (e.g., "x > 5")
    bool enabled;
    int hit_count;
    std::string function_name;  // For function breakpoints
    std::string variable_name;  // For variable breakpoints

    // Default constructor
    Breakpoint() : id(0), type(BreakpointType::Line), enabled(true), hit_count(0) {}

    Breakpoint(uint32_t bp_id, BreakpointType bp_type, const SourceLocation& loc)
        : id(bp_id), type(bp_type), location(loc), enabled(true), hit_count(0) {}
};

// Execution state for step debugging
enum class ExecutionState {
    Running,        // Normal execution
    Paused,         // Paused at breakpoint or step
    StepOver,       // Step to next line (same level)
    StepInto,       // Step into function calls
    StepOut,        // Step out of current function
    Finished,       // Execution completed
    Error          // Execution stopped due to error
};

// Debug session context
struct DebugContext {
    const ASTNode* current_node;
    SourceLocation current_location;
    std::map<std::string, std::string> variables;  // Variable name -> value
    std::vector<std::string> call_stack;
    int depth;  // Call stack depth

    DebugContext() : current_node(nullptr), depth(0) {}
};

// Interactive debugger for MADOLA
class InteractiveDebugger {
    friend class DebuggingEvaluator;  // Allow access to private members

public:
    InteractiveDebugger(const std::string& source_file, const std::string& source_content);

    // Breakpoint management
    uint32_t addBreakpoint(const SourceLocation& location, const std::string& condition = "");
    uint32_t addFunctionBreakpoint(const std::string& function_name);
    uint32_t addVariableBreakpoint(const std::string& variable_name);
    bool removeBreakpoint(uint32_t id);
    void enableBreakpoint(uint32_t id, bool enable = true);
    void clearAllBreakpoints();
    std::vector<Breakpoint> getBreakpoints() const;

    // Execution control
    void start(const Program& program);
    void continue_execution();
    void stepOver();
    void stepInto();
    void stepOut();
    void pause();
    void stop();

    // State inspection
    ExecutionState getState() const { return state_; }
    DebugContext getCurrentContext() const { return current_context_; }
    std::string getVariableValue(const std::string& name) const;
    std::vector<std::string> getCallStack() const;

    // Interactive commands
    void processCommand(const std::string& command);
    void showHelp() const;
    void showBreakpoints() const;
    void showVariables() const;
    void showCallStack() const;
    void showCurrentLine() const;

    // Callbacks for debugger events
    void setOnBreakpoint(std::function<void(const Breakpoint&)> callback) { on_breakpoint_ = callback; }
    void setOnStep(std::function<void(const DebugContext&)> callback) { on_step_ = callback; }
    void setOnVariableChange(std::function<void(const std::string&, const std::string&)> callback) { on_variable_change_ = callback; }

private:
    std::string source_file_;
    std::string source_content_;
    std::shared_ptr<DebugInfo> debug_info_;

    // Breakpoint management
    std::map<uint32_t, Breakpoint> breakpoints_;
    uint32_t next_breakpoint_id_;

    // Execution state
    ExecutionState state_;
    DebugContext current_context_;
    const Program* current_program_;

    // Step control
    int step_target_depth_;  // For step out
    SourceLocation step_start_location_;  // For step over

    // Event callbacks
    std::function<void(const Breakpoint&)> on_breakpoint_;
    std::function<void(const DebugContext&)> on_step_;
    std::function<void(const std::string&, const std::string&)> on_variable_change_;

    // Internal execution methods
    bool shouldBreakAt(const SourceLocation& location);
    bool evaluateBreakpointCondition(const Breakpoint& bp);
    void updateDebugContext(const ASTNode* node, const SourceLocation& location);
    void notifyBreakpoint(const Breakpoint& bp);
    void waitForUserCommand();

    // Command parsing
    std::vector<std::string> parseCommand(const std::string& input);
    void executeDebugCommand(const std::vector<std::string>& args);
};

// Debug-aware evaluator that integrates with the debugger
class DebuggingEvaluator {
public:
    DebuggingEvaluator(InteractiveDebugger& debugger);

    // Execute with debugging support
    void evaluate(const Program& program);
    void evaluateStatement(const Statement& stmt);
    void evaluateExpression(const Expression& expr);

    // Variable management
    void setVariable(const std::string& name, const std::string& value);
    std::string getVariable(const std::string& name) const;
    std::map<std::string, std::string> getAllVariables() const;

private:
    InteractiveDebugger& debugger_;
    std::map<std::string, std::string> variables_;
    std::vector<std::string> call_stack_;

    // Debug hooks
    void beforeStatement(const Statement& stmt);
    void afterStatement(const Statement& stmt);
    void beforeExpression(const Expression& expr);
    void afterExpression(const Expression& expr);

    // Check if debugger should pause
    void checkDebugger(const ASTNode& node);
};

} // namespace debug
} // namespace madola