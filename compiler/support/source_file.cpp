#include "support/source_file.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include <utility>

namespace walk {

Result<SourceFile> SourceFile::load(const std::string& path) {
    if (path.empty()) {
        return Result<SourceFile>::failure("source path is empty");
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return Result<SourceFile>::failure("source read failed: " + path);
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (input.bad()) {
        return Result<SourceFile>::failure("source read failed: " + path);
    }

    return Result<SourceFile>::success(SourceFile(path, buffer.str()));
}

SourceFile SourceFile::from_text(std::string path, std::string text) {
    return SourceFile(std::move(path), std::move(text));
}

const std::string& SourceFile::path() const {
    return path_;
}

std::string_view SourceFile::text() const {
    return text_;
}

std::size_t SourceFile::size() const {
    return text_.size();
}

SourcePosition SourceFile::position_for_offset(std::size_t offset) const {
    const std::size_t clamped = std::min(offset, text_.size());
    const auto upper = std::upper_bound(line_starts_.begin(), line_starts_.end(), clamped);
    const std::size_t line_index = upper == line_starts_.begin()
        ? 0
        : static_cast<std::size_t>(std::distance(line_starts_.begin(), upper) - 1);
    const std::size_t line_start = line_starts_.empty() ? 0 : line_starts_[line_index];
    return {line_index + 1, clamped - line_start + 1};
}

SourceRange SourceFile::range_for_offsets(std::size_t start_offset, std::size_t end_offset) const {
    const std::size_t start = std::min(start_offset, text_.size());
    const std::size_t end = std::min(std::max(end_offset, start), text_.size());
    return {path_, start, end, position_for_offset(start), position_for_offset(end)};
}

std::string SourceFile::line_text(std::size_t line) const {
    if (line == 0 || line > line_starts_.size()) {
        return "";
    }

    const std::size_t start = line_starts_[line - 1];
    std::size_t end = text_.size();
    if (line < line_starts_.size()) {
        end = line_starts_[line] - 1;
    }
    if (end > start && text_[end - 1] == '\n') {
        --end;
    }
    if (end > start && text_[end - 1] == '\r') {
        --end;
    }
    return text_.substr(start, end - start);
}

SourceFile::SourceFile(std::string path, std::string text)
    : path_(std::move(path)), text_(std::move(text)) {
    build_line_index();
}

void SourceFile::build_line_index() {
    line_starts_.clear();
    line_starts_.push_back(0);
    for (std::size_t index = 0; index < text_.size(); ++index) {
        if (text_[index] == '\n' && index + 1 < text_.size()) {
            line_starts_.push_back(index + 1);
        }
    }
}

}  // namespace walk
