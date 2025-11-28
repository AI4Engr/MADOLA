#pragma once
#include "../debug/source_map.h"
#include "../debug/error_reporting.h"
#include "../ast/ast.h"
#include <memory>
#include <string>
#include <map>

namespace madola {
namespace debug {

// Debug-enabled CLI tool for MADOLA
class DebugCLI {
public:
    DebugCLI(const std::string& source_file, const std::string& source_content);

    // Enable debug mode
    void enableDebug(bool enable = true) { debug_enabled_ = enable; }
    bool isDebugEnabled() const { return debug_enabled_; }

    // Enable source map generation
    void enableSourceMap(bool enable = true) { source_map_enabled_ = enable; }
    bool isSourceMapEnabled() const { return source_map_enabled_; }

    // Set output file for source map
    void setSourceMapOutput(const std::string& output_file) { source_map_output_ = output_file; }

    // Process MADOLA file with debugging
    int run(const std::vector<std::string>& args);

    // Get generated source map
    std::shared_ptr<SourceMapGenerator> getSourceMap() const { return source_map_generator_; }

    // Get debug info
    std::shared_ptr<DebugInfo> getDebugInfo() const { return debug_info_; }

private:
    std::string source_file_;
    std::string source_content_;
    bool debug_enabled_;
    bool source_map_enabled_;
    std::string source_map_output_;

    std::shared_ptr<SourceMapGenerator> source_map_generator_;
    std::shared_ptr<DebugInfo> debug_info_;
    StackTrace stack_trace_;

    // Command handlers
    void showHelp() const;
    void showVersion() const;
    void dumpSourceMap() const;
    void dumpAST(const Program& program) const;

    // Error formatting with debug info
    void handleError(const MadolaException& e) const;
};

// Debug utilities
class DebugUtils {
public:
    // Format AST with position information
    static std::string formatASTWithPositions(const ASTNode& node, int indent = 0);

    // Validate source map consistency
    static bool validateSourceMap(const SourceMapGenerator& source_map,
                                 const std::string& original_source,
                                 const std::string& generated_output);

    // Create debug report
    static std::string createDebugReport(const Program& program,
                                       const SourceMapGenerator& source_map,
                                       const std::string& source_content);
};

} // namespace debug
} // namespace madola