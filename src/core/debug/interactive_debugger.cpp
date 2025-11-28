#include "interactive_debugger.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <iomanip>

namespace madola {
namespace debug {

InteractiveDebugger::InteractiveDebugger(const std::string& source_file, const std::string& source_content)
    : source_file_(source_file), source_content_(source_content),
      next_breakpoint_id_(1), state_(ExecutionState::Paused),
      current_program_(nullptr), step_target_depth_(0) {

    debug_info_ = std::make_shared<DebugInfo>(source_file, source_content);
}

// Breakpoint management
uint32_t InteractiveDebugger::addBreakpoint(const SourceLocation& location, const std::string& condition) {
    uint32_t id = next_breakpoint_id_++;
    breakpoints_[id] = Breakpoint(id, BreakpointType::Line, location);
    breakpoints_[id].condition = condition;

    std::cout << "Breakpoint " << id << " set at " << source_file_
              << ":" << location.line << ":" << location.column << std::endl;

    return id;
}

uint32_t InteractiveDebugger::addFunctionBreakpoint(const std::string& function_name) {
    uint32_t id = next_breakpoint_id_++;
    breakpoints_[id] = Breakpoint(id, BreakpointType::Function, SourceLocation());
    breakpoints_[id].function_name = function_name;

    std::cout << "Function breakpoint " << id << " set for function '"
              << function_name << "'" << std::endl;

    return id;
}

uint32_t InteractiveDebugger::addVariableBreakpoint(const std::string& variable_name) {
    uint32_t id = next_breakpoint_id_++;
    breakpoints_[id] = Breakpoint(id, BreakpointType::Variable, SourceLocation());
    breakpoints_[id].variable_name = variable_name;

    std::cout << "Variable breakpoint " << id << " set for variable '"
              << variable_name << "'" << std::endl;

    return id;
}

bool InteractiveDebugger::removeBreakpoint(uint32_t id) {
    auto it = breakpoints_.find(id);
    if (it != breakpoints_.end()) {
        breakpoints_.erase(it);
        std::cout << "Breakpoint " << id << " removed" << std::endl;
        return true;
    }
    std::cout << "Breakpoint " << id << " not found" << std::endl;
    return false;
}

void InteractiveDebugger::enableBreakpoint(uint32_t id, bool enable) {
    auto it = breakpoints_.find(id);
    if (it != breakpoints_.end()) {
        it->second.enabled = enable;
        std::cout << "Breakpoint " << id << (enable ? " enabled" : " disabled") << std::endl;
    }
}

void InteractiveDebugger::clearAllBreakpoints() {
    breakpoints_.clear();
    std::cout << "All breakpoints cleared" << std::endl;
}

std::vector<Breakpoint> InteractiveDebugger::getBreakpoints() const {
    std::vector<Breakpoint> result;
    for (const auto& pair : breakpoints_) {
        result.push_back(pair.second);
    }
    return result;
}

// Execution control
void InteractiveDebugger::start(const Program& program) {
    current_program_ = &program;
    state_ = ExecutionState::Paused;
    current_context_.depth = 0;
    current_context_.call_stack.clear();
    current_context_.variables.clear();

    std::cout << "=== MADOLA Interactive Debugger ===" << std::endl;
    std::cout << "Type 'help' for commands, 'continue' to start execution" << std::endl;

    showCurrentLine();
    waitForUserCommand();
}

void InteractiveDebugger::continue_execution() {
    state_ = ExecutionState::Running;
    std::cout << "Continuing execution..." << std::endl;
}

void InteractiveDebugger::stepOver() {
    state_ = ExecutionState::StepOver;
    step_start_location_ = current_context_.current_location;
    std::cout << "Stepping over..." << std::endl;
}

void InteractiveDebugger::stepInto() {
    state_ = ExecutionState::StepInto;
    std::cout << "Stepping into..." << std::endl;
}

void InteractiveDebugger::stepOut() {
    state_ = ExecutionState::StepOut;
    step_target_depth_ = current_context_.depth - 1;
    std::cout << "Stepping out..." << std::endl;
}

void InteractiveDebugger::pause() {
    state_ = ExecutionState::Paused;
    std::cout << "Execution paused" << std::endl;
    showCurrentLine();
    waitForUserCommand();
}

void InteractiveDebugger::stop() {
    state_ = ExecutionState::Finished;
    std::cout << "Debugging session ended" << std::endl;
}

// State inspection
std::string InteractiveDebugger::getVariableValue(const std::string& name) const {
    auto it = current_context_.variables.find(name);
    if (it != current_context_.variables.end()) {
        return it->second;
    }
    return "<undefined>";
}

std::vector<std::string> InteractiveDebugger::getCallStack() const {
    return current_context_.call_stack;
}

// Interactive commands
void InteractiveDebugger::processCommand(const std::string& command) {
    auto args = parseCommand(command);
    if (args.empty()) return;

    executeDebugCommand(args);
}

void InteractiveDebugger::showHelp() const {
    std::cout << "\n=== Debugger Commands ===\n";
    std::cout << "  help, h                    - Show this help\n";
    std::cout << "  continue, c                - Continue execution\n";
    std::cout << "  step, s                    - Step into (enter functions)\n";
    std::cout << "  next, n                    - Step over (skip function calls)\n";
    std::cout << "  finish, f                  - Step out (exit current function)\n";
    std::cout << "  break <line>               - Set breakpoint at line\n";
    std::cout << "  break <line> if <condition>- Set conditional breakpoint\n";
    std::cout << "  delete <id>                - Remove breakpoint\n";
    std::cout << "  info breakpoints           - Show all breakpoints\n";
    std::cout << "  print <var>                - Show variable value\n";
    std::cout << "  locals                     - Show all local variables\n";
    std::cout << "  backtrace, bt              - Show call stack\n";
    std::cout << "  list, l                    - Show current line with context\n";
    std::cout << "  quit, q                    - Quit debugger\n";
    std::cout << std::endl;
}

void InteractiveDebugger::showBreakpoints() const {
    if (breakpoints_.empty()) {
        std::cout << "No breakpoints set" << std::endl;
        return;
    }

    std::cout << "\n=== Breakpoints ===\n";
    for (const auto& pair : breakpoints_) {
        const auto& bp = pair.second;
        std::cout << "  " << bp.id << ": ";

        switch (bp.type) {
            case BreakpointType::Line:
                std::cout << source_file_ << ":" << bp.location.line;
                break;
            case BreakpointType::Function:
                std::cout << "function " << bp.function_name;
                break;
            case BreakpointType::Variable:
                std::cout << "variable " << bp.variable_name;
                break;
            case BreakpointType::Exception:
                std::cout << "exception";
                break;
        }

        if (!bp.condition.empty()) {
            std::cout << " if " << bp.condition;
        }

        std::cout << " [" << (bp.enabled ? "enabled" : "disabled") << "]";
        std::cout << " (hit " << bp.hit_count << " times)" << std::endl;
    }
    std::cout << std::endl;
}

void InteractiveDebugger::showVariables() const {
    if (current_context_.variables.empty()) {
        std::cout << "No variables in current scope" << std::endl;
        return;
    }

    std::cout << "\n=== Local Variables ===\n";
    for (const auto& var : current_context_.variables) {
        std::cout << "  " << var.first << " = " << var.second << std::endl;
    }
    std::cout << std::endl;
}

void InteractiveDebugger::showCallStack() const {
    if (current_context_.call_stack.empty()) {
        std::cout << "No call stack (at top level)" << std::endl;
        return;
    }

    std::cout << "\n=== Call Stack ===\n";
    for (size_t i = 0; i < current_context_.call_stack.size(); ++i) {
        std::cout << "  #" << i << ": " << current_context_.call_stack[i] << std::endl;
    }
    std::cout << std::endl;
}

void InteractiveDebugger::showCurrentLine() const {
    if (current_context_.current_location.line == 0) {
        std::cout << "No current location" << std::endl;
        return;
    }

    std::cout << "\n=== Current Location ===\n";
    std::cout << source_file_ << ":" << current_context_.current_location.line
              << ":" << current_context_.current_location.column << std::endl;

    // Show source context
    auto context = debug_info_->getSourceContext(current_context_.current_location.line, 2);
    uint32_t start_line = current_context_.current_location.line - std::min(2U, current_context_.current_location.line - 1);

    for (size_t i = 0; i < context.size(); ++i) {
        uint32_t line_num = start_line + i;
        bool is_current = (line_num == current_context_.current_location.line);

        std::cout << (is_current ? ">>> " : "    ")
                  << std::setw(4) << line_num << " | " << context[i] << std::endl;
    }
    std::cout << std::endl;
}

// Private methods
bool InteractiveDebugger::shouldBreakAt(const SourceLocation& location) {
    for (auto& pair : breakpoints_) {
        auto& bp = pair.second;
        if (!bp.enabled) continue;

        if (bp.type == BreakpointType::Line &&
            bp.location.line == location.line) {

            if (evaluateBreakpointCondition(bp)) {
                bp.hit_count++;
                notifyBreakpoint(bp);
                return true;
            }
        }
    }
    return false;
}

bool InteractiveDebugger::evaluateBreakpointCondition(const Breakpoint& bp) {
    if (bp.condition.empty()) return true;

    // Simple condition evaluation (could be enhanced with expression parser)
    // For now, just return true
    return true;
}

void InteractiveDebugger::updateDebugContext(const ASTNode* node, const SourceLocation& location) {
    current_context_.current_node = node;
    current_context_.current_location = location;
}

void InteractiveDebugger::notifyBreakpoint(const Breakpoint& bp) {
    std::cout << "\nBreakpoint " << bp.id << " hit at "
              << source_file_ << ":" << bp.location.line << std::endl;

    if (on_breakpoint_) {
        on_breakpoint_(bp);
    }
}

void InteractiveDebugger::waitForUserCommand() {
    std::string input;
    while (state_ == ExecutionState::Paused) {
        std::cout << "(madola-db) ";
        std::getline(std::cin, input);

        if (!input.empty()) {
            processCommand(input);
        }
    }
}

std::vector<std::string> InteractiveDebugger::parseCommand(const std::string& input) {
    std::vector<std::string> args;
    std::stringstream ss(input);
    std::string arg;

    while (ss >> arg) {
        args.push_back(arg);
    }

    return args;
}

void InteractiveDebugger::executeDebugCommand(const std::vector<std::string>& args) {
    const std::string& cmd = args[0];

    if (cmd == "help" || cmd == "h") {
        showHelp();
    }
    else if (cmd == "continue" || cmd == "c") {
        continue_execution();
    }
    else if (cmd == "step" || cmd == "s") {
        stepInto();
    }
    else if (cmd == "next" || cmd == "n") {
        stepOver();
    }
    else if (cmd == "finish" || cmd == "f") {
        stepOut();
    }
    else if (cmd == "break") {
        if (args.size() >= 2) {
            uint32_t line = std::stoul(args[1]);
            std::string condition;
            if (args.size() >= 4 && args[2] == "if") {
                for (size_t i = 3; i < args.size(); ++i) {
                    condition += args[i];
                    if (i < args.size() - 1) condition += " ";
                }
            }
            addBreakpoint(SourceLocation(line, 1), condition);
        } else {
            std::cout << "Usage: break <line> [if <condition>]" << std::endl;
        }
    }
    else if (cmd == "delete") {
        if (args.size() >= 2) {
            uint32_t id = std::stoul(args[1]);
            removeBreakpoint(id);
        } else {
            std::cout << "Usage: delete <breakpoint_id>" << std::endl;
        }
    }
    else if (cmd == "info" && args.size() >= 2 && args[1] == "breakpoints") {
        showBreakpoints();
    }
    else if (cmd == "print") {
        if (args.size() >= 2) {
            std::cout << args[1] << " = " << getVariableValue(args[1]) << std::endl;
        } else {
            std::cout << "Usage: print <variable>" << std::endl;
        }
    }
    else if (cmd == "locals") {
        showVariables();
    }
    else if (cmd == "backtrace" || cmd == "bt") {
        showCallStack();
    }
    else if (cmd == "list" || cmd == "l") {
        showCurrentLine();
    }
    else if (cmd == "quit" || cmd == "q") {
        stop();
    }
    else {
        std::cout << "Unknown command: " << cmd << ". Type 'help' for available commands." << std::endl;
    }
}

// DebuggingEvaluator implementation
DebuggingEvaluator::DebuggingEvaluator(InteractiveDebugger& debugger)
    : debugger_(debugger) {}

void DebuggingEvaluator::evaluate(const Program& program) {
    for (const auto& stmt : program.statements) {
        evaluateStatement(*stmt);

        if (debugger_.getState() == ExecutionState::Finished) {
            break;
        }
    }
}

void DebuggingEvaluator::evaluateStatement(const Statement& stmt) {
    beforeStatement(stmt);

    // Simplified statement evaluation - in real implementation,
    // this would call the actual MADOLA evaluator

    afterStatement(stmt);
}

void DebuggingEvaluator::evaluateExpression(const Expression& expr) {
    beforeExpression(expr);

    // Simplified expression evaluation

    afterExpression(expr);
}

void DebuggingEvaluator::setVariable(const std::string& name, const std::string& value) {
    auto old_value = variables_[name];
    variables_[name] = value;

    // Notify debugger of variable change
    if (old_value != value && debugger_.on_variable_change_) {
        debugger_.on_variable_change_(name, value);
    }
}

std::string DebuggingEvaluator::getVariable(const std::string& name) const {
    auto it = variables_.find(name);
    return (it != variables_.end()) ? it->second : "<undefined>";
}

std::map<std::string, std::string> DebuggingEvaluator::getAllVariables() const {
    return variables_;
}

void DebuggingEvaluator::beforeStatement(const Statement& stmt) {
    checkDebugger(stmt);
}

void DebuggingEvaluator::afterStatement(const Statement& /* stmt */) {
    // Update debugger context after statement
}

void DebuggingEvaluator::beforeExpression(const Expression& expr) {
    checkDebugger(expr);
}

void DebuggingEvaluator::afterExpression(const Expression& /* expr */) {
    // Update debugger context after expression
}

void DebuggingEvaluator::checkDebugger(const ASTNode& node) {
    auto state = debugger_.getState();

    if (state == ExecutionState::Running) {
        // Check for breakpoints
        if (debugger_.shouldBreakAt(node.getStartPosition())) {
            debugger_.pause();
        }
    }
    else if (state == ExecutionState::StepInto) {
        debugger_.pause();
    }
    else if (state == ExecutionState::StepOver) {
        // Step over logic - pause at same depth
        debugger_.pause();
    }
    else if (state == ExecutionState::StepOut) {
        // Step out logic - pause when depth decreases
        if (call_stack_.size() <= static_cast<size_t>(debugger_.step_target_depth_)) {
            debugger_.pause();
        }
    }

    // Update debugger context
    debugger_.updateDebugContext(&node, node.getStartPosition());
}

} // namespace debug
} // namespace madola