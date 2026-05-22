package checker

import (
	"fmt"

	"walklang/internal/ast"
)

type symbol struct {
	typeName ast.TypeName
	mutable  bool
}

type Checker struct {
	symbols map[string]symbol
}

func Check(program *ast.Program) error {
	c := Checker{symbols: map[string]symbol{}}
	return c.check(program)
}

func (c *Checker) check(program *ast.Program) error {
	for _, statement := range program.Statements {
		switch s := statement.(type) {
		case *ast.VarDecl:
			if err := c.checkVarDecl(s); err != nil {
				return err
			}
		case *ast.Assignment:
			if err := c.checkAssignment(s); err != nil {
				return err
			}
		case *ast.Out:
			if _, err := c.checkExpression(s.Value); err != nil {
				return err
			}
		default:
			return errorAt(statement.Loc(), "internal error: unknown statement")
		}
	}
	return nil
}

func (c *Checker) checkVarDecl(statement *ast.VarDecl) error {
	if _, exists := c.symbols[statement.Name]; exists {
		return errorAt(statement.Location, "name error: %s is already defined", statement.Name)
	}
	valueType, err := c.checkExpression(statement.Value)
	if err != nil {
		return err
	}
	declaredType := valueType
	if statement.Annotation != "" {
		if !isBasicType(statement.Annotation) {
			return errorAt(statement.Location, "type error: unknown type %s", statement.Annotation)
		}
		declaredType = statement.Annotation
	}
	if !assignable(valueType, declaredType) {
		return errorAt(statement.Value.Loc(), "type error: %s is %s, got %s", statement.Name, declaredType, valueType)
	}
	statement.Value.SetExprType(declaredType)
	c.symbols[statement.Name] = symbol{typeName: declaredType, mutable: statement.Mutable}
	return nil
}

func (c *Checker) checkAssignment(statement *ast.Assignment) error {
	sym, exists := c.symbols[statement.Name]
	if !exists {
		return errorAt(statement.Location, "name error: %s is not defined", statement.Name)
	}
	if !sym.mutable {
		return errorAt(statement.Location, "type error: %s is const and cannot be reassigned", statement.Name)
	}
	valueType, err := c.checkExpression(statement.Value)
	if err != nil {
		return err
	}
	if !assignable(valueType, sym.typeName) {
		return errorAt(statement.Location, "type error: %s is %s, got %s", statement.Name, sym.typeName, valueType)
	}
	return nil
}

func (c *Checker) checkExpression(expression ast.Expression) (ast.TypeName, error) {
	var result ast.TypeName
	switch e := expression.(type) {
	case *ast.Literal:
		switch e.Kind {
		case ast.LiteralInt:
			result = ast.TypeInt
		case ast.LiteralFloat:
			result = ast.TypeFloat
		case ast.LiteralBool:
			result = ast.TypeBool
		case ast.LiteralString:
			result = ast.TypeString
		default:
			return "", errorAt(e.Loc(), "internal error: unknown literal")
		}
	case *ast.Name:
		sym, exists := c.symbols[e.Identifier]
		if !exists {
			return "", errorAt(e.Loc(), "name error: %s is not defined", e.Identifier)
		}
		result = sym.typeName
	case *ast.Prefix:
		var err error
		result, err = c.checkPrefix(e)
		if err != nil {
			return "", err
		}
	default:
		return "", errorAt(expression.Loc(), "internal error: unknown expression")
	}
	expression.SetExprType(result)
	return result, nil
}

func (c *Checker) checkPrefix(expression *ast.Prefix) (ast.TypeName, error) {
	var argTypes []ast.TypeName
	for _, arg := range expression.Args {
		argType, err := c.checkExpression(arg)
		if err != nil {
			return "", err
		}
		argTypes = append(argTypes, argType)
	}

	switch expression.Operator {
	case "+", "-", "*", "/", "^":
		for _, argType := range argTypes {
			if argType != ast.TypeInt && argType != ast.TypeFloat {
				return "", errorAt(expression.Loc(), "type error: operator %s needs numeric operands", expression.Operator)
			}
		}
		if expression.Operator == "/" || containsType(argTypes, ast.TypeFloat) {
			return ast.TypeFloat, nil
		}
		return ast.TypeInt, nil
	case ">", "<", ">=", "<=":
		for _, argType := range argTypes {
			if argType != ast.TypeInt && argType != ast.TypeFloat {
				return "", errorAt(expression.Loc(), "type error: operator %s needs numeric operands", expression.Operator)
			}
		}
		return ast.TypeBool, nil
	case "==", "!=":
		left, right := argTypes[0], argTypes[1]
		if !assignable(left, right) && !assignable(right, left) {
			return "", errorAt(expression.Loc(), "type error: cannot compare %s and %s", left, right)
		}
		return ast.TypeBool, nil
	case "and", "or":
		for _, argType := range argTypes {
			if argType != ast.TypeBool {
				return "", errorAt(expression.Loc(), "type error: operator %s needs bool operands", expression.Operator)
			}
		}
		return ast.TypeBool, nil
	case "not":
		if argTypes[0] != ast.TypeBool {
			return "", errorAt(expression.Loc(), "type error: operator not needs one bool operand")
		}
		return ast.TypeBool, nil
	}
	return "", errorAt(expression.Loc(), "internal error: unsupported operator %s", expression.Operator)
}

func containsType(types []ast.TypeName, target ast.TypeName) bool {
	for _, typeName := range types {
		if typeName == target {
			return true
		}
	}
	return false
}

func assignable(source ast.TypeName, target ast.TypeName) bool {
	return source == target || source == ast.TypeInt && target == ast.TypeFloat
}

func isBasicType(typeName ast.TypeName) bool {
	return typeName == ast.TypeInt || typeName == ast.TypeFloat || typeName == ast.TypeBool || typeName == ast.TypeString
}

func errorAt(location ast.Location, format string, args ...any) error {
	return fmt.Errorf("%s:%d: %s", location.Filename, location.Line, fmt.Sprintf(format, args...))
}
