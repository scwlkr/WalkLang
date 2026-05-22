package parser

import (
	"fmt"
	"strconv"

	"walklang/internal/ast"
	"walklang/internal/lexer"
)

var prefixArity = map[string][2]int{
	"+":   {2, -1},
	"*":   {2, -1},
	"and": {2, -1},
	"or":  {2, -1},
	"-":   {2, 2},
	"/":   {2, 2},
	"^":   {2, 2},
	">":   {2, 2},
	"<":   {2, 2},
	">=":  {2, 2},
	"<=":  {2, 2},
	"==":  {2, 2},
	"!=":  {2, 2},
	"not": {1, 1},
}

var reservedWords = map[string]bool{
	"var": true, "const": true, "out": true, "if": true, "else": true,
	"while": true, "for": true, "repeat": true, "break": true, "continue": true,
	"func": true, "return": true, "imp": true, "exp": true, "true": true,
	"false": true, "null": true, "and": true, "or": true, "not": true,
	"in": true, "test": true, "assert": true,
}

func ParseSource(source string, filename string) (*ast.Program, error) {
	lines, err := lexer.Lex(source, filename)
	if err != nil {
		return nil, err
	}
	return ParseLines(lines)
}

func ParseLines(lines []lexer.Line) (*ast.Program, error) {
	program := &ast.Program{}
	for _, line := range lines {
		if line.Indent != 0 {
			return nil, errorAt(line.Location, "syntax error: indented blocks are not supported in the initial compiler stage")
		}
		stmt, err := parseStatement(line.Tokens)
		if err != nil {
			return nil, err
		}
		program.Statements = append(program.Statements, stmt)
	}
	return program, nil
}

func parseStatement(tokens []lexer.Token) (ast.Statement, error) {
	c := cursor{tokens: tokens}
	first := c.peek()
	if first == nil {
		return nil, fmt.Errorf("syntax error: empty statement")
	}

	if c.matchName("var") {
		if err := c.expectSymbol(":"); err != nil {
			return nil, err
		}
		return parseVarDecl(&c, first.Location, true)
	}

	if c.matchName("const") {
		if err := c.expectSymbol(":"); err != nil {
			return nil, err
		}
		return parseVarDecl(&c, first.Location, false)
	}

	if c.matchName("out") {
		if err := c.expectSymbol(":"); err != nil {
			return nil, err
		}
		value, err := c.parseExpression(nil)
		if err != nil {
			return nil, err
		}
		if err := c.expectEnd(); err != nil {
			return nil, err
		}
		return &ast.Out{Location: first.Location, Value: value}, nil
	}

	if first.Kind == lexer.TokenName && len(tokens) > 1 && tokens[1].Value == "=" {
		name := c.advance().Value
		if err := c.expectSymbol("="); err != nil {
			return nil, err
		}
		value, err := c.parseExpression(nil)
		if err != nil {
			return nil, err
		}
		if err := c.expectEnd(); err != nil {
			return nil, err
		}
		return &ast.Assignment{Location: first.Location, Name: name, Value: value}, nil
	}

	return nil, errorAt(first.Location, "syntax error: unsupported statement starting with %q", first.Value)
}

func parseVarDecl(c *cursor, location ast.Location, mutable bool) (ast.Statement, error) {
	name, err := c.expectName("variable name")
	if err != nil {
		return nil, err
	}
	if reservedWords[name.Value] {
		return nil, errorAt(name.Location, "syntax error: reserved word %q cannot be used as variable name", name.Value)
	}

	var annotation ast.TypeName
	if c.peekKind(lexer.TokenName) {
		typeToken := c.advance()
		annotation = ast.TypeName(typeToken.Value)
	}

	if err := c.expectSymbol("="); err != nil {
		return nil, err
	}
	value, err := c.parseExpression(nil)
	if err != nil {
		return nil, err
	}
	if err := c.expectEnd(); err != nil {
		return nil, err
	}
	return &ast.VarDecl{Location: location, Name: name.Value, Annotation: annotation, Value: value, Mutable: mutable}, nil
}

type cursor struct {
	tokens []lexer.Token
	index  int
}

func (c *cursor) peek() *lexer.Token {
	if c.index >= len(c.tokens) {
		return nil
	}
	return &c.tokens[c.index]
}

func (c *cursor) peekKind(kind lexer.TokenKind) bool {
	token := c.peek()
	return token != nil && token.Kind == kind
}

func (c *cursor) advance() lexer.Token {
	token := c.tokens[c.index]
	c.index++
	return token
}

func (c *cursor) matchName(value string) bool {
	token := c.peek()
	if token != nil && token.Kind == lexer.TokenName && token.Value == value {
		c.index++
		return true
	}
	return false
}

func (c *cursor) expectSymbol(value string) error {
	token := c.peek()
	if token == nil || token.Kind != lexer.TokenSymbol || token.Value != value {
		return errorAt(c.currentLocation(), "syntax error: expected %q", value)
	}
	c.index++
	return nil
}

func (c *cursor) expectName(label string) (lexer.Token, error) {
	token := c.peek()
	if token == nil || token.Kind != lexer.TokenName {
		return lexer.Token{}, errorAt(c.currentLocation(), "syntax error: expected %s", label)
	}
	c.index++
	return *token, nil
}

func (c *cursor) expectEnd() error {
	token := c.peek()
	if token != nil {
		return errorAt(token.Location, "syntax error: unexpected token %q", token.Value)
	}
	return nil
}

func (c *cursor) currentLocation() ast.Location {
	if token := c.peek(); token != nil {
		return token.Location
	}
	return c.tokens[len(c.tokens)-1].Location
}

func (c *cursor) parseExpression(stop map[string]bool) (ast.Expression, error) {
	token := c.peek()
	if token == nil {
		return nil, errorAt(c.currentLocation(), "syntax error: expected expression")
	}
	if stop != nil && stop[token.Value] {
		return nil, errorAt(token.Location, "syntax error: expected expression")
	}
	if _, ok := prefixArity[token.Value]; ok {
		return c.parsePrefix(stop)
	}
	return c.parseAtom(stop)
}

func (c *cursor) parsePrefix(stop map[string]bool) (ast.Expression, error) {
	op := c.advance()
	arity := prefixArity[op.Value]
	var args []ast.Expression
	for c.peek() != nil {
		if stop != nil && stop[c.peek().Value] {
			break
		}
		if arity[1] >= 0 && len(args) >= arity[1] {
			break
		}
		arg, err := c.parseExpression(stop)
		if err != nil {
			return nil, err
		}
		args = append(args, arg)
	}
	if len(args) < arity[0] {
		return nil, errorAt(op.Location, "syntax error: operator %q expects at least %d operand(s)", op.Value, arity[0])
	}
	return &ast.Prefix{ExprBase: ast.ExprBase{Location: op.Location}, Operator: op.Value, Args: args}, nil
}

func (c *cursor) parseAtom(stop map[string]bool) (ast.Expression, error) {
	token := c.advance()
	switch token.Kind {
	case lexer.TokenNumber:
		if _, err := strconv.ParseFloat(token.Value, 64); err != nil {
			return nil, errorAt(token.Location, "syntax error: invalid number %q", token.Value)
		}
		kind := ast.LiteralInt
		if hasDot(token.Value) {
			kind = ast.LiteralFloat
		}
		return &ast.Literal{ExprBase: ast.ExprBase{Location: token.Location}, Kind: kind, Value: token.Value}, nil
	case lexer.TokenString:
		return &ast.Literal{ExprBase: ast.ExprBase{Location: token.Location}, Kind: ast.LiteralString, Value: token.Value}, nil
	case lexer.TokenName:
		switch token.Value {
		case "true", "false":
			return &ast.Literal{ExprBase: ast.ExprBase{Location: token.Location}, Kind: ast.LiteralBool, Value: token.Value}, nil
		case "null":
			return nil, errorAt(token.Location, "type error: null is not supported in the initial compiler stage")
		}
		if reservedWords[token.Value] {
			return nil, errorAt(token.Location, "syntax error: reserved word %q cannot be used as expression name", token.Value)
		}
		return &ast.Name{ExprBase: ast.ExprBase{Location: token.Location}, Identifier: token.Value}, nil
	case lexer.TokenSymbol:
		if token.Value == "(" {
			expr, err := c.parseExpression(map[string]bool{")": true})
			if err != nil {
				return nil, err
			}
			if err := c.expectSymbol(")"); err != nil {
				return nil, err
			}
			return expr, nil
		}
	}
	return nil, errorAt(token.Location, "syntax error: expected expression, got %q", token.Value)
}

func hasDot(value string) bool {
	for _, r := range value {
		if r == '.' {
			return true
		}
	}
	return false
}

func errorAt(location ast.Location, format string, args ...any) error {
	return fmt.Errorf("%s:%d: %s", location.Filename, location.Line, fmt.Sprintf(format, args...))
}
