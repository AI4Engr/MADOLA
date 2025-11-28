#include "debug_cli.h"
#include "../ast/ast_builder.h"
#include "../evaluator/evaluator.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace madola {
namespace debug {

DebugCLI::DebugCLI(const std::string& source_file, const std::string& source_content)
    : source_file_(source_file), source_content_(source_content),
      debug_enabled_(false), source_map_enabled_(false) {

    debug_info_ = std::make_shared<DebugInfo>(source_file, source_content);
    source_map_generator_ = std::make_shared<SourceMapGenerator>(source_file);
    debug_info_->setSourceMap(source_map_generator_);
}

int DebugCLI::run(const std::vector<std::string>& args) {
    try {
        // Parse command line arguments
        for (size_t i = 0; i < args.size(); ++i) {
            const std::string& arg = args[i];

            if (arg == "--debug") {
                enableDebug(true);
            } else if (arg == "--source-map") {
                enableSourceMap(true);
            } else if (arg == "--source-map-output" && i + 1 < args.size()) {
                setSourceMapOutput(args[++i]);
            } else if (arg == "--help" || arg == "-h") {
                showHelp();
                return 0;
            } else if (arg == "--version" || arg == "-v") {
                showVersion();
                return 0;
            }
        }

        // Build AST with position tracking
        ASTBuilder builder;
        auto program = builder.buildProgram(source_content_);

        if (debug_enabled_) {
            std::cout << "=== DEBUG: AST Structure ===" << std::endl;
            dumpAST(*program);
            std::cout << std::endl;
        }

        // Evaluate with debug context
        Evaluator evaluator;

        // Add source map mappings during evaluation would happen here
        // This is a simplified version - full implementation would track
        // generated output positions and map them back to source

        if (debug_enabled_) {
            std::cout << "=== DEBUG: Evaluation Started ===" << std::endl;
        }

        // Execute the program (simplified)
        // In a full implementation, this would integrate with the actual evaluator
        // and track execution context for stack traces

        if (source_map_enabled_ && !source_map_output_.empty()) {
            dumpSourceMap();
        }

        if (debug_enabled_) {
            std::cout << "=== DEBUG: Evaluation Completed ===" << std::endl;
        }

        return 0;

    } catch (const MadolaException& e) {
        handleError(e);
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
}

void DebugCLI::showHelp() const {
    std::cout << "MADOLA Debug CLI - Enhanced debugging for MADOLA language\n\n";
    std::cout << "Usage: madola [options] <input.mda>\n\n";
    std::cout << "Debug Options:\n";
    std::cout << "  --debug                    Enable debug output\n";
    std::cout << "  --source-map               Generate source map\n";
    std::cout << "  --source-map-output FILE   Output source map to file\n";
    std::cout << "  --help, -h                 Show this help message\n";
    std::cout << "  --version, -v              Show version information\n";
}

void DebugCLI::showVersion() const {
    std::cout << "MADOLA Debug CLI version 1.0.0\n";
    std::cout << "Built with source map support and enhanced error reporting\n";
}

void DebugCLI::dumpSourceMap() const {
    auto source_map_json = source_map_generator_->generateSourceMap();

    if (!source_map_output_.empty()) {
        std::ofstream file(source_map_output_);
        file << source_map_json.dump(2);
        std::cout << "Source map written to: " << source_map_output_ << std::endl;
    } else {
        std::cout << "=== SOURCE MAP ===" << std::endl;
        std::cout << source_map_json.dump(2) << std::endl;
    }
}

void DebugCLI::dumpAST(const Program& program) const {
    std::cout << DebugUtils::formatASTWithPositions(program);
}

void DebugCLI::handleError(const MadolaException& e) const {
    std::cerr << e.formatError(*debug_info_) << std::endl;

    if (debug_enabled_ && !stack_trace_.getContexts().empty()) {
        std::cerr << "\n" << stack_trace_.format(*debug_info_) << std::endl;
    }
}

// DebugUtils implementation
std::string DebugUtils::formatASTWithPositions(const ASTNode& node, int indent) {
    std::stringstream result;
    std::string indent_str(indent * 2, ' ');

    result << indent_str << node.toString();

    if (node.getStartPosition().isValid()) {
        result << " [" << node.getStartPosition().line << ":"
               << node.getStartPosition().column;

        if (node.getEndPosition().isValid()) {
            result << "-" << node.getEndPosition().line << ":"
                   << node.getEndPosition().column;
        }

        result << "]";
    }

    result << "\n";

    // For Program nodes, recursively format statements
    if (const auto* program = dynamic_cast<const Program*>(&node)) {
        for (const auto& stmt : program->statements) {
            result << formatASTWithPositions(*stmt, indent + 1);
        }
    }

    return result.str();
}

bool DebugUtils::validateSourceMap(const SourceMapGenerator& source_map,
                                  const std::string& original_source,
                                  const std::string& generated_output) {
    // Basic validation - check that all mappings have valid positions
    const auto& mappings = source_map.getMappings();

    std::vector<std::string> original_lines;
    std::stringstream ss(original_source);
    std::string line;
    while (std::getline(ss, line)) {
        original_lines.push_back(line);
    }

    std::vector<std::string> generated_lines;
    std::stringstream gs(generated_output);
    while (std::getline(gs, line)) {
        generated_lines.push_back(line);
    }

    for (const auto& mapping : mappings) {
        // Check original position is valid
        if (mapping.original_line == 0 || mapping.original_line > original_lines.size()) {
            return false;
        }

        // Check generated position is valid
        if (mapping.generated_line == 0 || mapping.generated_line > generated_lines.size()) {
            return false;
        }
    }

    return true;
}

std::string DebugUtils::createDebugReport(const Program& program,
                                         const SourceMapGenerator& source_map,
                                         const std::string& /* source_content */) {
    std::stringstream report;

    report << "=== MADOLA Debug Report ===\n\n";

    report << "Source Map Statistics:\n";
    report << "  Total mappings: " << source_map.getMappings().size() << "\n\n";

    report << "AST Structure:\n";
    report << formatASTWithPositions(program) << "\n";

    report << "Source Map JSON:\n";
    report << source_map.generateSourceMap().dump(2) << "\n";

    return report.str();
}

} // namespace debug
} // namespace madola