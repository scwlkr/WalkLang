#include "support/diagnostic.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>

namespace walk {
namespace {

std::string caret_line(const SourceRange& range) {
    std::string line;
    if (range.start.column > 1) {
        line.append(range.start.column - 1, ' ');
    }
    const std::size_t width = range.end.line == range.start.line && range.end.column > range.start.column
        ? range.end.column - range.start.column
        : 1;
    line.push_back('^');
    if (width > 1) {
        line.append(width - 1, '~');
    }
    return line;
}

bool comes_before(const Diagnostic& left, const Diagnostic& right) {
    const std::optional<SourceRange>& left_range = left.range();
    const std::optional<SourceRange>& right_range = right.range();
    if (left_range.has_value() != right_range.has_value()) {
        return left_range.has_value();
    }
    if (left_range && right_range) {
        if (left_range->path != right_range->path) {
            return left_range->path < right_range->path;
        }
        if (left_range->start.line != right_range->start.line) {
            return left_range->start.line < right_range->start.line;
        }
        if (left_range->start.column != right_range->start.column) {
            return left_range->start.column < right_range->start.column;
        }
    }
    if (left.code() != right.code()) {
        return left.code() < right.code();
    }
    return left.message() < right.message();
}

}  // namespace

Diagnostic::Diagnostic(DiagnosticSeverity severity, std::string code, std::string message)
    : severity_(severity), code_(std::move(code)), message_(std::move(message)) {}

Diagnostic::Diagnostic(DiagnosticSeverity severity, std::string code, std::string message, SourceRange range)
    : severity_(severity), code_(std::move(code)), message_(std::move(message)), range_(std::move(range)) {}

DiagnosticSeverity Diagnostic::severity() const {
    return severity_;
}

const std::string& Diagnostic::code() const {
    return code_;
}

const std::string& Diagnostic::message() const {
    return message_;
}

const std::optional<SourceRange>& Diagnostic::range() const {
    return range_;
}

void DiagnosticSet::add(Diagnostic diagnostic) {
    diagnostics_.push_back(std::move(diagnostic));
}

void DiagnosticSet::sort() {
    std::stable_sort(diagnostics_.begin(), diagnostics_.end(), comes_before);
}

bool DiagnosticSet::empty() const {
    return diagnostics_.empty();
}

bool DiagnosticSet::has_errors() const {
    return std::any_of(diagnostics_.begin(), diagnostics_.end(), [](const Diagnostic& diagnostic) {
        return diagnostic.severity() == DiagnosticSeverity::Error;
    });
}

const std::vector<Diagnostic>& DiagnosticSet::diagnostics() const {
    return diagnostics_;
}

std::string DiagnosticSet::format(const SourceFile* source) const {
    std::ostringstream output;
    for (const Diagnostic& diagnostic : diagnostics_) {
        output << format_diagnostic(diagnostic, source) << "\n";
    }
    return output.str();
}

std::string severity_name(DiagnosticSeverity severity) {
    switch (severity) {
    case DiagnosticSeverity::Error:
        return "error";
    case DiagnosticSeverity::Warning:
        return "warning";
    case DiagnosticSeverity::Note:
        return "note";
    }
    return "error";
}

std::string format_diagnostic(const Diagnostic& diagnostic, const SourceFile* source) {
    std::ostringstream output;
    if (diagnostic.range()) {
        const SourceRange& range = *diagnostic.range();
        output << range.path << ":" << range.start.line << ":" << range.start.column << ": ";
    }
    output << severity_name(diagnostic.severity()) << "[" << diagnostic.code() << "]: " << diagnostic.message();

    if (source != nullptr && diagnostic.range() && diagnostic.range()->path == source->path()) {
        const SourceRange& range = *diagnostic.range();
        const std::string source_line = source->line_text(range.start.line);
        if (!source_line.empty()) {
            output << "\n" << source_line << "\n" << caret_line(range);
        }
    }

    return output.str();
}

}  // namespace walk
