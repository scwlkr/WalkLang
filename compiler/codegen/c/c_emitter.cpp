#include "codegen/c/c_emitter.h"

#include "codegen/c/name_mangle.h"
#include "sema/builtins.h"
#include "sema/types.h"

#include <algorithm>
#include <filesystem>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace walk::codegen::c {
namespace {

class EmitError : public std::runtime_error {
public:
    explicit EmitError(const std::string& message) : std::runtime_error(message) {}
};

struct DeferScope {
    std::vector<std::string> cleanups;
};

struct GenericDecl {
    std::string module;
    const ir::FuncDecl* function = nullptr;
};

struct GenericInstance {
    std::string callee;
    std::string module;
    const ir::FuncDecl* function = nullptr;
    std::vector<ast::Type> type_args;
    std::string c_name;
};

std::string join(const std::vector<std::string>& values, const std::string& sep) {
    std::ostringstream out;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            out << sep;
        }
        out << values[index];
    }
    return out.str();
}

std::string replace_all(std::string value, const std::string& from, const std::string& to) {
    std::size_t index = 0;
    while ((index = value.find(from, index)) != std::string::npos) {
        value.replace(index, from.size(), to);
        index += to.size();
    }
    return value;
}

std::string escape_c_string(const std::string& value) {
    std::ostringstream out;
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            out << "\\\\";
            break;
        case '"':
            out << "\\\"";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            out << ch;
            break;
        }
    }
    return out.str();
}

bool has_source_position(const SourceRange& range) {
    return range.start.line != 0;
}

std::string source_comment(const SourceRange& range) {
    return "/* source: " + std::filesystem::path(range.path).filename().string() + ":" + std::to_string(range.start.line) + ":" + std::to_string(range.start.column) + " */";
}

std::vector<std::string> append_indented(std::vector<std::string> lines, const std::vector<std::string>& nested) {
    for (const std::string& line : nested) {
        lines.push_back("    " + line);
    }
    return lines;
}

std::vector<std::string> prepend_source_comment(const SourceRange& range, std::vector<std::string> lines) {
    if (!has_source_position(range)) {
        return lines;
    }
    std::vector<std::string> out;
    out.reserve(lines.size() + 1);
    out.push_back(source_comment(range));
    out.insert(out.end(), lines.begin(), lines.end());
    return out;
}

std::vector<std::string> struct_dependencies(const ast::Type& type) {
    switch (type.kind) {
    case ast::TypeKind::Struct:
        return {type.name};
    case ast::TypeKind::Array:
        if (type.elem == nullptr) {
            return {};
        }
        return struct_dependencies(*type.elem);
    case ast::TypeKind::Map: {
        std::vector<std::string> deps;
        if (type.key != nullptr) {
            std::vector<std::string> key_deps = struct_dependencies(*type.key);
            deps.insert(deps.end(), key_deps.begin(), key_deps.end());
        }
        if (type.elem != nullptr) {
            std::vector<std::string> value_deps = struct_dependencies(*type.elem);
            deps.insert(deps.end(), value_deps.begin(), value_deps.end());
        }
        return deps;
    }
    case ast::TypeKind::Function: {
        std::vector<std::string> deps;
        for (const ast::Type& param : type.params) {
            std::vector<std::string> param_deps = struct_dependencies(param);
            deps.insert(deps.end(), param_deps.begin(), param_deps.end());
        }
        if (type.return_type != nullptr) {
            std::vector<std::string> ret_deps = struct_dependencies(*type.return_type);
            deps.insert(deps.end(), ret_deps.begin(), ret_deps.end());
        }
        return deps;
    }
    default:
        return {};
    }
}

bool type_contains_generic(const ast::Type& type) {
    switch (type.kind) {
    case ast::TypeKind::Generic:
        return true;
    case ast::TypeKind::Array:
        return type.elem != nullptr && type_contains_generic(*type.elem);
    case ast::TypeKind::Map:
        return (type.key != nullptr && type_contains_generic(*type.key)) || (type.elem != nullptr && type_contains_generic(*type.elem));
    case ast::TypeKind::Function:
        for (const ast::Type& param : type.params) {
            if (type_contains_generic(param)) {
                return true;
            }
        }
        return type.return_type != nullptr && type_contains_generic(*type.return_type);
    default:
        return false;
    }
}

bool contains_generic_type(const std::vector<ast::Type>& types) {
    for (const ast::Type& type : types) {
        if (type_contains_generic(type)) {
            return true;
        }
    }
    return false;
}

std::string generic_instance_key(const std::string& callee, const std::vector<ast::Type>& type_args) {
    std::vector<std::string> parts;
    parts.reserve(type_args.size() + 1);
    parts.push_back(callee);
    for (const ast::Type& type_arg : type_args) {
        parts.push_back(type_signature(type_arg));
    }
    return join(parts, "|");
}

std::string c_struct_name(const std::string& name) {
    return name;
}

std::string c_struct_array_name(const std::string& name) {
    return "WalkArray" + name;
}

std::string c_array_item_type(const ast::Type& type) {
    switch (type.kind) {
    case ast::TypeKind::Int:
        return "WalkInt";
    case ast::TypeKind::Float:
        return "WalkFloat";
    case ast::TypeKind::Bool:
        return "WalkBool";
    case ast::TypeKind::String:
        return "WalkString";
    case ast::TypeKind::Struct:
        return c_struct_name(type.name);
    default:
        throw EmitError("internal error: unsupported array element type " + type.to_string());
    }
}

std::string c_value_type(const ast::Type& type) {
    switch (type.kind) {
    case ast::TypeKind::Int:
        return "WalkInt";
    case ast::TypeKind::Float:
        return "WalkFloat";
    case ast::TypeKind::Bool:
        return "WalkBool";
    case ast::TypeKind::String:
        return "WalkString";
    case ast::TypeKind::Struct:
        return c_struct_name(type.name);
    case ast::TypeKind::Map:
        if (sema::string_array_map(type)) {
            return "WalkMapStringArrayString";
        }
        break;
    case ast::TypeKind::Array:
        if (type.elem == nullptr) {
            throw EmitError("internal error: array type needs element");
        }
        switch (type.elem->kind) {
        case ast::TypeKind::Int:
            return "WalkArrayInt";
        case ast::TypeKind::Float:
            return "WalkArrayFloat";
        case ast::TypeKind::Bool:
            return "WalkArrayBool";
        case ast::TypeKind::String:
            return "WalkArrayString";
        case ast::TypeKind::Struct:
            return c_struct_array_name(type.elem->name);
        default:
            break;
        }
        break;
    default:
        break;
    }
    throw EmitError("internal error: unsupported C type " + type.to_string());
}

std::string c_return_type(const ast::Type& type) {
    if (type.kind == ast::TypeKind::Void) {
        return "void";
    }
    return c_value_type(type);
}

std::string c_decl(const ast::Type& type, const std::string& name, bool mutable_binding) {
    if (type.kind == ast::TypeKind::Function) {
        if (type.return_type == nullptr) {
            throw EmitError("internal error: function type needs return");
        }
        std::vector<std::string> params;
        params.reserve(type.params.size());
        for (const ast::Type& param : type.params) {
            params.push_back(c_value_type(param));
        }
        if (params.empty()) {
            params.push_back("void");
        }
        return c_return_type(*type.return_type) + " (*" + name + ")(" + join(params, ", ") + ")";
    }
    const std::string value_type = c_value_type(type);
    if (mutable_binding || type.nullable) {
        return value_type + " " + name;
    }
    return "const " + value_type + " " + name;
}

std::string native_array_helper_suffix(const ast::Type& type) {
    switch (type.kind) {
    case ast::TypeKind::Int:
        return "int";
    case ast::TypeKind::Float:
        return "float";
    case ast::TypeKind::Bool:
        return "bool";
    case ast::TypeKind::String:
        return "string";
    default:
        throw EmitError("internal error: unsupported native array helper type " + type.to_string());
    }
}

std::string builtin_runtime_name(const std::string& qualified) {
    return "walk_rt_" + replace_all(qualified, ".", "_");
}

void substitute_expression(ir::Expression& expression, const std::map<std::string, ast::Type>& bindings);
void substitute_statement(ir::Statement& statement, const std::map<std::string, ast::Type>& bindings);

void substitute_expressions(std::vector<std::unique_ptr<ir::Expression>>& expressions, const std::map<std::string, ast::Type>& bindings) {
    for (auto& expression : expressions) {
        substitute_expression(*expression, bindings);
    }
}

void substitute_statements(std::vector<std::unique_ptr<ir::Statement>>& statements, const std::map<std::string, ast::Type>& bindings) {
    for (auto& statement : statements) {
        substitute_statement(*statement, bindings);
    }
}

void substitute_expression(ir::Expression& expression, const std::map<std::string, ast::Type>& bindings) {
    expression.type = sema::substitute_type(expression.type, bindings);
    switch (expression.kind) {
    case ir::ExpressionKind::InterpolatedString: {
        auto& value = static_cast<ir::InterpolatedString&>(expression);
        for (ir::InterpolatedStringPart& part : value.parts) {
            if (part.expression != nullptr) {
                substitute_expression(*part.expression, bindings);
            }
        }
        break;
    }
    case ir::ExpressionKind::Prefix:
        substitute_expressions(static_cast<ir::Prefix&>(expression).args, bindings);
        break;
    case ir::ExpressionKind::Call: {
        auto& call = static_cast<ir::Call&>(expression);
        if (call.receiver != nullptr) {
            substitute_expression(*call.receiver, bindings);
        }
        substitute_expressions(call.args, bindings);
        for (ast::Type& type_arg : call.type_args) {
            type_arg = sema::substitute_type(type_arg, bindings);
        }
        break;
    }
    case ir::ExpressionKind::Input: {
        auto& input = static_cast<ir::Input&>(expression);
        if (input.prompt != nullptr) {
            substitute_expression(*input.prompt, bindings);
        }
        break;
    }
    case ir::ExpressionKind::ArrayLiteral:
        substitute_expressions(static_cast<ir::ArrayLiteral&>(expression).elements, bindings);
        break;
    case ir::ExpressionKind::Index: {
        auto& index = static_cast<ir::Index&>(expression);
        substitute_expression(*index.target, bindings);
        substitute_expression(*index.index, bindings);
        break;
    }
    case ir::ExpressionKind::FieldAccess:
        substitute_expression(*static_cast<ir::FieldAccess&>(expression).target, bindings);
        break;
    case ir::ExpressionKind::Literal:
    case ir::ExpressionKind::Name:
        break;
    }
}

void substitute_statement(ir::Statement& statement, const std::map<std::string, ast::Type>& bindings) {
    switch (statement.kind) {
    case ir::StatementKind::VarDecl: {
        auto& value = static_cast<ir::VarDecl&>(statement);
        value.annotation = sema::substitute_type(value.annotation, bindings);
        substitute_expression(*value.value, bindings);
        break;
    }
    case ir::StatementKind::Assignment: {
        auto& value = static_cast<ir::Assignment&>(statement);
        substitute_expression(*value.target, bindings);
        substitute_expression(*value.value, bindings);
        break;
    }
    case ir::StatementKind::Out:
        substitute_expression(*static_cast<ir::Out&>(statement).value, bindings);
        break;
    case ir::StatementKind::Do:
        substitute_expression(*static_cast<ir::Do&>(statement).value, bindings);
        break;
    case ir::StatementKind::Defer: {
        auto& value = static_cast<ir::Defer&>(statement);
        if (value.value != nullptr) {
            substitute_expression(*value.value, bindings);
        }
        break;
    }
    case ir::StatementKind::TestDecl:
        substitute_statements(static_cast<ir::TestDecl&>(statement).body, bindings);
        break;
    case ir::StatementKind::Assert:
        substitute_expression(*static_cast<ir::Assert&>(statement).value, bindings);
        break;
    case ir::StatementKind::FuncDecl: {
        auto& function = static_cast<ir::FuncDecl&>(statement);
        for (ir::Param& param : function.params) {
            param.type = sema::substitute_type(param.type, bindings);
        }
        function.return_type = sema::substitute_type(function.return_type, bindings);
        substitute_statements(function.body, bindings);
        break;
    }
    case ir::StatementKind::StructDecl: {
        for (ir::StructField& field : static_cast<ir::StructDecl&>(statement).fields) {
            field.type = sema::substitute_type(field.type, bindings);
        }
        break;
    }
    case ir::StatementKind::Return:
        substitute_expression(*static_cast<ir::Return&>(statement).value, bindings);
        break;
    case ir::StatementKind::If: {
        auto& branch = static_cast<ir::If&>(statement);
        substitute_expression(*branch.condition, bindings);
        substitute_statements(branch.then_block, bindings);
        substitute_statements(branch.else_block, bindings);
        break;
    }
    case ir::StatementKind::While: {
        auto& loop = static_cast<ir::While&>(statement);
        substitute_expression(*loop.condition, bindings);
        substitute_statements(loop.body, bindings);
        break;
    }
    case ir::StatementKind::Repeat: {
        auto& repeat = static_cast<ir::Repeat&>(statement);
        substitute_expression(*repeat.count, bindings);
        substitute_statements(repeat.body, bindings);
        break;
    }
    case ir::StatementKind::For: {
        auto& loop = static_cast<ir::For&>(statement);
        substitute_expression(*loop.iterable, bindings);
        substitute_statements(loop.body, bindings);
        break;
    }
    case ir::StatementKind::Import:
    case ir::StatementKind::Export:
    case ir::StatementKind::Break:
    case ir::StatementKind::Continue:
        break;
    }
}

std::unique_ptr<ir::FuncDecl> instantiate_generic(const GenericInstance& instance) {
    std::map<std::string, ast::Type> bindings;
    for (std::size_t index = 0; index < instance.function->type_params.size(); ++index) {
        bindings.emplace(instance.function->type_params[index], instance.type_args[index]);
    }
    std::unique_ptr<ir::FuncDecl> clone = ir::clone_function(*instance.function);
    clone->type_params.clear();
    clone->c_name = instance.c_name;
    substitute_statement(*clone, bindings);
    return clone;
}

class Emitter {
public:
    Emitter(const ir::LoweredProgram& lowered, bool tests_only) : lowered_(lowered), tests_only_(tests_only) {
        collect_builtin_structs();
        collect_program_structs_and_functions();
        struct_order_ = sorted_struct_names();
    }

    std::string emit() {
        std::ostringstream out;
        out << "/* Generated by WalkLang. C is the primary backend and is intended to stay inspectable. */\n\n";
        out << "#include \"walk_runtime.h\"\n";
        out << "/* Build generated C with runtime/walk_runtime.c and the host platform file. */\n\n";

        for (const std::string& name : struct_order_) {
            out << emit_struct_decl(*user_structs_.at(name));
            out << "typedef struct { " << c_struct_name(name) << " *items; WalkSize len; } " << c_struct_array_name(name) << ";\n\n";
        }

        for (const auto& item : lowered_.modules) {
            current_module_ = item.first;
            emit_function_prototypes(*item.second, out);
        }
        current_module_.clear();
        emit_function_prototypes(lowered_.program, out);

        std::vector<GenericInstance> generic_instances = collect_generic_instances();
        for (const GenericInstance& instance : generic_instances) {
            std::unique_ptr<ir::FuncDecl> clone = instantiate_generic(instance);
            out << emit_function_signature(*clone) << ";\n";
        }
        if (has_functions(lowered_.program) || has_module_functions() || !generic_instances.empty()) {
            out << "\n";
        }

        for (const auto& item : lowered_.modules) {
            current_module_ = item.first;
            emit_functions(*item.second, out);
        }
        current_module_.clear();
        emit_functions(lowered_.program, out);

        for (const GenericInstance& instance : generic_instances) {
            std::unique_ptr<ir::FuncDecl> clone = instantiate_generic(instance);
            const std::string previous_module = current_module_;
            current_module_ = instance.module;
            out << emit_function(*clone) << "\n";
            current_module_ = previous_module;
        }

        out << "int main(int argc, char **argv) {\n";
        indent_ = 1;
        out << "    walk_rt_init(argc, argv);\n";
        if (tests_only_) {
            out << "    long long walk_tests = 0;\n";
            out << "    long long walk_failures = 0;\n";
        }
        const int main_scope = push_defer_scope();
        for (const auto& statement : lowered_.program.statements) {
            if (dynamic_cast<const ir::FuncDecl*>(statement.get()) != nullptr || dynamic_cast<const ir::StructDecl*>(statement.get()) != nullptr ||
                dynamic_cast<const ir::Import*>(statement.get()) != nullptr || dynamic_cast<const ir::Export*>(statement.get()) != nullptr) {
                continue;
            }
            if (const auto* test = dynamic_cast<const ir::TestDecl*>(statement.get())) {
                if (!tests_only_) {
                    continue;
                }
                std::vector<std::string> lines = prepend_source_comment(test->range, emit_test_decl(*test));
                write_lines(out, lines);
                continue;
            }
            if (dynamic_cast<const ir::Assert*>(statement.get()) != nullptr && !tests_only_) {
                continue;
            }
            if (tests_only_ && dynamic_cast<const ir::TestDecl*>(statement.get()) == nullptr) {
                if (dynamic_cast<const ir::VarDecl*>(statement.get()) == nullptr && dynamic_cast<const ir::Assignment*>(statement.get()) == nullptr) {
                    continue;
                }
            }
            write_lines(out, emit_statement_with_comment(*statement));
        }
        write_lines(out, deferred_cleanup_lines(main_scope));
        pop_defer_scope();
        if (tests_only_) {
            out << "    if (walk_failures == 0) {\n";
            out << "        printf(\"ok %lld tests\\n\", walk_tests);\n";
            out << "    } else {\n";
            out << "        printf(\"failed %lld of %lld tests\\n\", walk_failures, walk_tests);\n";
            out << "    }\n";
            out << "    return walk_failures == 0 ? 0 : 1;\n";
        } else {
            out << "    return 0;\n";
        }
        out << "}\n";
        return out.str();
    }

private:
    void collect_builtin_structs() {
        for (const sema::BuiltinStruct& builtin : sema::builtin_structs()) {
            runtime_structs_.insert(builtin.name);
            std::vector<ir::StructField> fields;
            fields.reserve(builtin.fields.size());
            for (const ast::StructField& field : builtin.fields) {
                fields.push_back({field.range, field.name, field.type});
            }
            struct_fields_.emplace(builtin.name, std::move(fields));
        }
    }

    void collect_program_structs_and_functions() {
        for (const auto& item : lowered_.modules) {
            std::set<std::string> names;
            for (const auto& statement : item.second->statements) {
                if (const auto* decl = dynamic_cast<const ir::StructDecl*>(statement.get())) {
                    add_user_struct(*decl);
                }
                if (const auto* function = dynamic_cast<const ir::FuncDecl*>(statement.get())) {
                    if (function->receiver.empty() && function->type_params.empty()) {
                        names.insert(function->name);
                    }
                }
            }
            module_function_names_.emplace(item.first, std::move(names));
        }
        for (const auto& statement : lowered_.program.statements) {
            if (const auto* decl = dynamic_cast<const ir::StructDecl*>(statement.get())) {
                add_user_struct(*decl);
            }
        }
    }

    void add_user_struct(const ir::StructDecl& decl) {
        user_structs_[decl.name] = &decl;
        struct_fields_[decl.name] = decl.fields;
    }

    std::vector<std::string> sorted_struct_names() const {
        std::vector<std::string> names;
        names.reserve(user_structs_.size());
        std::set<std::string> seen;
        std::set<std::string> visiting;
        const auto visit = [&](const auto& self, const std::string& name) -> void {
            if (seen.count(name) != 0 || visiting.count(name) != 0) {
                return;
            }
            visiting.insert(name);
            const auto found = user_structs_.find(name);
            if (found != user_structs_.end()) {
                std::vector<std::string> deps;
                for (const ir::StructField& field : found->second->fields) {
                    std::vector<std::string> field_deps = struct_dependencies(field.type);
                    deps.insert(deps.end(), field_deps.begin(), field_deps.end());
                }
                std::sort(deps.begin(), deps.end());
                for (const std::string& dep : deps) {
                    if (dep != name && user_structs_.find(dep) != user_structs_.end()) {
                        self(self, dep);
                    }
                }
            }
            visiting.erase(name);
            seen.insert(name);
            names.push_back(name);
        };
        std::vector<std::string> all;
        all.reserve(user_structs_.size());
        for (const auto& item : user_structs_) {
            all.push_back(item.first);
        }
        std::sort(all.begin(), all.end());
        for (const std::string& name : all) {
            visit(visit, name);
        }
        return names;
    }

    bool has_functions(const ir::Program& program) const {
        for (const auto& statement : program.statements) {
            if (const auto* function = dynamic_cast<const ir::FuncDecl*>(statement.get()); function != nullptr && function->type_params.empty()) {
                return true;
            }
        }
        return false;
    }

    bool has_module_functions() const {
        for (const auto& item : lowered_.modules) {
            if (has_functions(*item.second)) {
                return true;
            }
        }
        return false;
    }

    void emit_function_prototypes(const ir::Program& program, std::ostringstream& out) {
        for (const auto& statement : program.statements) {
            const auto* function = dynamic_cast<const ir::FuncDecl*>(statement.get());
            if (function == nullptr || !function->type_params.empty()) {
                continue;
            }
            out << emit_function_signature(*function) << ";\n";
        }
    }

    void emit_functions(const ir::Program& program, std::ostringstream& out) {
        for (const auto& statement : program.statements) {
            const auto* function = dynamic_cast<const ir::FuncDecl*>(statement.get());
            if (function == nullptr || !function->type_params.empty()) {
                continue;
            }
            out << emit_function(*function) << "\n";
        }
    }

    std::string emit_struct_decl(const ir::StructDecl& decl) {
        std::ostringstream out;
        out << "typedef struct {\n";
        for (const ir::StructField& field : decl.fields) {
            out << "    " << c_value_type(field.type) << " " << field.name << ";\n";
        }
        out << "} " << c_struct_name(decl.name) << ";\n";
        return out.str();
    }

    std::string emit_function(const ir::FuncDecl& function) {
        std::ostringstream out;
        out << emit_function_signature(function) << " {\n";
        indent_ = 1;
        const std::vector<std::string> lines = emit_block(function.body);
        for (const std::string& line : lines) {
            out << indent_string() << line << "\n";
        }
        if (function.return_type.kind == ast::TypeKind::Void) {
            out << "    return;\n";
        }
        out << "}\n";
        return out.str();
    }

    std::string emit_function_signature(const ir::FuncDecl& function) {
        std::vector<std::string> params;
        params.reserve(function.params.size());
        for (const ir::Param& param : function.params) {
            params.push_back(c_decl(param.type, param.name, true));
        }
        if (params.empty()) {
            params.push_back("void");
        }
        return c_return_type(function.return_type) + " " + function_decl_name(function) + "(" + join(params, ", ") + ")";
    }

    std::string function_decl_name(const ir::FuncDecl& function) const {
        if (!function.c_name.empty()) {
            return function.c_name;
        }
        if (!function.receiver.empty()) {
            return method_symbol_name(function.receiver, function.name);
        }
        return function_name(function.name);
    }

    std::string function_name(const std::string& name) const {
        if (current_module_.empty()) {
            return name;
        }
        return module_symbol_name(current_module_, name);
    }

    std::vector<std::string> emit_block(const std::vector<std::unique_ptr<ir::Statement>>& statements) {
        return emit_block_with_options(statements, false);
    }

    std::vector<std::string> emit_loop_block(const std::vector<std::unique_ptr<ir::Statement>>& statements) {
        return emit_block_with_options(statements, true);
    }

    std::vector<std::string> emit_block_with_options(const std::vector<std::unique_ptr<ir::Statement>>& statements, bool loop_body) {
        const int scope_index = push_defer_scope();
        if (loop_body) {
            loop_cleanup_marks_.push_back(scope_index);
        }
        std::vector<std::string> out;
        for (const auto& statement : statements) {
            std::vector<std::string> lines = emit_statement_with_comment(*statement);
            out.insert(out.end(), lines.begin(), lines.end());
        }
        std::vector<std::string> cleanup = deferred_cleanup_lines(scope_index);
        out.insert(out.end(), cleanup.begin(), cleanup.end());
        if (loop_body) {
            loop_cleanup_marks_.pop_back();
        }
        pop_defer_scope();
        return out;
    }

    int push_defer_scope() {
        defer_scopes_.push_back({});
        return static_cast<int>(defer_scopes_.size()) - 1;
    }

    void pop_defer_scope() {
        defer_scopes_.pop_back();
    }

    void add_defer_cleanup(std::string line) {
        if (defer_scopes_.empty()) {
            return;
        }
        defer_scopes_.back().cleanups.push_back(std::move(line));
    }

    std::vector<std::string> deferred_cleanup_lines(int start) const {
        std::vector<std::string> lines;
        if (start < 0) {
            start = 0;
        }
        for (int index = static_cast<int>(defer_scopes_.size()) - 1; index >= start; --index) {
            const DeferScope& scope = defer_scopes_[static_cast<std::size_t>(index)];
            for (int cleanup = static_cast<int>(scope.cleanups.size()) - 1; cleanup >= 0; --cleanup) {
                lines.push_back(scope.cleanups[static_cast<std::size_t>(cleanup)]);
            }
        }
        return lines;
    }

    std::vector<std::string> emit_statement_with_comment(const ir::Statement& statement) {
        return prepend_source_comment(statement.range, emit_statement(statement));
    }

    std::vector<std::string> emit_statement(const ir::Statement& statement) {
        switch (statement.kind) {
        case ir::StatementKind::VarDecl:
            return emit_var_decl(static_cast<const ir::VarDecl&>(statement));
        case ir::StatementKind::Assignment: {
            const auto& value = static_cast<const ir::Assignment&>(statement);
            return {emit_expression(*value.target) + " = " + emit_expression(*value.value) + ";"};
        }
        case ir::StatementKind::Out:
            return {emit_out(*static_cast<const ir::Out&>(statement).value)};
        case ir::StatementKind::Do:
            return emit_do(static_cast<const ir::Do&>(statement));
        case ir::StatementKind::Defer:
            return emit_defer(static_cast<const ir::Defer&>(statement));
        case ir::StatementKind::Assert:
            return emit_assert(static_cast<const ir::Assert&>(statement));
        case ir::StatementKind::Return: {
            const auto& ret = static_cast<const ir::Return&>(statement);
            std::vector<std::string> lines = deferred_cleanup_lines(0);
            lines.push_back("return " + emit_expression(*ret.value) + ";");
            return lines;
        }
        case ir::StatementKind::If:
            return emit_if(static_cast<const ir::If&>(statement));
        case ir::StatementKind::While:
            return emit_while(static_cast<const ir::While&>(statement));
        case ir::StatementKind::Repeat:
            return emit_repeat(static_cast<const ir::Repeat&>(statement));
        case ir::StatementKind::For:
            return emit_for(static_cast<const ir::For&>(statement));
        case ir::StatementKind::Break: {
            std::vector<std::string> lines = deferred_cleanup_lines(current_loop_cleanup_mark());
            lines.push_back("break;");
            return lines;
        }
        case ir::StatementKind::Continue: {
            std::vector<std::string> lines = deferred_cleanup_lines(current_loop_cleanup_mark());
            lines.push_back("continue;");
            return lines;
        }
        default:
            throw EmitError("internal error: unknown statement");
        }
    }

    std::vector<std::string> emit_var_decl(const ir::VarDecl& statement) {
        ast::Type type = statement.value->type;
        if (statement.annotation.kind != ast::TypeKind::Invalid) {
            type = statement.annotation;
        }
        if (const auto* array = dynamic_cast<const ir::ArrayLiteral*>(statement.value.get())) {
            if (type.kind == ast::TypeKind::Map) {
                return emit_empty_map_decl(statement.name, type, *array, statement.mutable_binding);
            }
            return emit_array_decl(statement.name, type, *array, statement.mutable_binding);
        }
        return {c_decl(type, statement.name, statement.mutable_binding) + " = " + emit_expression(*statement.value) + ";"};
    }

    std::vector<std::string> emit_empty_map_decl(const std::string& name, const ast::Type& type, const ir::ArrayLiteral& array, bool mutable_binding) {
        if (!sema::string_array_map(type)) {
            throw EmitError("internal error: empty map literal has unsupported map type");
        }
        if (!array.elements.empty()) {
            throw EmitError("internal error: map literal supports only empty []");
        }
        return {c_decl(type, name, mutable_binding) + " = walk_rt_map_string_array_string_empty();"};
    }

    std::vector<std::string> emit_array_decl(const std::string& name, const ast::Type& type, const ir::ArrayLiteral& array, bool mutable_binding) {
        if (type.kind != ast::TypeKind::Array || type.elem == nullptr) {
            throw EmitError("internal error: array literal has non-array type");
        }
        const std::string item_name = next_temp(name + "_items");
        const std::string item_type = c_array_item_type(*type.elem);
        std::vector<std::string> values;
        values.reserve(array.elements.size());
        for (const auto& element : array.elements) {
            values.push_back(emit_expression(*element));
        }
        std::vector<std::string> lines;
        lines.push_back(item_type + " *" + item_name + " = (" + item_type + " *)walk_rt_alloc_array(" + std::to_string(values.size()) + ", sizeof(" + item_type + "));");
        for (std::size_t index = 0; index < values.size(); ++index) {
            lines.push_back(item_name + "[" + std::to_string(index) + "] = " + values[index] + ";");
        }
        lines.push_back(c_decl(type, name, mutable_binding) + " = {" + item_name + ", " + std::to_string(values.size()) + "};");
        return lines;
    }

    std::vector<std::string> emit_do(const ir::Do& statement) {
        const auto* call = dynamic_cast<const ir::Call*>(statement.value.get());
        if (call == nullptr) {
            throw EmitError("internal error: do has non-call value");
        }
        return {emit_call(*call) + ";"};
    }

    std::vector<std::string> emit_defer(const ir::Defer& statement) {
        const auto* call = dynamic_cast<const ir::Call*>(statement.value.get());
        if (call == nullptr) {
            throw EmitError("internal error: defer has non-call value");
        }
        const sema::BuiltinFunction* function = sema::lookup_qualified_builtin(call->callee);
        if (function == nullptr || !function->effect) {
            throw EmitError("internal error: defer has non-effect call");
        }
        std::vector<std::string> lines;
        std::vector<std::string> args;
        args.reserve(call->args.size());
        for (std::size_t index = 0; index < call->args.size(); ++index) {
            const ir::Expression& arg = *call->args[index];
            const std::string rendered = emit_expression(arg);
            const std::string temp = next_temp("__defer_" + replace_all(call->callee, ".", "_") + "_" + std::to_string(index + 1));
            lines.push_back(c_decl(arg.type, temp, true) + " = " + rendered + ";");
            args.push_back(temp);
        }
        add_defer_cleanup(builtin_runtime_name(function->qualified_name()) + "(" + join(args, ", ") + ");");
        return lines;
    }

    int current_loop_cleanup_mark() const {
        if (loop_cleanup_marks_.empty()) {
            return static_cast<int>(defer_scopes_.size());
        }
        return loop_cleanup_marks_.back();
    }

    std::vector<std::string> emit_test_decl(const ir::TestDecl& test) {
        std::vector<std::string> lines = {
            "walk_tests++;",
            "printf(\"test: " + escape_c_string(test.name) + "\\n\");",
            "{",
        };
        std::vector<std::string> body = emit_block(test.body);
        lines = append_indented(std::move(lines), body);
        lines.push_back("}");
        return lines;
    }

    std::vector<std::string> emit_assert(const ir::Assert& statement) {
        const std::string value = emit_expression(*statement.value);
        return {
            "if (!(" + value + ")) {",
            "    printf(\"FAIL " + escape_c_string(statement.range.path) + ":" + std::to_string(statement.range.start.line) + ":" + std::to_string(statement.range.start.column) + "\\n\");",
            "    walk_failures++;",
            "}",
        };
    }

    std::vector<std::string> emit_if(const ir::If& statement) {
        std::vector<std::string> lines = {"if (" + emit_expression(*statement.condition) + ") {"};
        std::vector<std::string> then_lines = emit_block(statement.then_block);
        lines = append_indented(std::move(lines), then_lines);
        if (statement.else_block.empty()) {
            lines.push_back("}");
            return lines;
        }
        lines.push_back("} else {");
        std::vector<std::string> else_lines = emit_block(statement.else_block);
        lines = append_indented(std::move(lines), else_lines);
        lines.push_back("}");
        return lines;
    }

    std::vector<std::string> emit_while(const ir::While& statement) {
        std::vector<std::string> lines = {"while (" + emit_expression(*statement.condition) + ") {"};
        std::vector<std::string> body = emit_loop_block(statement.body);
        lines = append_indented(std::move(lines), body);
        lines.push_back("}");
        return lines;
    }

    std::vector<std::string> emit_repeat(const ir::Repeat& statement) {
        const std::string count = emit_expression(*statement.count);
        const std::string i = next_temp("__repeat");
        std::vector<std::string> lines = {"for (long long " + i + " = 0; " + i + " < (" + count + "); " + i + "++) {"};
        std::vector<std::string> body = emit_loop_block(statement.body);
        lines = append_indented(std::move(lines), body);
        lines.push_back("}");
        return lines;
    }

    std::vector<std::string> emit_for(const ir::For& statement) {
        const std::string iterable = emit_expression(*statement.iterable);
        if (statement.iterable->type.elem == nullptr) {
            throw EmitError("internal error: for iterable has no element type");
        }
        const std::string item_type = c_array_item_type(*statement.iterable->type.elem);
        const std::string i = next_temp("__for");
        std::vector<std::string> lines = {"for (long long " + i + " = 0; " + i + " < " + iterable + ".len; " + i + "++) {"};
        lines.push_back("    " + item_type + " " + statement.name + " = " + iterable + ".items[" + i + "];");
        std::vector<std::string> body = emit_loop_block(statement.body);
        lines = append_indented(std::move(lines), body);
        lines.push_back("}");
        return lines;
    }

    std::string emit_out(const ir::Expression& expression) {
        const std::string value = emit_expression(expression);
        switch (expression.type.kind) {
        case ast::TypeKind::Int:
            return "walk_rt_print_int((WalkInt)(" + value + "));";
        case ast::TypeKind::Float:
            return "walk_rt_print_float((WalkFloat)(" + value + "));";
        case ast::TypeKind::Bool:
            return "walk_rt_print_bool((WalkBool)(" + value + "));";
        case ast::TypeKind::String:
            return "walk_rt_print_string((WalkString)(" + value + "));";
        default:
            throw EmitError("internal error: cannot print " + expression.type.to_string());
        }
    }

    std::string emit_expression(const ir::Expression& expression) {
        switch (expression.kind) {
        case ir::ExpressionKind::Literal:
            return emit_literal(static_cast<const ir::Literal&>(expression));
        case ir::ExpressionKind::InterpolatedString:
            return emit_interpolated_string(static_cast<const ir::InterpolatedString&>(expression));
        case ir::ExpressionKind::Name: {
            const auto& name = static_cast<const ir::Name&>(expression);
            if (!current_module_.empty()) {
                const auto found = module_function_names_.find(current_module_);
                if (found != module_function_names_.end() && found->second.count(name.identifier) != 0) {
                    return module_symbol_name(current_module_, name.identifier);
                }
            }
            return name.identifier;
        }
        case ir::ExpressionKind::Prefix:
            return emit_prefix(static_cast<const ir::Prefix&>(expression));
        case ir::ExpressionKind::Call:
            return emit_call(static_cast<const ir::Call&>(expression));
        case ir::ExpressionKind::Input: {
            const auto& input = static_cast<const ir::Input&>(expression);
            std::string prompt = "NULL";
            if (input.prompt != nullptr) {
                prompt = emit_expression(*input.prompt);
            }
            return "walk_rt_input_line(" + prompt + ")";
        }
        case ir::ExpressionKind::Index: {
            const auto& index = static_cast<const ir::Index&>(expression);
            const std::string target = emit_expression(*index.target);
            const std::string rendered_index = emit_expression(*index.index);
            if (index.target->type.kind == ast::TypeKind::String) {
                return "walk_rt_string_at(" + target + ", " + rendered_index + ")";
            }
            if (index.target->type.kind == ast::TypeKind::Map) {
                return "walk_rt_map_string_array_string_get(" + target + ", " + rendered_index + ")";
            }
            return target + ".items[" + rendered_index + "]";
        }
        case ir::ExpressionKind::FieldAccess: {
            const auto& field = static_cast<const ir::FieldAccess&>(expression);
            return "(" + emit_expression(*field.target) + ")." + field.field;
        }
        case ir::ExpressionKind::ArrayLiteral:
            throw EmitError("internal error: array literal cannot be emitted inline");
        }
        throw EmitError("internal error: unknown expression");
    }

    std::string emit_literal(const ir::Literal& literal) {
        switch (literal.literal_kind) {
        case ast::LiteralKind::Int:
        case ast::LiteralKind::Float:
            return literal.value;
        case ast::LiteralKind::Bool:
            return literal.value;
        case ast::LiteralKind::String:
            return "\"" + escape_c_string(literal.value) + "\"";
        case ast::LiteralKind::Null:
            return "NULL";
        }
        throw EmitError("internal error: unsupported literal");
    }

    std::string emit_interpolated_string(const ir::InterpolatedString& expression) {
        std::string value = "\"\"";
        for (const ir::InterpolatedStringPart& part : expression.parts) {
            std::string rendered;
            if (part.expression == nullptr) {
                rendered = "\"" + escape_c_string(part.literal) + "\"";
            } else {
                rendered = emit_interpolated_expression(*part.expression, emit_expression(*part.expression));
            }
            value = "walk_rt_string_concat(" + value + ", " + rendered + ")";
        }
        return value;
    }

    std::string emit_interpolated_expression(const ir::Expression& expression, const std::string& rendered) {
        switch (expression.type.kind) {
        case ast::TypeKind::Int:
            return "walk_rt_format_int((WalkInt)(" + rendered + "))";
        case ast::TypeKind::Float:
            return "walk_rt_format_float((WalkFloat)(" + rendered + "))";
        case ast::TypeKind::Bool:
            return "walk_rt_format_bool((WalkBool)(" + rendered + "))";
        case ast::TypeKind::String:
            return "walk_rt_format_string((WalkString)(" + rendered + "))";
        default:
            throw EmitError("internal error: cannot interpolate " + expression.type.to_string());
        }
    }

    std::string emit_prefix(const ir::Prefix& expression) {
        std::vector<std::string> args;
        args.reserve(expression.args.size());
        for (const auto& arg : expression.args) {
            args.push_back(emit_expression(*arg));
        }
        if (expression.op == "+" || expression.op == "*") {
            return "(" + join(args, " " + expression.op + " ") + ")";
        }
        if (expression.op == "and") {
            return "(" + join(args, " && ") + ")";
        }
        if (expression.op == "or") {
            return "(" + join(args, " || ") + ")";
        }
        if (expression.op == "not") {
            return "(!" + args[0] + ")";
        }
        if (expression.op == "-") {
            return "(" + args[0] + " - " + args[1] + ")";
        }
        if (expression.op == "/") {
            return "((double)(" + args[0] + ") / (" + args[1] + "))";
        }
        if (expression.op == ">" || expression.op == "<" || expression.op == ">=" || expression.op == "<=" || expression.op == "==" || expression.op == "!=") {
            if (expression.args[0]->type.kind == ast::TypeKind::String && expression.args[1]->type.kind == ast::TypeKind::String) {
                if (expression.op == "==" || expression.op == "!=") {
                    const std::string comparison = expression.op == "!=" ? "!=" : "==";
                    return "(strcmp(" + args[0] + ", " + args[1] + ") " + comparison + " 0)";
                }
            }
            return "(" + args[0] + " " + expression.op + " " + args[1] + ")";
        }
        if (expression.op == "^") {
            return "pow(" + args[0] + ", " + args[1] + ")";
        }
        throw EmitError("internal error: unsupported operator " + expression.op);
    }

    std::string emit_call(const ir::Call& call) {
        std::vector<std::string> args;
        args.reserve(call.args.size());
        for (const auto& arg : call.args) {
            args.push_back(emit_expression(*arg));
        }
        if (call.type.kind == ast::TypeKind::Struct && call.callee == call.type.name) {
            const auto found = struct_fields_.find(call.callee);
            if (found == struct_fields_.end()) {
                throw EmitError("internal error: unknown struct " + call.callee);
            }
            std::vector<std::string> fields;
            fields.reserve(found->second.size());
            for (std::size_t index = 0; index < found->second.size(); ++index) {
                fields.push_back("." + found->second[index].name + " = " + args[index]);
            }
            return "(" + c_struct_name(call.callee) + "){" + join(fields, ", ") + "}";
        }
        if (call.receiver != nullptr && !call.method.empty() && call.receiver->type.kind == ast::TypeKind::Struct) {
            std::vector<std::string> method_args;
            method_args.reserve(args.size() + 1);
            method_args.push_back(emit_expression(*call.receiver));
            method_args.insert(method_args.end(), args.begin(), args.end());
            return method_symbol_name(call.receiver->type.name, call.method) + "(" + join(method_args, ", ") + ")";
        }
        if (!call.type_args.empty()) {
            std::string callee = call.callee;
            if (!current_module_.empty() && callee.find('.') == std::string::npos) {
                callee = current_module_ + "." + callee;
            }
            return generic_instance_name(callee, call.type_args) + "(" + join(args, ", ") + ")";
        }
        if (const sema::BuiltinFunction* builtin = sema::lookup_qualified_builtin(call.callee)) {
            return builtin_runtime_name(builtin->qualified_name()) + "(" + join(args, ", ") + ")";
        }
        if (call.callee == "math.sqrt") {
            return "sqrt(" + join(args, ", ") + ")";
        }
        if (call.callee == "math.exp") {
            return "exp(" + join(args, ", ") + ")";
        }
        if (call.callee == "math.log") {
            return "log(" + join(args, ", ") + ")";
        }
        if (call.callee == "math.pow") {
            return "pow(" + join(args, ", ") + ")";
        }
        if (call.callee == "math.remainder") {
            return "walk_rt_math_remainder(" + join(args, ", ") + ")";
        }
        if (call.callee == "string.len") {
            return "walk_rt_string_len(" + args[0] + ")";
        }
        if (call.callee == "string.at") {
            return "walk_rt_string_at(" + args[0] + ", " + args[1] + ")";
        }
        if (call.callee == "string.slice") {
            return "walk_rt_string_slice(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
        }
        if (call.callee == "string.prefix") {
            return "walk_rt_string_prefix(" + args[0] + ", " + args[1] + ")";
        }
        if (call.callee == "string.contains") {
            return "walk_rt_string_contains(" + args[0] + ", " + args[1] + ")";
        }
        if (call.callee == "string.concat") {
            return "walk_rt_string_concat(" + args[0] + ", " + args[1] + ")";
        }
        if (call.callee == "string.lower") {
            return "walk_rt_string_lower(" + args[0] + ")";
        }
        if (call.callee == "string.split") {
            return "walk_rt_string_split(" + args[0] + ", " + args[1] + ")";
        }
        if (call.callee == "string.replace") {
            return "walk_rt_string_replace(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
        }
        if (call.callee == "array.len") {
            return args[0] + ".len";
        }
        if (call.callee == "array.contains") {
            if (call.args[0]->type.elem == nullptr) {
                throw EmitError("internal error: array.contains needs array element type");
            }
            return "walk_rt_array_contains_" + native_array_helper_suffix(*call.args[0]->type.elem) + "(" + args[0] + ", " + args[1] + ")";
        }
        if (call.callee == "array.push") {
            if (call.args[0]->type.elem == nullptr) {
                throw EmitError("internal error: array.push needs array element type");
            }
            return "walk_rt_array_push_" + native_array_helper_suffix(*call.args[0]->type.elem) + "(" + args[0] + ", " + args[1] + ")";
        }
        if (call.callee == "map.empty") {
            return "walk_rt_map_string_array_string_empty()";
        }
        if (call.callee == "map.has") {
            return "walk_rt_map_string_array_string_has(" + args[0] + ", " + args[1] + ")";
        }
        if (call.callee == "map.get") {
            return "walk_rt_map_string_array_string_get(" + args[0] + ", " + args[1] + ")";
        }
        if (call.callee == "map.set") {
            return "walk_rt_map_string_array_string_set(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
        }
        if (call.callee == "map.keys") {
            return "walk_rt_map_string_array_string_keys(" + args[0] + ")";
        }
        if (call.callee == "map.push") {
            return "walk_rt_map_string_array_string_push(" + args[0] + ", " + args[1] + ", " + args[2] + ")";
        }
        if (call.callee == "time.now") {
            return "(long long)time(NULL)";
        }
        if (call.callee == "random.int") {
            return "walk_rt_random_int(" + join(args, ", ") + ")";
        }
        if (call.callee == "random.float") {
            return "walk_rt_random_float(" + join(args, ", ") + ")";
        }
        if (call.callee == "random.choice") {
            if (call.args[0]->type.elem == nullptr) {
                throw EmitError("internal error: random.choice needs array element type");
            }
            return "walk_rt_array_choice_" + native_array_helper_suffix(*call.args[0]->type.elem) + "(" + args[0] + ")";
        }
        if (call.callee == "testing.assert") {
            return args[0];
        }
        if (call.callee.find('.') != std::string::npos) {
            return replace_all(call.callee, ".", "__") + "(" + join(args, ", ") + ")";
        }
        if (!current_module_.empty()) {
            const auto found = module_function_names_.find(current_module_);
            if (found != module_function_names_.end() && found->second.count(call.callee) != 0) {
                return module_symbol_name(current_module_, call.callee) + "(" + join(args, ", ") + ")";
            }
        }
        return call.callee + "(" + join(args, ", ") + ")";
    }

    std::map<std::string, GenericDecl> collect_generic_decls() const {
        std::map<std::string, GenericDecl> decls;
        for (const auto& item : lowered_.modules) {
            for (const auto& statement : item.second->statements) {
                const auto* function = dynamic_cast<const ir::FuncDecl*>(statement.get());
                if (function != nullptr && !function->type_params.empty()) {
                    decls.emplace(item.first + "." + function->name, GenericDecl{item.first, function});
                }
            }
        }
        for (const auto& statement : lowered_.program.statements) {
            const auto* function = dynamic_cast<const ir::FuncDecl*>(statement.get());
            if (function != nullptr && !function->type_params.empty()) {
                decls.emplace(function->name, GenericDecl{"", function});
            }
        }
        return decls;
    }

    void collect_generic_calls_from_expression(const ir::Expression& expression, std::vector<const ir::Call*>& calls) const {
        switch (expression.kind) {
        case ir::ExpressionKind::Prefix:
            for (const auto& arg : static_cast<const ir::Prefix&>(expression).args) {
                collect_generic_calls_from_expression(*arg, calls);
            }
            break;
        case ir::ExpressionKind::Call: {
            const auto& call = static_cast<const ir::Call&>(expression);
            if (!call.type_args.empty()) {
                calls.push_back(&call);
            }
            if (call.receiver != nullptr) {
                collect_generic_calls_from_expression(*call.receiver, calls);
            }
            for (const auto& arg : call.args) {
                collect_generic_calls_from_expression(*arg, calls);
            }
            break;
        }
        case ir::ExpressionKind::Input: {
            const auto& input = static_cast<const ir::Input&>(expression);
            if (input.prompt != nullptr) {
                collect_generic_calls_from_expression(*input.prompt, calls);
            }
            break;
        }
        case ir::ExpressionKind::ArrayLiteral:
            for (const auto& element : static_cast<const ir::ArrayLiteral&>(expression).elements) {
                collect_generic_calls_from_expression(*element, calls);
            }
            break;
        case ir::ExpressionKind::Index: {
            const auto& index = static_cast<const ir::Index&>(expression);
            collect_generic_calls_from_expression(*index.target, calls);
            collect_generic_calls_from_expression(*index.index, calls);
            break;
        }
        case ir::ExpressionKind::FieldAccess:
            collect_generic_calls_from_expression(*static_cast<const ir::FieldAccess&>(expression).target, calls);
            break;
        case ir::ExpressionKind::InterpolatedString:
            for (const ir::InterpolatedStringPart& part : static_cast<const ir::InterpolatedString&>(expression).parts) {
                if (part.expression != nullptr) {
                    collect_generic_calls_from_expression(*part.expression, calls);
                }
            }
            break;
        case ir::ExpressionKind::Literal:
        case ir::ExpressionKind::Name:
            break;
        }
    }

    void collect_generic_calls_from_statement(const ir::Statement& statement, bool skip_generic_functions, std::vector<const ir::Call*>& calls) const {
        switch (statement.kind) {
        case ir::StatementKind::FuncDecl: {
            const auto& function = static_cast<const ir::FuncDecl&>(statement);
            if (skip_generic_functions && !function.type_params.empty()) {
                return;
            }
            for (const auto& nested : function.body) {
                collect_generic_calls_from_statement(*nested, skip_generic_functions, calls);
            }
            break;
        }
        case ir::StatementKind::VarDecl:
            collect_generic_calls_from_expression(*static_cast<const ir::VarDecl&>(statement).value, calls);
            break;
        case ir::StatementKind::Assignment: {
            const auto& assignment = static_cast<const ir::Assignment&>(statement);
            collect_generic_calls_from_expression(*assignment.target, calls);
            collect_generic_calls_from_expression(*assignment.value, calls);
            break;
        }
        case ir::StatementKind::Out:
            collect_generic_calls_from_expression(*static_cast<const ir::Out&>(statement).value, calls);
            break;
        case ir::StatementKind::Do:
            collect_generic_calls_from_expression(*static_cast<const ir::Do&>(statement).value, calls);
            break;
        case ir::StatementKind::Defer: {
            const auto& value = static_cast<const ir::Defer&>(statement);
            if (value.value != nullptr) {
                collect_generic_calls_from_expression(*value.value, calls);
            }
            break;
        }
        case ir::StatementKind::TestDecl:
            for (const auto& nested : static_cast<const ir::TestDecl&>(statement).body) {
                collect_generic_calls_from_statement(*nested, skip_generic_functions, calls);
            }
            break;
        case ir::StatementKind::Assert:
            collect_generic_calls_from_expression(*static_cast<const ir::Assert&>(statement).value, calls);
            break;
        case ir::StatementKind::Return:
            collect_generic_calls_from_expression(*static_cast<const ir::Return&>(statement).value, calls);
            break;
        case ir::StatementKind::If: {
            const auto& branch = static_cast<const ir::If&>(statement);
            collect_generic_calls_from_expression(*branch.condition, calls);
            for (const auto& nested : branch.then_block) {
                collect_generic_calls_from_statement(*nested, skip_generic_functions, calls);
            }
            for (const auto& nested : branch.else_block) {
                collect_generic_calls_from_statement(*nested, skip_generic_functions, calls);
            }
            break;
        }
        case ir::StatementKind::While: {
            const auto& loop = static_cast<const ir::While&>(statement);
            collect_generic_calls_from_expression(*loop.condition, calls);
            for (const auto& nested : loop.body) {
                collect_generic_calls_from_statement(*nested, skip_generic_functions, calls);
            }
            break;
        }
        case ir::StatementKind::Repeat: {
            const auto& repeat = static_cast<const ir::Repeat&>(statement);
            collect_generic_calls_from_expression(*repeat.count, calls);
            for (const auto& nested : repeat.body) {
                collect_generic_calls_from_statement(*nested, skip_generic_functions, calls);
            }
            break;
        }
        case ir::StatementKind::For: {
            const auto& loop = static_cast<const ir::For&>(statement);
            collect_generic_calls_from_expression(*loop.iterable, calls);
            for (const auto& nested : loop.body) {
                collect_generic_calls_from_statement(*nested, skip_generic_functions, calls);
            }
            break;
        }
        case ir::StatementKind::Import:
        case ir::StatementKind::Export:
        case ir::StatementKind::StructDecl:
        case ir::StatementKind::Break:
        case ir::StatementKind::Continue:
            break;
        }
    }

    std::vector<const ir::Call*> collect_generic_calls(const std::vector<std::unique_ptr<ir::Statement>>& statements, bool skip_generic_functions) const {
        std::vector<const ir::Call*> calls;
        for (const auto& statement : statements) {
            collect_generic_calls_from_statement(*statement, skip_generic_functions, calls);
        }
        return calls;
    }

    std::vector<GenericInstance> collect_generic_instances() const {
        const std::map<std::string, GenericDecl> decls = collect_generic_decls();
        std::map<std::string, GenericInstance> instances;
        std::vector<GenericInstance> queue;
        const auto add_call = [&](const ir::Call& call, const std::string& module) {
            if (call.type_args.empty() || contains_generic_type(call.type_args)) {
                return;
            }
            std::string callee = call.callee;
            if (!module.empty() && callee.find('.') == std::string::npos && decls.find(module + "." + callee) != decls.end()) {
                callee = module + "." + callee;
            }
            const auto decl_found = decls.find(callee);
            if (decl_found == decls.end()) {
                throw EmitError("internal error: generic function " + callee + " is not defined");
            }
            const std::string key = generic_instance_key(callee, call.type_args);
            if (instances.find(key) != instances.end()) {
                return;
            }
            GenericInstance instance{callee, decl_found->second.module, decl_found->second.function, call.type_args, generic_instance_name(callee, call.type_args)};
            instances.emplace(key, instance);
            queue.push_back(std::move(instance));
        };

        for (const auto& item : lowered_.modules) {
            for (const ir::Call* call : collect_generic_calls(item.second->statements, true)) {
                add_call(*call, item.first);
            }
        }
        for (const ir::Call* call : collect_generic_calls(lowered_.program.statements, true)) {
            add_call(*call, "");
        }
        for (std::size_t index = 0; index < queue.size(); ++index) {
            std::unique_ptr<ir::FuncDecl> clone = instantiate_generic(queue[index]);
            for (const ir::Call* call : collect_generic_calls(clone->body, false)) {
                add_call(*call, queue[index].module);
            }
        }

        std::vector<GenericInstance> result;
        result.reserve(instances.size());
        for (const auto& item : instances) {
            result.push_back(item.second);
        }
        std::sort(result.begin(), result.end(), [](const GenericInstance& left, const GenericInstance& right) {
            return left.c_name < right.c_name;
        });
        return result;
    }

    std::string next_temp(const std::string& prefix) {
        ++temp_id_;
        return "__" + replace_all(prefix, ".", "_") + "_" + std::to_string(temp_id_);
    }

    std::string indent_string() const {
        return std::string(static_cast<std::size_t>(indent_) * 4, ' ');
    }

    void write_lines(std::ostringstream& out, const std::vector<std::string>& lines) const {
        for (const std::string& line : lines) {
            out << indent_string() << line << "\n";
        }
    }

    const ir::LoweredProgram& lowered_;
    bool tests_only_ = false;
    int temp_id_ = 0;
    int indent_ = 0;
    std::string current_module_;
    std::map<std::string, std::set<std::string>> module_function_names_;
    std::map<std::string, const ir::StructDecl*> user_structs_;
    std::map<std::string, std::vector<ir::StructField>> struct_fields_;
    std::set<std::string> runtime_structs_;
    std::vector<std::string> struct_order_;
    std::vector<DeferScope> defer_scopes_;
    std::vector<int> loop_cleanup_marks_;
};

}  // namespace

Result<std::string> emit_c(const ir::LoweredProgram& lowered, EmitOptions options) {
    try {
        Emitter emitter(lowered, options.tests_only);
        return Result<std::string>::success(emitter.emit());
    } catch (const EmitError& error) {
        return Result<std::string>::failure(error.what());
    }
}

}  // namespace walk::codegen::c
