#include "ir/ir.h"

#include <utility>

namespace walk::ir {

Statement::Statement(StatementKind statement_kind, SourceRange statement_range)
    : kind(statement_kind), range(std::move(statement_range)) {}
Statement::~Statement() = default;

Expression::Expression(ExpressionKind expression_kind, SourceRange expression_range, Type expression_type)
    : kind(expression_kind), range(std::move(expression_range)), type(std::move(expression_type)) {}
Expression::~Expression() = default;

VarDecl::VarDecl(SourceRange range) : Statement(StatementKind::VarDecl, std::move(range)) {}
Assignment::Assignment(SourceRange range) : Statement(StatementKind::Assignment, std::move(range)) {}
Out::Out(SourceRange range) : Statement(StatementKind::Out, std::move(range)) {}
Do::Do(SourceRange range) : Statement(StatementKind::Do, std::move(range)) {}
Defer::Defer(SourceRange range) : Statement(StatementKind::Defer, std::move(range)) {}
TestDecl::TestDecl(SourceRange range) : Statement(StatementKind::TestDecl, std::move(range)) {}
Assert::Assert(SourceRange range) : Statement(StatementKind::Assert, std::move(range)) {}
Import::Import(SourceRange range) : Statement(StatementKind::Import, std::move(range)) {}
Export::Export(SourceRange range) : Statement(StatementKind::Export, std::move(range)) {}
FuncDecl::FuncDecl(SourceRange range) : Statement(StatementKind::FuncDecl, std::move(range)) {}
StructDecl::StructDecl(SourceRange range) : Statement(StatementKind::StructDecl, std::move(range)) {}
Return::Return(SourceRange range) : Statement(StatementKind::Return, std::move(range)) {}
If::If(SourceRange range) : Statement(StatementKind::If, std::move(range)) {}
While::While(SourceRange range) : Statement(StatementKind::While, std::move(range)) {}
Repeat::Repeat(SourceRange range) : Statement(StatementKind::Repeat, std::move(range)) {}
For::For(SourceRange range) : Statement(StatementKind::For, std::move(range)) {}
Break::Break(SourceRange range) : Statement(StatementKind::Break, std::move(range)) {}
Continue::Continue(SourceRange range) : Statement(StatementKind::Continue, std::move(range)) {}

Literal::Literal(SourceRange range, Type type) : Expression(ExpressionKind::Literal, std::move(range), std::move(type)) {}
InterpolatedString::InterpolatedString(SourceRange range, Type type) : Expression(ExpressionKind::InterpolatedString, std::move(range), std::move(type)) {}
Name::Name(SourceRange range, Type type) : Expression(ExpressionKind::Name, std::move(range), std::move(type)) {}
Prefix::Prefix(SourceRange range, Type type) : Expression(ExpressionKind::Prefix, std::move(range), std::move(type)) {}
Call::Call(SourceRange range, Type type) : Expression(ExpressionKind::Call, std::move(range), std::move(type)) {}
Input::Input(SourceRange range, Type type) : Expression(ExpressionKind::Input, std::move(range), std::move(type)) {}
ArrayLiteral::ArrayLiteral(SourceRange range, Type type) : Expression(ExpressionKind::ArrayLiteral, std::move(range), std::move(type)) {}
Index::Index(SourceRange range, Type type) : Expression(ExpressionKind::Index, std::move(range), std::move(type)) {}
FieldAccess::FieldAccess(SourceRange range, Type type) : Expression(ExpressionKind::FieldAccess, std::move(range), std::move(type)) {}

namespace {

std::vector<std::unique_ptr<Expression>> clone_expressions(const std::vector<std::unique_ptr<Expression>>& expressions) {
    std::vector<std::unique_ptr<Expression>> result;
    result.reserve(expressions.size());
    for (const auto& expression : expressions) {
        result.push_back(clone_expression(*expression));
    }
    return result;
}

std::vector<std::unique_ptr<Statement>> clone_statements(const std::vector<std::unique_ptr<Statement>>& statements) {
    std::vector<std::unique_ptr<Statement>> result;
    result.reserve(statements.size());
    for (const auto& statement : statements) {
        result.push_back(clone_statement(*statement));
    }
    return result;
}

}  // namespace

std::unique_ptr<Expression> clone_expression(const Expression& expression) {
    switch (expression.kind) {
    case ExpressionKind::Literal: {
        const auto& source = static_cast<const Literal&>(expression);
        auto out = std::make_unique<Literal>(source.range, source.type);
        out->literal_kind = source.literal_kind;
        out->value = source.value;
        return out;
    }
    case ExpressionKind::InterpolatedString: {
        const auto& source = static_cast<const InterpolatedString&>(expression);
        auto out = std::make_unique<InterpolatedString>(source.range, source.type);
        out->parts.reserve(source.parts.size());
        for (const InterpolatedStringPart& part : source.parts) {
            InterpolatedStringPart clone;
            clone.literal = part.literal;
            if (part.expression != nullptr) {
                clone.expression = clone_expression(*part.expression);
            }
            out->parts.push_back(std::move(clone));
        }
        return out;
    }
    case ExpressionKind::Name: {
        const auto& source = static_cast<const Name&>(expression);
        auto out = std::make_unique<Name>(source.range, source.type);
        out->identifier = source.identifier;
        return out;
    }
    case ExpressionKind::Prefix: {
        const auto& source = static_cast<const Prefix&>(expression);
        auto out = std::make_unique<Prefix>(source.range, source.type);
        out->op = source.op;
        out->args = clone_expressions(source.args);
        return out;
    }
    case ExpressionKind::Call: {
        const auto& source = static_cast<const Call&>(expression);
        auto out = std::make_unique<Call>(source.range, source.type);
        out->callee = source.callee;
        if (source.receiver != nullptr) {
            out->receiver = clone_expression(*source.receiver);
        }
        out->method = source.method;
        out->args = clone_expressions(source.args);
        out->type_args = source.type_args;
        return out;
    }
    case ExpressionKind::Input: {
        const auto& source = static_cast<const Input&>(expression);
        auto out = std::make_unique<Input>(source.range, source.type);
        if (source.prompt != nullptr) {
            out->prompt = clone_expression(*source.prompt);
        }
        return out;
    }
    case ExpressionKind::ArrayLiteral: {
        const auto& source = static_cast<const ArrayLiteral&>(expression);
        auto out = std::make_unique<ArrayLiteral>(source.range, source.type);
        out->elements = clone_expressions(source.elements);
        return out;
    }
    case ExpressionKind::Index: {
        const auto& source = static_cast<const Index&>(expression);
        auto out = std::make_unique<Index>(source.range, source.type);
        out->target = clone_expression(*source.target);
        out->index = clone_expression(*source.index);
        return out;
    }
    case ExpressionKind::FieldAccess: {
        const auto& source = static_cast<const FieldAccess&>(expression);
        auto out = std::make_unique<FieldAccess>(source.range, source.type);
        out->target = clone_expression(*source.target);
        out->field = source.field;
        return out;
    }
    }
    return nullptr;
}

std::unique_ptr<Statement> clone_statement(const Statement& statement) {
    switch (statement.kind) {
    case StatementKind::VarDecl: {
        const auto& source = static_cast<const VarDecl&>(statement);
        auto out = std::make_unique<VarDecl>(source.range);
        out->name = source.name;
        out->annotation = source.annotation;
        out->value = clone_expression(*source.value);
        out->mutable_binding = source.mutable_binding;
        return out;
    }
    case StatementKind::Assignment: {
        const auto& source = static_cast<const Assignment&>(statement);
        auto out = std::make_unique<Assignment>(source.range);
        out->target = clone_expression(*source.target);
        out->value = clone_expression(*source.value);
        return out;
    }
    case StatementKind::Out: {
        const auto& source = static_cast<const Out&>(statement);
        auto out = std::make_unique<Out>(source.range);
        out->value = clone_expression(*source.value);
        return out;
    }
    case StatementKind::Do: {
        const auto& source = static_cast<const Do&>(statement);
        auto out = std::make_unique<Do>(source.range);
        out->value = clone_expression(*source.value);
        return out;
    }
    case StatementKind::Defer: {
        const auto& source = static_cast<const Defer&>(statement);
        auto out = std::make_unique<Defer>(source.range);
        if (source.value != nullptr) {
            out->value = clone_expression(*source.value);
        }
        return out;
    }
    case StatementKind::TestDecl: {
        const auto& source = static_cast<const TestDecl&>(statement);
        auto out = std::make_unique<TestDecl>(source.range);
        out->name = source.name;
        out->body = clone_statements(source.body);
        return out;
    }
    case StatementKind::Assert: {
        const auto& source = static_cast<const Assert&>(statement);
        auto out = std::make_unique<Assert>(source.range);
        out->value = clone_expression(*source.value);
        return out;
    }
    case StatementKind::Import: {
        const auto& source = static_cast<const Import&>(statement);
        auto out = std::make_unique<Import>(source.range);
        out->module = source.module;
        return out;
    }
    case StatementKind::Export: {
        const auto& source = static_cast<const Export&>(statement);
        auto out = std::make_unique<Export>(source.range);
        out->name = source.name;
        return out;
    }
    case StatementKind::FuncDecl:
        return clone_function(static_cast<const FuncDecl&>(statement));
    case StatementKind::StructDecl: {
        const auto& source = static_cast<const StructDecl&>(statement);
        auto out = std::make_unique<StructDecl>(source.range);
        out->name = source.name;
        out->fields = source.fields;
        return out;
    }
    case StatementKind::Return: {
        const auto& source = static_cast<const Return&>(statement);
        auto out = std::make_unique<Return>(source.range);
        out->value = clone_expression(*source.value);
        return out;
    }
    case StatementKind::If: {
        const auto& source = static_cast<const If&>(statement);
        auto out = std::make_unique<If>(source.range);
        out->condition = clone_expression(*source.condition);
        out->then_block = clone_statements(source.then_block);
        out->else_block = clone_statements(source.else_block);
        return out;
    }
    case StatementKind::While: {
        const auto& source = static_cast<const While&>(statement);
        auto out = std::make_unique<While>(source.range);
        out->condition = clone_expression(*source.condition);
        out->body = clone_statements(source.body);
        return out;
    }
    case StatementKind::Repeat: {
        const auto& source = static_cast<const Repeat&>(statement);
        auto out = std::make_unique<Repeat>(source.range);
        out->count = clone_expression(*source.count);
        out->body = clone_statements(source.body);
        return out;
    }
    case StatementKind::For: {
        const auto& source = static_cast<const For&>(statement);
        auto out = std::make_unique<For>(source.range);
        out->name = source.name;
        out->iterable = clone_expression(*source.iterable);
        out->body = clone_statements(source.body);
        return out;
    }
    case StatementKind::Break:
        return std::make_unique<Break>(statement.range);
    case StatementKind::Continue:
        return std::make_unique<Continue>(statement.range);
    }
    return nullptr;
}

std::unique_ptr<FuncDecl> clone_function(const FuncDecl& function) {
    auto out = std::make_unique<FuncDecl>(function.range);
    out->name = function.name;
    out->receiver = function.receiver;
    out->type_params = function.type_params;
    out->params = function.params;
    out->return_type = function.return_type;
    out->body = clone_statements(function.body);
    out->c_name = function.c_name;
    return out;
}

}  // namespace walk::ir
