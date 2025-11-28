#pragma once
#include "../ast/ast.h"
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <cstdint>

// Conditional JSON include
#ifdef WITH_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#else
// Minimal JSON placeholder for when nlohmann/json is not available
namespace nlohmann {
    struct json {
        json() = default;
        json& operator[](const std::string&) { return *this; }
        void push_back(const std::string&) {}
        std::string dump(int = 0) const { return "{}"; }
        static json array() { return json{}; }
        template<typename T> json& operator=(const T&) { return *this; }
    };
}
#endif

namespace madola {
namespace debug {

// Represents a mapping entry in the source map
struct SourceMapEntry {
    uint32_t generated_line;
    uint32_t generated_column;
    uint32_t original_line;
    uint32_t original_column;
    std::string source_file;
    std::string name;  // Optional symbol name

    SourceMapEntry(uint32_t gen_line, uint32_t gen_col,
                   uint32_t orig_line, uint32_t orig_col,
                   const std::string& source = "",
                   const std::string& symbol_name = "")
        : generated_line(gen_line), generated_column(gen_col),
          original_line(orig_line), original_column(orig_col),
          source_file(source), name(symbol_name) {}
};

// Source map generator for MADOLA debugging
class SourceMapGenerator {
public:
    explicit SourceMapGenerator(const std::string& source_file);

    // Add a mapping from generated position to original position
    void addMapping(uint32_t generated_line, uint32_t generated_column,
                   uint32_t original_line, uint32_t original_column,
                   const std::string& name = "");

    // Add mapping from AST node
    void addMapping(uint32_t generated_line, uint32_t generated_column,
                   const ASTNode& node, const std::string& name = "");

    // Generate source map in JSON format
    nlohmann::json generateSourceMap(const std::string& generated_file = "") const;

    // Get all mappings
    const std::vector<SourceMapEntry>& getMappings() const { return mappings_; }

    // Find original position for generated position
    SourceLocation findOriginalPosition(uint32_t generated_line, uint32_t generated_column) const;

    // Find generated position for original position
    std::vector<SourceLocation> findGeneratedPositions(uint32_t original_line, uint32_t original_column) const;

    // Clear all mappings
    void clear();

    // Set source content for embedding in source map
    void setSourceContent(const std::string& content) { source_content_ = content; }

    // Get source file name
    const std::string& getSourceFile() const { return source_file_; }

private:
    std::string source_file_;
    std::string source_content_;
    std::vector<SourceMapEntry> mappings_;

    // Convert mappings to VLQ encoded string
    std::string encodeMappings() const;

    // VLQ encoding utility
    std::string encodeVLQ(int32_t value) const;
};

// Source map consumer for reading existing source maps
class SourceMapConsumer {
public:
    explicit SourceMapConsumer(const std::string& source_map_json);

    // Find original position for generated position
    SourceLocation findOriginalPosition(uint32_t generated_line, uint32_t generated_column) const;

    // Get source content
    const std::string& getSourceContent(const std::string& source_file) const;

    // Get all source files
    std::vector<std::string> getSourceFiles() const;

private:
    nlohmann::json source_map_;
    std::vector<SourceMapEntry> mappings_;

    void parseMappings(const std::string& mappings_string);
    int32_t decodeVLQ(const std::string& encoded, size_t& index) const;
};

// Utility class for enhanced error reporting with source maps
class DebugInfo {
public:
    DebugInfo(const std::string& source_file, const std::string& source_content);

    // Set source map for debugging
    void setSourceMap(std::shared_ptr<SourceMapGenerator> source_map);

    // Format error with source context
    std::string formatError(const std::string& message,
                           const SourceLocation& location,
                           int context_lines = 2) const;

    // Format error from generated position using source map
    std::string formatErrorFromGenerated(const std::string& message,
                                        uint32_t generated_line,
                                        uint32_t generated_column,
                                        int context_lines = 2) const;

    // Get source line
    std::string getSourceLine(uint32_t line_number) const;

    // Get source context (lines around a position)
    std::vector<std::string> getSourceContext(uint32_t line_number, int context_lines = 2) const;

    // Get source file name
    const std::string& getSourceFile() const { return source_file_; }

private:
    std::string source_file_;
    std::string source_content_;
    std::vector<std::string> source_lines_;
    std::shared_ptr<SourceMapGenerator> source_map_;

    void splitSourceIntoLines();
};

} // namespace debug
} // namespace madola