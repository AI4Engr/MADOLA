#include "core/debug/interactive_debugger.h"
#include "core/ast/ast_builder.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: madola-debug <file.mda> [options]" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --break <line>    Set initial breakpoint at line" << std::endl;
        return 1;
    }

    std::string filename = argv[1];

    // Read source file
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source_content = buffer.str();
    file.close();

    try {
        // Create debugger
        madola::debug::InteractiveDebugger debugger(filename, source_content);

        // Process command line arguments
        for (int i = 2; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--break" && i + 1 < argc) {
                uint32_t line = std::stoul(argv[++i]);
                debugger.addBreakpoint(madola::SourceLocation(line, 1));
            }
        }

        // Build AST
        madola::ASTBuilder builder;
        auto program = builder.buildProgram(source_content);

        // Start debugging session
        debugger.start(*program);

        // Create debug-aware evaluator
        madola::debug::DebuggingEvaluator evaluator(debugger);
        evaluator.evaluate(*program);

        std::cout << "Program execution completed." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}