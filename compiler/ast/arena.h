#pragma once

#include "ast/ast.h"

#include <memory>
#include <utility>
#include <vector>

namespace walk::ast {

class Arena {
public:
    template <typename T, typename... Args>
    T* make(Args&&... args) {
        auto node = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = node.get();
        nodes_.push_back(std::move(node));
        return raw;
    }

private:
    std::vector<std::unique_ptr<Node>> nodes_;
};

}  // namespace walk::ast
