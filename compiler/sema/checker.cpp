#include "sema/checker.h"

#include "sema/builtins.h"
#include "sema/scope.h"
#include "sema/types.h"

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace walk::sema {
namespace {

constexpr const char* kSemanticDiagnostic = "W4001";

void add_error(DiagnosticSet& diagnostics, const SourceRange& range, std::string message) {
    diagnostics.add(Diagnostic(DiagnosticSeverity::Error, kSemanticDiagnostic, std::move(message), range));
}

std::string method_name(const std::string& receiver, const std::string& name) {
    return receiver + "." + name;
}

std::string func_name(const ast::FuncDecl& function) {
    if (!function.receiver.empty()) {
        return method_name(function.receiver, function.name);
    }
    return function.name;
}

ast::Type function_type(const ast::FuncDecl& function) {
    std::vector<ast::Type> params;
    params.reserve(function.params.size());
    for (const ast::Param& param : function.params) {
        params.push_back(param.type);
    }
    ast::Type ret = function.return_type;
    if (!known_type(ret)) {
        ret = ast::basic(ast::TypeKind::Void);
    }
    return ast::function_type(std::move(params), std::move(ret));
}

std::optional<std::pair<std::string, std::string>> split_qualified_call(const std::string& callee) {
    const std::size_t index = callee.rfind('.');
    if (index == std::string::npos || index == 0 || index == callee.size() - 1) {
        return std::nullopt;
    }
    return std::pair<std::string, std::string>{callee.substr(0, index), callee.substr(index + 1)};
}

std::set<std::string> type_param_set(const std::vector<std::string>& params) {
    return std::set<std::string>(params.begin(), params.end());
}

bool reserved_type_name(const std::string& name) {
    return name == "void" || name == "int" || name == "float" || name == "bool" || name == "string" || name == "null" || name == "array" || name == "map" ||
        name == "func";
}

bool reserved_value_name(const std::string& name) {
    static const std::set<std::string> reserved = {
        "var", "const", "out", "if", "else", "while", "for", "repeat", "break", "continue", "func", "return",
        "imp", "exp", "true", "false", "null", "and", "or", "not", "in", "test", "assert", "struct",
    };
    return reserved.count(name) != 0;
}

std::vector<std::string> struct_type_dependencies(const ast::Type& type) {
    switch (type.kind) {
    case ast::TypeKind::Struct:
        return {type.name};
    case ast::TypeKind::Array:
        if (type.elem == nullptr) {
            return {};
        }
        return struct_type_dependencies(*type.elem);
    case ast::TypeKind::Map: {
        std::vector<std::string> deps;
        if (type.key != nullptr) {
            std::vector<std::string> key_deps = struct_type_dependencies(*type.key);
            deps.insert(deps.end(), key_deps.begin(), key_deps.end());
        }
        if (type.elem != nullptr) {
            std::vector<std::string> value_deps = struct_type_dependencies(*type.elem);
            deps.insert(deps.end(), value_deps.begin(), value_deps.end());
        }
        return deps;
    }
    case ast::TypeKind::Function: {
        std::vector<std::string> deps;
        for (const ast::Type& param : type.params) {
            std::vector<std::string> param_deps = struct_type_dependencies(param);
            deps.insert(deps.end(), param_deps.begin(), param_deps.end());
        }
        if (type.return_type != nullptr) {
            std::vector<std::string> ret_deps = struct_type_dependencies(*type.return_type);
            deps.insert(deps.end(), ret_deps.begin(), ret_deps.end());
        }
        return deps;
    }
    default:
        return {};
    }
}

bool root_name(ast::Expression* expression, std::string& out) {
    if (auto* name = dynamic_cast<ast::Name*>(expression)) {
        out = name->identifier;
        return true;
    }
    if (auto* index = dynamic_cast<ast::Index*>(expression)) {
        return root_name(index->target, out);
    }
    if (auto* field = dynamic_cast<ast::FieldAccess*>(expression)) {
        return root_name(field->target, out);
    }
    return false;
}

bool calls_qualified_effect(ast::Expression* expression, const std::string& qualified) {
    const auto* call = dynamic_cast<ast::Call*>(expression);
    return call != nullptr && call->callee == qualified;
}

bool block_returns(const std::vector<ast::Statement*>& statements);
bool block_terminates(const std::vector<ast::Statement*>& statements);

bool statement_terminates(ast::Statement* statement) {
    if (dynamic_cast<ast::Return*>(statement) != nullptr || dynamic_cast<ast::Break*>(statement) != nullptr || dynamic_cast<ast::Continue*>(statement) != nullptr) {
        return true;
    }
    if (auto* branch = dynamic_cast<ast::If*>(statement)) {
        return !branch->else_block.empty() && block_terminates(branch->then_block) && block_terminates(branch->else_block);
    }
    if (auto* effect = dynamic_cast<ast::Do*>(statement)) {
        return calls_qualified_effect(effect->value, "process.exit");
    }
    return false;
}

bool block_terminates(const std::vector<ast::Statement*>& statements) {
    for (ast::Statement* statement : statements) {
        if (statement_terminates(statement)) {
            return true;
        }
    }
    return false;
}

bool block_returns(const std::vector<ast::Statement*>& statements) {
    for (ast::Statement* statement : statements) {
        if (dynamic_cast<ast::Return*>(statement) != nullptr) {
            return true;
        }
        if (auto* effect = dynamic_cast<ast::Do*>(statement)) {
            if (calls_qualified_effect(effect->value, "process.exit")) {
                return true;
            }
        }
        if (auto* branch = dynamic_cast<ast::If*>(statement)) {
            if (!branch->else_block.empty() && block_returns(branch->then_block) && block_returns(branch->else_block)) {
                return true;
            }
        }
    }
    return false;
}

bool is_effect_builtin_call(ast::Expression* expression) {
    const auto* call = dynamic_cast<ast::Call*>(expression);
    if (call == nullptr) {
        return false;
    }
    const BuiltinFunction* builtin = lookup_qualified_builtin(call->callee);
    return builtin != nullptr && builtin->effect;
}

class Checker {
public:
    Checker(
        std::map<std::string, std::unique_ptr<Module>>& modules,
        std::map<std::string, StructDef> structs,
        std::map<std::string, std::map<std::string, MethodDef>> methods)
        : modules_(modules), structs_(std::move(structs)), methods_(std::move(methods)) {}

    CheckResult check(ast::Program& program) {
        CheckResult result;
        diagnostics_ = &result.diagnostics;
        warnings_ = &result.warnings;
        if (!merge_builtin_structs() || !check_structs() || !check_methods() || !register_functions(program)) {
            return result;
        }
        for (ast::Statement* statement : program.statements) {
            if (!check_statement(statement)) {
                return result;
            }
        }
        return result;
    }

private:
    bool merge_builtin_structs() {
        for (const BuiltinStruct& builtin : builtin_structs()) {
            const auto existing = structs_.find(builtin.name);
            if (existing != structs_.end()) {
                add_error(*diagnostics_, existing->second.range, "type error: struct " + builtin.name + " is reserved by draft built-in APIs");
                return false;
            }
            StructDef def;
            def.name = builtin.name;
            def.fields = builtin.fields;
            for (const ast::StructField& field : def.fields) {
                def.field_map.emplace(field.name, field);
            }
            structs_.emplace(def.name, std::move(def));
        }
        return true;
    }

    bool check_structs() {
        for (const auto& item : structs_) {
            for (const ast::StructField& field : item.second.fields) {
                if (!validate_type(field.type, field.range)) {
                    return false;
                }
            }
        }
        std::set<std::string> visiting;
        std::set<std::string> visited;
        for (const auto& item : structs_) {
            if (!visit_struct(item.first, visiting, visited)) {
                return false;
            }
        }
        return true;
    }

    bool visit_struct(const std::string& name, std::set<std::string>& visiting, std::set<std::string>& visited) {
        if (visited.count(name) != 0) {
            return true;
        }
        if (visiting.count(name) != 0) {
            add_error(*diagnostics_, structs_[name].range, "type error: struct " + name + " cannot contain itself");
            return false;
        }
        visiting.insert(name);
        for (const ast::StructField& field : structs_[name].fields) {
            for (const std::string& dep : struct_type_dependencies(field.type)) {
                if (structs_.find(dep) != structs_.end() && !visit_struct(dep, visiting, visited)) {
                    return false;
                }
            }
        }
        visiting.erase(name);
        visited.insert(name);
        return true;
    }

    bool check_methods() {
        for (const auto& item : methods_) {
            const std::string& receiver = item.first;
            if (structs_.find(receiver) == structs_.end()) {
                for (const auto& method : item.second) {
                    add_error(*diagnostics_, method.second.range, "type error: method receiver " + receiver + " is not a struct");
                    return false;
                }
            }
            for (const auto& method : item.second) {
                ast::FuncDecl* function = method.second.function;
                if (function == nullptr) {
                    continue;
                }
                if (!function->type_params.empty()) {
                    add_error(*diagnostics_, function->range, "type error: generic methods are not supported yet");
                    return false;
                }
                if (function->params.empty()) {
                    add_error(*diagnostics_, function->range, "type error: method " + method_name(function->receiver, function->name) + " needs receiver parameter");
                    return false;
                }
                const ast::Type want_receiver = ast::struct_type(function->receiver);
                if (!type_equal(function->params[0].type, want_receiver)) {
                    add_error(
                        *diagnostics_,
                        function->range,
                        "type error: method " + method_name(function->receiver, function->name) + " receiver param must be " + want_receiver.to_string() + ", got " +
                            function->params[0].type.to_string());
                    return false;
                }
                if (!validate_type(function->return_type, function->range)) {
                    return false;
                }
                for (const ast::Param& param : function->params) {
                    if (!validate_type(param.type, function->range)) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool register_functions(ast::Program& program) {
        for (ast::Statement* statement : program.statements) {
            auto* function = dynamic_cast<ast::FuncDecl*>(statement);
            if (function == nullptr || !function->receiver.empty()) {
                continue;
            }
            if (!function->type_params.empty()) {
                if (!validate_type_params(*function)) {
                    return false;
                }
                if (scope_.current_has(function->name)) {
                    add_error(*diagnostics_, function->range, "name error: " + function->name + " is already defined");
                    return false;
                }
                if (structs_.find(function->name) != structs_.end()) {
                    add_error(*diagnostics_, function->range, "name error: " + function->name + " is already defined as struct");
                    return false;
                }
                if (generic_funcs_.find(function->name) != generic_funcs_.end()) {
                    add_error(*diagnostics_, function->range, "name error: " + function->name + " is already defined");
                    return false;
                }
                generic_funcs_.emplace(function->name, function);
                continue;
            }
            if (!define(function->name, function_type(*function), false, function->range)) {
                return false;
            }
        }
        return true;
    }

    bool check_statement(ast::Statement* statement) {
        if (auto* import = dynamic_cast<ast::Import*>(statement)) {
            if (!is_builtin_module(import->module) && modules_.find(import->module) == modules_.end()) {
                add_error(*diagnostics_, import->range, "module error: module " + import->module + " is not available");
                return false;
            }
            imports_.insert(import->module);
            return true;
        }
        if (auto* export_statement = dynamic_cast<ast::Export*>(statement)) {
            if (!scope_.resolve(export_statement->name) && generic_funcs_.find(export_statement->name) == generic_funcs_.end()) {
                add_error(*diagnostics_, export_statement->range, "name error: " + export_statement->name + " is not defined");
                return false;
            }
            return true;
        }
        if (auto* function = dynamic_cast<ast::FuncDecl*>(statement)) {
            if (!function->type_params.empty()) {
                return check_generic_func_decl(*function);
            }
            return check_func_decl(*function);
        }
        if (auto* struct_decl = dynamic_cast<ast::StructDecl*>(statement)) {
            if (block_depth_ > 0) {
                add_error(*diagnostics_, struct_decl->range, "syntax error: struct declarations must be top level");
                return false;
            }
            return true;
        }
        if (auto* var = dynamic_cast<ast::VarDecl*>(statement)) {
            return check_var_decl(*var);
        }
        if (auto* assignment = dynamic_cast<ast::Assignment*>(statement)) {
            return check_assignment(*assignment);
        }
        if (auto* out = dynamic_cast<ast::Out*>(statement)) {
            ast::Type value_type;
            if (!check_expression(out->value, value_type)) {
                return false;
            }
            if (value_type.kind == ast::TypeKind::Array || value_type.kind == ast::TypeKind::Function || value_type.kind == ast::TypeKind::Void ||
                value_type.kind == ast::TypeKind::Struct) {
                add_error(*diagnostics_, out->range, "type error: cannot output " + value_type.to_string());
                return false;
            }
            return true;
        }
        if (auto* effect = dynamic_cast<ast::Do*>(statement)) {
            return check_do(*effect);
        }
        if (auto* defer = dynamic_cast<ast::Defer*>(statement)) {
            return check_defer(*defer);
        }
        if (auto* test = dynamic_cast<ast::TestDecl*>(statement)) {
            return check_nested_block(test->body);
        }
        if (auto* assertion = dynamic_cast<ast::Assert*>(statement)) {
            ast::Type value_type;
            if (!check_expression(assertion->value, value_type)) {
                return false;
            }
            if (!type_equal(value_type, ast::basic(ast::TypeKind::Bool))) {
                add_error(*diagnostics_, assertion->value->range, "type error: assert needs bool, got " + value_type.to_string());
                return false;
            }
            return true;
        }
        if (auto* ret = dynamic_cast<ast::Return*>(statement)) {
            if (!in_function_) {
                add_error(*diagnostics_, ret->range, "syntax error: return outside function");
                return false;
            }
            ast::Type value_type;
            if (!check_expression(ret->value, value_type)) {
                return false;
            }
            if (!assignable(value_type, current_return_)) {
                add_error(*diagnostics_, ret->range, "type error: function returns " + current_return_.to_string() + ", got " + value_type.to_string());
                return false;
            }
            return true;
        }
        if (auto* branch = dynamic_cast<ast::If*>(statement)) {
            ast::Type cond_type;
            if (!check_expression(branch->condition, cond_type)) {
                return false;
            }
            if (!type_equal(cond_type, ast::basic(ast::TypeKind::Bool))) {
                add_error(*diagnostics_, branch->condition->range, "type error: if condition must be bool, got " + cond_type.to_string());
                return false;
            }
            return check_nested_block(branch->then_block) && check_nested_block(branch->else_block);
        }
        if (auto* loop = dynamic_cast<ast::While*>(statement)) {
            ast::Type cond_type;
            if (!check_expression(loop->condition, cond_type)) {
                return false;
            }
            if (!type_equal(cond_type, ast::basic(ast::TypeKind::Bool))) {
                add_error(*diagnostics_, loop->condition->range, "type error: while condition must be bool, got " + cond_type.to_string());
                return false;
            }
            ++loop_depth_;
            const bool ok = check_nested_block(loop->body);
            --loop_depth_;
            return ok;
        }
        if (auto* repeat = dynamic_cast<ast::Repeat*>(statement)) {
            ast::Type count_type;
            if (!check_expression(repeat->count, count_type)) {
                return false;
            }
            if (!type_equal(count_type, ast::basic(ast::TypeKind::Int))) {
                add_error(*diagnostics_, repeat->count->range, "type error: repeat count must be int, got " + count_type.to_string());
                return false;
            }
            ++loop_depth_;
            const bool ok = check_nested_block(repeat->body);
            --loop_depth_;
            return ok;
        }
        if (auto* loop = dynamic_cast<ast::For*>(statement)) {
            ast::Type iter_type;
            if (!check_expression(loop->iterable, iter_type)) {
                return false;
            }
            if (iter_type.kind != ast::TypeKind::Array || iter_type.elem == nullptr) {
                add_error(*diagnostics_, loop->iterable->range, "type error: for needs array, got " + iter_type.to_string());
                return false;
            }
            scope_.push();
            if (!define(loop->name, *iter_type.elem, true, loop->range)) {
                scope_.pop();
                return false;
            }
            ++loop_depth_;
            const bool ok = check_block(loop->body);
            --loop_depth_;
            scope_.pop();
            return ok;
        }
        if (auto* br = dynamic_cast<ast::Break*>(statement)) {
            if (loop_depth_ == 0) {
                add_error(*diagnostics_, br->range, "syntax error: break outside loop");
                return false;
            }
            return true;
        }
        if (auto* cont = dynamic_cast<ast::Continue*>(statement)) {
            if (loop_depth_ == 0) {
                add_error(*diagnostics_, cont->range, "syntax error: continue outside loop");
                return false;
            }
            return true;
        }
        add_error(*diagnostics_, statement->range, "internal error: unknown statement");
        return false;
    }

    bool check_do(ast::Do& statement) {
        const bool previous = effect_context_;
        effect_context_ = true;
        ast::Type value_type;
        const bool ok = check_expression(statement.value, value_type);
        effect_context_ = previous;
        if (!ok) {
            return false;
        }
        if (!is_effect_builtin_call(statement.value)) {
            add_error(*diagnostics_, statement.range, "type error: do needs effect call, got " + value_type.to_string());
            return false;
        }
        return true;
    }

    bool check_defer(ast::Defer& statement) {
        if (statement.value == nullptr) {
            add_error(*diagnostics_, statement.range, "type error: defer requires do: effect call");
            return false;
        }
        const bool previous = effect_context_;
        effect_context_ = true;
        ast::Type value_type;
        const bool ok = check_expression(statement.value, value_type);
        effect_context_ = previous;
        if (!ok) {
            return false;
        }
        if (!is_effect_builtin_call(statement.value) || value_type.kind != ast::TypeKind::Void) {
            add_error(*diagnostics_, statement.range, "type error: defer requires do: effect call");
            return false;
        }
        return true;
    }

    bool check_func_decl(ast::FuncDecl& function) {
        if (!known_type(function.return_type)) {
            function.return_type = ast::basic(ast::TypeKind::Void);
        }
        if (!validate_type(function.return_type, function.range)) {
            return false;
        }
        scope_.push();
        const ast::Type previous_return = current_return_;
        const bool previous_in_function = in_function_;
        const int previous_block_depth = block_depth_;
        current_return_ = function.return_type;
        in_function_ = true;
        ++block_depth_;
        for (const ast::Param& param : function.params) {
            if (!validate_type(param.type, function.range)) {
                scope_.pop();
                return false;
            }
            if (!define(param.name, param.type, true, function.range)) {
                scope_.pop();
                return false;
            }
        }
        const bool ok = check_block(function.body);
        current_return_ = previous_return;
        in_function_ = previous_in_function;
        block_depth_ = previous_block_depth;
        scope_.pop();
        if (!ok) {
            return false;
        }
        if (function.return_type.kind != ast::TypeKind::Void && !block_returns(function.body)) {
            add_error(*diagnostics_, function.range, "type error: function " + func_name(function) + " may not return on all paths");
            return false;
        }
        return true;
    }

    bool check_generic_func_decl(ast::FuncDecl& function) {
        if (!function.receiver.empty()) {
            add_error(*diagnostics_, function.range, "type error: generic methods are not supported yet");
            return false;
        }
        if (!validate_type_params(function)) {
            return false;
        }
        const std::set<std::string> previous = type_params_;
        type_params_ = type_param_set(function.type_params);
        const bool ok = check_func_decl(function);
        type_params_ = previous;
        return ok;
    }

    bool validate_type_params(const ast::FuncDecl& function) {
        std::set<std::string> seen;
        for (const std::string& param : function.type_params) {
            if (reserved_type_name(param) || reserved_value_name(param)) {
                add_error(*diagnostics_, function.range, "type error: " + param + " cannot be used as a type parameter");
                return false;
            }
            if (structs_.find(param) != structs_.end()) {
                add_error(*diagnostics_, function.range, "type error: " + param + " is already a struct");
                return false;
            }
            if (seen.count(param) != 0) {
                add_error(*diagnostics_, function.range, "type error: duplicate type parameter " + param);
                return false;
            }
            seen.insert(param);
        }
        return true;
    }

    bool check_var_decl(ast::VarDecl& statement) {
        if (scope_.current_has(statement.name)) {
            add_error(*diagnostics_, statement.range, "name error: " + statement.name + " is already defined");
            return false;
        }
        if (known_type(statement.annotation) && !validate_type(statement.annotation, statement.range)) {
            return false;
        }
        ast::Type value_type;
        if (auto* array = dynamic_cast<ast::ArrayLiteral*>(statement.value); array != nullptr && known_type(statement.annotation)) {
            if (statement.annotation.kind == ast::TypeKind::Array) {
                if (!check_array_literal_as(*array, statement.annotation, value_type)) {
                    return false;
                }
            } else if (statement.annotation.kind == ast::TypeKind::Map) {
                if (!check_empty_map_literal_as(*array, statement.annotation, value_type)) {
                    return false;
                }
            } else if (!check_expression(statement.value, value_type)) {
                return false;
            }
        } else if (!check_expression(statement.value, value_type)) {
            return false;
        }
        ast::Type declared_type = known_type(statement.annotation) ? statement.annotation : value_type;
        if (!assignable(value_type, declared_type)) {
            add_error(*diagnostics_, statement.value->range, "type error: " + statement.name + " is " + declared_type.to_string() + ", got " + value_type.to_string());
            return false;
        }
        statement.value->type = declared_type;
        return define(statement.name, declared_type, statement.mutable_binding, statement.range);
    }

    bool check_assignment(ast::Assignment& statement) {
        ast::Type value_type;
        if (!check_expression(statement.value, value_type)) {
            return false;
        }
        ast::Type target_type;
        if (!check_assignable_target(statement.target, target_type)) {
            return false;
        }
        if (!assignable(value_type, target_type)) {
            if (auto* name = dynamic_cast<ast::Name*>(statement.target)) {
                add_error(*diagnostics_, statement.range, "type error: " + name->identifier + " is " + target_type.to_string() + ", got " + value_type.to_string());
                return false;
            }
            if (auto* field = dynamic_cast<ast::FieldAccess*>(statement.target)) {
                add_error(*diagnostics_, statement.range, "type error: field " + field->field + " is " + target_type.to_string() + ", got " + value_type.to_string());
                return false;
            }
            add_error(*diagnostics_, statement.range, "type error: target is " + target_type.to_string() + ", got " + value_type.to_string());
            return false;
        }
        return true;
    }

    bool check_assignable_target(ast::Expression* target, ast::Type& out) {
        if (auto* name = dynamic_cast<ast::Name*>(target)) {
            const std::optional<Symbol> symbol = scope_.resolve(name->identifier);
            if (!symbol) {
                add_error(*diagnostics_, name->range, "name error: " + name->identifier + " is not defined");
                return false;
            }
            if (!symbol->mutable_binding) {
                add_error(*diagnostics_, name->range, "type error: " + name->identifier + " is const and cannot be reassigned");
                return false;
            }
            name->type = symbol->type;
            out = symbol->type;
            return true;
        }
        if (auto* index = dynamic_cast<ast::Index*>(target)) {
            std::string base;
            if (root_name(index->target, base)) {
                const std::optional<Symbol> symbol = scope_.resolve(base);
                if (symbol && !symbol->mutable_binding) {
                    add_error(*diagnostics_, index->range, "type error: " + base + " is const and cannot be reassigned");
                    return false;
                }
            }
            ast::Type target_type;
            ast::Type index_type;
            if (!check_expression(index->target, target_type) || !check_expression(index->index, index_type)) {
                return false;
            }
            if (target_type.kind != ast::TypeKind::Array || target_type.elem == nullptr) {
                add_error(*diagnostics_, index->range, "type error: index target must be array, got " + target_type.to_string());
                return false;
            }
            if (!type_equal(index_type, ast::basic(ast::TypeKind::Int))) {
                add_error(*diagnostics_, index->index->range, "type error: array index must be int, got " + index_type.to_string());
                return false;
            }
            index->type = *target_type.elem;
            out = *target_type.elem;
            return true;
        }
        if (auto* field = dynamic_cast<ast::FieldAccess*>(target)) {
            std::string base;
            if (root_name(field->target, base)) {
                const std::optional<Symbol> symbol = scope_.resolve(base);
                if (symbol && !symbol->mutable_binding) {
                    add_error(*diagnostics_, field->range, "type error: " + base + " is const and cannot be reassigned");
                    return false;
                }
            }
            ast::Type target_type;
            if (!check_expression(field->target, target_type)) {
                return false;
            }
            ast::StructField found;
            if (!lookup_field(target_type, field->field, field->range, found)) {
                return false;
            }
            field->type = found.type;
            out = found.type;
            return true;
        }
        add_error(*diagnostics_, target->range, "syntax error: invalid assignment target");
        return false;
    }

    bool check_expression(ast::Expression* expression, ast::Type& out) {
        if (auto* literal = dynamic_cast<ast::Literal*>(expression)) {
            switch (literal->literal_kind) {
            case ast::LiteralKind::Int:
                out = ast::basic(ast::TypeKind::Int);
                break;
            case ast::LiteralKind::Float:
                out = ast::basic(ast::TypeKind::Float);
                break;
            case ast::LiteralKind::Bool:
                out = ast::basic(ast::TypeKind::Bool);
                break;
            case ast::LiteralKind::String:
                out = ast::basic(ast::TypeKind::String);
                break;
            case ast::LiteralKind::Null:
                out = ast::basic(ast::TypeKind::Null);
                break;
            }
            expression->type = out;
            return true;
        }
        if (auto* string = dynamic_cast<ast::InterpolatedString*>(expression)) {
            for (const ast::InterpolatedStringPart& part : string->parts) {
                if (part.expression == nullptr) {
                    continue;
                }
                ast::Type part_type;
                if (!check_expression(part.expression, part_type)) {
                    return false;
                }
                if (!interpolatable(part_type)) {
                    add_error(*diagnostics_, part.expression->range, "type error: interpolation needs int, float, bool, or string, got " + part_type.to_string());
                    return false;
                }
            }
            out = ast::basic(ast::TypeKind::String);
            expression->type = out;
            return true;
        }
        if (auto* name = dynamic_cast<ast::Name*>(expression)) {
            const std::optional<Symbol> symbol = scope_.resolve(name->identifier);
            if (!symbol) {
                if (generic_funcs_.find(name->identifier) != generic_funcs_.end()) {
                    add_error(*diagnostics_, name->range, "type error: generic function " + name->identifier + " must be called directly");
                    return false;
                }
                add_error(*diagnostics_, name->range, "name error: " + name->identifier + " is not defined");
                return false;
            }
            out = symbol->type;
            expression->type = out;
            return true;
        }
        if (auto* prefix = dynamic_cast<ast::Prefix*>(expression)) {
            if (!check_prefix(*prefix, out)) {
                return false;
            }
            expression->type = out;
            return true;
        }
        if (auto* call = dynamic_cast<ast::Call*>(expression)) {
            if (!check_call(*call, out)) {
                return false;
            }
            expression->type = out;
            return true;
        }
        if (auto* input = dynamic_cast<ast::Input*>(expression)) {
            if (input->prompt != nullptr) {
                ast::Type prompt_type;
                if (!check_expression(input->prompt, prompt_type)) {
                    return false;
                }
                if (!type_equal(prompt_type, ast::basic(ast::TypeKind::String))) {
                    add_error(*diagnostics_, input->range, "type error: in prompt must be string, got " + prompt_type.to_string());
                    return false;
                }
            }
            out = ast::basic(ast::TypeKind::String);
            expression->type = out;
            return true;
        }
        if (auto* array = dynamic_cast<ast::ArrayLiteral*>(expression)) {
            if (!check_array_literal(*array, out)) {
                return false;
            }
            expression->type = out;
            return true;
        }
        if (auto* index = dynamic_cast<ast::Index*>(expression)) {
            ast::Type target_type;
            ast::Type index_type;
            if (!check_expression(index->target, target_type) || !check_expression(index->index, index_type)) {
                return false;
            }
            if (target_type.kind == ast::TypeKind::Map && target_type.key != nullptr && target_type.elem != nullptr) {
                if (!assignable(index_type, *target_type.key)) {
                    add_error(*diagnostics_, index->index->range, "type error: map index must be " + target_type.key->to_string() + ", got " + index_type.to_string());
                    return false;
                }
                out = *target_type.elem;
                expression->type = out;
                return true;
            }
            if (!type_equal(index_type, ast::basic(ast::TypeKind::Int))) {
                add_error(*diagnostics_, index->index->range, "type error: index must be int, got " + index_type.to_string());
                return false;
            }
            if (target_type.kind == ast::TypeKind::Array && target_type.elem != nullptr) {
                out = *target_type.elem;
                expression->type = out;
                return true;
            }
            if (target_type.kind == ast::TypeKind::String) {
                out = ast::basic(ast::TypeKind::String);
                expression->type = out;
                return true;
            }
            add_error(*diagnostics_, index->range, "type error: index target must be array, map, or string, got " + target_type.to_string());
            return false;
        }
        if (auto* field = dynamic_cast<ast::FieldAccess*>(expression)) {
            ast::Type target_type;
            if (!check_expression(field->target, target_type)) {
                return false;
            }
            ast::StructField found;
            if (!lookup_field(target_type, field->field, field->range, found)) {
                return false;
            }
            out = found.type;
            expression->type = out;
            return true;
        }
        add_error(*diagnostics_, expression->range, "internal error: unknown expression");
        return false;
    }

    bool check_prefix(ast::Prefix& expression, ast::Type& out) {
        std::vector<ast::Type> arg_types;
        arg_types.reserve(expression.args.size());
        for (ast::Expression* arg : expression.args) {
            ast::Type arg_type;
            if (!check_expression(arg, arg_type)) {
                return false;
            }
            arg_types.push_back(arg_type);
        }
        if (expression.op == "+" || expression.op == "-" || expression.op == "*" || expression.op == "/" || expression.op == "^") {
            for (const ast::Type& arg_type : arg_types) {
                if (!is_numeric(arg_type)) {
                    add_error(*diagnostics_, expression.range, "type error: operator " + expression.op + " needs numeric operands");
                    return false;
                }
            }
            out = expression.op == "/" || contains_kind(arg_types, ast::TypeKind::Float) ? ast::basic(ast::TypeKind::Float) : ast::basic(ast::TypeKind::Int);
            return true;
        }
        if (expression.op == ">" || expression.op == "<" || expression.op == ">=" || expression.op == "<=") {
            for (const ast::Type& arg_type : arg_types) {
                if (!is_numeric(arg_type)) {
                    add_error(*diagnostics_, expression.range, "type error: operator " + expression.op + " needs numeric operands");
                    return false;
                }
            }
            out = ast::basic(ast::TypeKind::Bool);
            return true;
        }
        if (expression.op == "==" || expression.op == "!=") {
            if (!comparable(arg_types[0], arg_types[1])) {
                add_error(*diagnostics_, expression.range, "type error: cannot compare " + arg_types[0].to_string() + " and " + arg_types[1].to_string());
                return false;
            }
            out = ast::basic(ast::TypeKind::Bool);
            return true;
        }
        if (expression.op == "and" || expression.op == "or") {
            for (const ast::Type& arg_type : arg_types) {
                if (!type_equal(arg_type, ast::basic(ast::TypeKind::Bool))) {
                    add_error(*diagnostics_, expression.range, "type error: operator " + expression.op + " needs bool operands");
                    return false;
                }
            }
            out = ast::basic(ast::TypeKind::Bool);
            return true;
        }
        if (expression.op == "not") {
            if (!type_equal(arg_types[0], ast::basic(ast::TypeKind::Bool))) {
                add_error(*diagnostics_, expression.range, "type error: operator not needs one bool operand");
                return false;
            }
            out = ast::basic(ast::TypeKind::Bool);
            return true;
        }
        add_error(*diagnostics_, expression.range, "internal error: unsupported operator " + expression.op);
        return false;
    }

    bool check_call(ast::Call& call, ast::Type& out) {
        if (call.receiver != nullptr) {
            bool handled = false;
            if (!check_dotted_call(call, handled, out)) {
                return false;
            }
            if (handled) {
                return true;
            }
        }
        if (call.callee.find('.') != std::string::npos) {
            return check_module_call(call, out);
        }
        const auto struct_found = structs_.find(call.callee);
        if (struct_found != structs_.end()) {
            return check_struct_constructor(call, struct_found->second, out);
        }
        const auto generic_found = generic_funcs_.find(call.callee);
        if (generic_found != generic_funcs_.end()) {
            return check_generic_function_call(call, *generic_found->second, out);
        }
        const std::optional<Symbol> symbol = scope_.resolve(call.callee);
        if (!symbol) {
            add_error(*diagnostics_, call.range, "name error: " + call.callee + " is not defined");
            return false;
        }
        return check_function_call(call, symbol->type, out);
    }

    bool check_dotted_call(ast::Call& call, bool& handled, ast::Type& out) {
        if (is_imported_module_call(call)) {
            handled = true;
            return check_module_call(call, out);
        }
        ast::Type receiver_type;
        if (!check_expression(call.receiver, receiver_type)) {
            handled = true;
            return false;
        }
        if (receiver_type.kind != ast::TypeKind::Struct) {
            handled = true;
            add_error(*diagnostics_, call.receiver->range, "type error: method call needs struct receiver, got " + receiver_type.to_string());
            return false;
        }
        const auto receiver_methods = methods_.find(receiver_type.name);
        if (receiver_methods == methods_.end() || receiver_methods->second.find(call.method) == receiver_methods->second.end()) {
            handled = true;
            add_error(*diagnostics_, call.range, "type error: " + receiver_type.to_string() + " has no method " + call.method);
            return false;
        }
        const MethodDef& method = receiver_methods->second.at(call.method);
        if (method.type.params.empty() || method.type.return_type == nullptr) {
            handled = true;
            add_error(*diagnostics_, call.range, "internal error: method " + method_name(receiver_type.name, call.method) + " has invalid type");
            return false;
        }
        if (!assignable(receiver_type, method.type.params[0])) {
            handled = true;
            add_error(
                *diagnostics_,
                call.receiver->range,
                "type error: method " + method_name(receiver_type.name, call.method) + " receiver is " + method.type.params[0].to_string() + ", got " +
                    receiver_type.to_string());
            return false;
        }
        const std::size_t expected_args = method.type.params.size() - 1;
        if (call.args.size() != expected_args) {
            handled = true;
            add_error(
                *diagnostics_,
                call.range,
                "type error: " + method_name(receiver_type.name, call.method) + " expects " + std::to_string(expected_args) + " args, got " +
                    std::to_string(call.args.size()));
            return false;
        }
        for (std::size_t index = 0; index < call.args.size(); ++index) {
            ast::Type arg_type;
            if (!check_expression(call.args[index], arg_type)) {
                handled = true;
                return false;
            }
            const ast::Type& param_type = method.type.params[index + 1];
            if (!assignable(arg_type, param_type)) {
                handled = true;
                add_error(
                    *diagnostics_,
                    call.args[index]->range,
                    "type error: arg " + std::to_string(index + 1) + " to " + method_name(receiver_type.name, call.method) + " is " + param_type.to_string() +
                        ", got " + arg_type.to_string());
                return false;
            }
        }
        out = *method.type.return_type;
        handled = true;
        return true;
    }

    bool is_imported_module_call(const ast::Call& call) const {
        const std::optional<std::pair<std::string, std::string>> split = split_qualified_call(call.callee);
        return split && imports_.count(split->first) != 0;
    }

    bool check_struct_constructor(ast::Call& call, const StructDef& def, ast::Type& out) {
        if (call.args.size() != def.fields.size()) {
            add_error(
                *diagnostics_,
                call.range,
                "type error: " + def.name + " expects " + std::to_string(def.fields.size()) + " field values, got " + std::to_string(call.args.size()));
            return false;
        }
        for (std::size_t index = 0; index < call.args.size(); ++index) {
            ast::Type arg_type;
            if (!check_expression(call.args[index], arg_type)) {
                return false;
            }
            const ast::StructField& field = def.fields[index];
            if (!assignable(arg_type, field.type)) {
                add_error(*diagnostics_, call.args[index]->range, "type error: field " + field.name + " is " + field.type.to_string() + ", got " + arg_type.to_string());
                return false;
            }
        }
        out = ast::struct_type(def.name);
        return true;
    }

    bool check_module_call(ast::Call& call, ast::Type& out) {
        const std::optional<std::pair<std::string, std::string>> split = split_qualified_call(call.callee);
        if (!split) {
            add_error(*diagnostics_, call.range, "name error: unsupported qualified call " + call.callee);
            return false;
        }
        const std::string& module = split->first;
        const std::string& name = split->second;
        if (imports_.count(module) == 0) {
            add_error(*diagnostics_, call.range, "name error: module " + module + " is not imported");
            return false;
        }
        const auto module_found = modules_.find(module);
        if (module_found != modules_.end()) {
            Module& user_module = *module_found->second;
            const auto generic_found = user_module.generic_exports.find(name);
            if (generic_found != user_module.generic_exports.end()) {
                return check_generic_function_call(call, *generic_found->second, out);
            }
            const auto export_found = user_module.exports.find(name);
            if (export_found == user_module.exports.end()) {
                add_error(*diagnostics_, call.range, "name error: module " + module + " does not export " + name);
                return false;
            }
            return check_function_call(call, export_found->second, out);
        }
        if (const BuiltinFunction* builtin = lookup_builtin(module, name)) {
            if (builtin->effect && !effect_context_) {
                add_error(*diagnostics_, call.range, "type error: " + builtin->qualified_name() + " is an effect; use do: " + builtin->qualified_name() + "(...)");
                return false;
            }
            return check_builtin_function_call(call, *builtin, out);
        }
        return check_core_builtin_call(call, module, name, out);
    }

    bool check_core_builtin_call(ast::Call& call, const std::string& module, const std::string& name, ast::Type& out) {
        const std::string qualified = module + "." + name;
        if (qualified == "math.sqrt" || qualified == "math.exp" || qualified == "math.log") {
            if (call.args.size() != 1) {
                add_error(*diagnostics_, call.range, "type error: " + qualified + " expects 1 arg, got " + std::to_string(call.args.size()));
                return false;
            }
            ast::Type arg_type;
            if (!check_expression(call.args[0], arg_type)) {
                return false;
            }
            if (!is_numeric(arg_type)) {
                add_error(*diagnostics_, call.args[0]->range, "type error: " + qualified + " needs numeric arg, got " + arg_type.to_string());
                return false;
            }
            out = ast::basic(ast::TypeKind::Float);
            return true;
        }
        if (qualified == "math.pow") {
            if (call.args.size() != 2) {
                add_error(*diagnostics_, call.range, "type error: math.pow expects 2 args, got " + std::to_string(call.args.size()));
                return false;
            }
            for (ast::Expression* arg : call.args) {
                ast::Type arg_type;
                if (!check_expression(arg, arg_type)) {
                    return false;
                }
                if (!is_numeric(arg_type)) {
                    add_error(*diagnostics_, arg->range, "type error: math.pow needs numeric args, got " + arg_type.to_string());
                    return false;
                }
            }
            out = ast::basic(ast::TypeKind::Float);
            return true;
        }
        if (qualified == "string.len" || qualified == "string.at" || qualified == "string.contains" || qualified == "string.concat" ||
            qualified == "string.lower" || qualified == "string.split" || qualified == "string.replace") {
            return check_string_builtin(call, qualified, out);
        }
        if (qualified == "array.len" || qualified == "array.contains" || qualified == "array.push") {
            return check_array_builtin(call, qualified, out);
        }
        if (qualified == "map.empty" || qualified == "map.has" || qualified == "map.get" || qualified == "map.set" || qualified == "map.keys" || qualified == "map.push") {
            return check_map_builtin(call, qualified, out);
        }
        if (qualified == "time.now") {
            if (!expect_arg_count(call, "time.now", 0)) {
                return false;
            }
            out = ast::basic(ast::TypeKind::Int);
            return true;
        }
        if (qualified == "random.int") {
            if (!expect_arg_count(call, "random.int", 2)) {
                return false;
            }
            for (ast::Expression* arg : call.args) {
                ast::Type arg_type;
                if (!check_expression(arg, arg_type)) {
                    return false;
                }
                if (!type_equal(arg_type, ast::basic(ast::TypeKind::Int))) {
                    add_error(*diagnostics_, arg->range, "type error: random.int args must be int, got " + arg_type.to_string());
                    return false;
                }
            }
            out = ast::basic(ast::TypeKind::Int);
            return true;
        }
        if (qualified == "random.float") {
            if (!expect_arg_count(call, "random.float", 2)) {
                return false;
            }
            for (ast::Expression* arg : call.args) {
                ast::Type arg_type;
                if (!check_expression(arg, arg_type)) {
                    return false;
                }
                if (!is_numeric(arg_type)) {
                    add_error(*diagnostics_, arg->range, "type error: random.float args must be numeric, got " + arg_type.to_string());
                    return false;
                }
            }
            out = ast::basic(ast::TypeKind::Float);
            return true;
        }
        if (qualified == "random.choice") {
            if (!expect_arg_count(call, "random.choice", 1)) {
                return false;
            }
            ast::Type array_type;
            if (!check_expression(call.args[0], array_type)) {
                return false;
            }
            if (array_type.kind != ast::TypeKind::Array || array_type.elem == nullptr) {
                add_error(*diagnostics_, call.args[0]->range, "type error: random.choice needs array arg, got " + array_type.to_string());
                return false;
            }
            if (!native_array_element(*array_type.elem)) {
                add_error(*diagnostics_, call.args[0]->range, "type error: random.choice needs array of int, float, bool, or string, got " + array_type.to_string());
                return false;
            }
            out = *array_type.elem;
            return true;
        }
        if (qualified == "testing.assert") {
            if (!expect_arg_count(call, "testing.assert", 1)) {
                return false;
            }
            ast::Type arg_type;
            if (!check_expression(call.args[0], arg_type)) {
                return false;
            }
            if (!type_equal(arg_type, ast::basic(ast::TypeKind::Bool))) {
                add_error(*diagnostics_, call.args[0]->range, "type error: testing.assert needs bool arg, got " + arg_type.to_string());
                return false;
            }
            out = ast::basic(ast::TypeKind::Bool);
            return true;
        }
        add_error(*diagnostics_, call.range, "name error: unknown library function " + call.callee);
        return false;
    }

    bool check_string_builtin(ast::Call& call, const std::string& qualified, ast::Type& out) {
        if (qualified == "string.len") {
            if (!expect_arg_count(call, "string.len", 1)) {
                return false;
            }
            ast::Type arg_type;
            if (!check_expression(call.args[0], arg_type)) {
                return false;
            }
            if (arg_type.kind != ast::TypeKind::String) {
                add_error(*diagnostics_, call.args[0]->range, "type error: string.len needs string arg, got " + arg_type.to_string());
                return false;
            }
            out = ast::basic(ast::TypeKind::Int);
            return true;
        }
        if (qualified == "string.at") {
            if (!expect_arg_count(call, "string.at", 2)) {
                return false;
            }
            ast::Type text_type;
            if (!check_expression(call.args[0], text_type)) {
                return false;
            }
            if (text_type.kind != ast::TypeKind::String) {
                add_error(*diagnostics_, call.args[0]->range, "type error: string.at needs string first arg, got " + text_type.to_string());
                return false;
            }
            ast::Type index_type;
            if (!check_expression(call.args[1], index_type)) {
                return false;
            }
            if (!type_equal(index_type, ast::basic(ast::TypeKind::Int))) {
                add_error(*diagnostics_, call.args[1]->range, "type error: string.at index must be int, got " + index_type.to_string());
                return false;
            }
            out = ast::basic(ast::TypeKind::String);
            return true;
        }
        if (qualified == "string.lower") {
            if (!expect_arg_count(call, "string.lower", 1)) {
                return false;
            }
            ast::Type arg_type;
            if (!check_expression(call.args[0], arg_type)) {
                return false;
            }
            if (arg_type.kind != ast::TypeKind::String) {
                add_error(*diagnostics_, call.args[0]->range, "type error: string.lower needs string arg, got " + arg_type.to_string());
                return false;
            }
            out = ast::basic(ast::TypeKind::String);
            return true;
        }
        const std::string name =
            qualified == "string.contains" ? "string.contains" : qualified == "string.concat" ? "string.concat" : qualified == "string.split" ? "string.split" : "string.replace";
        const std::size_t want_args = qualified == "string.replace" ? 3 : 2;
        if (!expect_arg_count(call, name, want_args)) {
            return false;
        }
        for (std::size_t index = 0; index < call.args.size(); ++index) {
            ast::Type arg_type;
            if (!check_expression(call.args[index], arg_type)) {
                return false;
            }
            if (arg_type.kind != ast::TypeKind::String) {
                add_error(
                    *diagnostics_,
                    call.args[index]->range,
                    "type error: arg " + std::to_string(index + 1) + " to " + name + " must be string, got " + arg_type.to_string());
                return false;
            }
        }
        if (qualified == "string.contains") {
            out = ast::basic(ast::TypeKind::Bool);
        } else if (qualified == "string.split") {
            out = ast::array_of(ast::basic(ast::TypeKind::String));
        } else {
            out = ast::basic(ast::TypeKind::String);
        }
        return true;
    }

    bool check_array_builtin(ast::Call& call, const std::string& qualified, ast::Type& out) {
        if (qualified == "array.len") {
            if (!expect_arg_count(call, "array.len", 1)) {
                return false;
            }
            ast::Type arg_type;
            if (!check_expression(call.args[0], arg_type)) {
                return false;
            }
            if (arg_type.kind != ast::TypeKind::Array) {
                add_error(*diagnostics_, call.args[0]->range, "type error: array.len needs array arg, got " + arg_type.to_string());
                return false;
            }
            out = ast::basic(ast::TypeKind::Int);
            return true;
        }
        const std::string name = qualified == "array.contains" ? "array.contains" : "array.push";
        if (!expect_arg_count(call, name, 2)) {
            return false;
        }
        ast::Type array_type;
        if (!check_expression(call.args[0], array_type)) {
            return false;
        }
        if (array_type.kind != ast::TypeKind::Array || array_type.elem == nullptr) {
            add_error(*diagnostics_, call.args[0]->range, "type error: " + name + " needs array first arg, got " + array_type.to_string());
            return false;
        }
        if (!native_array_element(*array_type.elem)) {
            add_error(*diagnostics_, call.args[0]->range, "type error: " + name + " needs array of int, float, bool, or string, got " + array_type.to_string());
            return false;
        }
        ast::Type item_type;
        if (!check_expression(call.args[1], item_type)) {
            return false;
        }
        if (!assignable(item_type, *array_type.elem)) {
            add_error(*diagnostics_, call.args[1]->range, "type error: arg 2 to " + name + " is " + array_type.elem->to_string() + ", got " + item_type.to_string());
            return false;
        }
        out = qualified == "array.contains" ? ast::basic(ast::TypeKind::Bool) : array_type;
        return true;
    }

    bool check_map_builtin(ast::Call& call, const std::string& qualified, ast::Type& out) {
        if (qualified == "map.empty") {
            if (!expect_arg_count(call, "map.empty", 0)) {
                return false;
            }
            out = string_array_map_type();
            return true;
        }
        if (qualified == "map.keys") {
            if (!expect_arg_count(call, "map.keys", 1)) {
                return false;
            }
            ast::Type table_type;
            if (!check_expression(call.args[0], table_type)) {
                return false;
            }
            if (!string_array_map(table_type)) {
                add_error(*diagnostics_, call.args[0]->range, "type error: map.keys needs map[string]array[string], got " + table_type.to_string());
                return false;
            }
            out = ast::array_of(ast::basic(ast::TypeKind::String));
            return true;
        }
        const std::string name = qualified;
        const std::size_t want_args = qualified == "map.set" || qualified == "map.push" ? 3 : 2;
        if (!expect_arg_count(call, name, want_args)) {
            return false;
        }
        ast::Type table_type;
        if (!check_expression(call.args[0], table_type)) {
            return false;
        }
        if (!string_array_map(table_type)) {
            add_error(*diagnostics_, call.args[0]->range, "type error: " + name + " needs map[string]array[string], got " + table_type.to_string());
            return false;
        }
        ast::Type key_type;
        if (!check_expression(call.args[1], key_type)) {
            return false;
        }
        if (!type_equal(key_type, ast::basic(ast::TypeKind::String))) {
            add_error(*diagnostics_, call.args[1]->range, "type error: arg 2 to " + name + " must be string, got " + key_type.to_string());
            return false;
        }
        if (qualified == "map.has") {
            out = ast::basic(ast::TypeKind::Bool);
            return true;
        }
        if (qualified == "map.get") {
            out = *table_type.elem;
            return true;
        }
        ast::Type value_type;
        if (!check_expression(call.args[2], value_type)) {
            return false;
        }
        if (qualified == "map.set") {
            if (!assignable(value_type, *table_type.elem)) {
                add_error(*diagnostics_, call.args[2]->range, "type error: arg 3 to map.set is " + table_type.elem->to_string() + ", got " + value_type.to_string());
                return false;
            }
        } else if (!type_equal(value_type, ast::basic(ast::TypeKind::String))) {
            add_error(*diagnostics_, call.args[2]->range, "type error: arg 3 to map.push must be string, got " + value_type.to_string());
            return false;
        }
        out = table_type;
        return true;
    }

    bool expect_arg_count(const ast::Call& call, const std::string& qualified, std::size_t count) {
        if (call.args.size() != count) {
            add_error(*diagnostics_, call.range, "type error: " + qualified + " expects " + std::to_string(count) + " args, got " + std::to_string(call.args.size()));
            return false;
        }
        return true;
    }

    bool check_builtin_function_call(ast::Call& call, const BuiltinFunction& function, ast::Type& out) {
        if (call.args.size() != function.params.size()) {
            add_error(
                *diagnostics_,
                call.range,
                "type error: " + function.qualified_name() + " expects " + std::to_string(function.params.size()) + " args, got " + std::to_string(call.args.size()));
            return false;
        }
        for (std::size_t index = 0; index < call.args.size(); ++index) {
            ast::Type arg_type;
            if (!check_expression(call.args[index], arg_type)) {
                return false;
            }
            if (!assignable(arg_type, function.params[index])) {
                add_error(
                    *diagnostics_,
                    call.args[index]->range,
                    "type error: arg " + std::to_string(index + 1) + " to " + function.qualified_name() + " is " + function.params[index].to_string() + ", got " +
                        arg_type.to_string());
                return false;
            }
        }
        out = function.return_type;
        return true;
    }

    bool check_function_call(ast::Call& call, const ast::Type& function_type_value, ast::Type& out) {
        if (function_type_value.kind != ast::TypeKind::Function || function_type_value.return_type == nullptr) {
            add_error(*diagnostics_, call.range, "type error: " + call.callee + " is not callable");
            return false;
        }
        if (call.args.size() != function_type_value.params.size()) {
            add_error(
                *diagnostics_,
                call.range,
                "type error: " + call.callee + " expects " + std::to_string(function_type_value.params.size()) + " args, got " + std::to_string(call.args.size()));
            return false;
        }
        for (std::size_t index = 0; index < call.args.size(); ++index) {
            ast::Type arg_type;
            if (!check_expression(call.args[index], arg_type)) {
                return false;
            }
            if (!assignable(arg_type, function_type_value.params[index])) {
                add_error(
                    *diagnostics_,
                    call.args[index]->range,
                    "type error: arg " + std::to_string(index + 1) + " to " + call.callee + " is " + function_type_value.params[index].to_string() + ", got " +
                        arg_type.to_string());
                return false;
            }
        }
        out = *function_type_value.return_type;
        return true;
    }

    bool check_generic_function_call(ast::Call& call, ast::FuncDecl& function, ast::Type& out) {
        if (function.type_params.empty()) {
            return check_function_call(call, function_type(function), out);
        }
        if (call.args.size() != function.params.size()) {
            add_error(
                *diagnostics_,
                call.range,
                "type error: " + call.callee + " expects " + std::to_string(function.params.size()) + " args, got " + std::to_string(call.args.size()));
            return false;
        }
        std::map<std::string, ast::Type> bindings;
        for (std::size_t index = 0; index < call.args.size(); ++index) {
            ast::Type arg_type;
            if (!check_expression(call.args[index], arg_type)) {
                return false;
            }
            if (!match_generic_arg(function.params[index].type, arg_type, bindings, call.callee, index + 1, call.args[index]->range)) {
                return false;
            }
        }
        call.type_args.clear();
        for (const std::string& name : function.type_params) {
            const auto found = bindings.find(name);
            if (found == bindings.end()) {
                add_error(*diagnostics_, call.range, "type error: cannot infer type " + name + " for " + call.callee);
                return false;
            }
            call.type_args.push_back(found->second);
        }
        out = substitute_type(function.return_type, bindings);
        return true;
    }

    bool match_generic_arg(
        const ast::Type& param_type,
        const ast::Type& arg_type,
        std::map<std::string, ast::Type>& bindings,
        const std::string& callee,
        std::size_t index,
        const SourceRange& range) {
        switch (param_type.kind) {
        case ast::TypeKind::Generic:
            return bind_generic_type(param_type, arg_type, bindings, callee, index, range);
        case ast::TypeKind::Array:
            if (arg_type.kind != ast::TypeKind::Array || arg_type.elem == nullptr || param_type.elem == nullptr) {
                add_error(*diagnostics_, range, "type error: arg " + std::to_string(index) + " to " + callee + " is " + param_type.to_string() + ", got " + arg_type.to_string());
                return false;
            }
            return match_generic_arg(*param_type.elem, *arg_type.elem, bindings, callee, index, range);
        case ast::TypeKind::Function:
            if (arg_type.kind != ast::TypeKind::Function || param_type.params.size() != arg_type.params.size() || param_type.return_type == nullptr ||
                arg_type.return_type == nullptr) {
                add_error(*diagnostics_, range, "type error: arg " + std::to_string(index) + " to " + callee + " is " + param_type.to_string() + ", got " + arg_type.to_string());
                return false;
            }
            for (std::size_t param_index = 0; param_index < param_type.params.size(); ++param_index) {
                if (!match_generic_arg(param_type.params[param_index], arg_type.params[param_index], bindings, callee, index, range)) {
                    return false;
                }
            }
            return match_generic_arg(*param_type.return_type, *arg_type.return_type, bindings, callee, index, range);
        default:
            if (!assignable(arg_type, param_type)) {
                add_error(*diagnostics_, range, "type error: arg " + std::to_string(index) + " to " + callee + " is " + param_type.to_string() + ", got " + arg_type.to_string());
                return false;
            }
            return true;
        }
    }

    bool bind_generic_type(
        const ast::Type& param_type,
        const ast::Type& arg_type,
        std::map<std::string, ast::Type>& bindings,
        const std::string& callee,
        std::size_t index,
        const SourceRange& range) {
        const auto found = bindings.find(param_type.name);
        if (found == bindings.end()) {
            bindings.emplace(param_type.name, arg_type);
            return true;
        }
        if (!type_equal(arg_type, found->second)) {
            add_error(
                *diagnostics_,
                range,
                "type error: arg " + std::to_string(index) + " to " + callee + " needs " + param_type.name + " as " + found->second.to_string() + ", got " +
                    arg_type.to_string());
            return false;
        }
        return true;
    }

    bool check_array_literal(ast::ArrayLiteral& array, ast::Type& out) {
        if (array.elements.empty()) {
            add_error(*diagnostics_, array.range, "type error: empty arrays need an explicit type");
            return false;
        }
        ast::Type first_type;
        if (!check_expression(array.elements[0], first_type)) {
            return false;
        }
        for (std::size_t index = 1; index < array.elements.size(); ++index) {
            ast::Type element_type;
            if (!check_expression(array.elements[index], element_type)) {
                return false;
            }
            if (!assignable(element_type, first_type) || !assignable(first_type, element_type)) {
                add_error(
                    *diagnostics_,
                    array.elements[index]->range,
                    "type error: arrays must be homogeneous, got " + first_type.to_string() + " and " + element_type.to_string());
                return false;
            }
        }
        out = ast::array_of(first_type);
        return true;
    }

    bool check_array_literal_as(ast::ArrayLiteral& array, const ast::Type& expected, ast::Type& out) {
        if (expected.kind != ast::TypeKind::Array || expected.elem == nullptr) {
            add_error(*diagnostics_, array.range, "type error: array literal needs array type, got " + expected.to_string());
            return false;
        }
        for (ast::Expression* element : array.elements) {
            ast::Type element_type;
            if (!check_expression(element, element_type)) {
                return false;
            }
            if (!assignable(element_type, *expected.elem)) {
                add_error(*diagnostics_, element->range, "type error: array element needs " + expected.elem->to_string() + ", got " + element_type.to_string());
                return false;
            }
        }
        out = expected;
        return true;
    }

    bool check_empty_map_literal_as(ast::ArrayLiteral& array, const ast::Type& expected, ast::Type& out) {
        if (!string_array_map(expected)) {
            add_error(*diagnostics_, array.range, "type error: empty map literal needs map[string]array[string], got " + expected.to_string());
            return false;
        }
        if (!array.elements.empty()) {
            add_error(*diagnostics_, array.range, "type error: map literal supports only empty []");
            return false;
        }
        out = expected;
        return true;
    }

    bool lookup_field(const ast::Type& target_type, const std::string& field_name, const SourceRange& range, ast::StructField& out) {
        if (target_type.kind != ast::TypeKind::Struct) {
            add_error(*diagnostics_, range, "type error: field access needs struct, got " + target_type.to_string());
            return false;
        }
        const auto struct_found = structs_.find(target_type.name);
        if (struct_found == structs_.end()) {
            add_error(*diagnostics_, range, "type error: unknown type " + target_type.to_string());
            return false;
        }
        const auto field_found = struct_found->second.field_map.find(field_name);
        if (field_found == struct_found->second.field_map.end()) {
            add_error(*diagnostics_, range, "type error: " + target_type.to_string() + " has no field " + field_name);
            return false;
        }
        out = field_found->second;
        return true;
    }

    bool check_nested_block(const std::vector<ast::Statement*>& statements) {
        scope_.push();
        ++block_depth_;
        const bool ok = check_block(statements);
        --block_depth_;
        scope_.pop();
        return ok;
    }

    bool check_block(const std::vector<ast::Statement*>& statements) {
        bool unreachable = false;
        for (ast::Statement* statement : statements) {
            if (unreachable) {
                warnings_->push_back({statement->range, "unreachable statement"});
            }
            if (!check_statement(statement)) {
                return false;
            }
            if (statement_terminates(statement)) {
                unreachable = true;
            }
        }
        return true;
    }

    bool define(const std::string& name, const ast::Type& type, bool mutable_binding, const SourceRange& range) {
        if (scope_.current_has(name)) {
            add_error(*diagnostics_, range, "name error: " + name + " is already defined");
            return false;
        }
        if (structs_.find(name) != structs_.end()) {
            add_error(*diagnostics_, range, "name error: " + name + " is already defined as struct");
            return false;
        }
        if (scope_.outer_has(name)) {
            warnings_->push_back({range, name + " shadows outer name"});
        }
        scope_.define_current(name, {type, mutable_binding});
        return true;
    }

    bool validate_type(const ast::Type& type, const SourceRange& range) {
        switch (type.kind) {
        case ast::TypeKind::Void:
        case ast::TypeKind::Int:
        case ast::TypeKind::Float:
        case ast::TypeKind::Bool:
        case ast::TypeKind::String:
            return true;
        case ast::TypeKind::Struct:
            if (type.nullable) {
                add_error(*diagnostics_, range, "type error: struct type " + type.name + " cannot be nullable");
                return false;
            }
            if (structs_.find(type.name) == structs_.end()) {
                add_error(*diagnostics_, range, "type error: unknown type " + type.to_string());
                return false;
            }
            return true;
        case ast::TypeKind::Array:
            if (type.elem == nullptr) {
                add_error(*diagnostics_, range, "type error: array type needs element type");
                return false;
            }
            return validate_type(*type.elem, range);
        case ast::TypeKind::Map:
            if (type.nullable) {
                add_error(*diagnostics_, range, "type error: map type cannot be nullable");
                return false;
            }
            if (type.key == nullptr || type.elem == nullptr) {
                add_error(*diagnostics_, range, "type error: map type needs key and value types");
                return false;
            }
            if (!validate_type(*type.key, range) || !validate_type(*type.elem, range)) {
                return false;
            }
            if (!type_equal(*type.key, ast::basic(ast::TypeKind::String))) {
                add_error(*diagnostics_, range, "type error: map key type must be string, got " + type.key->to_string());
                return false;
            }
            if (type.elem->kind != ast::TypeKind::Array || type.elem->elem == nullptr || !type_equal(*type.elem->elem, ast::basic(ast::TypeKind::String))) {
                add_error(*diagnostics_, range, "type error: map value type must be array[string], got " + type.elem->to_string());
                return false;
            }
            return true;
        case ast::TypeKind::Function:
            for (const ast::Type& param : type.params) {
                if (!validate_type(param, range)) {
                    return false;
                }
            }
            if (type.return_type == nullptr) {
                add_error(*diagnostics_, range, "type error: function type needs return type");
                return false;
            }
            return validate_type(*type.return_type, range);
        case ast::TypeKind::Generic:
            if (type_params_.count(type.name) != 0) {
                return true;
            }
            add_error(*diagnostics_, range, "type error: unknown type " + type.to_string());
            return false;
        case ast::TypeKind::Null:
        case ast::TypeKind::Invalid:
            add_error(*diagnostics_, range, "type error: unknown type " + type.to_string());
            return false;
        }
        add_error(*diagnostics_, range, "type error: unknown type " + type.to_string());
        return false;
    }

    std::map<std::string, std::unique_ptr<Module>>& modules_;
    std::map<std::string, StructDef> structs_;
    std::map<std::string, std::map<std::string, MethodDef>> methods_;
    ScopeStack scope_;
    std::set<std::string> imports_;
    std::map<std::string, ast::FuncDecl*> generic_funcs_;
    std::set<std::string> type_params_;
    DiagnosticSet* diagnostics_ = nullptr;
    std::vector<Warning>* warnings_ = nullptr;
    ast::Type current_return_;
    bool in_function_ = false;
    int loop_depth_ = 0;
    int block_depth_ = 0;
    bool effect_context_ = false;
};

std::vector<ast::Import*> imports(ast::Program& program) {
    std::vector<ast::Import*> result;
    for (ast::Statement* statement : program.statements) {
        if (auto* import = dynamic_cast<ast::Import*>(statement)) {
            result.push_back(import);
        }
    }
    return result;
}

CheckResult check_one_program(
    ast::Program& program,
    std::map<std::string, std::unique_ptr<Module>>& modules,
    const std::map<std::string, StructDef>& structs,
    const std::map<std::string, std::map<std::string, MethodDef>>& methods) {
    Checker checker(modules, structs, methods);
    return checker.check(program);
}

}  // namespace

std::map<std::string, StructDef> struct_definitions(ast::Program& program, DiagnosticSet& diagnostics) {
    std::map<std::string, StructDef> structs;
    for (ast::Statement* statement : program.statements) {
        auto* decl = dynamic_cast<ast::StructDecl*>(statement);
        if (decl == nullptr) {
            continue;
        }
        if (structs.find(decl->name) != structs.end()) {
            add_error(diagnostics, decl->range, "type error: struct " + decl->name + " is already defined");
            return {};
        }
        StructDef def;
        def.range = decl->range;
        def.name = decl->name;
        def.fields = decl->fields;
        for (const ast::StructField& field : decl->fields) {
            if (def.field_map.find(field.name) != def.field_map.end()) {
                add_error(diagnostics, field.range, "type error: struct " + decl->name + " already has field " + field.name);
                return {};
            }
            def.field_map.emplace(field.name, field);
        }
        structs.emplace(decl->name, std::move(def));
    }
    return structs;
}

std::map<std::string, std::map<std::string, MethodDef>> method_definitions(ast::Program& program, DiagnosticSet& diagnostics) {
    std::map<std::string, std::map<std::string, MethodDef>> methods;
    for (ast::Statement* statement : program.statements) {
        auto* function = dynamic_cast<ast::FuncDecl*>(statement);
        if (function == nullptr || function->receiver.empty()) {
            continue;
        }
        auto& receiver_methods = methods[function->receiver];
        if (receiver_methods.find(function->name) != receiver_methods.end()) {
            add_error(diagnostics, function->range, "type error: method " + method_name(function->receiver, function->name) + " is already defined");
            return {};
        }
        receiver_methods.emplace(
            function->name,
            MethodDef{function->range, function->receiver, function->name, function_type(*function), function});
    }
    return methods;
}

std::map<std::string, ast::Type> exported_functions(ast::Program& program, DiagnosticSet& diagnostics) {
    std::map<std::string, ast::Type> functions;
    std::set<std::string> generic_functions;
    for (ast::Statement* statement : program.statements) {
        auto* function = dynamic_cast<ast::FuncDecl*>(statement);
        if (function == nullptr || !function->receiver.empty()) {
            continue;
        }
        if (!function->type_params.empty()) {
            generic_functions.insert(function->name);
            continue;
        }
        functions[function->name] = function_type(*function);
    }
    std::map<std::string, ast::Type> exports;
    for (ast::Statement* statement : program.statements) {
        auto* export_statement = dynamic_cast<ast::Export*>(statement);
        if (export_statement == nullptr) {
            continue;
        }
        if (generic_functions.count(export_statement->name) != 0) {
            continue;
        }
        const auto found = functions.find(export_statement->name);
        if (found == functions.end()) {
            add_error(diagnostics, export_statement->range, "module error: " + export_statement->name + " is not an exported function");
            return {};
        }
        exports.emplace(export_statement->name, found->second);
    }
    return exports;
}

std::map<std::string, ast::FuncDecl*> exported_generic_functions(ast::Program& program) {
    std::map<std::string, ast::FuncDecl*> functions;
    for (ast::Statement* statement : program.statements) {
        auto* function = dynamic_cast<ast::FuncDecl*>(statement);
        if (function == nullptr || !function->receiver.empty() || function->type_params.empty()) {
            continue;
        }
        functions.emplace(function->name, function);
    }
    std::map<std::string, ast::FuncDecl*> exports;
    for (ast::Statement* statement : program.statements) {
        auto* export_statement = dynamic_cast<ast::Export*>(statement);
        if (export_statement == nullptr) {
            continue;
        }
        const auto found = functions.find(export_statement->name);
        if (found != functions.end()) {
            exports.emplace(export_statement->name, found->second);
        }
    }
    return exports;
}

CheckResult check_programs(ast::Program& program, std::map<std::string, std::unique_ptr<Module>>& modules) {
    CheckResult result;

    std::map<std::string, StructDef> structs;
    for (auto& item : modules) {
        DiagnosticSet local_diagnostics;
        std::map<std::string, StructDef> local = struct_definitions(*item.second->parsed.program, local_diagnostics);
        if (local_diagnostics.has_errors()) {
            result.diagnostics = std::move(local_diagnostics);
            return result;
        }
        for (auto& def : local) {
            if (structs.find(def.first) != structs.end()) {
                add_error(result.diagnostics, def.second.range, "type error: struct " + def.first + " is already defined");
                return result;
            }
            structs.emplace(def.first, std::move(def.second));
        }
    }
    {
        DiagnosticSet local_diagnostics;
        std::map<std::string, StructDef> local = struct_definitions(program, local_diagnostics);
        if (local_diagnostics.has_errors()) {
            result.diagnostics = std::move(local_diagnostics);
            return result;
        }
        for (auto& def : local) {
            if (structs.find(def.first) != structs.end()) {
                add_error(result.diagnostics, def.second.range, "type error: struct " + def.first + " is already defined");
                return result;
            }
            structs.emplace(def.first, std::move(def.second));
        }
    }

    std::map<std::string, std::map<std::string, MethodDef>> methods;
    auto merge_methods = [&](ast::Program& target) -> bool {
        DiagnosticSet local_diagnostics;
        std::map<std::string, std::map<std::string, MethodDef>> local = method_definitions(target, local_diagnostics);
        if (local_diagnostics.has_errors()) {
            result.diagnostics = std::move(local_diagnostics);
            return false;
        }
        for (auto& receiver : local) {
            auto& receiver_methods = methods[receiver.first];
            for (auto& method : receiver.second) {
                if (receiver_methods.find(method.first) != receiver_methods.end()) {
                    add_error(result.diagnostics, method.second.range, "type error: method " + receiver.first + "." + method.first + " is already defined");
                    return false;
                }
                receiver_methods.emplace(method.first, std::move(method.second));
            }
        }
        return true;
    };
    for (auto& item : modules) {
        if (!merge_methods(*item.second->parsed.program)) {
            return result;
        }
    }
    if (!merge_methods(program)) {
        return result;
    }

    std::set<std::string> checked;
    auto check_module = [&](auto& self, const std::string& name) -> bool {
        if (checked.count(name) != 0) {
            return true;
        }
        Module& module = *modules[name];
        for (ast::Import* import : imports(*module.parsed.program)) {
            if (!is_builtin_module(import->module)) {
                if (!self(self, import->module)) {
                    return false;
                }
            }
        }
        CheckResult module_result = check_one_program(*module.parsed.program, modules, structs, methods);
        result.warnings.insert(result.warnings.end(), module_result.warnings.begin(), module_result.warnings.end());
        if (!module_result.ok()) {
            result.diagnostics = std::move(module_result.diagnostics);
            return false;
        }
        module.exports = exported_functions(*module.parsed.program, result.diagnostics);
        if (result.diagnostics.has_errors()) {
            return false;
        }
        module.generic_exports = exported_generic_functions(*module.parsed.program);
        checked.insert(name);
        return true;
    };

    for (auto& item : modules) {
        if (!check_module(check_module, item.first)) {
            return result;
        }
    }

    CheckResult entry_result = check_one_program(program, modules, structs, methods);
    result.warnings.insert(result.warnings.end(), entry_result.warnings.begin(), entry_result.warnings.end());
    if (!entry_result.ok()) {
        result.diagnostics = std::move(entry_result.diagnostics);
    }
    return result;
}

}  // namespace walk::sema
