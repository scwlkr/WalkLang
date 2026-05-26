#pragma once

#include "support/source_file.h"

#include <optional>
#include <string>
#include <vector>

namespace walk {

enum class DiagnosticSeverity {
    Error,
    Warning,
    Note,
};

class Diagnostic {
public:
    Diagnostic(DiagnosticSeverity severity, std::string code, std::string message);
    Diagnostic(DiagnosticSeverity severity, std::string code, std::string message, SourceRange range);

    [[nodiscard]] DiagnosticSeverity severity() const;
    [[nodiscard]] const std::string& code() const;
    [[nodiscard]] const std::string& message() const;
    [[nodiscard]] const std::optional<SourceRange>& range() const;

private:
    DiagnosticSeverity severity_;
    std::string code_;
    std::string message_;
    std::optional<SourceRange> range_;
};

class DiagnosticSet {
public:
    void add(Diagnostic diagnostic);
    void sort();

    [[nodiscard]] bool empty() const;
    [[nodiscard]] bool has_errors() const;
    [[nodiscard]] const std::vector<Diagnostic>& diagnostics() const;
    [[nodiscard]] std::string format(const SourceFile* source = nullptr) const;

private:
    std::vector<Diagnostic> diagnostics_;
};

std::string severity_name(DiagnosticSeverity severity);
std::string format_diagnostic(const Diagnostic& diagnostic, const SourceFile* source = nullptr);

}  // namespace walk
