package emitter

import (
	"fmt"
	"strings"

	"walklang/internal/ast"
)

func EmitC(program *ast.Program) (string, error) {
	e := cEmitter{}
	return e.emit(program)
}

type cEmitter struct{}

func (e cEmitter) emit(program *ast.Program) (string, error) {
	var out strings.Builder
	out.WriteString("#include <math.h>\n")
	out.WriteString("#include <stdbool.h>\n")
	out.WriteString("#include <stdio.h>\n")
	out.WriteString("#include <string.h>\n\n")
	out.WriteString("int main(void) {\n")
	for _, statement := range program.Statements {
		line, err := e.emitStatement(statement)
		if err != nil {
			return "", err
		}
		out.WriteString("    ")
		out.WriteString(line)
		out.WriteByte('\n')
	}
	out.WriteString("    return 0;\n")
	out.WriteString("}\n")
	return out.String(), nil
}

func (e cEmitter) emitStatement(statement ast.Statement) (string, error) {
	switch s := statement.(type) {
	case *ast.VarDecl:
		typeName := s.Value.ExprType()
		if s.Annotation != "" {
			typeName = s.Annotation
		}
		cType, err := cDeclType(typeName, s.Mutable)
		if err != nil {
			return "", err
		}
		value, err := e.emitExpression(s.Value)
		if err != nil {
			return "", err
		}
		return fmt.Sprintf("%s %s = %s;", cType, s.Name, value), nil
	case *ast.Assignment:
		value, err := e.emitExpression(s.Value)
		if err != nil {
			return "", err
		}
		return fmt.Sprintf("%s = %s;", s.Name, value), nil
	case *ast.Out:
		return e.emitOut(s.Value)
	default:
		return "", errorAt(statement.Loc(), "internal error: unknown statement")
	}
}

func (e cEmitter) emitOut(expression ast.Expression) (string, error) {
	value, err := e.emitExpression(expression)
	if err != nil {
		return "", err
	}
	switch expression.ExprType() {
	case ast.TypeInt:
		return fmt.Sprintf("printf(\"%%lld\\n\", (long long)(%s));", value), nil
	case ast.TypeFloat:
		return fmt.Sprintf("printf(\"%%g\\n\", (double)(%s));", value), nil
	case ast.TypeBool:
		return fmt.Sprintf("printf(\"%%s\\n\", (%s) ? \"true\" : \"false\");", value), nil
	case ast.TypeString:
		return fmt.Sprintf("printf(\"%%s\\n\", %s);", value), nil
	default:
		return "", errorAt(expression.Loc(), "internal error: cannot print %s", expression.ExprType())
	}
}

func (e cEmitter) emitExpression(expression ast.Expression) (string, error) {
	switch ex := expression.(type) {
	case *ast.Literal:
		return emitLiteral(ex)
	case *ast.Name:
		return ex.Identifier, nil
	case *ast.Prefix:
		return e.emitPrefix(ex)
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
	default:
		return "", errorAt(literal.Loc(), "internal error: unsupported literal")
	}
}

func (e cEmitter) emitPrefix(expression *ast.Prefix) (string, error) {
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
		if expression.Operator == "==" && expression.Args[0].ExprType() == ast.TypeString {
			return fmt.Sprintf("(strcmp(%s, %s) == 0)", args[0], args[1]), nil
		}
		if expression.Operator == "!=" && expression.Args[0].ExprType() == ast.TypeString {
			return fmt.Sprintf("(strcmp(%s, %s) != 0)", args[0], args[1]), nil
		}
		return fmt.Sprintf("(%s %s %s)", args[0], expression.Operator, args[1]), nil
	case "^":
		return fmt.Sprintf("pow(%s, %s)", args[0], args[1]), nil
	default:
		return "", errorAt(expression.Loc(), "internal error: unsupported operator %s", expression.Operator)
	}
}

func cDeclType(typeName ast.TypeName, mutable bool) (string, error) {
	switch typeName {
	case ast.TypeInt:
		if mutable {
			return "long long", nil
		}
		return "const long long", nil
	case ast.TypeFloat:
		if mutable {
			return "double", nil
		}
		return "const double", nil
	case ast.TypeBool:
		if mutable {
			return "bool", nil
		}
		return "const bool", nil
	case ast.TypeString:
		if mutable {
			return "const char *", nil
		}
		return "const char * const", nil
	default:
		return "", fmt.Errorf("internal error: unsupported C type %s", typeName)
	}
}

func escapeCString(value string) string {
	replacer := strings.NewReplacer("\\", "\\\\", "\"", "\\\"", "\n", "\\n", "\t", "\\t")
	return replacer.Replace(value)
}

func errorAt(location ast.Location, format string, args ...any) error {
	return fmt.Errorf("%s:%d: %s", location.Filename, location.Line, fmt.Sprintf(format, args...))
}
