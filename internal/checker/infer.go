package checker

import (
	"strings"

	"walklang/internal/ast"
)

type inferSymbol struct {
	typeName ast.Type
	mutable  bool
}

type functionInferencer struct {
	checker   *Checker
	fn        *ast.FuncDecl
	scopes    []map[string]inferSymbol
	functions map[string]*ast.FuncDecl
	returns   []ast.Type
}

func (c *Checker) inferFunctionTypes(program *ast.Program) error {
	functions := map[string]*ast.FuncDecl{}
	for _, statement := range program.Statements {
		fn, ok := statement.(*ast.FuncDecl)
		if !ok || fn.Receiver != "" || len(fn.TypeParams) > 0 {
			continue
		}
		functions[fn.Name] = fn
	}

	for _, statement := range program.Statements {
		fn, ok := statement.(*ast.FuncDecl)
		if !ok || len(fn.TypeParams) > 0 {
			continue
		}
		inf := functionInferencer{
			checker:   c,
			fn:        fn,
			scopes:    []map[string]inferSymbol{{}},
			functions: functions,
		}
		if err := inf.infer(); err != nil {
			return err
		}
	}
	return nil
}

func (c *Checker) refreshMethodTypes(program *ast.Program) {
	for _, statement := range program.Statements {
		fn, ok := statement.(*ast.FuncDecl)
		if !ok || fn.Receiver == "" {
			continue
		}
		if c.methods[fn.Receiver] == nil {
			continue
		}
		def, ok := c.methods[fn.Receiver][fn.Name]
		if !ok {
			continue
		}
		def.Type = functionType(fn)
		def.Func = fn
		c.methods[fn.Receiver][fn.Name] = def
	}
}

func (i *functionInferencer) infer() error {
	i.pushScope()
	for index, param := range i.fn.Params {
		if i.fn.Receiver != "" && index == 0 && !knownType(param.Type) {
			return errorAt(i.fn.Location, "type error: method %s receiver parameter needs an explicit type", funcName(i.fn))
		}
		if knownType(param.Type) {
			if err := i.define(param.Name, param.Type, true, i.fn.Location); err != nil {
				return err
			}
			continue
		}
		if err := i.define(param.Name, ast.Type{}, true, i.fn.Location); err != nil {
			return err
		}
	}
	if err := i.inferBlock(i.fn.Body); err != nil {
		i.popScope()
		return err
	}
	for index, param := range i.fn.Params {
		if knownType(param.Type) {
			continue
		}
		sym, _ := i.resolve(param.Name)
		if !knownType(sym.typeName) {
			i.popScope()
			return errorAt(i.fn.Location, "type error: cannot infer type for parameter %s in function %s; add an annotation", param.Name, funcName(i.fn))
		}
		i.fn.Params[index].Type = sym.typeName
	}
	if !knownType(i.fn.ReturnType) {
		returnType := ast.Basic(ast.TypeVoid)
		if len(i.returns) > 0 {
			returnType = i.returns[0]
			for _, next := range i.returns[1:] {
				merged, ok := commonType(returnType, next)
				if !ok {
					i.popScope()
					return errorAt(i.fn.Location, "type error: cannot infer return type for function %s from %s and %s; add an annotation", funcName(i.fn), returnType.String(), next.String())
				}
				returnType = merged
			}
			if !knownType(returnType) {
				i.popScope()
				return errorAt(i.fn.Location, "type error: cannot infer return type for function %s; add an annotation", funcName(i.fn))
			}
		}
		i.fn.ReturnType = returnType
	}
	i.popScope()
	return nil
}

func (i *functionInferencer) inferBlock(statements []ast.Statement) error {
	for _, statement := range statements {
		if err := i.inferStatement(statement); err != nil {
			return err
		}
	}
	return nil
}

func (i *functionInferencer) inferNestedBlock(statements []ast.Statement) error {
	i.pushScope()
	err := i.inferBlock(statements)
	i.popScope()
	return err
}

func (i *functionInferencer) inferStatement(statement ast.Statement) error {
	switch s := statement.(type) {
	case *ast.VarDecl:
		valueType, err := i.inferExpression(s.Value, s.Annotation)
		if err != nil {
			return err
		}
		declaredType := valueType
		if knownType(s.Annotation) {
			declaredType = s.Annotation
		}
		return i.define(s.Name, declaredType, s.Mutable, s.Location)
	case *ast.Assignment:
		targetType, err := i.inferAssignableTarget(s.Target)
		if err != nil {
			return err
		}
		valueType, err := i.inferExpression(s.Value, targetType)
		if err != nil {
			return err
		}
		if !knownType(targetType) && knownType(valueType) {
			return i.assignTargetType(s.Target, valueType)
		}
	case *ast.Out:
		_, err := i.inferExpression(s.Value, ast.Type{})
		return err
	case *ast.Assert:
		_, err := i.inferExpression(s.Value, ast.Basic(ast.TypeBool))
		return err
	case *ast.Return:
		expected := i.fn.ReturnType
		valueType, err := i.inferExpression(s.Value, expected)
		if err != nil {
			return err
		}
		i.returns = append(i.returns, valueType)
	case *ast.If:
		if _, err := i.inferExpression(s.Cond, ast.Basic(ast.TypeBool)); err != nil {
			return err
		}
		if err := i.inferNestedBlock(s.Then); err != nil {
			return err
		}
		return i.inferNestedBlock(s.Else)
	case *ast.While:
		if _, err := i.inferExpression(s.Cond, ast.Basic(ast.TypeBool)); err != nil {
			return err
		}
		return i.inferNestedBlock(s.Body)
	case *ast.Repeat:
		if _, err := i.inferExpression(s.Count, ast.Basic(ast.TypeInt)); err != nil {
			return err
		}
		return i.inferNestedBlock(s.Body)
	case *ast.For:
		iterType, err := i.inferExpression(s.Iterable, ast.Type{})
		if err != nil {
			return err
		}
		i.pushScope()
		if iterType.Kind == ast.TypeArray && iterType.Elem != nil {
			if err := i.define(s.Name, *iterType.Elem, true, s.Location); err != nil {
				i.popScope()
				return err
			}
		} else if err := i.define(s.Name, ast.Type{}, true, s.Location); err != nil {
			i.popScope()
			return err
		}
		err = i.inferBlock(s.Body)
		i.popScope()
		return err
	}
	return nil
}

func (i *functionInferencer) inferExpression(expression ast.Expression, expected ast.Type) (ast.Type, error) {
	switch e := expression.(type) {
	case *ast.Literal:
		return literalType(e), nil
	case *ast.InterpolatedString:
		for _, part := range e.Parts {
			if part.Expression == nil {
				continue
			}
			if _, err := i.inferExpression(part.Expression, ast.Type{}); err != nil {
				return ast.Type{}, err
			}
		}
		return ast.Basic(ast.TypeString), nil
	case *ast.Name:
		if sym, ok := i.resolve(e.Identifier); ok {
			if knownType(expected) && !knownType(sym.typeName) {
				if err := i.setNameType(e.Identifier, expected, e.Loc()); err != nil {
					return ast.Type{}, err
				}
				return expected, nil
			}
			return sym.typeName, nil
		}
		if fn, ok := i.functions[e.Identifier]; ok && functionTypeKnown(fn) {
			return functionType(fn), nil
		}
		return ast.Type{}, nil
	case *ast.Prefix:
		return i.inferPrefix(e, expected)
	case *ast.Call:
		return i.inferCall(e, expected)
	case *ast.Input:
		if e.Prompt != nil {
			if _, err := i.inferExpression(e.Prompt, ast.Basic(ast.TypeString)); err != nil {
				return ast.Type{}, err
			}
		}
		return ast.Basic(ast.TypeString), nil
	case *ast.ArrayLiteral:
		return i.inferArrayLiteral(e, expected)
	case *ast.Index:
		targetType, err := i.inferExpression(e.Target, ast.Type{})
		if err != nil {
			return ast.Type{}, err
		}
		if _, err := i.inferExpression(e.Index, ast.Basic(ast.TypeInt)); err != nil {
			return ast.Type{}, err
		}
		if targetType.Kind == ast.TypeArray && targetType.Elem != nil {
			return *targetType.Elem, nil
		}
		if targetType.Kind == ast.TypeString {
			return ast.Basic(ast.TypeString), nil
		}
		return ast.Type{}, nil
	case *ast.FieldAccess:
		targetType, err := i.inferExpression(e.Target, ast.Type{})
		if err != nil {
			return ast.Type{}, err
		}
		if targetType.Kind != ast.TypeStruct {
			return ast.Type{}, nil
		}
		field, err := i.checker.lookupField(targetType, e.Field, e.Loc())
		if err != nil {
			return ast.Type{}, err
		}
		return field.Type, nil
	}
	return ast.Type{}, nil
}

func (i *functionInferencer) inferPrefix(expression *ast.Prefix, expected ast.Type) (ast.Type, error) {
	switch expression.Operator {
	case "+", "-", "*", "/", "^":
		argTypes, err := i.inferNumericArgs(expression.Args, expression.Operator, expected)
		if err != nil {
			return ast.Type{}, err
		}
		if expression.Operator == "/" || containsKind(argTypes, ast.TypeFloat) {
			return ast.Basic(ast.TypeFloat), nil
		}
		return ast.Basic(ast.TypeInt), nil
	case ">", "<", ">=", "<=":
		_, err := i.inferNumericArgs(expression.Args, expression.Operator, ast.Type{})
		if err != nil {
			return ast.Type{}, err
		}
		return ast.Basic(ast.TypeBool), nil
	case "==", "!=":
		left, err := i.inferExpression(expression.Args[0], ast.Type{})
		if err != nil {
			return ast.Type{}, err
		}
		right, err := i.inferExpression(expression.Args[1], left)
		if err != nil {
			return ast.Type{}, err
		}
		if !knownType(left) && knownType(right) {
			if _, err := i.inferExpression(expression.Args[0], right); err != nil {
				return ast.Type{}, err
			}
		}
		return ast.Basic(ast.TypeBool), nil
	case "and", "or":
		for _, arg := range expression.Args {
			if _, err := i.inferExpression(arg, ast.Basic(ast.TypeBool)); err != nil {
				return ast.Type{}, err
			}
		}
		return ast.Basic(ast.TypeBool), nil
	case "not":
		if _, err := i.inferExpression(expression.Args[0], ast.Basic(ast.TypeBool)); err != nil {
			return ast.Type{}, err
		}
		return ast.Basic(ast.TypeBool), nil
	default:
		return ast.Type{}, nil
	}
}

func (i *functionInferencer) inferNumericArgs(args []ast.Expression, operator string, expected ast.Type) ([]ast.Type, error) {
	argTypes := make([]ast.Type, 0, len(args))
	preferred := ast.Basic(ast.TypeInt)
	if operator == "/" || expected.Kind == ast.TypeFloat {
		preferred = ast.Basic(ast.TypeFloat)
	}
	for _, arg := range args {
		argType, err := i.inferExpression(arg, ast.Type{})
		if err != nil {
			return nil, err
		}
		if argType.Kind == ast.TypeFloat {
			preferred = ast.Basic(ast.TypeFloat)
		}
		argTypes = append(argTypes, argType)
	}
	for index, argType := range argTypes {
		if knownType(argType) {
			continue
		}
		if _, err := i.inferExpression(args[index], preferred); err != nil {
			return nil, err
		}
		argTypes[index] = preferred
	}
	return argTypes, nil
}

func (i *functionInferencer) inferCall(call *ast.Call, expected ast.Type) (ast.Type, error) {
	if call.Receiver != nil {
		return ast.Type{}, nil
	}
	if strings.Contains(call.Callee, ".") {
		return i.inferModuleCall(call)
	}
	if def, ok := i.checker.structs[call.Callee]; ok {
		for index, arg := range call.Args {
			if index >= len(def.Fields) {
				break
			}
			if _, err := i.inferExpression(arg, def.Fields[index].Type); err != nil {
				return ast.Type{}, err
			}
		}
		return ast.Struct(def.Name), nil
	}
	if fn, ok := i.functions[call.Callee]; ok && functionTypeKnown(fn) {
		fnType := functionType(fn)
		for index, arg := range call.Args {
			if index >= len(fnType.Params) {
				break
			}
			if _, err := i.inferExpression(arg, fnType.Params[index]); err != nil {
				return ast.Type{}, err
			}
		}
		if fnType.Return != nil {
			return *fnType.Return, nil
		}
	}
	if knownType(expected) {
		return expected, nil
	}
	return ast.Type{}, nil
}

func (i *functionInferencer) inferModuleCall(call *ast.Call) (ast.Type, error) {
	module, name, ok := splitQualifiedCall(call.Callee)
	if !ok {
		return ast.Type{}, nil
	}
	if userModule, ok := i.checker.modules[module]; ok && userModule.Exports != nil {
		if fnType, ok := userModule.Exports[name]; ok {
			for index, arg := range call.Args {
				if index >= len(fnType.Params) {
					break
				}
				if _, err := i.inferExpression(arg, fnType.Params[index]); err != nil {
					return ast.Type{}, err
				}
			}
			if fnType.Return != nil {
				return *fnType.Return, nil
			}
		}
	}
	switch module + "." + name {
	case "math.sqrt", "math.pow":
		for _, arg := range call.Args {
			if _, err := i.inferExpression(arg, ast.Basic(ast.TypeFloat)); err != nil {
				return ast.Type{}, err
			}
		}
		return ast.Basic(ast.TypeFloat), nil
	case "string.len":
		if len(call.Args) > 0 {
			if _, err := i.inferExpression(call.Args[0], ast.Basic(ast.TypeString)); err != nil {
				return ast.Type{}, err
			}
		}
		return ast.Basic(ast.TypeInt), nil
	case "string.at":
		if len(call.Args) > 0 {
			if _, err := i.inferExpression(call.Args[0], ast.Basic(ast.TypeString)); err != nil {
				return ast.Type{}, err
			}
		}
		if len(call.Args) > 1 {
			if _, err := i.inferExpression(call.Args[1], ast.Basic(ast.TypeInt)); err != nil {
				return ast.Type{}, err
			}
		}
		return ast.Basic(ast.TypeString), nil
	case "string.contains":
		for _, arg := range call.Args {
			if _, err := i.inferExpression(arg, ast.Basic(ast.TypeString)); err != nil {
				return ast.Type{}, err
			}
		}
		return ast.Basic(ast.TypeBool), nil
	case "string.concat":
		for _, arg := range call.Args {
			if _, err := i.inferExpression(arg, ast.Basic(ast.TypeString)); err != nil {
				return ast.Type{}, err
			}
		}
		return ast.Basic(ast.TypeString), nil
	case "array.len":
		if len(call.Args) > 0 {
			if _, err := i.inferExpression(call.Args[0], ast.Type{}); err != nil {
				return ast.Type{}, err
			}
		}
		return ast.Basic(ast.TypeInt), nil
	case "array.contains":
		var arrayType ast.Type
		if len(call.Args) > 0 {
			var err error
			arrayType, err = i.inferExpression(call.Args[0], ast.Type{})
			if err != nil {
				return ast.Type{}, err
			}
		}
		if len(call.Args) > 1 {
			expected := ast.Type{}
			if arrayType.Kind == ast.TypeArray && arrayType.Elem != nil {
				expected = *arrayType.Elem
			}
			if _, err := i.inferExpression(call.Args[1], expected); err != nil {
				return ast.Type{}, err
			}
		}
		return ast.Basic(ast.TypeBool), nil
	case "array.push":
		var arrayType ast.Type
		if len(call.Args) > 0 {
			var err error
			arrayType, err = i.inferExpression(call.Args[0], ast.Type{})
			if err != nil {
				return ast.Type{}, err
			}
		}
		if len(call.Args) > 1 {
			expected := ast.Type{}
			if arrayType.Kind == ast.TypeArray && arrayType.Elem != nil {
				expected = *arrayType.Elem
			}
			if _, err := i.inferExpression(call.Args[1], expected); err != nil {
				return ast.Type{}, err
			}
		}
		if arrayType.Kind == ast.TypeArray {
			return arrayType, nil
		}
		return ast.Type{}, nil
	case "random.int":
		for _, arg := range call.Args {
			if _, err := i.inferExpression(arg, ast.Basic(ast.TypeInt)); err != nil {
				return ast.Type{}, err
			}
		}
		return ast.Basic(ast.TypeInt), nil
	case "random.choice":
		var arrayType ast.Type
		if len(call.Args) > 0 {
			var err error
			arrayType, err = i.inferExpression(call.Args[0], ast.Type{})
			if err != nil {
				return ast.Type{}, err
			}
		}
		if arrayType.Kind == ast.TypeArray && arrayType.Elem != nil {
			return *arrayType.Elem, nil
		}
		return ast.Type{}, nil
	case "time.now":
		return ast.Basic(ast.TypeInt), nil
	case "testing.assert":
		if len(call.Args) > 0 {
			if _, err := i.inferExpression(call.Args[0], ast.Basic(ast.TypeBool)); err != nil {
				return ast.Type{}, err
			}
		}
		return ast.Basic(ast.TypeBool), nil
	}
	return ast.Type{}, nil
}

func (i *functionInferencer) inferArrayLiteral(array *ast.ArrayLiteral, expected ast.Type) (ast.Type, error) {
	var elementExpected ast.Type
	if expected.Kind == ast.TypeArray && expected.Elem != nil {
		elementExpected = *expected.Elem
	}
	if len(array.Elements) == 0 && knownType(elementExpected) {
		return ast.ArrayOf(elementExpected), nil
	}
	var elementType ast.Type
	for _, element := range array.Elements {
		current, err := i.inferExpression(element, elementExpected)
		if err != nil {
			return ast.Type{}, err
		}
		if !knownType(current) {
			continue
		}
		if !knownType(elementType) {
			elementType = current
			continue
		}
		merged, ok := commonType(elementType, current)
		if !ok {
			return ast.Type{}, nil
		}
		elementType = merged
	}
	if knownType(elementType) {
		return ast.ArrayOf(elementType), nil
	}
	return ast.Type{}, nil
}

func (i *functionInferencer) inferAssignableTarget(target ast.Expression) (ast.Type, error) {
	switch t := target.(type) {
	case *ast.Name:
		sym, ok := i.resolve(t.Identifier)
		if !ok {
			return ast.Type{}, nil
		}
		return sym.typeName, nil
	case *ast.Index:
		targetType, err := i.inferExpression(t.Target, ast.Type{})
		if err != nil {
			return ast.Type{}, err
		}
		if _, err := i.inferExpression(t.Index, ast.Basic(ast.TypeInt)); err != nil {
			return ast.Type{}, err
		}
		if targetType.Kind == ast.TypeArray && targetType.Elem != nil {
			return *targetType.Elem, nil
		}
		if targetType.Kind == ast.TypeString {
			return ast.Basic(ast.TypeString), nil
		}
	case *ast.FieldAccess:
		targetType, err := i.inferExpression(t.Target, ast.Type{})
		if err != nil {
			return ast.Type{}, err
		}
		if targetType.Kind == ast.TypeStruct {
			field, err := i.checker.lookupField(targetType, t.Field, t.Loc())
			if err != nil {
				return ast.Type{}, err
			}
			return field.Type, nil
		}
	}
	return ast.Type{}, nil
}

func (i *functionInferencer) assignTargetType(target ast.Expression, typeName ast.Type) error {
	if name, ok := target.(*ast.Name); ok {
		return i.setNameType(name.Identifier, typeName, name.Loc())
	}
	return nil
}

func (i *functionInferencer) define(name string, typeName ast.Type, mutable bool, location ast.Location) error {
	scope := i.scopes[len(i.scopes)-1]
	if _, ok := scope[name]; ok {
		return errorAt(location, "name error: %s is already defined", name)
	}
	scope[name] = inferSymbol{typeName: typeName, mutable: mutable}
	return nil
}

func (i *functionInferencer) resolve(name string) (inferSymbol, bool) {
	for index := len(i.scopes) - 1; index >= 0; index-- {
		if sym, ok := i.scopes[index][name]; ok {
			return sym, true
		}
	}
	return inferSymbol{}, false
}

func (i *functionInferencer) setNameType(name string, typeName ast.Type, location ast.Location) error {
	for index := len(i.scopes) - 1; index >= 0; index-- {
		sym, ok := i.scopes[index][name]
		if !ok {
			continue
		}
		if knownType(sym.typeName) {
			merged, ok := commonType(sym.typeName, typeName)
			if !ok {
				return errorAt(location, "type error: cannot infer %s as both %s and %s", name, sym.typeName.String(), typeName.String())
			}
			sym.typeName = merged
		} else {
			sym.typeName = typeName
		}
		i.scopes[index][name] = sym
		return nil
	}
	return nil
}

func (i *functionInferencer) pushScope() {
	i.scopes = append(i.scopes, map[string]inferSymbol{})
}

func (i *functionInferencer) popScope() {
	i.scopes = i.scopes[:len(i.scopes)-1]
}

func literalType(literal *ast.Literal) ast.Type {
	switch literal.Kind {
	case ast.LiteralInt:
		return ast.Basic(ast.TypeInt)
	case ast.LiteralFloat:
		return ast.Basic(ast.TypeFloat)
	case ast.LiteralBool:
		return ast.Basic(ast.TypeBool)
	case ast.LiteralString:
		return ast.Basic(ast.TypeString)
	case ast.LiteralNull:
		return ast.Basic(ast.TypeNull)
	default:
		return ast.Type{}
	}
}

func knownType(typeName ast.Type) bool {
	return typeName.Kind != ast.TypeInvalid
}

func functionTypeKnown(fn *ast.FuncDecl) bool {
	if !knownType(fn.ReturnType) {
		return false
	}
	for _, param := range fn.Params {
		if !knownType(param.Type) {
			return false
		}
	}
	return true
}

func commonType(left ast.Type, right ast.Type) (ast.Type, bool) {
	if !knownType(left) {
		return right, knownType(right)
	}
	if !knownType(right) {
		return left, true
	}
	if left.Equal(right) {
		return left, true
	}
	if left.Kind == ast.TypeInt && right.Kind == ast.TypeFloat && !left.Nullable && !right.Nullable {
		return right, true
	}
	if left.Kind == ast.TypeFloat && right.Kind == ast.TypeInt && !left.Nullable && !right.Nullable {
		return left, true
	}
	return ast.Type{}, false
}
