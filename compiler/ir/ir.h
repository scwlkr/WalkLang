#pragma once

#include "ast/ast.h"
#include "support/source_file.h"

#include <memory>
#include <string>
#include <vector>

namespace walk::ir {

using Type = ast::Type;

enum class StatementKind {
    VarDecl,
    Assignment,
    Out,
    Do,
    Defer,
    TestDecl,
    Assert,
    Import,
    Export,
    FuncDecl,
    StructDecl,
    Return,
    If,
    While,
    Repeat,
    For,
    Break,
    Continue,
};

enum class ExpressionKind {
    Literal,
    InterpolatedString,
    Name,
    Prefix,
    Call,
    Input,
    ArrayLiteral,
    Index,
    FieldAccess,
};

struct Statement {
    Statement(StatementKind kind, SourceRange range);
    virtual ~Statement();

    StatementKind kind;
    SourceRange range;
};

struct Expression {
    Expression(ExpressionKind kind, SourceRange range, Type type);
    virtual ~Expression();

    ExpressionKind kind;
    SourceRange range;
    Type type;
};

struct Param {
    std::string name;
    Type type;
};

struct StructField {
    SourceRange range;
    std::string name;
    Type type;
};

struct VarDecl : Statement {
    explicit VarDecl(SourceRange range);

    std::string name;
    Type annotation;
    std::unique_ptr<Expression> value;
    bool mutable_binding = false;
};

struct Assignment : Statement {
    explicit Assignment(SourceRange range);

    std::unique_ptr<Expression> target;
    std::unique_ptr<Expression> value;
};

struct Out : Statement {
    explicit Out(SourceRange range);

    std::unique_ptr<Expression> value;
};

struct Do : Statement {
    explicit Do(SourceRange range);

    std::unique_ptr<Expression> value;
};

struct Defer : Statement {
    explicit Defer(SourceRange range);

    std::unique_ptr<Expression> value;
};

struct TestDecl : Statement {
    explicit TestDecl(SourceRange range);

    std::string name;
    std::vector<std::unique_ptr<Statement>> body;
};

struct Assert : Statement {
    explicit Assert(SourceRange range);

    std::unique_ptr<Expression> value;
};

struct Import : Statement {
    explicit Import(SourceRange range);

    std::string module;
};

struct Export : Statement {
    explicit Export(SourceRange range);

    std::string name;
};

struct FuncDecl : Statement {
    explicit FuncDecl(SourceRange range);

    std::string name;
    std::string receiver;
    std::vector<std::string> type_params;
    std::vector<Param> params;
    Type return_type;
    std::vector<std::unique_ptr<Statement>> body;
    std::string c_name;
};

struct StructDecl : Statement {
    explicit StructDecl(SourceRange range);

    std::string name;
    std::vector<StructField> fields;
};

struct Return : Statement {
    explicit Return(SourceRange range);

    std::unique_ptr<Expression> value;
};

struct If : Statement {
    explicit If(SourceRange range);

    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Statement>> then_block;
    std::vector<std::unique_ptr<Statement>> else_block;
};

struct While : Statement {
    explicit While(SourceRange range);

    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Statement>> body;
};

struct Repeat : Statement {
    explicit Repeat(SourceRange range);

    std::unique_ptr<Expression> count;
    std::vector<std::unique_ptr<Statement>> body;
};

struct For : Statement {
    explicit For(SourceRange range);

    std::string name;
    std::unique_ptr<Expression> iterable;
    std::vector<std::unique_ptr<Statement>> body;
};

struct Break : Statement {
    explicit Break(SourceRange range);
};

struct Continue : Statement {
    explicit Continue(SourceRange range);
};

struct Literal : Expression {
    Literal(SourceRange range, Type type);

    ast::LiteralKind literal_kind = ast::LiteralKind::String;
    std::string value;
};

struct InterpolatedStringPart {
    std::string literal;
    std::unique_ptr<Expression> expression;
};

struct InterpolatedString : Expression {
    InterpolatedString(SourceRange range, Type type);

    std::vector<InterpolatedStringPart> parts;
};

struct Name : Expression {
    Name(SourceRange range, Type type);

    std::string identifier;
};

struct Prefix : Expression {
    Prefix(SourceRange range, Type type);

    std::string op;
    std::vector<std::unique_ptr<Expression>> args;
};

struct Call : Expression {
    Call(SourceRange range, Type type);

    std::string callee;
    std::unique_ptr<Expression> receiver;
    std::string method;
    std::vector<std::unique_ptr<Expression>> args;
    std::vector<Type> type_args;
};

struct Input : Expression {
    Input(SourceRange range, Type type);

    std::unique_ptr<Expression> prompt;
};

struct ArrayLiteral : Expression {
    ArrayLiteral(SourceRange range, Type type);

    std::vector<std::unique_ptr<Expression>> elements;
};

struct Index : Expression {
    Index(SourceRange range, Type type);

    std::unique_ptr<Expression> target;
    std::unique_ptr<Expression> index;
};

struct FieldAccess : Expression {
    FieldAccess(SourceRange range, Type type);

    std::unique_ptr<Expression> target;
    std::string field;
};

struct Program {
    std::string module_name;
    std::vector<std::unique_ptr<Statement>> statements;
};

std::unique_ptr<Expression> clone_expression(const Expression& expression);
std::unique_ptr<Statement> clone_statement(const Statement& statement);
std::unique_ptr<FuncDecl> clone_function(const FuncDecl& function);

}  // namespace walk::ir
