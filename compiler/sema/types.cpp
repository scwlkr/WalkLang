#include "sema/types.h"

#include <utility>

namespace walk::sema {

bool known_type(const ast::Type& type) {
    return type.kind != ast::TypeKind::Invalid;
}

bool type_equal(const ast::Type& left, const ast::Type& right) {
    if (left.kind != right.kind || left.nullable != right.nullable || left.name != right.name) {
        return false;
    }
    switch (left.kind) {
    case ast::TypeKind::Array:
        return left.elem != nullptr && right.elem != nullptr && type_equal(*left.elem, *right.elem);
    case ast::TypeKind::Function:
        if (left.params.size() != right.params.size()) {
            return false;
        }
        for (std::size_t index = 0; index < left.params.size(); ++index) {
            if (!type_equal(left.params[index], right.params[index])) {
                return false;
            }
        }
        if (left.return_type == nullptr || right.return_type == nullptr) {
            return left.return_type == right.return_type;
        }
        return type_equal(*left.return_type, *right.return_type);
    default:
        return true;
    }
}

bool assignable(const ast::Type& source, const ast::Type& target) {
    if (source.kind == ast::TypeKind::Null) {
        return target.nullable;
    }
    if (type_equal(source, target)) {
        return true;
    }
    if (target.nullable) {
        ast::Type non_nullable_target = target;
        non_nullable_target.nullable = false;
        if (type_equal(source, non_nullable_target)) {
            return true;
        }
    }
    return source.kind == ast::TypeKind::Int && target.kind == ast::TypeKind::Float && !source.nullable && !target.nullable;
}

bool comparable(const ast::Type& left, const ast::Type& right) {
    if (left.kind == ast::TypeKind::Struct || right.kind == ast::TypeKind::Struct) {
        return false;
    }
    if (left.kind == ast::TypeKind::Generic || right.kind == ast::TypeKind::Generic) {
        return false;
    }
    if (left.kind == ast::TypeKind::Null) {
        return right.nullable;
    }
    if (right.kind == ast::TypeKind::Null) {
        return left.nullable;
    }
    return assignable(left, right) || assignable(right, left);
}

bool is_numeric(const ast::Type& type) {
    return !type.nullable && (type.kind == ast::TypeKind::Int || type.kind == ast::TypeKind::Float);
}

bool contains_kind(const std::vector<ast::Type>& types, ast::TypeKind kind) {
    for (const ast::Type& type : types) {
        if (type.kind == kind) {
            return true;
        }
    }
    return false;
}

bool native_array_element(const ast::Type& type) {
    return !type.nullable &&
        (type.kind == ast::TypeKind::Int || type.kind == ast::TypeKind::Float || type.kind == ast::TypeKind::Bool || type.kind == ast::TypeKind::String);
}

bool interpolatable(const ast::Type& type) {
    if (type.nullable) {
        return type.kind == ast::TypeKind::String;
    }
    return type.kind == ast::TypeKind::Int || type.kind == ast::TypeKind::Float || type.kind == ast::TypeKind::Bool || type.kind == ast::TypeKind::String;
}

ast::Type nullable_string() {
    ast::Type type = ast::basic(ast::TypeKind::String);
    type.nullable = true;
    return type;
}

ast::Type substitute_type(const ast::Type& type, const std::map<std::string, ast::Type>& bindings) {
    switch (type.kind) {
    case ast::TypeKind::Generic: {
        const auto found = bindings.find(type.name);
        if (found == bindings.end()) {
            return type;
        }
        ast::Type replacement = found->second;
        if (type.nullable) {
            replacement.nullable = true;
        }
        return replacement;
    }
    case ast::TypeKind::Array:
        if (type.elem == nullptr) {
            return type;
        }
        {
            ast::Type result = ast::array_of(substitute_type(*type.elem, bindings));
            result.nullable = type.nullable;
            return result;
        }
    case ast::TypeKind::Function: {
        std::vector<ast::Type> params;
        params.reserve(type.params.size());
        for (const ast::Type& param : type.params) {
            params.push_back(substitute_type(param, bindings));
        }
        if (type.return_type == nullptr) {
            return type;
        }
        ast::Type result = ast::function_type(std::move(params), substitute_type(*type.return_type, bindings));
        result.nullable = type.nullable;
        return result;
    }
    default:
        return type;
    }
}

}  // namespace walk::sema
