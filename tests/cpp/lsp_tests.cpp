#include "lsp/server.h"

#include <iostream>
#include <sstream>
#include <string>

namespace {

int failures = 0;

void expect_contains(const std::string& name, const std::string& text, const std::string& needle) {
    if (text.find(needle) == std::string::npos) {
        std::cerr << "FAIL " << name << ": missing " << needle << " in " << text << "\n";
        ++failures;
    }
}

std::string message(const std::string& body) {
    return "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
}

void test_lsp_initialize_diagnostics_and_formatting() {
    const std::string uri = "file:///tmp/main.walk";
    const std::string init = R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{}})";
    const std::string did_open = R"({"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":")" + uri + R"(","languageId":"walk","version":1,"text":"var: age int = 'old'\n"}}})";
    const std::string formatting = R"({"jsonrpc":"2.0","id":2,"method":"textDocument/formatting","params":{"textDocument":{"uri":")" + uri + R"("}}})";
    std::istringstream input(message(init) + message(did_open) + message(formatting));
    std::ostringstream output;
    const int code = walk::lsp::serve(input, output);
    if (code != 0) {
        std::cerr << "FAIL lsp serve: exit " << code << "\n";
        ++failures;
        return;
    }
    const std::string text = output.str();
    expect_contains("lsp initialize", text, "\"documentFormattingProvider\":true");
    expect_contains("lsp diagnostics", text, "type error: age is int, got string");
    expect_contains("lsp formatting", text, "var: age int = 'old'\\n");
}

}  // namespace

int run_lsp_tests() {
    failures = 0;
    test_lsp_initialize_diagnostics_and_formatting();
    return failures;
}
