#include "codegen/c/name_mangle.h"

#include <sstream>
#include <string>

namespace walk::codegen::c {
namespace {

std::string replace_all(std::string value, const std::string& from, const std::string& to) {
    std::size_t index = 0;
    while ((index = value.find(from, index)) != std::string::npos) {
        value.replace(index, from.size(), to);
        index += to.size();
    }
    return value;
}

std::string type_kind_signature(ast::TypeKind kind) {
    switch (kind) {
    case ast::TypeKind::Void:
        return "void";
    case ast::TypeKind::Int:
        return "int";
    case ast::TypeKind::Float:
        return "float";
    case ast::TypeKind::Bool:
        return "bool";
    case ast::TypeKind::String:
        return "string";
    case ast::TypeKind::Null:
        return "null";
    case ast::TypeKind::Array:
        return "array";
    case ast::TypeKind::Function:
        return "func";
    case ast::TypeKind::Struct:
    case ast::TypeKind::Generic:
    case ast::TypeKind::Invalid:
        return "unknown";
    }
    return "unknown";
}

}  // namespace

std::string module_symbol_name(const std::string& module, const std::string& name) {
    return replace_all(module, ".", "__") + "__" + name;
}

std::string method_symbol_name(const std::string& receiver, const std::string& name) {
    return receiver + "__" + name;
}

std::string type_signature(const ast::Type& type) {
    std::string out;
    switch (type.kind) {
    case ast::TypeKind::Array:
        if (type.elem == nullptr) {
            out = "array_unknown";
        } else {
            out = "array_" + type_signature(*type.elem);
        }
        break;
    case ast::TypeKind::Function: {
        std::vector<std::string> parts;
        parts.reserve(type.params.size());
        for (const ast::Type& param : type.params) {
            parts.push_back(type_signature(param));
        }
        std::ostringstream joined;
        for (std::size_t index = 0; index < parts.size(); ++index) {
            if (index != 0) {
                joined << "_";
            }
            joined << parts[index];
        }
        const std::string ret = type.return_type == nullptr ? "void" : type_signature(*type.return_type);
        out = "func_" + joined.str() + "_to_" + ret;
        break;
    }
    case ast::TypeKind::Struct:
    case ast::TypeKind::Generic:
        out = type.name;
        break;
    default:
        out = type_kind_signature(type.kind);
        break;
    }
    out = replace_all(out, "?", "_nullable");
    if (type.nullable) {
        out += "_nullable";
    }
    return out;
}

std::string generic_instance_name(const std::string& callee, const std::vector<ast::Type>& type_args) {
    std::ostringstream out;
    out << replace_all(callee, ".", "__");
    for (const ast::Type& type_arg : type_args) {
        out << "__" << type_signature(type_arg);
    }
    return out.str();
}

}  // namespace walk::codegen::c
