#include "ast/ast.h"

#include <sstream>
#include <utility>

namespace walk::ast {

namespace {

std::string type_kind_name(TypeKind kind) {
    switch (kind) {
    case TypeKind::Invalid:
        return "";
    case TypeKind::Void:
        return "void";
    case TypeKind::Int:
        return "int";
    case TypeKind::Float:
        return "float";
    case TypeKind::Bool:
        return "bool";
    case TypeKind::String:
        return "string";
    case TypeKind::Null:
        return "null";
    case TypeKind::Array:
        return "array";
    case TypeKind::Function:
        return "func";
    case TypeKind::Struct:
    case TypeKind::Generic:
        return "";
    }
    return "";
}

}  // namespace

std::string Type::to_string() const {
    std::ostringstream output;
    switch (kind) {
    case TypeKind::Array:
        output << "array[";
        if (elem) {
            output << elem->to_string();
        } else {
            output << "?";
        }
        output << "]";
        break;
    case TypeKind::Function:
        output << "func(";
        for (std::size_t index = 0; index < params.size(); ++index) {
            if (index != 0) {
                output << ", ";
            }
            output << params[index].to_string();
        }
        output << ") ";
        if (return_type) {
            output << return_type->to_string();
        } else {
            output << "void";
        }
        break;
    case TypeKind::Struct:
    case TypeKind::Generic:
        output << name;
        break;
    default:
        output << type_kind_name(kind);
        break;
    }
    if (nullable) {
        output << "?";
    }
    return output.str();
}

Type basic(TypeKind kind) {
    Type type;
    type.kind = kind;
    return type;
}

Type array_of(Type elem) {
    Type type;
    type.kind = TypeKind::Array;
    type.elem = std::make_shared<Type>(std::move(elem));
    return type;
}

Type function_type(std::vector<Type> params, Type return_type) {
    Type type;
    type.kind = TypeKind::Function;
    type.params = std::move(params);
    type.return_type = std::make_shared<Type>(std::move(return_type));
    return type;
}

Type struct_type(std::string name) {
    Type type;
    type.kind = TypeKind::Struct;
    type.name = std::move(name);
    return type;
}

Type generic_type(std::string name) {
    Type type;
    type.kind = TypeKind::Generic;
    type.name = std::move(name);
    return type;
}

Node::Node(SourceRange node_range) : range(std::move(node_range)) {}
Node::~Node() = default;

Statement::Statement(StatementKind statement_kind, SourceRange statement_range)
    : Node(std::move(statement_range)), kind(statement_kind) {}
Statement::~Statement() = default;

Expression::Expression(ExpressionKind expression_kind, SourceRange expression_range)
    : Node(std::move(expression_range)), kind(expression_kind) {}
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

Literal::Literal(SourceRange range) : Expression(ExpressionKind::Literal, std::move(range)) {}
InterpolatedString::InterpolatedString(SourceRange range) : Expression(ExpressionKind::InterpolatedString, std::move(range)) {}
Name::Name(SourceRange range) : Expression(ExpressionKind::Name, std::move(range)) {}
Prefix::Prefix(SourceRange range) : Expression(ExpressionKind::Prefix, std::move(range)) {}
Call::Call(SourceRange range) : Expression(ExpressionKind::Call, std::move(range)) {}
Input::Input(SourceRange range) : Expression(ExpressionKind::Input, std::move(range)) {}
ArrayLiteral::ArrayLiteral(SourceRange range) : Expression(ExpressionKind::ArrayLiteral, std::move(range)) {}
Index::Index(SourceRange range) : Expression(ExpressionKind::Index, std::move(range)) {}
FieldAccess::FieldAccess(SourceRange range) : Expression(ExpressionKind::FieldAccess, std::move(range)) {}

}  // namespace walk::ast
