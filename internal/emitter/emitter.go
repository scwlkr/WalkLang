package emitter

import (
	"fmt"
	"strings"

	"walklang/internal/ast"
)

func EmitC(program *ast.Program) (string, error) {
	e := &cEmitter{}
	return e.emit(program)
}

type cEmitter struct {
	tempID int
	indent int
}

func (e *cEmitter) emit(program *ast.Program) (string, error) {
	var out strings.Builder
	out.WriteString("#include <math.h>\n")
	out.WriteString("#include <stdbool.h>\n")
	out.WriteString("#include <stddef.h>\n")
	out.WriteString("#include <stdio.h>\n")
	out.WriteString("#include <stdlib.h>\n")
	out.WriteString("#include <string.h>\n\n")
	out.WriteString("typedef struct { long long *items; long long len; } WalkArrayInt;\n")
	out.WriteString("typedef struct { double *items; long long len; } WalkArrayFloat;\n")
	out.WriteString("typedef struct { bool *items; long long len; } WalkArrayBool;\n")
	out.WriteString("typedef struct { const char **items; long long len; } WalkArrayString;\n\n")
	out.WriteString("static long long __walk_random_int(long long min, long long max) {\n")
	out.WriteString("    if (max < min) { return min; }\n")
	out.WriteString("    return min + (rand() % (max - min + 1));\n")
	out.WriteString("}\n\n")

	for _, statement := range program.Statements {
		fn, ok := statement.(*ast.FuncDecl)
		if !ok {
			continue
		}
		prototype, err := e.emitFunctionSignature(fn, true)
		if err != nil {
			return "", err
		}
		out.WriteString(prototype)
		out.WriteString(";\n")
	}
	if hasFunctions(program) {
		out.WriteString("\n")
	}
	for _, statement := range program.Statements {
		fn, ok := statement.(*ast.FuncDecl)
		if !ok {
			continue
		}
		rendered, err := e.emitFunction(fn)
		if err != nil {
			return "", err
		}
		out.WriteString(rendered)
		out.WriteString("\n")
	}

	out.WriteString("int main(void) {\n")
	e.indent = 1
	for _, statement := range program.Statements {
		switch statement.(type) {
		case *ast.FuncDecl, *ast.Import, *ast.Export:
			continue
		}
		lines, err := e.emitStatement(statement)
		if err != nil {
			return "", err
		}
		for _, line := range lines {
			out.WriteString(e.indentString())
			out.WriteString(line)
			out.WriteByte('\n')
		}
	}
	out.WriteString("    return 0;\n")
	out.WriteString("}\n")
	return out.String(), nil
}

func hasFunctions(program *ast.Program) bool {
	for _, statement := range program.Statements {
		if _, ok := statement.(*ast.FuncDecl); ok {
			return true
		}
	}
	return false
}

func (e *cEmitter) emitFunction(fn *ast.FuncDecl) (string, error) {
	signature, err := e.emitFunctionSignature(fn, false)
	if err != nil {
		return "", err
	}
	var out strings.Builder
	out.WriteString(signature)
	out.WriteString(" {\n")
	e.indent = 1
	lines, err := e.emitBlock(fn.Body)
	if err != nil {
		return "", err
	}
	for _, line := range lines {
		out.WriteString(e.indentString())
		out.WriteString(line)
		out.WriteByte('\n')
	}
	if fn.ReturnType.Kind == ast.TypeVoid {
		out.WriteString("    return;\n")
	}
	out.WriteString("}\n")
	return out.String(), nil
}

func (e *cEmitter) emitFunctionSignature(fn *ast.FuncDecl, prototype bool) (string, error) {
	ret, err := cReturnType(fn.ReturnType)
	if err != nil {
		return "", err
	}
	params := make([]string, 0, len(fn.Params))
	for _, param := range fn.Params {
		rendered, err := cDecl(param.Type, param.Name, true)
		if err != nil {
			return "", err
		}
		params = append(params, rendered)
	}
	if len(params) == 0 {
		params = append(params, "void")
	}
	return fmt.Sprintf("%s %s(%s)", ret, fn.Name, strings.Join(params, ", ")), nil
}

func (e *cEmitter) emitBlock(statements []ast.Statement) ([]string, error) {
	var out []string
	for _, statement := range statements {
		lines, err := e.emitStatement(statement)
		if err != nil {
			return nil, err
		}
		out = append(out, lines...)
	}
	return out, nil
}

func (e *cEmitter) emitStatement(statement ast.Statement) ([]string, error) {
	switch s := statement.(type) {
	case *ast.VarDecl:
		return e.emitVarDecl(s)
	case *ast.Assignment:
		target, err := e.emitExpression(s.Target)
		if err != nil {
			return nil, err
		}
		value, err := e.emitExpression(s.Value)
		if err != nil {
			return nil, err
		}
		return []string{fmt.Sprintf("%s = %s;", target, value)}, nil
	case *ast.Out:
		line, err := e.emitOut(s.Value)
		return []string{line}, err
	case *ast.Return:
		value, err := e.emitExpression(s.Value)
		if err != nil {
			return nil, err
		}
		return []string{fmt.Sprintf("return %s;", value)}, nil
	case *ast.If:
		return e.emitIf(s)
	case *ast.While:
		return e.emitWhile(s)
	case *ast.Repeat:
		return e.emitRepeat(s)
	case *ast.For:
		return e.emitFor(s)
	case *ast.Break:
		return []string{"break;"}, nil
	case *ast.Continue:
		return []string{"continue;"}, nil
	default:
		return nil, errorAt(statement.Loc(), "internal error: unknown statement")
	}
}

func (e *cEmitter) emitVarDecl(statement *ast.VarDecl) ([]string, error) {
	typeName := statement.Value.ExprType()
	if statement.Annotation.Kind != ast.TypeInvalid {
		typeName = statement.Annotation
	}
	array, ok := statement.Value.(*ast.ArrayLiteral)
	if ok {
		return e.emitArrayDecl(statement.Name, typeName, array, statement.Mutable)
	}
	value, err := e.emitExpression(statement.Value)
	if err != nil {
		return nil, err
	}
	decl, err := cDecl(typeName, statement.Name, statement.Mutable)
	if err != nil {
		return nil, err
	}
	return []string{fmt.Sprintf("%s = %s;", decl, value)}, nil
}

func (e *cEmitter) emitArrayDecl(name string, typeName ast.Type, array *ast.ArrayLiteral, mutable bool) ([]string, error) {
	if typeName.Kind != ast.TypeArray || typeName.Elem == nil {
		return nil, errorAt(array.Loc(), "internal error: array literal has non-array type")
	}
	itemName := e.nextTemp(name + "_items")
	itemType, err := cArrayItemType(*typeName.Elem)
	if err != nil {
		return nil, err
	}
	values := make([]string, 0, len(array.Elements))
	for _, element := range array.Elements {
		value, err := e.emitExpression(element)
		if err != nil {
			return nil, err
		}
		values = append(values, value)
	}
	decl, err := cDecl(typeName, name, mutable)
	if err != nil {
		return nil, err
	}
	return []string{
		fmt.Sprintf("%s %s[] = {%s};", itemType, itemName, strings.Join(values, ", ")),
		fmt.Sprintf("%s = {%s, %d};", decl, itemName, len(values)),
	}, nil
}

func (e *cEmitter) emitIf(statement *ast.If) ([]string, error) {
	cond, err := e.emitExpression(statement.Cond)
	if err != nil {
		return nil, err
	}
	lines := []string{fmt.Sprintf("if (%s) {", cond)}
	e.indent++
	thenLines, err := e.emitBlock(statement.Then)
	e.indent--
	if err != nil {
		return nil, err
	}
	lines = appendIndented(lines, thenLines)
	if len(statement.Else) == 0 {
		lines = append(lines, "}")
		return lines, nil
	}
	lines = append(lines, "} else {")
	e.indent++
	elseLines, err := e.emitBlock(statement.Else)
	e.indent--
	if err != nil {
		return nil, err
	}
	lines = appendIndented(lines, elseLines)
	lines = append(lines, "}")
	return lines, nil
}

func (e *cEmitter) emitWhile(statement *ast.While) ([]string, error) {
	cond, err := e.emitExpression(statement.Cond)
	if err != nil {
		return nil, err
	}
	lines := []string{fmt.Sprintf("while (%s) {", cond)}
	e.indent++
	body, err := e.emitBlock(statement.Body)
	e.indent--
	if err != nil {
		return nil, err
	}
	lines = appendIndented(lines, body)
	lines = append(lines, "}")
	return lines, nil
}

func (e *cEmitter) emitRepeat(statement *ast.Repeat) ([]string, error) {
	count, err := e.emitExpression(statement.Count)
	if err != nil {
		return nil, err
	}
	i := e.nextTemp("__repeat")
	lines := []string{fmt.Sprintf("for (long long %s = 0; %s < (%s); %s++) {", i, i, count, i)}
	e.indent++
	body, err := e.emitBlock(statement.Body)
	e.indent--
	if err != nil {
		return nil, err
	}
	lines = appendIndented(lines, body)
	lines = append(lines, "}")
	return lines, nil
}

func (e *cEmitter) emitFor(statement *ast.For) ([]string, error) {
	iterable, err := e.emitExpression(statement.Iterable)
	if err != nil {
		return nil, err
	}
	itemType, err := cArrayItemType(*statement.Iterable.ExprType().Elem)
	if err != nil {
		return nil, err
	}
	i := e.nextTemp("__for")
	lines := []string{fmt.Sprintf("for (long long %s = 0; %s < %s.len; %s++) {", i, i, iterable, i)}
	lines = append(lines, fmt.Sprintf("    %s %s = %s.items[%s];", itemType, statement.Name, iterable, i))
	e.indent++
	body, err := e.emitBlock(statement.Body)
	e.indent--
	if err != nil {
		return nil, err
	}
	lines = appendIndented(lines, body)
	lines = append(lines, "}")
	return lines, nil
}

func appendIndented(lines []string, nested []string) []string {
	for _, line := range nested {
		lines = append(lines, "    "+line)
	}
	return lines
}

func (e *cEmitter) emitOut(expression ast.Expression) (string, error) {
	value, err := e.emitExpression(expression)
	if err != nil {
		return "", err
	}
	switch expression.ExprType().Kind {
	case ast.TypeInt:
		return fmt.Sprintf("printf(\"%%lld\\n\", (long long)(%s));", value), nil
	case ast.TypeFloat:
		return fmt.Sprintf("printf(\"%%g\\n\", (double)(%s));", value), nil
	case ast.TypeBool:
		return fmt.Sprintf("printf(\"%%s\\n\", (%s) ? \"true\" : \"false\");", value), nil
	case ast.TypeString:
		return fmt.Sprintf("printf(\"%%s\\n\", %s == NULL ? \"null\" : %s);", value, value), nil
	default:
		return "", errorAt(expression.Loc(), "internal error: cannot print %s", expression.ExprType().String())
	}
}

func (e *cEmitter) emitExpression(expression ast.Expression) (string, error) {
	switch ex := expression.(type) {
	case *ast.Literal:
		return emitLiteral(ex)
	case *ast.Name:
		return ex.Identifier, nil
	case *ast.Prefix:
		return e.emitPrefix(ex)
	case *ast.Call:
		return e.emitCall(ex)
	case *ast.Index:
		target, err := e.emitExpression(ex.Target)
		if err != nil {
			return "", err
		}
		index, err := e.emitExpression(ex.Index)
		if err != nil {
			return "", err
		}
		return fmt.Sprintf("%s.items[%s]", target, index), nil
	case *ast.ArrayLiteral:
		return "", errorAt(ex.Loc(), "internal error: array literal cannot be emitted inline")
	default:
		return "", errorAt(expression.Loc(), "internal error: unknown expression")
	}
}

func emitLiteral(literal *ast.Literal) (string, error) {
	switch literal.Kind {
	case ast.LiteralInt, ast.LiteralFloat:
		return literal.Value, nil
	case ast.LiteralBool:
		return literal.Value, nil
	case ast.LiteralString:
		return `"` + escapeCString(literal.Value) + `"`, nil
	case ast.LiteralNull:
		return "NULL", nil
	default:
		return "", errorAt(literal.Loc(), "internal error: unsupported literal")
	}
}

func (e *cEmitter) emitPrefix(expression *ast.Prefix) (string, error) {
	var args []string
	for _, arg := range expression.Args {
		rendered, err := e.emitExpression(arg)
		if err != nil {
			return "", err
		}
		args = append(args, rendered)
	}

	switch expression.Operator {
	case "+", "*":
		return "(" + strings.Join(args, " "+expression.Operator+" ") + ")", nil
	case "and":
		return "(" + strings.Join(args, " && ") + ")", nil
	case "or":
		return "(" + strings.Join(args, " || ") + ")", nil
	case "not":
		return "(!" + args[0] + ")", nil
	case "-":
		return fmt.Sprintf("(%s - %s)", args[0], args[1]), nil
	case "/":
		return fmt.Sprintf("((double)(%s) / (%s))", args[0], args[1]), nil
	case ">", "<", ">=", "<=", "==", "!=":
		if expression.Args[0].ExprType().Kind == ast.TypeString && expression.Args[1].ExprType().Kind == ast.TypeString {
			comparison := "=="
			if expression.Operator == "!=" {
				comparison = "!="
			}
			if expression.Operator == "==" || expression.Operator == "!=" {
				return fmt.Sprintf("(strcmp(%s, %s) %s 0)", args[0], args[1], comparison), nil
			}
		}
		return fmt.Sprintf("(%s %s %s)", args[0], expression.Operator, args[1]), nil
	case "^":
		return fmt.Sprintf("pow(%s, %s)", args[0], args[1]), nil
	default:
		return "", errorAt(expression.Loc(), "internal error: unsupported operator %s", expression.Operator)
	}
}

func (e *cEmitter) emitCall(call *ast.Call) (string, error) {
	args := make([]string, 0, len(call.Args))
	for _, arg := range call.Args {
		rendered, err := e.emitExpression(arg)
		if err != nil {
			return "", err
		}
		args = append(args, rendered)
	}
	switch call.Callee {
	case "math.sqrt":
		return fmt.Sprintf("sqrt(%s)", strings.Join(args, ", ")), nil
	case "random.int":
		return fmt.Sprintf("__walk_random_int(%s)", strings.Join(args, ", ")), nil
	default:
		return fmt.Sprintf("%s(%s)", call.Callee, strings.Join(args, ", ")), nil
	}
}

func cReturnType(typeName ast.Type) (string, error) {
	if typeName.Kind == ast.TypeVoid {
		return "void", nil
	}
	return cValueType(typeName)
}

func cDecl(typeName ast.Type, name string, mutable bool) (string, error) {
	if typeName.Kind == ast.TypeFunction {
		if typeName.Return == nil {
			return "", fmt.Errorf("internal error: function type needs return")
		}
		ret, err := cReturnType(*typeName.Return)
		if err != nil {
			return "", err
		}
		params := make([]string, 0, len(typeName.Params))
		for _, param := range typeName.Params {
			rendered, err := cValueType(param)
			if err != nil {
				return "", err
			}
			params = append(params, rendered)
		}
		if len(params) == 0 {
			params = append(params, "void")
		}
		return fmt.Sprintf("%s (*%s)(%s)", ret, name, strings.Join(params, ", ")), nil
	}
	valueType, err := cValueType(typeName)
	if err != nil {
		return "", err
	}
	if mutable || typeName.Nullable {
		return valueType + " " + name, nil
	}
	return "const " + valueType + " " + name, nil
}

func cValueType(typeName ast.Type) (string, error) {
	switch typeName.Kind {
	case ast.TypeInt:
		return "long long", nil
	case ast.TypeFloat:
		return "double", nil
	case ast.TypeBool:
		return "bool", nil
	case ast.TypeString:
		return "const char *", nil
	case ast.TypeArray:
		if typeName.Elem == nil {
			return "", fmt.Errorf("internal error: array type needs element")
		}
		switch typeName.Elem.Kind {
		case ast.TypeInt:
			return "WalkArrayInt", nil
		case ast.TypeFloat:
			return "WalkArrayFloat", nil
		case ast.TypeBool:
			return "WalkArrayBool", nil
		case ast.TypeString:
			return "WalkArrayString", nil
		}
	}
	return "", fmt.Errorf("internal error: unsupported C type %s", typeName.String())
}

func cArrayItemType(typeName ast.Type) (string, error) {
	switch typeName.Kind {
	case ast.TypeInt:
		return "long long", nil
	case ast.TypeFloat:
		return "double", nil
	case ast.TypeBool:
		return "bool", nil
	case ast.TypeString:
		return "const char *", nil
	default:
		return "", fmt.Errorf("internal error: unsupported array element type %s", typeName.String())
	}
}

func (e *cEmitter) nextTemp(prefix string) string {
	e.tempID++
	clean := strings.ReplaceAll(prefix, ".", "_")
	return fmt.Sprintf("__%s_%d", clean, e.tempID)
}

func (e *cEmitter) indentString() string {
	return strings.Repeat("    ", e.indent)
}

func escapeCString(value string) string {
	replacer := strings.NewReplacer("\\", "\\\\", "\"", "\\\"", "\n", "\\n", "\t", "\\t")
	return replacer.Replace(value)
}

func errorAt(location ast.Location, format string, args ...any) error {
	return fmt.Errorf("%s:%d: %s", location.Filename, location.Line, fmt.Sprintf(format, args...))
}
