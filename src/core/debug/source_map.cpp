#include "source_map.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace madola {
namespace debug {

namespace {
    // Base64 characters for VLQ encoding
    const char BASE64_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    // VLQ base value
    const int VLQ_BASE = 32;
    const int VLQ_BASE_MASK = VLQ_BASE - 1;
    const int VLQ_CONTINUATION_BIT = VLQ_BASE;
}

// SourceMapGenerator implementation
SourceMapGenerator::SourceMapGenerator(const std::string& source_file)
    : source_file_(source_file) {}

void SourceMapGenerator::addMapping(uint32_t generated_line, uint32_t generated_column,
                                  uint32_t original_line, uint32_t original_column,
                                  const std::string& name) {
    mappings_.emplace_back(generated_line, generated_column,
                          original_line, original_column,
                          source_file_, name);
}

void SourceMapGenerator::addMapping(uint32_t generated_line, uint32_t generated_column,
                                  const ASTNode& node, const std::string& name) {
    if (node.getStartPosition().isValid()) {
        addMapping(generated_line, generated_column,
                  node.getStartPosition().line, node.getStartPosition().column, name);
    }
}

nlohmann::json SourceMapGenerator::generateSourceMap(const std::string& generated_file) const {
    nlohmann::json source_map;

    // Source map version (always 3)
    source_map["version"] = 3;

    // Generated file name
    if (!generated_file.empty()) {
        source_map["file"] = generated_file;
    }

    // Source root (optional)
    source_map["sourceRoot"] = "";

    // Source files
    nlohmann::json sources = nlohmann::json::array();
    sources.push_back(source_file_);
    source_map["sources"] = sources;

    // Source names (for symbol mapping)
    nlohmann::json names = nlohmann::json::array();
    std::map<std::string, int> name_indices;
    int name_index = 0;

    for (const auto& mapping : mappings_) {
        if (!mapping.name.empty() && name_indices.find(mapping.name) == name_indices.end()) {
            names.push_back(mapping.name);
            name_indices[mapping.name] = name_index++;
        }
    }
    source_map["names"] = names;

    // Source content (optional but helpful for debugging)
    if (!source_content_.empty()) {
        nlohmann::json sourcesContent = nlohmann::json::array();
        sourcesContent.push_back(source_content_);
        source_map["sourcesContent"] = sourcesContent;
    }

    // Encoded mappings
    source_map["mappings"] = encodeMappings();

    return source_map;
}

SourceLocation SourceMapGenerator::findOriginalPosition(uint32_t generated_line, uint32_t generated_column) const {
    // Find the closest mapping
    const SourceMapEntry* closest = nullptr;

    for (const auto& mapping : mappings_) {
        if (mapping.generated_line > generated_line) break;
        if (mapping.generated_line == generated_line && mapping.generated_column > generated_column) break;
        closest = &mapping;
    }

    if (closest) {
        return SourceLocation(closest->original_line, closest->original_column, 0);
    }

    return SourceLocation(); // Invalid location
}

std::vector<SourceLocation> SourceMapGenerator::findGeneratedPositions(uint32_t original_line, uint32_t original_column) const {
    std::vector<SourceLocation> positions;

    for (const auto& mapping : mappings_) {
        if (mapping.original_line == original_line && mapping.original_column == original_column) {
            positions.emplace_back(mapping.generated_line, mapping.generated_column, 0);
        }
    }

    return positions;
}

void SourceMapGenerator::clear() {
    mappings_.clear();
}

std::string SourceMapGenerator::encodeMappings() const {
    if (mappings_.empty()) return "";

    std::stringstream result;

    // Sort mappings by generated position
    auto sorted_mappings = mappings_;
    std::sort(sorted_mappings.begin(), sorted_mappings.end(),
              [](const SourceMapEntry& a, const SourceMapEntry& b) {
                  if (a.generated_line != b.generated_line) {
                      return a.generated_line < b.generated_line;
                  }
                  return a.generated_column < b.generated_column;
              });

    uint32_t prev_generated_line = 0;
    uint32_t prev_generated_column = 0;
    uint32_t prev_original_line = 0;
    uint32_t prev_original_column = 0;

    for (size_t i = 0; i < sorted_mappings.size(); ++i) {
        const auto& mapping = sorted_mappings[i];

        // Add line separators
        while (prev_generated_line < mapping.generated_line) {
            result << ";";
            prev_generated_line++;
            prev_generated_column = 0;
        }

        if (i > 0 && sorted_mappings[i-1].generated_line == mapping.generated_line) {
            result << ",";
        }

        // Encode VLQ values
        result << encodeVLQ(static_cast<int32_t>(mapping.generated_column) - static_cast<int32_t>(prev_generated_column));
        result << encodeVLQ(0); // Source index (always 0 for single source)
        result << encodeVLQ(static_cast<int32_t>(mapping.original_line) - static_cast<int32_t>(prev_original_line));
        result << encodeVLQ(static_cast<int32_t>(mapping.original_column) - static_cast<int32_t>(prev_original_column));

        // Update previous values
        prev_generated_column = mapping.generated_column;
        prev_original_line = mapping.original_line;
        prev_original_column = mapping.original_column;
    }

    return result.str();
}

std::string SourceMapGenerator::encodeVLQ(int32_t value) const {
    std::string result;

    uint32_t vlq = (value < 0) ? ((-value) << 1) | 1 : value << 1;

    do {
        uint32_t digit = vlq & VLQ_BASE_MASK;
        vlq >>= 5;

        if (vlq > 0) {
            digit |= VLQ_CONTINUATION_BIT;
        }

        result += BASE64_CHARS[digit];
    } while (vlq > 0);

    return result;
}

// DebugInfo implementation
DebugInfo::DebugInfo(const std::string& source_file, const std::string& source_content)
    : source_file_(source_file), source_content_(source_content) {
    splitSourceIntoLines();
}

void DebugInfo::setSourceMap(std::shared_ptr<SourceMapGenerator> source_map) {
    source_map_ = source_map;
}

std::string DebugInfo::formatError(const std::string& message,
                                  const SourceLocation& location,
                                  int context_lines) const {
    std::stringstream result;

    result << "Error in " << source_file_ << ":" << location.line << ":" << location.column << "\n";
    result << message << "\n\n";

    // Add source context
    auto context = getSourceContext(location.line, context_lines);
    uint32_t start_line = location.line - std::min(static_cast<uint32_t>(context_lines), location.line - 1);

    for (size_t i = 0; i < context.size(); ++i) {
        uint32_t line_num = start_line + i;
        bool is_error_line = (line_num == location.line);

        result << std::setw(4) << line_num << " | " << context[i] << "\n";

        if (is_error_line && location.column > 0) {
            result << "     | " << std::string(location.column - 1, ' ') << "^\n";
        }
    }

    return result.str();
}

std::string DebugInfo::formatErrorFromGenerated(const std::string& message,
                                               uint32_t generated_line,
                                               uint32_t generated_column,
                                               int context_lines) const {
    if (source_map_) {
        SourceLocation original = source_map_->findOriginalPosition(generated_line, generated_column);
        if (original.isValid()) {
            return formatError(message, original, context_lines);
        }
    }

    // Fallback to generated position
    return formatError(message, SourceLocation(generated_line, generated_column, 0), context_lines);
}

std::string DebugInfo::getSourceLine(uint32_t line_number) const {
    if (line_number > 0 && line_number <= source_lines_.size()) {
        return source_lines_[line_number - 1];
    }
    return "";
}

std::vector<std::string> DebugInfo::getSourceContext(uint32_t line_number, int context_lines) const {
    std::vector<std::string> context;

    uint32_t start = (line_number > static_cast<uint32_t>(context_lines))
                     ? line_number - context_lines : 1;
    uint32_t end = std::min(line_number + context_lines, static_cast<uint32_t>(source_lines_.size()));

    for (uint32_t i = start; i <= end; ++i) {
        context.push_back(getSourceLine(i));
    }

    return context;
}

void DebugInfo::splitSourceIntoLines() {
    source_lines_.clear();
    std::stringstream ss(source_content_);
    std::string line;

    while (std::getline(ss, line)) {
        source_lines_.push_back(line);
    }

    // Handle case where source doesn't end with newline
    if (!source_content_.empty() && source_content_.back() != '\n') {
        // Last line is already added by getline
    }
}

} // namespace debug
} // namespace madola