#pragma once

#include "ast/ast.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace walk::sema {

struct Symbol {
    ast::Type type;
    bool mutable_binding = false;
};

class ScopeStack {
public:
    ScopeStack();

    void push();
    void pop();
    [[nodiscard]] bool current_has(const std::string& name) const;
    [[nodiscard]] bool outer_has(const std::string& name) const;
    [[nodiscard]] std::optional<Symbol> resolve(const std::string& name) const;
    void define_current(std::string name, Symbol symbol);

private:
    std::vector<std::map<std::string, Symbol>> scopes_;
};

}  // namespace walk::sema
