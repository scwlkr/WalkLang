#include "parse/parser.h"

#include "lex/lexer.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace walk::parse {
namespace {

constexpr const char* kSyntaxDiagnostic = "W1001";

struct LineNode {
    lex::Line line;
    std::vector<LineNode> children;
};

struct StatementParse {
    ast::Statement* statement = nullptr;
    std::size_t next = 0;
};

std::string quote(std::string_view value) {
    return "\"" + std::string(value) + "\"";
}

bool is_stop(const std::vector<std::string>& stop, const std::string& value) {
    return std::find(stop.begin(), stop.end(), value) != stop.end();
}

std::optional<std::pair<int, int>> prefix_arity(const std::string& op) {
    if (op == "+" || op == "*" || op == "and" || op == "or") {
        return std::pair<int, int>{2, -1};
    }
    if (op == "-" || op == "/" || op == "^" || op == ">" || op == "<" || op == ">=" || op == "<=" || op == "==" || op == "!=") {
        return std::pair<int, int>{2, 2};
    }
    if (op == "not") {
        return std::pair<int, int>{1, 1};
    }
    return std::nullopt;
}

bool is_reserved_word(const std::string& value) {
    static const std::set<std::string> reserved = {
        "var", "const", "out", "if", "else", "while", "for", "repeat", "break", "continue", "func", "return",
        "imp", "exp", "true", "false", "null", "and", "or", "not", "in", "test", "assert", "struct", "defer",
    };
    return reserved.count(value) != 0;
}

bool is_close_only(const std::vector<lex::Token>& tokens) {
    return tokens.size() == 1 && (tokens[0].value == ")" || tokens[0].value == "]");
}

std::optional<std::string> command_keyword(const std::vector<lex::Token>& tokens) {
    if (tokens.size() >= 2 && tokens[0].kind == lex::TokenKind::Name && tokens[1].value == ":") {
        return tokens[0].value;
    }
    return std::nullopt;
}

bool is_command(const std::vector<lex::Token>& tokens, const std::string& keyword) {
    const std::optional<std::string> found = command_keyword(tokens);
    return found && *found == keyword;
}

SourceRange zero_range(const SourceFile& source) {
    return source.range_for_offsets(0, 0);
}

SourceRange interpolation_range(const lex::Token& token, std::size_t value_index) {
    SourceRange range = token.range;
    range.start_offset = token.range.start_offset + 1 + value_index;
    range.end_offset = range.start_offset + 1;
    range.start.column = token.range.start.column + 1 + value_index;
    range.end = range.start;
    range.end.column += 1;
    return range;
}

}  // namespace

class Parser {
public:
    explicit Parser(const SourceFile& source) : source_(source) {}

    bool build_nodes(const std::vector<lex::Line>& lines, std::size_t index, std::size_t indent, std::vector<LineNode>& nodes, std::size_t& next);
    std::vector<ast::Statement*> parse_block(const std::vector<LineNode>& nodes);
    StatementParse parse_statement_at(const std::vector<LineNode>& nodes, std::size_t index);
    void add_error(const SourceRange& range, std::string message);

    [[nodiscard]] bool failed() const {
        return diagnostics_.has_errors();
    }

    [[nodiscard]] DiagnosticSet take_diagnostics() {
        diagnostics_.sort();
        return std::move(diagnostics_);
    }

    [[nodiscard]] ast::Arena take_arena() {
        return std::move(arena_);
    }

private:
    class Cursor {
    public:
        Cursor(Parser& parser, std::vector<lex::Token> tokens) : parser_(parser), tokens_(std::move(tokens)) {}

        const lex::Token* peek() const;
        bool peek_kind(lex::TokenKind kind) const;
        const lex::Token& advance();
        bool match_name(const std::string& value);
        bool expect_symbol(const std::string& value);
        const lex::Token* expect_name(const std::string& label);
        bool expect_end();
        SourceRange current_range() const;
        ast::Expression* parse_expression(const std::vector<std::string>& stop = {});
        ast::Type parse_type();
        std::vector<ast::Expression*> parse_call_args();
        std::vector<std::string> parse_type_params();

        std::set<std::string> type_params;

    private:
        ast::Expression* parse_prefix(const std::vector<std::string>& stop);
        ast::Expression* parse_postfix(const std::vector<std::string>& stop);
        ast::Expression* parse_atom(const std::vector<std::string>& stop);

        Parser& parser_;
        std::vector<lex::Token> tokens_;
        std::size_t index_ = 0;
    };

    bool reject_unexpected_children(const std::vector<LineNode>& children);
    std::string parse_bare_name(const std::vector<lex::Token>& tokens, const SourceRange& range);
    std::string parse_import_name(const std::vector<lex::Token>& tokens, const SourceRange& range);
    ast::Statement* parse_defer(const std::vector<lex::Token>& tokens, const std::vector<LineNode>& children, const SourceRange& range);
    ast::Statement* parse_var_decl(const std::vector<lex::Token>& tokens, const std::vector<LineNode>& children, const SourceRange& range, bool mutable_binding);
    ast::Statement* parse_test_decl(const std::vector<lex::Token>& tokens, const std::vector<LineNode>& children, const SourceRange& range);
    ast::Statement* parse_func_decl(const std::vector<lex::Token>& tokens, const std::vector<LineNode>& children, const SourceRange& range);
    ast::Statement* parse_struct_decl(const std::vector<lex::Token>& tokens, const std::vector<LineNode>& children, const SourceRange& range);
    ast::Statement* parse_for(const std::vector<lex::Token>& tokens, const std::vector<LineNode>& children, const SourceRange& range);
    ast::Expression* parse_command_expression(const std::vector<lex::Token>& tokens, const std::vector<LineNode>& children, const SourceRange& range);
    ast::Expression* parse_expression_block(const std::vector<LineNode>& children, const SourceRange& range);
    std::vector<ast::Expression*> parse_expression_list(const std::vector<LineNode>& children);
    ast::Expression* parse_expression_node(const LineNode& node);
    ast::Expression* parse_expression_tokens(const std::vector<lex::Token>& tokens, const SourceRange& range);
    ast::Expression* parse_string_expression(const lex::Token& token);
    std::vector<ast::InterpolatedStringPart> parse_interpolated_string_parts(const lex::Token& token, bool& interpolated);
    ast::Expression* parse_interpolated_expression(const std::string& source, const SourceRange& range);
    ast::Call* call_from_target(ast::Expression* target, std::vector<ast::Expression*> args, const SourceRange& invalid_range);
    std::optional<std::string> expression_callee(ast::Expression* expression) const;
    SourceRange callee_range(ast::Expression* expression) const;
    int find_assignment(const std::vector<lex::Token>& tokens) const;
    int find_interpolation_end(const std::string& value, std::size_t start) const;

    const SourceFile& source_;
    DiagnosticSet diagnostics_;
    ast::Arena arena_;
    int loop_depth_ = 0;
};

bool Parser::build_nodes(const std::vector<lex::Line>& lines, std::size_t index, std::size_t indent, std::vector<LineNode>& nodes, std::size_t& next) {
    while (index < lines.size()) {
        const lex::Line& line = lines[index];
        if (line.indent < indent) {
            next = index;
            return true;
        }
        if (line.indent > indent) {
            add_error(line.range, "syntax error: unexpected indentation");
            next = index;
            return false;
        }

        LineNode node{line, {}};
        ++index;
        if (index < lines.size() && lines[index].indent > indent) {
            std::size_t child_next = index;
            if (!build_nodes(lines, index, lines[index].indent, node.children, child_next)) {
                next = child_next;
                return false;
            }
            index = child_next;
        }
        nodes.push_back(std::move(node));
    }
    next = index;
    return true;
}

std::vector<ast::Statement*> Parser::parse_block(const std::vector<LineNode>& nodes) {
    std::vector<ast::Statement*> statements;
    for (std::size_t index = 0; index < nodes.size();) {
        if (is_close_only(nodes[index].line.tokens)) {
            ++index;
            continue;
        }
        StatementParse parsed = parse_statement_at(nodes, index);
        if (parsed.statement == nullptr) {
            return {};
        }
        statements.push_back(parsed.statement);
        index = parsed.next;
    }
    return statements;
}

StatementParse Parser::parse_statement_at(const std::vector<LineNode>& nodes, std::size_t index) {
    const LineNode& node = nodes[index];
    const std::vector<lex::Token>& tokens = node.line.tokens;
    const lex::Token& first = tokens.front();

    if (const std::optional<std::string> keyword = command_keyword(tokens)) {
        const std::vector<lex::Token> payload(tokens.begin() + 2, tokens.end());
        if (*keyword == "imp") {
            if (!reject_unexpected_children(node.children)) {
                return {};
            }
            std::string name = parse_import_name(payload, first.range);
            if (failed()) {
                return {};
            }
            ast::Import* statement = arena_.make<ast::Import>(first.range);
            statement->module = std::move(name);
            return {statement, index + 1};
        }
        if (*keyword == "exp") {
            if (!reject_unexpected_children(node.children)) {
                return {};
            }
            std::string name = parse_bare_name(payload, first.range);
            if (failed()) {
                return {};
            }
            ast::Export* statement = arena_.make<ast::Export>(first.range);
            statement->name = std::move(name);
            return {statement, index + 1};
        }
        if (*keyword == "var" || *keyword == "const") {
            ast::Statement* statement = parse_var_decl(payload, node.children, first.range, *keyword == "var");
            return {statement, statement == nullptr ? index : index + 1};
        }
        if (*keyword == "out") {
            ast::Expression* value = parse_command_expression(payload, node.children, first.range);
            if (value == nullptr) {
                return {};
            }
            ast::Out* statement = arena_.make<ast::Out>(first.range);
            statement->value = value;
            return {statement, index + 1};
        }
        if (*keyword == "do") {
            ast::Expression* value = parse_command_expression(payload, node.children, first.range);
            if (value == nullptr) {
                return {};
            }
            ast::Do* statement = arena_.make<ast::Do>(first.range);
            statement->value = value;
            return {statement, index + 1};
        }
        if (*keyword == "defer") {
            ast::Statement* statement = parse_defer(payload, node.children, first.range);
            return {statement, statement == nullptr ? index : index + 1};
        }
        if (*keyword == "test") {
            ast::Statement* statement = parse_test_decl(payload, node.children, first.range);
            return {statement, statement == nullptr ? index : index + 1};
        }
        if (*keyword == "assert") {
            ast::Expression* value = parse_command_expression(payload, node.children, first.range);
            if (value == nullptr) {
                return {};
            }
            ast::Assert* statement = arena_.make<ast::Assert>(first.range);
            statement->value = value;
            return {statement, index + 1};
        }
        if (*keyword == "func") {
            ast::Statement* statement = parse_func_decl(payload, node.children, first.range);
            return {statement, statement == nullptr ? index : index + 1};
        }
        if (*keyword == "struct") {
            ast::Statement* statement = parse_struct_decl(payload, node.children, first.range);
            return {statement, statement == nullptr ? index : index + 1};
        }
        if (*keyword == "return") {
            ast::Expression* value = parse_command_expression(payload, node.children, first.range);
            if (value == nullptr) {
                return {};
            }
            ast::Return* statement = arena_.make<ast::Return>(first.range);
            statement->value = value;
            return {statement, index + 1};
        }
        if (*keyword == "if") {
            ast::Expression* condition = parse_command_expression(payload, {}, first.range);
            if (condition == nullptr) {
                return {};
            }
            std::vector<ast::Statement*> then_block = parse_block(node.children);
            if (failed()) {
                return {};
            }
            std::vector<ast::Statement*> else_block;
            std::size_t next = index + 1;
            if (next < nodes.size() && is_command(nodes[next].line.tokens, "else")) {
                else_block = parse_block(nodes[next].children);
                if (failed()) {
                    return {};
                }
                ++next;
            }
            ast::If* statement = arena_.make<ast::If>(first.range);
            statement->condition = condition;
            statement->then_block = std::move(then_block);
            statement->else_block = std::move(else_block);
            return {statement, next};
        }
        if (*keyword == "else") {
            add_error(first.range, "syntax error: else without matching if");
            return {};
        }
        if (*keyword == "while") {
            ast::Expression* condition = parse_command_expression(payload, {}, first.range);
            if (condition == nullptr) {
                return {};
            }
            ++loop_depth_;
            std::vector<ast::Statement*> body = parse_block(node.children);
            --loop_depth_;
            if (failed()) {
                return {};
            }
            ast::While* statement = arena_.make<ast::While>(first.range);
            statement->condition = condition;
            statement->body = std::move(body);
            return {statement, index + 1};
        }
        if (*keyword == "repeat") {
            ast::Expression* count = parse_command_expression(payload, {}, first.range);
            if (count == nullptr) {
                return {};
            }
            ++loop_depth_;
            std::vector<ast::Statement*> body = parse_block(node.children);
            --loop_depth_;
            if (failed()) {
                return {};
            }
            ast::Repeat* statement = arena_.make<ast::Repeat>(first.range);
            statement->count = count;
            statement->body = std::move(body);
            return {statement, index + 1};
        }
        if (*keyword == "for") {
            ast::Statement* statement = parse_for(payload, node.children, first.range);
            return {statement, statement == nullptr ? index : index + 1};
        }
        if (*keyword == "break") {
            if (!payload.empty()) {
                add_error(first.range, "syntax error: break takes no value");
                return {};
            }
            if (!reject_unexpected_children(node.children)) {
                return {};
            }
            if (loop_depth_ == 0) {
                add_error(first.range, "syntax error: break outside loop");
                return {};
            }
            return {arena_.make<ast::Break>(first.range), index + 1};
        }
        if (*keyword == "continue") {
            if (!payload.empty()) {
                add_error(first.range, "syntax error: continue takes no value");
                return {};
            }
            if (!reject_unexpected_children(node.children)) {
                return {};
            }
            if (loop_depth_ == 0) {
                add_error(first.range, "syntax error: continue outside loop");
                return {};
            }
            return {arena_.make<ast::Continue>(first.range), index + 1};
        }
    }

    const int assignment = find_assignment(tokens);
    if (assignment >= 0) {
        std::vector<lex::Token> target_tokens(tokens.begin(), tokens.begin() + assignment);
        std::vector<lex::Token> value_tokens(tokens.begin() + assignment + 1, tokens.end());
        ast::Expression* target = parse_expression_tokens(target_tokens, first.range);
        if (target == nullptr) {
            return {};
        }
        ast::Expression* value = parse_expression_tokens(value_tokens, first.range);
        if (value == nullptr) {
            return {};
        }
        ast::Assignment* statement = arena_.make<ast::Assignment>(first.range);
        statement->target = target;
        statement->value = value;
        return {statement, index + 1};
    }

    add_error(first.range, "syntax error: unsupported statement starting with " + quote(first.value));
    return {};
}

const lex::Token* Parser::Cursor::peek() const {
    if (index_ >= tokens_.size()) {
        return nullptr;
    }
    return &tokens_[index_];
}

bool Parser::Cursor::peek_kind(lex::TokenKind kind) const {
    const lex::Token* token = peek();
    return token != nullptr && token->kind == kind;
}

const lex::Token& Parser::Cursor::advance() {
    const lex::Token& token = tokens_[index_];
    ++index_;
    return token;
}

bool Parser::Cursor::match_name(const std::string& value) {
    const lex::Token* token = peek();
    if (token != nullptr && token->kind == lex::TokenKind::Name && token->value == value) {
        ++index_;
        return true;
    }
    return false;
}

bool Parser::Cursor::expect_symbol(const std::string& value) {
    const lex::Token* token = peek();
    if (token == nullptr || token->kind != lex::TokenKind::Symbol || token->value != value) {
        parser_.add_error(current_range(), "syntax error: expected " + quote(value));
        return false;
    }
    ++index_;
    return true;
}

const lex::Token* Parser::Cursor::expect_name(const std::string& label) {
    const lex::Token* token = peek();
    if (token == nullptr || token->kind != lex::TokenKind::Name) {
        parser_.add_error(current_range(), "syntax error: expected " + label);
        return nullptr;
    }
    ++index_;
    return token;
}

bool Parser::Cursor::expect_end() {
    const lex::Token* token = peek();
    if (token != nullptr) {
        parser_.add_error(token->range, "syntax error: unexpected token " + quote(token->value));
        return false;
    }
    return true;
}

SourceRange Parser::Cursor::current_range() const {
    const lex::Token* token = peek();
    if (token != nullptr) {
        return token->range;
    }
    if (!tokens_.empty()) {
        return tokens_.back().range;
    }
    return zero_range(parser_.source_);
}

ast::Expression* Parser::Cursor::parse_expression(const std::vector<std::string>& stop) {
    const lex::Token* token = peek();
    if (token == nullptr) {
        parser_.add_error(current_range(), "syntax error: expected expression");
        return nullptr;
    }
    if (is_stop(stop, token->value)) {
        parser_.add_error(token->range, "syntax error: expected expression");
        return nullptr;
    }
    if (prefix_arity(token->value)) {
        return parse_prefix(stop);
    }
    return parse_postfix(stop);
}

ast::Expression* Parser::Cursor::parse_prefix(const std::vector<std::string>& stop) {
    const lex::Token op = advance();
    const std::pair<int, int> arity = *prefix_arity(op.value);
    std::vector<ast::Expression*> args;
    while (peek() != nullptr) {
        if (is_stop(stop, peek()->value)) {
            break;
        }
        if (arity.second >= 0 && static_cast<int>(args.size()) >= arity.second) {
            break;
        }
        ast::Expression* arg = parse_expression(stop);
        if (arg == nullptr) {
            return nullptr;
        }
        args.push_back(arg);
    }
    if (static_cast<int>(args.size()) < arity.first) {
        parser_.add_error(op.range, "syntax error: operator " + quote(op.value) + " expects at least " + std::to_string(arity.first) + " operand(s)");
        return nullptr;
    }
    ast::Prefix* expression = parser_.arena_.make<ast::Prefix>(op.range);
    expression->op = op.value;
    expression->args = std::move(args);
    return expression;
}

ast::Expression* Parser::Cursor::parse_postfix(const std::vector<std::string>& stop) {
    ast::Expression* expression = parse_atom(stop);
    if (expression == nullptr) {
        return nullptr;
    }
    while (peek() != nullptr) {
        if (is_stop(stop, peek()->value)) {
            break;
        }
        if (peek()->value == "[") {
            const lex::Token open = advance();
            ast::Expression* index = parse_expression({"]"});
            if (index == nullptr || !expect_symbol("]")) {
                return nullptr;
            }
            ast::Index* indexed = parser_.arena_.make<ast::Index>(open.range);
            indexed->target = expression;
            indexed->index = index;
            expression = indexed;
            continue;
        }
        if (peek()->value == ".") {
            const lex::Token dot = advance();
            const lex::Token* field = expect_name("field name");
            if (field == nullptr) {
                return nullptr;
            }
            ast::FieldAccess* access = parser_.arena_.make<ast::FieldAccess>(dot.range);
            access->target = expression;
            access->field = field->value;
            expression = access;
            continue;
        }
        if (peek()->value == "(") {
            const lex::Token open = advance();
            std::vector<ast::Expression*> args = parse_call_args();
            if (parser_.failed()) {
                return nullptr;
            }
            ast::Call* call = parser_.call_from_target(expression, std::move(args), open.range);
            if (call == nullptr) {
                return nullptr;
            }
            expression = call;
            continue;
        }
        break;
    }
    return expression;
}

ast::Expression* Parser::Cursor::parse_atom(const std::vector<std::string>& stop) {
    (void)stop;
    const lex::Token token = advance();
    if (token.kind == lex::TokenKind::Number) {
        errno = 0;
        char* end = nullptr;
        std::strtod(token.value.c_str(), &end);
        if (errno != 0 || end == token.value.c_str() || *end != '\0') {
            parser_.add_error(token.range, "syntax error: invalid number " + quote(token.value));
            return nullptr;
        }
        ast::Literal* literal = parser_.arena_.make<ast::Literal>(token.range);
        literal->literal_kind = token.value.find('.') == std::string::npos ? ast::LiteralKind::Int : ast::LiteralKind::Float;
        literal->value = token.value;
        return literal;
    }
    if (token.kind == lex::TokenKind::String) {
        return parser_.parse_string_expression(token);
    }
    if (token.kind == lex::TokenKind::Name) {
        if (token.value == "true" || token.value == "false") {
            ast::Literal* literal = parser_.arena_.make<ast::Literal>(token.range);
            literal->literal_kind = ast::LiteralKind::Bool;
            literal->value = token.value;
            return literal;
        }
        if (token.value == "null") {
            ast::Literal* literal = parser_.arena_.make<ast::Literal>(token.range);
            literal->literal_kind = ast::LiteralKind::Null;
            literal->value = "null";
            return literal;
        }
        if (token.value == "in") {
            if (!expect_symbol(":")) {
                return nullptr;
            }
            ast::Expression* prompt = nullptr;
            if (peek() != nullptr && !is_stop(stop, peek()->value)) {
                prompt = parse_expression(stop);
                if (prompt == nullptr) {
                    return nullptr;
                }
            }
            ast::Input* input = parser_.arena_.make<ast::Input>(token.range);
            input->prompt = prompt;
            return input;
        }
        if (is_reserved_word(token.value)) {
            parser_.add_error(token.range, "syntax error: reserved word " + quote(token.value) + " cannot be used as expression name");
            return nullptr;
        }
        ast::Name* name = parser_.arena_.make<ast::Name>(token.range);
        name->identifier = token.value;
        return name;
    }
    if (token.kind == lex::TokenKind::Symbol) {
        if (token.value == "(") {
            ast::Expression* expression = parse_expression({")"});
            if (expression == nullptr || !expect_symbol(")")) {
                return nullptr;
            }
            return expression;
        }
        if (token.value == "[") {
            std::vector<ast::Expression*> elements;
            while (peek() != nullptr && peek()->value != "]") {
                ast::Expression* element = parse_expression({",", "]"});
                if (element == nullptr) {
                    return nullptr;
                }
                elements.push_back(element);
                if (peek() != nullptr && peek()->value == ",") {
                    advance();
                }
            }
            if (!expect_symbol("]")) {
                return nullptr;
            }
            ast::ArrayLiteral* array = parser_.arena_.make<ast::ArrayLiteral>(token.range);
            array->elements = std::move(elements);
            return array;
        }
    }
    parser_.add_error(token.range, "syntax error: expected expression, got " + quote(token.value));
    return nullptr;
}

ast::Type Parser::Cursor::parse_type() {
    const lex::Token* token = expect_name("type name");
    if (token == nullptr) {
        return {};
    }
    if (token->value == "array") {
        if (!expect_symbol("[")) {
            return {};
        }
        ast::Type elem = parse_type();
        if (parser_.failed() || !expect_symbol("]")) {
            return {};
        }
        ast::Type result = ast::array_of(std::move(elem));
        if (peek() != nullptr && peek()->value == "?") {
            advance();
            result.nullable = true;
        }
        return result;
    }
    if (token->value == "map") {
        if (!expect_symbol("[")) {
            return {};
        }
        ast::Type key = parse_type();
        if (parser_.failed() || !expect_symbol("]")) {
            return {};
        }
        ast::Type value = parse_type();
        if (parser_.failed()) {
            return {};
        }
        ast::Type result = ast::map_of(std::move(key), std::move(value));
        if (peek() != nullptr && peek()->value == "?") {
            advance();
            result.nullable = true;
        }
        return result;
    }
    if (token->value == "func") {
        if (!expect_symbol("(")) {
            return {};
        }
        std::vector<ast::Type> params;
        while (peek() != nullptr && peek()->value != ")") {
            ast::Type param = parse_type();
            if (parser_.failed()) {
                return {};
            }
            params.push_back(std::move(param));
            if (peek() != nullptr && peek()->value == ",") {
                advance();
            }
        }
        if (!expect_symbol(")")) {
            return {};
        }
        ast::Type ret = parse_type();
        if (parser_.failed()) {
            return {};
        }
        return ast::function_type(std::move(params), std::move(ret));
    }

    ast::Type result;
    if (token->value == "void") {
        result = ast::basic(ast::TypeKind::Void);
    } else if (token->value == "int") {
        result = ast::basic(ast::TypeKind::Int);
    } else if (token->value == "float") {
        result = ast::basic(ast::TypeKind::Float);
    } else if (token->value == "bool") {
        result = ast::basic(ast::TypeKind::Bool);
    } else if (token->value == "string") {
        result = ast::basic(ast::TypeKind::String);
    } else if (type_params.count(token->value) != 0) {
        result = ast::generic_type(token->value);
    } else {
        result = ast::struct_type(token->value);
    }
    if (peek() != nullptr && peek()->value == "?") {
        advance();
        result.nullable = true;
    }
    return result;
}

std::vector<ast::Expression*> Parser::Cursor::parse_call_args() {
    std::vector<ast::Expression*> args;
    while (peek() != nullptr && peek()->value != ")") {
        ast::Expression* arg = parse_expression({",", ")"});
        if (arg == nullptr) {
            return {};
        }
        args.push_back(arg);
        if (peek() != nullptr && peek()->value == ",") {
            advance();
        }
    }
    if (!expect_symbol(")")) {
        return {};
    }
    return args;
}

std::vector<std::string> Parser::Cursor::parse_type_params() {
    if (peek() == nullptr || peek()->value != "[") {
        return {};
    }
    advance();
    std::vector<std::string> params;
    while (peek() != nullptr && peek()->value != "]") {
        const lex::Token* param = expect_name("type parameter");
        if (param == nullptr) {
            return {};
        }
        params.push_back(param->value);
        if (peek() != nullptr && peek()->value == ",") {
            advance();
        }
    }
    if (params.empty()) {
        parser_.add_error(current_range(), "syntax error: expected type parameter");
        return {};
    }
    if (!expect_symbol("]")) {
        return {};
    }
    return params;
}

void Parser::add_error(const SourceRange& range, std::string message) {
    diagnostics_.add(Diagnostic(DiagnosticSeverity::Error, kSyntaxDiagnostic, std::move(message), range));
}

bool Parser::reject_unexpected_children(const std::vector<LineNode>& children) {
    for (const LineNode& child : children) {
        if (is_close_only(child.line.tokens)) {
            continue;
        }
        add_error(child.line.range, "syntax error: unexpected indented block");
        return false;
    }
    return true;
}

std::string Parser::parse_bare_name(const std::vector<lex::Token>& tokens, const SourceRange& range) {
    if (tokens.size() != 1 || tokens[0].kind != lex::TokenKind::Name) {
        add_error(range, "syntax error: expected name");
        return "";
    }
    return tokens[0].value;
}

std::string Parser::parse_import_name(const std::vector<lex::Token>& tokens, const SourceRange& range) {
    if (tokens.empty()) {
        add_error(range, "syntax error: expected import name");
        return "";
    }
    std::vector<std::string> parts;
    bool expect_name = true;
    for (const lex::Token& token : tokens) {
        if (expect_name) {
            if (token.kind != lex::TokenKind::Name) {
                add_error(range, "syntax error: expected import name");
                return "";
            }
            parts.push_back(token.value);
            expect_name = false;
            continue;
        }
        if (token.kind != lex::TokenKind::Symbol || token.value != ".") {
            add_error(range, "syntax error: expected import name");
            return "";
        }
        expect_name = true;
    }
    if (expect_name) {
        add_error(range, "syntax error: expected import name");
        return "";
    }
    std::ostringstream joined;
    for (std::size_t index = 0; index < parts.size(); ++index) {
        if (index != 0) {
            joined << ".";
        }
        joined << parts[index];
    }
    return joined.str();
}

ast::Statement* Parser::parse_defer(const std::vector<lex::Token>& tokens, const std::vector<LineNode>& children, const SourceRange& range) {
    if (!reject_unexpected_children(children)) {
        return nullptr;
    }
    ast::Defer* statement = arena_.make<ast::Defer>(range);
    if (tokens.size() >= 2 && tokens[0].kind == lex::TokenKind::Name && tokens[0].value == "do") {
        std::vector<lex::Token> expression_tokens(tokens.begin() + 1, tokens.end());
        statement->value = parse_expression_tokens(expression_tokens, range);
        if (statement->value == nullptr) {
            return nullptr;
        }
    }
    return statement;
}

ast::Statement* Parser::parse_var_decl(const std::vector<lex::Token>& tokens, const std::vector<LineNode>& children, const SourceRange& range, bool mutable_binding) {
    Cursor cursor(*this, tokens);
    const lex::Token* name = cursor.expect_name("variable name");
    if (name == nullptr) {
        return nullptr;
    }
    if (is_reserved_word(name->value)) {
        add_error(name->range, "syntax error: reserved word " + quote(name->value) + " cannot be used as variable name");
        return nullptr;
    }

    ast::Type annotation;
    if (cursor.peek_kind(lex::TokenKind::Name)) {
        annotation = cursor.parse_type();
        if (failed()) {
            return nullptr;
        }
    }
    if (!cursor.expect_symbol("=")) {
        return nullptr;
    }

    ast::Expression* value = nullptr;
    if (cursor.peek() == nullptr) {
        value = parse_expression_block(children, range);
    } else {
        if (!reject_unexpected_children(children)) {
            return nullptr;
        }
        value = cursor.parse_expression();
        if (value == nullptr || !cursor.expect_end()) {
            return nullptr;
        }
    }
    if (value == nullptr) {
        return nullptr;
    }

    ast::VarDecl* statement = arena_.make<ast::VarDecl>(range);
    statement->name = name->value;
    statement->annotation = std::move(annotation);
    statement->value = value;
    statement->mutable_binding = mutable_binding;
    return statement;
}

ast::Statement* Parser::parse_test_decl(const std::vector<lex::Token>& tokens, const std::vector<LineNode>& children, const SourceRange& range) {
    Cursor cursor(*this, tokens);
    std::string name = "unnamed test";
    if (cursor.peek() != nullptr) {
        const lex::Token token = cursor.advance();
        if (token.kind != lex::TokenKind::String) {
            add_error(token.range, "syntax error: test name must be a string");
            return nullptr;
        }
        name = token.value;
    }
    if (!cursor.expect_end()) {
        return nullptr;
    }
    std::vector<ast::Statement*> body = parse_block(children);
    if (failed()) {
        return nullptr;
    }
    ast::TestDecl* statement = arena_.make<ast::TestDecl>(range);
    statement->name = std::move(name);
    statement->body = std::move(body);
    return statement;
}

ast::Statement* Parser::parse_func_decl(const std::vector<lex::Token>& tokens, const std::vector<LineNode>& children, const SourceRange& range) {
    Cursor cursor(*this, tokens);
    const lex::Token* name = cursor.expect_name("function name");
    if (name == nullptr) {
        return nullptr;
    }
    std::string receiver;
    if (cursor.peek() != nullptr && cursor.peek()->value == ".") {
        if (is_reserved_word(name->value)) {
            add_error(name->range, "syntax error: reserved word " + quote(name->value) + " cannot be used as receiver type");
            return nullptr;
        }
        receiver = name->value;
        cursor.advance();
        name = cursor.expect_name("method name");
        if (name == nullptr) {
            return nullptr;
        }
    }
    if (is_reserved_word(name->value)) {
        add_error(name->range, "syntax error: reserved word " + quote(name->value) + " cannot be used as function name");
        return nullptr;
    }

    std::vector<std::string> type_params = cursor.parse_type_params();
    if (failed()) {
        return nullptr;
    }
    cursor.type_params = std::set<std::string>(type_params.begin(), type_params.end());

    if (!cursor.expect_symbol("(")) {
        return nullptr;
    }
    std::vector<ast::Param> params;
    while (cursor.peek() != nullptr && cursor.peek()->value != ")") {
        const lex::Token* param_name = cursor.expect_name("parameter name");
        if (param_name == nullptr) {
            return nullptr;
        }
        ast::Type param_type;
        if (cursor.peek() != nullptr && cursor.peek()->value != "," && cursor.peek()->value != ")") {
            param_type = cursor.parse_type();
            if (failed()) {
                return nullptr;
            }
        }
        params.push_back({param_name->value, std::move(param_type)});
        if (cursor.peek() != nullptr && cursor.peek()->value == ",") {
            cursor.advance();
        }
    }
    if (!cursor.expect_symbol(")")) {
        return nullptr;
    }
    ast::Type return_type;
    if (cursor.peek() != nullptr) {
        return_type = cursor.parse_type();
        if (failed()) {
            return nullptr;
        }
    }
    if (!cursor.expect_end()) {
        return nullptr;
    }
    std::vector<ast::Statement*> body = parse_block(children);
    if (failed()) {
        return nullptr;
    }

    ast::FuncDecl* statement = arena_.make<ast::FuncDecl>(range);
    statement->name = name->value;
    statement->receiver = std::move(receiver);
    statement->type_params = std::move(type_params);
    statement->params = std::move(params);
    statement->return_type = std::move(return_type);
    statement->body = std::move(body);
    return statement;
}

ast::Statement* Parser::parse_struct_decl(const std::vector<lex::Token>& tokens, const std::vector<LineNode>& children, const SourceRange& range) {
    std::string name = parse_bare_name(tokens, range);
    if (failed()) {
        return nullptr;
    }
    if (is_reserved_word(name)) {
        add_error(range, "syntax error: reserved word " + quote(name) + " cannot be used as struct name");
        return nullptr;
    }

    std::vector<ast::StructField> fields;
    for (const LineNode& child : children) {
        if (is_close_only(child.line.tokens)) {
            continue;
        }
        if (!reject_unexpected_children(child.children)) {
            return nullptr;
        }
        Cursor cursor(*this, child.line.tokens);
        const lex::Token* field_name = cursor.expect_name("field name");
        if (field_name == nullptr) {
            return nullptr;
        }
        if (is_reserved_word(field_name->value)) {
            add_error(field_name->range, "syntax error: reserved word " + quote(field_name->value) + " cannot be used as field name");
            return nullptr;
        }
        ast::Type field_type = cursor.parse_type();
        if (failed() || !cursor.expect_end()) {
            return nullptr;
        }
        fields.push_back({field_name->range, field_name->value, std::move(field_type)});
    }
    if (fields.empty()) {
        add_error(range, "type error: struct " + name + " needs at least one field");
        return nullptr;
    }

    ast::StructDecl* statement = arena_.make<ast::StructDecl>(range);
    statement->name = std::move(name);
    statement->fields = std::move(fields);
    return statement;
}

ast::Statement* Parser::parse_for(const std::vector<lex::Token>& tokens, const std::vector<LineNode>& children, const SourceRange& range) {
    Cursor cursor(*this, tokens);
    const lex::Token* name = cursor.expect_name("loop variable");
    if (name == nullptr) {
        return nullptr;
    }
    if (!cursor.match_name("in")) {
        add_error(cursor.current_range(), "syntax error: expected in");
        return nullptr;
    }
    ast::Expression* iterable = cursor.parse_expression();
    if (iterable == nullptr || !cursor.expect_end()) {
        return nullptr;
    }
    ++loop_depth_;
    std::vector<ast::Statement*> body = parse_block(children);
    --loop_depth_;
    if (failed()) {
        return nullptr;
    }
    ast::For* statement = arena_.make<ast::For>(range);
    statement->name = name->value;
    statement->iterable = iterable;
    statement->body = std::move(body);
    return statement;
}

ast::Expression* Parser::parse_command_expression(const std::vector<lex::Token>& tokens, const std::vector<LineNode>& children, const SourceRange& range) {
    if (!tokens.empty()) {
        if (!reject_unexpected_children(children)) {
            return nullptr;
        }
        return parse_expression_tokens(tokens, range);
    }
    return parse_expression_block(children, range);
}

ast::Expression* Parser::parse_expression_block(const std::vector<LineNode>& children, const SourceRange& range) {
    for (const LineNode& child : children) {
        if (is_close_only(child.line.tokens)) {
            continue;
        }
        return parse_expression_node(child);
    }
    add_error(range, "syntax error: expected expression block");
    return nullptr;
}

std::vector<ast::Expression*> Parser::parse_expression_list(const std::vector<LineNode>& children) {
    std::vector<ast::Expression*> expressions;
    for (const LineNode& child : children) {
        if (is_close_only(child.line.tokens)) {
            continue;
        }
        ast::Expression* expression = parse_expression_node(child);
        if (expression == nullptr) {
            return {};
        }
        expressions.push_back(expression);
    }
    return expressions;
}

ast::Expression* Parser::parse_expression_node(const LineNode& node) {
    const std::vector<lex::Token>& tokens = node.line.tokens;
    if (tokens.size() == 2 && tokens[1].value == ":") {
        const std::string& op = tokens[0].value;
        if (prefix_arity(op)) {
            std::vector<ast::Expression*> args = parse_expression_list(node.children);
            if (failed()) {
                return nullptr;
            }
            ast::Prefix* expression = arena_.make<ast::Prefix>(tokens[0].range);
            expression->op = op;
            expression->args = std::move(args);
            return expression;
        }
    }
    if (!tokens.empty() && tokens.back().value == "(") {
        std::vector<lex::Token> target_tokens(tokens.begin(), tokens.end() - 1);
        ast::Expression* target = parse_expression_tokens(target_tokens, node.line.range);
        if (target == nullptr) {
            return nullptr;
        }
        std::vector<ast::Expression*> args = parse_expression_list(node.children);
        if (failed()) {
            return nullptr;
        }
        return call_from_target(target, std::move(args), tokens.back().range);
    }
    return parse_expression_tokens(tokens, node.line.range);
}

ast::Expression* Parser::parse_expression_tokens(const std::vector<lex::Token>& tokens, const SourceRange& range) {
    Cursor cursor(*this, tokens);
    ast::Expression* value = cursor.parse_expression();
    if (value == nullptr || !cursor.expect_end()) {
        if (!failed()) {
            add_error(range, "syntax error: expected expression");
        }
        return nullptr;
    }
    return value;
}

ast::Expression* Parser::parse_string_expression(const lex::Token& token) {
    if (token.value.find_first_of("{}") == std::string::npos) {
        ast::Literal* literal = arena_.make<ast::Literal>(token.range);
        literal->literal_kind = ast::LiteralKind::String;
        literal->value = token.value;
        return literal;
    }

    bool interpolated = false;
    std::vector<ast::InterpolatedStringPart> parts = parse_interpolated_string_parts(token, interpolated);
    if (failed()) {
        return nullptr;
    }
    if (!interpolated) {
        std::ostringstream literal_value;
        for (const ast::InterpolatedStringPart& part : parts) {
            literal_value << part.literal;
        }
        ast::Literal* literal = arena_.make<ast::Literal>(token.range);
        literal->literal_kind = ast::LiteralKind::String;
        literal->value = literal_value.str();
        return literal;
    }
    ast::InterpolatedString* string = arena_.make<ast::InterpolatedString>(token.range);
    string->parts = std::move(parts);
    return string;
}

std::vector<ast::InterpolatedStringPart> Parser::parse_interpolated_string_parts(const lex::Token& token, bool& interpolated) {
    std::vector<ast::InterpolatedStringPart> parts;
    std::ostringstream literal;
    interpolated = false;

    for (std::size_t index = 0; index < token.value.size();) {
        const char ch = token.value[index];
        if (ch == '{') {
            if (index + 1 < token.value.size() && token.value[index + 1] == '{') {
                literal << '{';
                index += 2;
                continue;
            }
            if (literal.tellp() > 0) {
                parts.push_back({literal.str(), nullptr});
                literal.str("");
                literal.clear();
            }
            const std::size_t expr_start = index + 1;
            const int expr_end = find_interpolation_end(token.value, expr_start);
            if (expr_end < 0) {
                add_error(interpolation_range(token, index), "syntax error: unterminated interpolation");
                return {};
            }
            std::string expr_text = token.value.substr(expr_start, static_cast<std::size_t>(expr_end) - expr_start);
            const std::size_t left_trim = expr_text.find_first_not_of(" \t");
            const std::size_t expr_offset = left_trim == std::string::npos ? expr_start + expr_text.size() : expr_start + left_trim;
            if (left_trim == std::string::npos) {
                expr_text.clear();
            } else {
                expr_text = expr_text.substr(left_trim);
                const std::size_t right_trim = expr_text.find_last_not_of(" \t");
                expr_text = expr_text.substr(0, right_trim + 1);
            }
            if (expr_text.empty()) {
                add_error(interpolation_range(token, expr_start), "syntax error: expected interpolation expression");
                return {};
            }
            ast::Expression* expression = parse_interpolated_expression(expr_text, interpolation_range(token, expr_offset));
            if (expression == nullptr) {
                return {};
            }
            parts.push_back({"", expression});
            interpolated = true;
            index = static_cast<std::size_t>(expr_end) + 1;
            continue;
        }
        if (ch == '}') {
            if (index + 1 < token.value.size() && token.value[index + 1] == '}') {
                literal << '}';
                index += 2;
                continue;
            }
            add_error(interpolation_range(token, index), "syntax error: unmatched } in string");
            return {};
        }
        literal << ch;
        ++index;
    }
    if (literal.tellp() > 0) {
        parts.push_back({literal.str(), nullptr});
    }
    return parts;
}

ast::Expression* Parser::parse_interpolated_expression(const std::string& source, const SourceRange& range) {
    SourceFile expression_source = SourceFile::from_text(range.path, source);
    lex::LexResult lexed = lex::lex_source(expression_source);
    if (!lexed.ok()) {
        add_error(range, "syntax error: invalid interpolation expression");
        return nullptr;
    }
    if (lexed.lines.size() != 1 || lexed.lines[0].tokens.empty()) {
        add_error(range, "syntax error: expected interpolation expression");
        return nullptr;
    }
    std::vector<lex::Token> tokens = lexed.lines[0].tokens;
    for (lex::Token& token : tokens) {
        token.range.path = range.path;
        token.range.start.line = range.start.line;
        token.range.end.line = range.start.line;
        token.range.start.column = range.start.column + token.range.start.column - 1;
        token.range.end.column = range.start.column + token.range.end.column - 1;
        token.range.start_offset = range.start_offset + token.range.start_offset;
        token.range.end_offset = range.start_offset + token.range.end_offset;
    }
    return parse_expression_tokens(tokens, range);
}

ast::Call* Parser::call_from_target(ast::Expression* target, std::vector<ast::Expression*> args, const SourceRange& invalid_range) {
    if (auto* field = dynamic_cast<ast::FieldAccess*>(target)) {
        std::optional<std::string> callee = expression_callee(target);
        ast::Call* call = arena_.make<ast::Call>(callee ? callee_range(target) : field->range);
        call->callee = callee.value_or(field->field);
        call->receiver = field->target;
        call->method = field->field;
        call->args = std::move(args);
        return call;
    }
    std::optional<std::string> callee = expression_callee(target);
    if (!callee) {
        add_error(invalid_range, "syntax error: invalid call target");
        return nullptr;
    }
    ast::Call* call = arena_.make<ast::Call>(callee_range(target));
    call->callee = *callee;
    call->args = std::move(args);
    return call;
}

std::optional<std::string> Parser::expression_callee(ast::Expression* expression) const {
    if (auto* name = dynamic_cast<ast::Name*>(expression)) {
        return name->identifier;
    }
    if (auto* field = dynamic_cast<ast::FieldAccess*>(expression)) {
        std::optional<std::string> target = expression_callee(field->target);
        if (!target) {
            return std::nullopt;
        }
        return *target + "." + field->field;
    }
    return std::nullopt;
}

SourceRange Parser::callee_range(ast::Expression* expression) const {
    if (auto* field = dynamic_cast<ast::FieldAccess*>(expression)) {
        return callee_range(field->target);
    }
    return expression->range;
}

int Parser::find_assignment(const std::vector<lex::Token>& tokens) const {
    int depth = 0;
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        const std::string& value = tokens[index].value;
        if (value == "(" || value == "[") {
            ++depth;
        } else if (value == ")" || value == "]") {
            --depth;
        } else if (value == "=" && depth == 0) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

int Parser::find_interpolation_end(const std::string& value, std::size_t start) const {
    int depth = 1;
    bool in_string = false;
    bool escaped = false;
    for (std::size_t index = start; index < value.size(); ++index) {
        const char ch = value[index];
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '\'') {
                in_string = false;
            }
            continue;
        }
        if (ch == '\'') {
            in_string = true;
        } else if (ch == '{') {
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0) {
                return static_cast<int>(index);
            }
        }
    }
    return -1;
}

ParseResult parse_lines(const SourceFile& source, const std::vector<lex::Line>& lines) {
    Parser parser(source);
    std::vector<LineNode> nodes;
    std::size_t next = 0;
    parser.build_nodes(lines, 0, 0, nodes, next);
    if (!parser.failed() && next != lines.size()) {
        parser.add_error(lines[next].range, "syntax error: unexpected indentation");
    }

    ParseResult result;
    if (!parser.failed()) {
        std::vector<ast::Statement*> statements = parser.parse_block(nodes);
        if (!parser.failed()) {
            auto program = std::make_unique<ast::Program>();
            program->statements = std::move(statements);
            result.program = std::move(program);
        }
    }
    result.arena = parser.take_arena();
    result.diagnostics = parser.take_diagnostics();
    return result;
}

ParseResult parse_source(const SourceFile& source) {
    lex::LexResult lexed = lex::lex_source(source);
    if (!lexed.ok()) {
        ParseResult result;
        lexed.diagnostics.sort();
        result.diagnostics = std::move(lexed.diagnostics);
        return result;
    }
    return parse_lines(source, lexed.lines);
}

}  // namespace walk::parse
