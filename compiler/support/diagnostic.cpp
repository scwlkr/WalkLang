#include "support/diagnostic.h"

#include <algorithm>
#include <fstream>
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
    line.push_back('^');
    return line;
}

bool starts_with(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

std::string category_for_message(const std::string& message) {
    for (const std::string& category : {"syntax error", "type error", "name error", "module error", "warning", "internal error"}) {
        if (starts_with(message, category + ": ")) {
            return category;
        }
    }
    return "";
}

std::string type_suggestion(const std::string& message) {
    const std::size_t is_pos = message.find(" is ");
    const std::size_t got_pos = message.find(", got ");
    if (is_pos != std::string::npos && got_pos != std::string::npos && is_pos < got_pos) {
        const std::string expected = message.substr(is_pos + 4, got_pos - (is_pos + 4));
        const std::string got = message.substr(got_pos + 6);
        if (!expected.empty() && !got.empty()) {
            return got + " cannot initialize " + expected;
        }
    }
    if (starts_with(message, "function returns ")) {
        const std::string rest = message.substr(std::string("function returns ").size());
        const std::size_t split = rest.find(", got ");
        if (split != std::string::npos) {
            return rest.substr(split + 6) + " cannot be returned from function returning " + rest.substr(0, split);
        }
    }
    if (message.find("condition must be bool") != std::string::npos) {
        return "use a bool expression for the condition";
    }
    if (message.find("assert needs bool") != std::string::npos) {
        return "assert a bool expression";
    }
    if (message.find("repeat count must be int") != std::string::npos) {
        return "use an int expression for the repeat count";
    }
    return "";
}

std::string suggestion_for(const std::string& category, const std::string& message) {
    if (category == "syntax error") {
        if (message.find("tabs are invalid") != std::string::npos) {
            return "replace tabs with spaces";
        }
        if (message.find("unexpected indentation") != std::string::npos) {
            return "align indentation with the surrounding block";
        }
        if (message.find("expected expression block") != std::string::npos) {
            return "indent the expression on the next line";
        }
    }
    if (category == "type error") {
        return type_suggestion(message);
    }
    if (category == "name error") {
        if (starts_with(message, "module ") && message.find(" is not imported") != std::string::npos) {
            std::string module = message.substr(std::string("module ").size());
            const std::size_t split = module.find(' ');
            if (split != std::string::npos) {
                module = module.substr(0, split);
            }
            if (!module.empty()) {
                return "add imp: " + module;
            }
        }
        if (message.find(" is not defined") != std::string::npos) {
            return "define the name before using it";
        }
    }
    if (category == "warning") {
        if (message.find("shadows outer name") != std::string::npos) {
            return "rename this binding or assign to the existing name";
        }
        if (message.find("unreachable statement") != std::string::npos) {
            return "remove this statement or move it before the terminating statement";
        }
    }
    return "";
}

std::string source_line_from_path(const SourceRange& range) {
    std::ifstream input(range.path);
    if (!input) {
        return "";
    }
    std::string line;
    for (std::size_t current = 1; std::getline(input, line); ++current) {
        if (current == range.start.line) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return line;
        }
    }
    return "";
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

    const std::string category = category_for_message(diagnostic.message());
    std::string display_message = diagnostic.message();
    std::string suggestion_message;
    if (!category.empty()) {
        suggestion_message = display_message.substr(category.size() + 2);
    }
    if (diagnostic.severity() == DiagnosticSeverity::Warning && category.empty()) {
        display_message = "warning: " + display_message;
        suggestion_message = diagnostic.message();
    } else if (category.empty() && !diagnostic.code().empty()) {
        display_message = severity_name(diagnostic.severity()) + "[" + diagnostic.code() + "]: " + display_message;
    }
    output << display_message;

    if (diagnostic.range()) {
        const SourceRange& range = *diagnostic.range();
        std::string source_line;
        if (source != nullptr && range.path == source->path()) {
            source_line = source->line_text(range.start.line);
        } else {
            source_line = source_line_from_path(range);
        }
        if (!source_line.empty()) {
            output << "\n\n" << source_line << "\n" << caret_line(range);
            const std::string message_without_category = suggestion_message.empty() && !category.empty()
                ? diagnostic.message().substr(category.size() + 2)
                : suggestion_message;
            const std::string suggestion = suggestion_for(category.empty() && diagnostic.severity() == DiagnosticSeverity::Warning ? "warning" : category, message_without_category);
            if (!suggestion.empty()) {
                output << " " << suggestion;
            }
        }
    }

    return output.str();
}

}  // namespace walk
