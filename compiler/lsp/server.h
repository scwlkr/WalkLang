#pragma once

#include <istream>
#include <ostream>
#include <string>

namespace walk::lsp {

int serve(std::istream& input, std::ostream& output);
std::string initialize_result_json();
std::string diagnostics_json(const std::string& path, const std::string& source);
std::string formatting_edits_json(const std::string& path, const std::string& source);

}  // namespace walk::lsp
