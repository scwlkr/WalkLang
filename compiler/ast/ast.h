#pragma once

#include "support/source_file.h"

#include <memory>
#include <string>
#include <vector>

namespace walk::ast {

enum class TypeKind {
    Invalid,
    Void,
    Int,
    Float,
    Bool,
    String,
    Null,
    Array,
    Map,
    Function,
    Struct,
    Generic,
};

struct Type {
    TypeKind kind = TypeKind::Invalid;
    std::string name;
    bool nullable = false;
    std::shared_ptr<Type> key;
    std::shared_ptr<Type> elem;
    std::vector<Type> params;
    std::shared_ptr<Type> return_type;

    [[nodiscard]] std::string to_string() const;
};

[[nodiscard]] Type basic(TypeKind kind);
[[nodiscard]] Type array_of(Type elem);
[[nodiscard]] Type map_of(Type key, Type value);
[[nodiscard]] Type function_type(std::vector<Type> params, Type return_type);
[[nodiscard]] Type struct_type(std::string name);
[[nodiscard]] Type generic_type(std::string name);

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

enum class LiteralKind {
    Int,
    Float,
    Bool,
    String,
    Null,
};

struct Node {
    explicit Node(SourceRange range);
    virtual ~Node();

    SourceRange range;
};

struct Statement : Node {
    Statement(StatementKind kind, SourceRange range);
    ~Statement() override;

    StatementKind kind;
};

struct Expression : Node {
    Expression(ExpressionKind kind, SourceRange range);
    ~Expression() override;

    ExpressionKind kind;
    Type type;
};

struct Param {
    std::string name;
    Type type;
};

struct VarDecl : Statement {
    explicit VarDecl(SourceRange range);

    std::string name;
    Type annotation;
    Expression* value = nullptr;
    bool mutable_binding = false;
};

struct Assignment : Statement {
    explicit Assignment(SourceRange range);

    Expression* target = nullptr;
    Expression* value = nullptr;
};

struct Out : Statement {
    explicit Out(SourceRange range);

    Expression* value = nullptr;
};

struct Do : Statement {
    explicit Do(SourceRange range);

    Expression* value = nullptr;
};

struct Defer : Statement {
    explicit Defer(SourceRange range);

    Expression* value = nullptr;
};

struct TestDecl : Statement {
    explicit TestDecl(SourceRange range);

    std::string name;
    std::vector<Statement*> body;
};

struct Assert : Statement {
    explicit Assert(SourceRange range);

    Expression* value = nullptr;
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
    std::vector<Statement*> body;
    std::string c_name;
};

struct StructField {
    SourceRange range;
    std::string name;
    Type type;
};

struct StructDecl : Statement {
    explicit StructDecl(SourceRange range);

    std::string name;
    std::vector<StructField> fields;
};

struct Return : Statement {
    explicit Return(SourceRange range);

    Expression* value = nullptr;
};

struct If : Statement {
    explicit If(SourceRange range);

    Expression* condition = nullptr;
    std::vector<Statement*> then_block;
    std::vector<Statement*> else_block;
};

struct While : Statement {
    explicit While(SourceRange range);

    Expression* condition = nullptr;
    std::vector<Statement*> body;
};

struct Repeat : Statement {
    explicit Repeat(SourceRange range);

    Expression* count = nullptr;
    std::vector<Statement*> body;
};

struct For : Statement {
    explicit For(SourceRange range);

    std::string name;
    Expression* iterable = nullptr;
    std::vector<Statement*> body;
};

struct Break : Statement {
    explicit Break(SourceRange range);
};

struct Continue : Statement {
    explicit Continue(SourceRange range);
};

struct Literal : Expression {
    explicit Literal(SourceRange range);

    LiteralKind literal_kind = LiteralKind::String;
    std::string value;
};

struct InterpolatedStringPart {
    std::string literal;
    Expression* expression = nullptr;
};

struct InterpolatedString : Expression {
    explicit InterpolatedString(SourceRange range);

    std::vector<InterpolatedStringPart> parts;
};

struct Name : Expression {
    explicit Name(SourceRange range);

    std::string identifier;
};

struct Prefix : Expression {
    explicit Prefix(SourceRange range);

    std::string op;
    std::vector<Expression*> args;
};

struct Call : Expression {
    explicit Call(SourceRange range);

    std::string callee;
    Expression* receiver = nullptr;
    std::string method;
    std::vector<Expression*> args;
    std::vector<Type> type_args;
};

struct Input : Expression {
    explicit Input(SourceRange range);

    Expression* prompt = nullptr;
};

struct ArrayLiteral : Expression {
    explicit ArrayLiteral(SourceRange range);

    std::vector<Expression*> elements;
};

struct Index : Expression {
    explicit Index(SourceRange range);

    Expression* target = nullptr;
    Expression* index = nullptr;
};

struct FieldAccess : Expression {
    explicit FieldAccess(SourceRange range);

    Expression* target = nullptr;
    std::string field;
};

struct Program {
    std::vector<Statement*> statements;
};

}  // namespace walk::ast
