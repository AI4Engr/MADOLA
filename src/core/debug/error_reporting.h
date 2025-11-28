#pragma once
#include "../ast/ast.h"
#include "source_map.h"
#include <memory>
#include <string>
#include <exception>
#include <vector>
#include <sstream>

namespace madola {
namespace debug {

// Enhanced exception class with source location information
class MadolaException : public std::exception {
public:
    MadolaException(const std::string& message,
                   const SourceLocation& location = SourceLocation(),
                   const std::string& source_file = "")
        : message_(message), location_(location), source_file_(source_file) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

    const SourceLocation& getLocation() const { return location_; }
    const std::string& getSourceFile() const { return source_file_; }

    // Format error with source context using DebugInfo
    std::string formatError(const DebugInfo& debug_info, int context_lines = 2) const {
        return debug_info.formatError(message_, location_, context_lines);
    }

private:
    std::string message_;
    SourceLocation location_;
    std::string source_file_;
};

// Runtime error with source location
class MadolaRuntimeError : public MadolaException {
public:
    MadolaRuntimeError(const std::string& message,
                      const SourceLocation& location = SourceLocation(),
                      const std::string& source_file = "")
        : MadolaException("Runtime Error: " + message, location, source_file) {}
};

// Parse error with source location
class MadolaParseError : public MadolaException {
public:
    MadolaParseError(const std::string& message,
                    const SourceLocation& location = SourceLocation(),
                    const std::string& source_file = "")
        : MadolaException("Parse Error: " + message, location, source_file) {}
};

// Type error with source location
class MadolaTypeError : public MadolaException {
public:
    MadolaTypeError(const std::string& message,
                   const SourceLocation& location = SourceLocation(),
                   const std::string& source_file = "")
        : MadolaException("Type Error: " + message, location, source_file) {}
};

// Evaluation error with source location
class MadolaEvaluationError : public MadolaException {
public:
    MadolaEvaluationError(const std::string& message,
                         const SourceLocation& location = SourceLocation(),
                         const std::string& source_file = "")
        : MadolaException("Evaluation Error: " + message, location, source_file) {}
};

// Error context for stack traces
struct ErrorContext {
    SourceLocation location;
    std::string function_name;
    std::string description;

    ErrorContext(const SourceLocation& loc, const std::string& func = "", const std::string& desc = "")
        : location(loc), function_name(func), description(desc) {}
};

// Stack trace for debugging
class StackTrace {
public:
    void pushContext(const ErrorContext& context) {
        contexts_.push_back(context);
    }

    void popContext() {
        if (!contexts_.empty()) {
            contexts_.pop_back();
        }
    }

    const std::vector<ErrorContext>& getContexts() const { return contexts_; }

    // Format stack trace
    std::string format(const DebugInfo& debug_info) const {
        std::stringstream result;
        result << "Stack trace:\n";

        for (int i = static_cast<int>(contexts_.size()) - 1; i >= 0; --i) {
            const auto& context = contexts_[i];
            result << "  at ";

            if (!context.function_name.empty()) {
                result << context.function_name << "() ";
            }

            result << "(" << debug_info.getSourceFile() << ":"
                   << context.location.line << ":" << context.location.column << ")";

            if (!context.description.empty()) {
                result << " - " << context.description;
            }

            result << "\n";
        }

        return result.str();
    }

    void clear() { contexts_.clear(); }

private:
    std::vector<ErrorContext> contexts_;
};

// RAII helper for automatic stack trace management
class StackTraceGuard {
public:
    StackTraceGuard(StackTrace& stack, const ErrorContext& context)
        : stack_(stack) {
        stack_.pushContext(context);
    }

    ~StackTraceGuard() {
        stack_.popContext();
    }

private:
    StackTrace& stack_;
};

// Macro for easy stack trace context
#define MADOLA_STACK_TRACE(stack, location, func_name, desc) \
    madola::debug::StackTraceGuard _guard(stack, madola::debug::ErrorContext(location, func_name, desc))

} // namespace debug
} // namespace madola