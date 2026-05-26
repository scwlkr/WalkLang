#include "sema/scope.h"

#include <utility>

namespace walk::sema {

ScopeStack::ScopeStack() : scopes_({{}}) {}

void ScopeStack::push() {
    scopes_.push_back({});
}

void ScopeStack::pop() {
    if (scopes_.size() > 1) {
        scopes_.pop_back();
    }
}

bool ScopeStack::current_has(const std::string& name) const {
    return !scopes_.empty() && scopes_.back().find(name) != scopes_.back().end();
}

bool ScopeStack::outer_has(const std::string& name) const {
    if (scopes_.size() < 2) {
        return false;
    }
    for (std::size_t index = scopes_.size() - 1; index-- > 0;) {
        if (scopes_[index].find(name) != scopes_[index].end()) {
            return true;
        }
    }
    return false;
}

std::optional<Symbol> ScopeStack::resolve(const std::string& name) const {
    for (std::size_t index = scopes_.size(); index-- > 0;) {
        const auto found = scopes_[index].find(name);
        if (found != scopes_[index].end()) {
            return found->second;
        }
    }
    return std::nullopt;
}

void ScopeStack::define_current(std::string name, Symbol symbol) {
    scopes_.back().emplace(std::move(name), std::move(symbol));
}

}  // namespace walk::sema
