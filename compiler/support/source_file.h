#pragma once

#include "support/result.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace walk {

struct SourcePosition {
    std::size_t line = 1;
    std::size_t column = 1;
};

struct SourceRange {
    std::string path;
    std::size_t start_offset = 0;
    std::size_t end_offset = 0;
    SourcePosition start;
    SourcePosition end;
};

class SourceFile {
public:
    static Result<SourceFile> load(const std::string& path);
    static SourceFile from_text(std::string path, std::string text);

    [[nodiscard]] const std::string& path() const;
    [[nodiscard]] std::string_view text() const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] SourcePosition position_for_offset(std::size_t offset) const;
    [[nodiscard]] SourceRange range_for_offsets(std::size_t start_offset, std::size_t end_offset) const;
    [[nodiscard]] std::string line_text(std::size_t line) const;

private:
    SourceFile(std::string path, std::string text);
    void build_line_index();

    std::string path_;
    std::string text_;
    std::vector<std::size_t> line_starts_;
};

}  // namespace walk
