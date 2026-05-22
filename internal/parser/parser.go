package parser

import (
	"fmt"
	"strconv"
	"strings"

	"walklang/internal/ast"
	"walklang/internal/diagnostic"
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
	"in": true, "test": true, "assert": true, "struct": true,
}

type lineNode struct {
	line     lexer.Line
	children []lineNode
}

func ParseSource(source string, filename string) (*ast.Program, error) {
	lines, err := lexer.Lex(source, filename)
	if err != nil {
		return nil, err
	}
	return ParseLines(lines)
}

func ParseLines(lines []lexer.Line) (*ast.Program, error) {
	nodes, next, err := buildNodes(lines, 0, 0)
	if err != nil {
		return nil, err
	}
	if next != len(lines) {
		return nil, errorAt(lines[next].Location, "syntax error: unexpected indentation")
	}
	statements, err := parseBlock(nodes)
	if err != nil {
		return nil, err
	}
	return &ast.Program{Statements: statements}, nil
}

func buildNodes(lines []lexer.Line, index int, indent int) ([]lineNode, int, error) {
	var nodes []lineNode
	for index < len(lines) {
		line := lines[index]
		if line.Indent < indent {
			return nodes, index, nil
		}
		if line.Indent > indent {
			return nil, index, errorAt(line.Location, "syntax error: unexpected indentation")
		}
		node := lineNode{line: line}
		index++
		if index < len(lines) && lines[index].Indent > indent {
			children, next, err := buildNodes(lines, index, lines[index].Indent)
			if err != nil {
				return nil, index, err
			}
			node.children = children
			index = next
		}
		nodes = append(nodes, node)
	}
	return nodes, index, nil
}

func parseBlock(nodes []lineNode) ([]ast.Statement, error) {
	var statements []ast.Statement
	for i := 0; i < len(nodes); i++ {
		if isCloseOnly(nodes[i].line.Tokens) {
			continue
		}
		statement, next, err := parseStatementAt(nodes, i)
		if err != nil {
			return nil, err
		}
		statements = append(statements, statement)
		i = next - 1
	}
	return statements, nil
}

func parseStatementAt(nodes []lineNode, index int) (ast.Statement, int, error) {
	node := nodes[index]
	tokens := node.line.Tokens
	first := tokens[0]

	if keyword, ok := commandKeyword(tokens); ok {
		payload := tokens[2:]
		switch keyword {
		case "imp":
			if err := rejectUnexpectedChildren(node.children); err != nil {
				return nil, index, err
			}
			name, err := parseBareName(payload, first.Location)
			return &ast.Import{Location: first.Location, Module: name}, index + 1, err
		case "exp":
			if err := rejectUnexpectedChildren(node.children); err != nil {
				return nil, index, err
			}
			name, err := parseBareName(payload, first.Location)
			return &ast.Export{Location: first.Location, Name: name}, index + 1, err
		case "var", "const":
			statement, err := parseVarDecl(payload, node.children, first.Location, keyword == "var")
			return statement, index + 1, err
		case "out":
			value, err := parseCommandExpression(payload, node.children, first.Location)
			return &ast.Out{Location: first.Location, Value: value}, index + 1, err
		case "test":
			statement, err := parseTestDecl(payload, node.children, first.Location)
			return statement, index + 1, err
		case "assert":
			value, err := parseCommandExpression(payload, node.children, first.Location)
			return &ast.Assert{Location: first.Location, Value: value}, index + 1, err
		case "func":
			statement, err := parseFuncDecl(payload, node.children, first.Location)
			return statement, index + 1, err
		case "struct":
			statement, err := parseStructDecl(payload, node.children, first.Location)
			return statement, index + 1, err
		case "return":
			value, err := parseCommandExpression(payload, node.children, first.Location)
			return &ast.Return{Location: first.Location, Value: value}, index + 1, err
		case "if":
			cond, err := parseCommandExpression(payload, nil, first.Location)
			if err != nil {
				return nil, index, err
			}
			thenBlock, err := parseBlock(node.children)
			if err != nil {
				return nil, index, err
			}
			var elseBlock []ast.Statement
			next := index + 1
			if next < len(nodes) && isCommand(nodes[next].line.Tokens, "else") {
				parsedElse, err := parseBlock(nodes[next].children)
				if err != nil {
					return nil, index, err
				}
				elseBlock = parsedElse
				next++
			}
			return &ast.If{Location: first.Location, Cond: cond, Then: thenBlock, Else: elseBlock}, next, nil
		case "else":
			return nil, index, errorAt(first.Location, "syntax error: else without matching if")
		case "while":
			cond, err := parseCommandExpression(payload, nil, first.Location)
			if err != nil {
				return nil, index, err
			}
			body, err := parseBlock(node.children)
			return &ast.While{Location: first.Location, Cond: cond, Body: body}, index + 1, err
		case "repeat":
			count, err := parseCommandExpression(payload, nil, first.Location)
			if err != nil {
				return nil, index, err
			}
			body, err := parseBlock(node.children)
			return &ast.Repeat{Location: first.Location, Count: count, Body: body}, index + 1, err
		case "for":
			statement, err := parseFor(payload, node.children, first.Location)
			return statement, index + 1, err
		case "break":
			if len(payload) != 0 {
				return nil, index, errorAt(first.Location, "syntax error: break takes no value")
			}
			if err := rejectUnexpectedChildren(node.children); err != nil {
				return nil, index, err
			}
			return &ast.Break{Location: first.Location}, index + 1, nil
		case "continue":
			if len(payload) != 0 {
				return nil, index, errorAt(first.Location, "syntax error: continue takes no value")
			}
			if err := rejectUnexpectedChildren(node.children); err != nil {
				return nil, index, err
			}
			return &ast.Continue{Location: first.Location}, index + 1, nil
		}
	}

	if eq := findAssignment(tokens); eq >= 0 {
		target, err := parseExpressionTokens(tokens[:eq], first.Location)
		if err != nil {
			return nil, index, err
		}
		value, err := parseExpressionTokens(tokens[eq+1:], first.Location)
		if err != nil {
			return nil, index, err
		}
		return &ast.Assignment{Location: first.Location, Target: target, Value: value}, index + 1, nil
	}

	return nil, index, errorAt(first.Location, "syntax error: unsupported statement starting with %q", first.Value)
}

func commandKeyword(tokens []lexer.Token) (string, bool) {
	if len(tokens) >= 2 && tokens[0].Kind == lexer.TokenName && tokens[1].Value == ":" {
		return tokens[0].Value, true
	}
	return "", false
}

func isCommand(tokens []lexer.Token, keyword string) bool {
	found, ok := commandKeyword(tokens)
	return ok && found == keyword
}

func parseBareName(tokens []lexer.Token, location ast.Location) (string, error) {
	if len(tokens) != 1 || tokens[0].Kind != lexer.TokenName {
		return "", errorAt(location, "syntax error: expected name")
	}
	return tokens[0].Value, nil
}

func parseVarDecl(tokens []lexer.Token, children []lineNode, location ast.Location, mutable bool) (ast.Statement, error) {
	c := cursor{tokens: tokens}
	name, err := c.expectName("variable name")
	if err != nil {
		return nil, err
	}
	if reservedWords[name.Value] {
		return nil, errorAt(name.Location, "syntax error: reserved word %q cannot be used as variable name", name.Value)
	}

	var annotation ast.Type
	if c.peekKind(lexer.TokenName) {
		annotation, err = c.parseType()
		if err != nil {
			return nil, err
		}
	}
	if err := c.expectSymbol("="); err != nil {
		return nil, err
	}

	var value ast.Expression
	if c.peek() == nil {
		value, err = parseExpressionBlock(children, location)
	} else {
		if err := rejectUnexpectedChildren(children); err != nil {
			return nil, err
		}
		value, err = c.parseExpression(nil)
		if err == nil {
			err = c.expectEnd()
		}
	}
	if err != nil {
		return nil, err
	}
	return &ast.VarDecl{Location: location, Name: name.Value, Annotation: annotation, Value: value, Mutable: mutable}, nil
}

func parseTestDecl(tokens []lexer.Token, children []lineNode, location ast.Location) (ast.Statement, error) {
	c := cursor{tokens: tokens}
	name := "unnamed test"
	if c.peek() != nil {
		token := c.advance()
		if token.Kind != lexer.TokenString {
			return nil, errorAt(token.Location, "syntax error: test name must be a string")
		}
		name = token.Value
	}
	if err := c.expectEnd(); err != nil {
		return nil, err
	}
	body, err := parseBlock(children)
	if err != nil {
		return nil, err
	}
	return &ast.TestDecl{Location: location, Name: name, Body: body}, nil
}

func parseFuncDecl(tokens []lexer.Token, children []lineNode, location ast.Location) (ast.Statement, error) {
	c := cursor{tokens: tokens}
	name, err := c.expectName("function name")
	if err != nil {
		return nil, err
	}
	if reservedWords[name.Value] {
		return nil, errorAt(name.Location, "syntax error: reserved word %q cannot be used as function name", name.Value)
	}
	if err := c.expectSymbol("("); err != nil {
		return nil, err
	}
	var params []ast.Param
	for c.peek() != nil && c.peek().Value != ")" {
		paramName, err := c.expectName("parameter name")
		if err != nil {
			return nil, err
		}
		paramType, err := c.parseType()
		if err != nil {
			return nil, err
		}
		params = append(params, ast.Param{Name: paramName.Value, Type: paramType})
		if c.peek() != nil && c.peek().Value == "," {
			c.advance()
		}
	}
	if err := c.expectSymbol(")"); err != nil {
		return nil, err
	}
	returnType := ast.Basic(ast.TypeVoid)
	if c.peek() != nil {
		returnType, err = c.parseType()
		if err != nil {
			return nil, err
		}
	}
	if err := c.expectEnd(); err != nil {
		return nil, err
	}
	body, err := parseBlock(children)
	if err != nil {
		return nil, err
	}
	return &ast.FuncDecl{Location: location, Name: name.Value, Params: params, ReturnType: returnType, Body: body}, nil
}

func parseStructDecl(tokens []lexer.Token, children []lineNode, location ast.Location) (ast.Statement, error) {
	name, err := parseBareName(tokens, location)
	if err != nil {
		return nil, err
	}
	if reservedWords[name] {
		return nil, errorAt(location, "syntax error: reserved word %q cannot be used as struct name", name)
	}
	var fields []ast.StructField
	for _, child := range children {
		if isCloseOnly(child.line.Tokens) {
			continue
		}
		if err := rejectUnexpectedChildren(child.children); err != nil {
			return nil, err
		}
		c := cursor{tokens: child.line.Tokens}
		fieldName, err := c.expectName("field name")
		if err != nil {
			return nil, err
		}
		if reservedWords[fieldName.Value] {
			return nil, errorAt(fieldName.Location, "syntax error: reserved word %q cannot be used as field name", fieldName.Value)
		}
		fieldType, err := c.parseType()
		if err != nil {
			return nil, err
		}
		if err := c.expectEnd(); err != nil {
			return nil, err
		}
		fields = append(fields, ast.StructField{Location: fieldName.Location, Name: fieldName.Value, Type: fieldType})
	}
	if len(fields) == 0 {
		return nil, errorAt(location, "type error: struct %s needs at least one field", name)
	}
	return &ast.StructDecl{Location: location, Name: name, Fields: fields}, nil
}

func parseFor(tokens []lexer.Token, children []lineNode, location ast.Location) (ast.Statement, error) {
	c := cursor{tokens: tokens}
	name, err := c.expectName("loop variable")
	if err != nil {
		return nil, err
	}
	if !c.matchName("in") {
		return nil, errorAt(c.currentLocation(), "syntax error: expected in")
	}
	iterable, err := c.parseExpression(nil)
	if err != nil {
		return nil, err
	}
	if err := c.expectEnd(); err != nil {
		return nil, err
	}
	body, err := parseBlock(children)
	if err != nil {
		return nil, err
	}
	return &ast.For{Location: location, Name: name.Value, Iterable: iterable, Body: body}, nil
}

func parseCommandExpression(tokens []lexer.Token, children []lineNode, location ast.Location) (ast.Expression, error) {
	if len(tokens) > 0 {
		if err := rejectUnexpectedChildren(children); err != nil {
			return nil, err
		}
		return parseExpressionTokens(tokens, location)
	}
	return parseExpressionBlock(children, location)
}

func rejectUnexpectedChildren(children []lineNode) error {
	for _, child := range children {
		if isCloseOnly(child.line.Tokens) {
			continue
		}
		return errorAt(child.line.Location, "syntax error: unexpected indented block")
	}
	return nil
}

func parseExpressionBlock(children []lineNode, location ast.Location) (ast.Expression, error) {
	for _, child := range children {
		if isCloseOnly(child.line.Tokens) {
			continue
		}
		return parseExpressionNode(child)
	}
	return nil, errorAt(location, "syntax error: expected expression block")
}

func parseExpressionList(children []lineNode) ([]ast.Expression, error) {
	var expressions []ast.Expression
	for _, child := range children {
		if isCloseOnly(child.line.Tokens) {
			continue
		}
		expr, err := parseExpressionNode(child)
		if err != nil {
			return nil, err
		}
		expressions = append(expressions, expr)
	}
	return expressions, nil
}

func parseExpressionNode(node lineNode) (ast.Expression, error) {
	tokens := node.line.Tokens
	if len(tokens) == 2 && tokens[1].Value == ":" {
		op := tokens[0].Value
		if _, ok := prefixArity[op]; ok {
			args, err := parseExpressionList(node.children)
			if err != nil {
				return nil, err
			}
			return &ast.Prefix{ExprBase: ast.ExprBase{Location: tokens[0].Location}, Operator: op, Args: args}, nil
		}
	}
	if len(tokens) > 0 && tokens[len(tokens)-1].Value == "(" {
		callee, err := parseCallee(tokens[:len(tokens)-1])
		if err != nil {
			return nil, err
		}
		args, err := parseExpressionList(node.children)
		if err != nil {
			return nil, err
		}
		return &ast.Call{ExprBase: ast.ExprBase{Location: tokens[0].Location}, Callee: callee, Args: args}, nil
	}
	return parseExpressionTokens(tokens, node.line.Location)
}

func parseExpressionTokens(tokens []lexer.Token, location ast.Location) (ast.Expression, error) {
	c := cursor{tokens: tokens}
	value, err := c.parseExpression(nil)
	if err != nil {
		return nil, err
	}
	if err := c.expectEnd(); err != nil {
		return nil, err
	}
	return value, nil
}

func findAssignment(tokens []lexer.Token) int {
	depth := 0
	for i, token := range tokens {
		switch token.Value {
		case "(", "[":
			depth++
		case ")", "]":
			depth--
		case "=":
			if depth == 0 {
				return i
			}
		}
	}
	return -1
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
	if len(c.tokens) == 0 {
		return ast.Location{}
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
	return c.parsePostfix(stop)
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

func (c *cursor) parsePostfix(stop map[string]bool) (ast.Expression, error) {
	expr, err := c.parseAtom(stop)
	if err != nil {
		return nil, err
	}
	for c.peek() != nil {
		if stop != nil && stop[c.peek().Value] {
			break
		}
		if c.peek().Value == "[" {
			open := c.advance()
			index, err := c.parseExpression(map[string]bool{"]": true})
			if err != nil {
				return nil, err
			}
			if err := c.expectSymbol("]"); err != nil {
				return nil, err
			}
			expr = &ast.Index{ExprBase: ast.ExprBase{Location: open.Location}, Target: expr, Index: index}
			continue
		}
		if c.peek().Value == "." {
			dot := c.advance()
			field, err := c.expectName("field name")
			if err != nil {
				return nil, err
			}
			expr = &ast.FieldAccess{ExprBase: ast.ExprBase{Location: dot.Location}, Target: expr, Field: field.Value}
			continue
		}
		if c.peek().Value == "(" {
			open := c.advance()
			callee, ok := expressionCallee(expr)
			if !ok {
				return nil, errorAt(open.Location, "syntax error: invalid call target")
			}
			args, err := c.parseCallArgs()
			if err != nil {
				return nil, err
			}
			expr = &ast.Call{ExprBase: ast.ExprBase{Location: calleeLocation(expr)}, Callee: callee, Args: args}
			continue
		}
		break
	}
	return expr, nil
}

func (c *cursor) parseAtom(stop map[string]bool) (ast.Expression, error) {
	token := c.advance()
	switch token.Kind {
	case lexer.TokenNumber:
		if _, err := strconv.ParseFloat(token.Value, 64); err != nil {
			return nil, errorAt(token.Location, "syntax error: invalid number %q", token.Value)
		}
		kind := ast.LiteralInt
		if strings.Contains(token.Value, ".") {
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
			return &ast.Literal{ExprBase: ast.ExprBase{Location: token.Location}, Kind: ast.LiteralNull, Value: "null"}, nil
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
		if token.Value == "[" {
			var elements []ast.Expression
			for c.peek() != nil && c.peek().Value != "]" {
				element, err := c.parseExpression(map[string]bool{",": true, "]": true})
				if err != nil {
					return nil, err
				}
				elements = append(elements, element)
				if c.peek() != nil && c.peek().Value == "," {
					c.advance()
				}
			}
			if err := c.expectSymbol("]"); err != nil {
				return nil, err
			}
			return &ast.ArrayLiteral{ExprBase: ast.ExprBase{Location: token.Location}, Elements: elements}, nil
		}
	}
	return nil, errorAt(token.Location, "syntax error: expected expression, got %q", token.Value)
}

func (c *cursor) parseCallArgs() ([]ast.Expression, error) {
	var args []ast.Expression
	for c.peek() != nil && c.peek().Value != ")" {
		arg, err := c.parseExpression(map[string]bool{",": true, ")": true})
		if err != nil {
			return nil, err
		}
		args = append(args, arg)
		if c.peek() != nil && c.peek().Value == "," {
			c.advance()
		}
	}
	if err := c.expectSymbol(")"); err != nil {
		return nil, err
	}
	return args, nil
}

func expressionCallee(expression ast.Expression) (string, bool) {
	switch e := expression.(type) {
	case *ast.Name:
		return e.Identifier, true
	case *ast.FieldAccess:
		target, ok := expressionCallee(e.Target)
		if !ok {
			return "", false
		}
		return target + "." + e.Field, true
	default:
		return "", false
	}
}

func calleeLocation(expression ast.Expression) ast.Location {
	switch e := expression.(type) {
	case *ast.FieldAccess:
		return calleeLocation(e.Target)
	default:
		return expression.Loc()
	}
}

func (c *cursor) parseType() (ast.Type, error) {
	token, err := c.expectName("type name")
	if err != nil {
		return ast.Type{}, err
	}
	if token.Value == "array" {
		if err := c.expectSymbol("["); err != nil {
			return ast.Type{}, err
		}
		elem, err := c.parseType()
		if err != nil {
			return ast.Type{}, err
		}
		if err := c.expectSymbol("]"); err != nil {
			return ast.Type{}, err
		}
		result := ast.ArrayOf(elem)
		if c.peek() != nil && c.peek().Value == "?" {
			c.advance()
			result.Nullable = true
		}
		return result, nil
	}
	if token.Value == "func" {
		if err := c.expectSymbol("("); err != nil {
			return ast.Type{}, err
		}
		var params []ast.Type
		for c.peek() != nil && c.peek().Value != ")" {
			paramType, err := c.parseType()
			if err != nil {
				return ast.Type{}, err
			}
			params = append(params, paramType)
			if c.peek() != nil && c.peek().Value == "," {
				c.advance()
			}
		}
		if err := c.expectSymbol(")"); err != nil {
			return ast.Type{}, err
		}
		ret, err := c.parseType()
		if err != nil {
			return ast.Type{}, err
		}
		return ast.FuncType(params, ret), nil
	}
	var result ast.Type
	switch token.Value {
	case string(ast.TypeVoid), string(ast.TypeInt), string(ast.TypeFloat), string(ast.TypeBool), string(ast.TypeString):
		result = ast.Basic(ast.TypeKind(token.Value))
	default:
		result = ast.Struct(token.Value)
	}
	if c.peek() != nil && c.peek().Value == "?" {
		c.advance()
		result.Nullable = true
	}
	return result, nil
}

func parseCallee(tokens []lexer.Token) (string, error) {
	if len(tokens) == 0 {
		return "", fmt.Errorf("syntax error: expected call name")
	}
	var parts []string
	expectName := true
	for _, token := range tokens {
		if expectName {
			if token.Kind != lexer.TokenName {
				return "", errorAt(token.Location, "syntax error: expected call name")
			}
			parts = append(parts, token.Value)
			expectName = false
			continue
		}
		if token.Value != "." {
			return "", errorAt(token.Location, "syntax error: expected .")
		}
		expectName = true
	}
	if expectName {
		return "", errorAt(tokens[len(tokens)-1].Location, "syntax error: expected qualified name")
	}
	return strings.Join(parts, "."), nil
}

func isCloseOnly(tokens []lexer.Token) bool {
	return len(tokens) == 1 && (tokens[0].Value == ")" || tokens[0].Value == "]")
}

func errorAt(location ast.Location, format string, args ...any) error {
	return diagnostic.Errorf(location, format, args...)
}
