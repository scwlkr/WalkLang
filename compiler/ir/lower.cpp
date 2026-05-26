#include "ir/lower.h"

#include <utility>

namespace walk::ir {
namespace {

std::unique_ptr<Expression> lower_expression(ast::Expression* expression);
std::unique_ptr<Statement> lower_statement(ast::Statement* statement);

std::vector<std::unique_ptr<Expression>> lower_expressions(const std::vector<ast::Expression*>& expressions) {
    std::vector<std::unique_ptr<Expression>> result;
    result.reserve(expressions.size());
    for (ast::Expression* expression : expressions) {
        result.push_back(lower_expression(expression));
    }
    return result;
}

std::vector<std::unique_ptr<Statement>> lower_statements(const std::vector<ast::Statement*>& statements) {
    std::vector<std::unique_ptr<Statement>> result;
    result.reserve(statements.size());
    for (ast::Statement* statement : statements) {
        result.push_back(lower_statement(statement));
    }
    return result;
}

std::vector<Param> lower_params(const std::vector<ast::Param>& params) {
    std::vector<Param> result;
    result.reserve(params.size());
    for (const ast::Param& param : params) {
        result.push_back({param.name, param.type});
    }
    return result;
}

std::vector<StructField> lower_fields(const std::vector<ast::StructField>& fields) {
    std::vector<StructField> result;
    result.reserve(fields.size());
    for (const ast::StructField& field : fields) {
        result.push_back({field.range, field.name, field.type});
    }
    return result;
}

std::unique_ptr<Expression> lower_expression(ast::Expression* expression) {
    if (auto* source = dynamic_cast<ast::Literal*>(expression)) {
        auto out = std::make_unique<Literal>(source->range, source->type);
        out->literal_kind = source->literal_kind;
        out->value = source->value;
        return out;
    }
    if (auto* source = dynamic_cast<ast::InterpolatedString*>(expression)) {
        auto out = std::make_unique<InterpolatedString>(source->range, source->type);
        out->parts.reserve(source->parts.size());
        for (const ast::InterpolatedStringPart& part : source->parts) {
            InterpolatedStringPart lowered;
            lowered.literal = part.literal;
            if (part.expression != nullptr) {
                lowered.expression = lower_expression(part.expression);
            }
            out->parts.push_back(std::move(lowered));
        }
        return out;
    }
    if (auto* source = dynamic_cast<ast::Name*>(expression)) {
        auto out = std::make_unique<Name>(source->range, source->type);
        out->identifier = source->identifier;
        return out;
    }
    if (auto* source = dynamic_cast<ast::Prefix*>(expression)) {
        auto out = std::make_unique<Prefix>(source->range, source->type);
        out->op = source->op;
        out->args = lower_expressions(source->args);
        return out;
    }
    if (auto* source = dynamic_cast<ast::Call*>(expression)) {
        auto out = std::make_unique<Call>(source->range, source->type);
        out->callee = source->callee;
        if (source->receiver != nullptr) {
            out->receiver = lower_expression(source->receiver);
        }
        out->method = source->method;
        out->args = lower_expressions(source->args);
        out->type_args = source->type_args;
        return out;
    }
    if (auto* source = dynamic_cast<ast::Input*>(expression)) {
        auto out = std::make_unique<Input>(source->range, source->type);
        if (source->prompt != nullptr) {
            out->prompt = lower_expression(source->prompt);
        }
        return out;
    }
    if (auto* source = dynamic_cast<ast::ArrayLiteral*>(expression)) {
        auto out = std::make_unique<ArrayLiteral>(source->range, source->type);
        out->elements = lower_expressions(source->elements);
        return out;
    }
    if (auto* source = dynamic_cast<ast::Index*>(expression)) {
        auto out = std::make_unique<Index>(source->range, source->type);
        out->target = lower_expression(source->target);
        out->index = lower_expression(source->index);
        return out;
    }
    if (auto* source = dynamic_cast<ast::FieldAccess*>(expression)) {
        auto out = std::make_unique<FieldAccess>(source->range, source->type);
        out->target = lower_expression(source->target);
        out->field = source->field;
        return out;
    }
    return nullptr;
}

std::unique_ptr<Statement> lower_statement(ast::Statement* statement) {
    if (auto* source = dynamic_cast<ast::VarDecl*>(statement)) {
        auto out = std::make_unique<VarDecl>(source->range);
        out->name = source->name;
        out->annotation = source->annotation;
        out->value = lower_expression(source->value);
        out->mutable_binding = source->mutable_binding;
        return out;
    }
    if (auto* source = dynamic_cast<ast::Assignment*>(statement)) {
        auto out = std::make_unique<Assignment>(source->range);
        out->target = lower_expression(source->target);
        out->value = lower_expression(source->value);
        return out;
    }
    if (auto* source = dynamic_cast<ast::Out*>(statement)) {
        auto out = std::make_unique<Out>(source->range);
        out->value = lower_expression(source->value);
        return out;
    }
    if (auto* source = dynamic_cast<ast::Do*>(statement)) {
        auto out = std::make_unique<Do>(source->range);
        out->value = lower_expression(source->value);
        return out;
    }
    if (auto* source = dynamic_cast<ast::Defer*>(statement)) {
        auto out = std::make_unique<Defer>(source->range);
        if (source->value != nullptr) {
            out->value = lower_expression(source->value);
        }
        return out;
    }
    if (auto* source = dynamic_cast<ast::TestDecl*>(statement)) {
        auto out = std::make_unique<TestDecl>(source->range);
        out->name = source->name;
        out->body = lower_statements(source->body);
        return out;
    }
    if (auto* source = dynamic_cast<ast::Assert*>(statement)) {
        auto out = std::make_unique<Assert>(source->range);
        out->value = lower_expression(source->value);
        return out;
    }
    if (auto* source = dynamic_cast<ast::Import*>(statement)) {
        auto out = std::make_unique<Import>(source->range);
        out->module = source->module;
        return out;
    }
    if (auto* source = dynamic_cast<ast::Export*>(statement)) {
        auto out = std::make_unique<Export>(source->range);
        out->name = source->name;
        return out;
    }
    if (auto* source = dynamic_cast<ast::FuncDecl*>(statement)) {
        auto out = std::make_unique<FuncDecl>(source->range);
        out->name = source->name;
        out->receiver = source->receiver;
        out->type_params = source->type_params;
        out->params = lower_params(source->params);
        out->return_type = source->return_type;
        out->body = lower_statements(source->body);
        out->c_name = source->c_name;
        return out;
    }
    if (auto* source = dynamic_cast<ast::StructDecl*>(statement)) {
        auto out = std::make_unique<StructDecl>(source->range);
        out->name = source->name;
        out->fields = lower_fields(source->fields);
        return out;
    }
    if (auto* source = dynamic_cast<ast::Return*>(statement)) {
        auto out = std::make_unique<Return>(source->range);
        out->value = lower_expression(source->value);
        return out;
    }
    if (auto* source = dynamic_cast<ast::If*>(statement)) {
        auto out = std::make_unique<If>(source->range);
        out->condition = lower_expression(source->condition);
        out->then_block = lower_statements(source->then_block);
        out->else_block = lower_statements(source->else_block);
        return out;
    }
    if (auto* source = dynamic_cast<ast::While*>(statement)) {
        auto out = std::make_unique<While>(source->range);
        out->condition = lower_expression(source->condition);
        out->body = lower_statements(source->body);
        return out;
    }
    if (auto* source = dynamic_cast<ast::Repeat*>(statement)) {
        auto out = std::make_unique<Repeat>(source->range);
        out->count = lower_expression(source->count);
        out->body = lower_statements(source->body);
        return out;
    }
    if (auto* source = dynamic_cast<ast::For*>(statement)) {
        auto out = std::make_unique<For>(source->range);
        out->name = source->name;
        out->iterable = lower_expression(source->iterable);
        out->body = lower_statements(source->body);
        return out;
    }
    if (auto* source = dynamic_cast<ast::Break*>(statement)) {
        return std::make_unique<Break>(source->range);
    }
    if (auto* source = dynamic_cast<ast::Continue*>(statement)) {
        return std::make_unique<Continue>(source->range);
    }
    return nullptr;
}

Program lower_program_only(ast::Program& program, std::string module_name) {
    Program lowered;
    lowered.module_name = std::move(module_name);
    lowered.statements = lower_statements(program.statements);
    return lowered;
}

}  // namespace

LoweredProgram lower_program(ast::Program& program, const std::map<std::string, std::unique_ptr<sema::Module>>& modules) {
    LoweredProgram lowered;
    for (const auto& item : modules) {
        lowered.modules.emplace(item.first, std::make_unique<Program>(lower_program_only(*item.second->parsed.program, item.first)));
    }
    lowered.program = lower_program_only(program, "");
    return lowered;
}

}  // namespace walk::ir
