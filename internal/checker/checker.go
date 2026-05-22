package checker

import (
	"fmt"
	"strings"

	"walklang/internal/ast"
)

type symbol struct {
	typeName ast.Type
	mutable  bool
}

type Checker struct {
	scopes        []map[string]symbol
	imports       map[string]bool
	currentReturn ast.Type
	inFunction    bool
	loopDepth     int
}

func Check(program *ast.Program) error {
	c := Checker{
		scopes:  []map[string]symbol{{}},
		imports: map[string]bool{},
	}
	return c.check(program)
}

func (c *Checker) check(program *ast.Program) error {
	for _, statement := range program.Statements {
		fn, ok := statement.(*ast.FuncDecl)
		if !ok {
			continue
		}
		fnType := functionType(fn)
		if err := c.define(fn.Name, fnType, false, fn.Location); err != nil {
			return err
		}
	}
	for _, statement := range program.Statements {
		if err := c.checkStatement(statement); err != nil {
			return err
		}
	}
	return nil
}

func functionType(fn *ast.FuncDecl) ast.Type {
	params := make([]ast.Type, 0, len(fn.Params))
	for _, param := range fn.Params {
		params = append(params, param.Type)
	}
	return ast.FuncType(params, fn.ReturnType)
}

func (c *Checker) checkStatement(statement ast.Statement) error {
	switch s := statement.(type) {
	case *ast.Import:
		c.imports[s.Module] = true
		return nil
	case *ast.Export:
		if _, ok := c.resolve(s.Name); !ok {
			return errorAt(s.Location, "name error: %s is not defined", s.Name)
		}
		return nil
	case *ast.FuncDecl:
		return c.checkFuncDecl(s)
	case *ast.VarDecl:
		return c.checkVarDecl(s)
	case *ast.Assignment:
		return c.checkAssignment(s)
	case *ast.Out:
		valueType, err := c.checkExpression(s.Value)
		if err != nil {
			return err
		}
		if valueType.Kind == ast.TypeArray || valueType.Kind == ast.TypeFunction || valueType.Kind == ast.TypeVoid {
			return errorAt(s.Location, "type error: cannot output %s", valueType.String())
		}
		return nil
	case *ast.Return:
		if !c.inFunction {
			return errorAt(s.Location, "syntax error: return outside function")
		}
		valueType, err := c.checkExpression(s.Value)
		if err != nil {
			return err
		}
		if !assignable(valueType, c.currentReturn) {
			return errorAt(s.Location, "type error: function returns %s, got %s", c.currentReturn.String(), valueType.String())
		}
		return nil
	case *ast.If:
		condType, err := c.checkExpression(s.Cond)
		if err != nil {
			return err
		}
		if !condType.Equal(ast.Basic(ast.TypeBool)) {
			return errorAt(s.Cond.Loc(), "type error: if condition must be bool, got %s", condType.String())
		}
		if err := c.checkNestedBlock(s.Then); err != nil {
			return err
		}
		return c.checkNestedBlock(s.Else)
	case *ast.While:
		condType, err := c.checkExpression(s.Cond)
		if err != nil {
			return err
		}
		if !condType.Equal(ast.Basic(ast.TypeBool)) {
			return errorAt(s.Cond.Loc(), "type error: while condition must be bool, got %s", condType.String())
		}
		c.loopDepth++
		err = c.checkNestedBlock(s.Body)
		c.loopDepth--
		return err
	case *ast.Repeat:
		countType, err := c.checkExpression(s.Count)
		if err != nil {
			return err
		}
		if !countType.Equal(ast.Basic(ast.TypeInt)) {
			return errorAt(s.Count.Loc(), "type error: repeat count must be int, got %s", countType.String())
		}
		c.loopDepth++
		err = c.checkNestedBlock(s.Body)
		c.loopDepth--
		return err
	case *ast.For:
		iterType, err := c.checkExpression(s.Iterable)
		if err != nil {
			return err
		}
		if iterType.Kind != ast.TypeArray || iterType.Elem == nil {
			return errorAt(s.Iterable.Loc(), "type error: for needs array, got %s", iterType.String())
		}
		c.pushScope()
		if err := c.define(s.Name, *iterType.Elem, true, s.Location); err != nil {
			c.popScope()
			return err
		}
		c.loopDepth++
		err = c.checkBlock(s.Body)
		c.loopDepth--
		c.popScope()
		return err
	case *ast.Break:
		if c.loopDepth == 0 {
			return errorAt(s.Location, "syntax error: break outside loop")
		}
		return nil
	case *ast.Continue:
		if c.loopDepth == 0 {
			return errorAt(s.Location, "syntax error: continue outside loop")
		}
		return nil
	default:
		return errorAt(statement.Loc(), "internal error: unknown statement")
	}
}

func (c *Checker) checkFuncDecl(fn *ast.FuncDecl) error {
	if err := validateType(fn.ReturnType, fn.Location); err != nil {
		return err
	}
	c.pushScope()
	previousReturn := c.currentReturn
	previousInFunction := c.inFunction
	c.currentReturn = fn.ReturnType
	c.inFunction = true
	for _, param := range fn.Params {
		if err := validateType(param.Type, fn.Location); err != nil {
			c.popScope()
			return err
		}
		if err := c.define(param.Name, param.Type, true, fn.Location); err != nil {
			c.popScope()
			return err
		}
	}
	err := c.checkBlock(fn.Body)
	c.currentReturn = previousReturn
	c.inFunction = previousInFunction
	c.popScope()
	if err != nil {
		return err
	}
	if fn.ReturnType.Kind != ast.TypeVoid && !blockReturns(fn.Body) {
		return errorAt(fn.Location, "type error: function %s may not return on all paths", fn.Name)
	}
	return nil
}

func (c *Checker) checkVarDecl(statement *ast.VarDecl) error {
	if c.currentScopeHas(statement.Name) {
		return errorAt(statement.Location, "name error: %s is already defined", statement.Name)
	}
	valueType, err := c.checkExpression(statement.Value)
	if err != nil {
		return err
	}
	declaredType := valueType
	if statement.Annotation.Kind != ast.TypeInvalid {
		if err := validateType(statement.Annotation, statement.Location); err != nil {
			return err
		}
		declaredType = statement.Annotation
	}
	if !assignable(valueType, declaredType) {
		return errorAt(statement.Value.Loc(), "type error: %s is %s, got %s", statement.Name, declaredType.String(), valueType.String())
	}
	statement.Value.SetExprType(declaredType)
	return c.define(statement.Name, declaredType, statement.Mutable, statement.Location)
}

func (c *Checker) checkAssignment(statement *ast.Assignment) error {
	valueType, err := c.checkExpression(statement.Value)
	if err != nil {
		return err
	}
	targetType, err := c.checkAssignableTarget(statement.Target)
	if err != nil {
		return err
	}
	if !assignable(valueType, targetType) {
		if name, ok := statement.Target.(*ast.Name); ok {
			return errorAt(statement.Location, "type error: %s is %s, got %s", name.Identifier, targetType.String(), valueType.String())
		}
		return errorAt(statement.Location, "type error: target is %s, got %s", targetType.String(), valueType.String())
	}
	return nil
}

func (c *Checker) checkAssignableTarget(target ast.Expression) (ast.Type, error) {
	switch t := target.(type) {
	case *ast.Name:
		sym, ok := c.resolve(t.Identifier)
		if !ok {
			return ast.Type{}, errorAt(t.Loc(), "name error: %s is not defined", t.Identifier)
		}
		if !sym.mutable {
			return ast.Type{}, errorAt(t.Loc(), "type error: %s is const and cannot be reassigned", t.Identifier)
		}
		t.SetExprType(sym.typeName)
		return sym.typeName, nil
	case *ast.Index:
		if name, ok := rootName(t.Target); ok {
			if sym, exists := c.resolve(name); exists && !sym.mutable {
				return ast.Type{}, errorAt(t.Loc(), "type error: %s is const and cannot be reassigned", name)
			}
		}
		targetType, err := c.checkExpression(t.Target)
		if err != nil {
			return ast.Type{}, err
		}
		indexType, err := c.checkExpression(t.Index)
		if err != nil {
			return ast.Type{}, err
		}
		if targetType.Kind != ast.TypeArray || targetType.Elem == nil {
			return ast.Type{}, errorAt(t.Loc(), "type error: index target must be array, got %s", targetType.String())
		}
		if !indexType.Equal(ast.Basic(ast.TypeInt)) {
			return ast.Type{}, errorAt(t.Index.Loc(), "type error: array index must be int, got %s", indexType.String())
		}
		t.SetExprType(*targetType.Elem)
		return *targetType.Elem, nil
	default:
		return ast.Type{}, errorAt(target.Loc(), "syntax error: invalid assignment target")
	}
}

func (c *Checker) checkExpression(expression ast.Expression) (ast.Type, error) {
	var result ast.Type
	switch e := expression.(type) {
	case *ast.Literal:
		switch e.Kind {
		case ast.LiteralInt:
			result = ast.Basic(ast.TypeInt)
		case ast.LiteralFloat:
			result = ast.Basic(ast.TypeFloat)
		case ast.LiteralBool:
			result = ast.Basic(ast.TypeBool)
		case ast.LiteralString:
			result = ast.Basic(ast.TypeString)
		case ast.LiteralNull:
			result = ast.Basic(ast.TypeNull)
		default:
			return ast.Type{}, errorAt(e.Loc(), "internal error: unknown literal")
		}
	case *ast.Name:
		sym, exists := c.resolve(e.Identifier)
		if !exists {
			return ast.Type{}, errorAt(e.Loc(), "name error: %s is not defined", e.Identifier)
		}
		result = sym.typeName
	case *ast.Prefix:
		var err error
		result, err = c.checkPrefix(e)
		if err != nil {
			return ast.Type{}, err
		}
	case *ast.Call:
		var err error
		result, err = c.checkCall(e)
		if err != nil {
			return ast.Type{}, err
		}
	case *ast.ArrayLiteral:
		var err error
		result, err = c.checkArrayLiteral(e)
		if err != nil {
			return ast.Type{}, err
		}
	case *ast.Index:
		targetType, err := c.checkExpression(e.Target)
		if err != nil {
			return ast.Type{}, err
		}
		indexType, err := c.checkExpression(e.Index)
		if err != nil {
			return ast.Type{}, err
		}
		if targetType.Kind != ast.TypeArray || targetType.Elem == nil {
			return ast.Type{}, errorAt(e.Loc(), "type error: index target must be array, got %s", targetType.String())
		}
		if !indexType.Equal(ast.Basic(ast.TypeInt)) {
			return ast.Type{}, errorAt(e.Index.Loc(), "type error: array index must be int, got %s", indexType.String())
		}
		result = *targetType.Elem
	default:
		return ast.Type{}, errorAt(expression.Loc(), "internal error: unknown expression")
	}
	expression.SetExprType(result)
	return result, nil
}

func (c *Checker) checkPrefix(expression *ast.Prefix) (ast.Type, error) {
	var argTypes []ast.Type
	for _, arg := range expression.Args {
		argType, err := c.checkExpression(arg)
		if err != nil {
			return ast.Type{}, err
		}
		argTypes = append(argTypes, argType)
	}

	switch expression.Operator {
	case "+", "-", "*", "/", "^":
		for _, argType := range argTypes {
			if !isNumeric(argType) {
				return ast.Type{}, errorAt(expression.Loc(), "type error: operator %s needs numeric operands", expression.Operator)
			}
		}
		if expression.Operator == "/" || containsKind(argTypes, ast.TypeFloat) {
			return ast.Basic(ast.TypeFloat), nil
		}
		return ast.Basic(ast.TypeInt), nil
	case ">", "<", ">=", "<=":
		for _, argType := range argTypes {
			if !isNumeric(argType) {
				return ast.Type{}, errorAt(expression.Loc(), "type error: operator %s needs numeric operands", expression.Operator)
			}
		}
		return ast.Basic(ast.TypeBool), nil
	case "==", "!=":
		left, right := argTypes[0], argTypes[1]
		if !comparable(left, right) {
			return ast.Type{}, errorAt(expression.Loc(), "type error: cannot compare %s and %s", left.String(), right.String())
		}
		return ast.Basic(ast.TypeBool), nil
	case "and", "or":
		for _, argType := range argTypes {
			if !argType.Equal(ast.Basic(ast.TypeBool)) {
				return ast.Type{}, errorAt(expression.Loc(), "type error: operator %s needs bool operands", expression.Operator)
			}
		}
		return ast.Basic(ast.TypeBool), nil
	case "not":
		if !argTypes[0].Equal(ast.Basic(ast.TypeBool)) {
			return ast.Type{}, errorAt(expression.Loc(), "type error: operator not needs one bool operand")
		}
		return ast.Basic(ast.TypeBool), nil
	}
	return ast.Type{}, errorAt(expression.Loc(), "internal error: unsupported operator %s", expression.Operator)
}

func (c *Checker) checkCall(call *ast.Call) (ast.Type, error) {
	if strings.Contains(call.Callee, ".") {
		return c.checkModuleCall(call)
	}
	sym, ok := c.resolve(call.Callee)
	if !ok {
		return ast.Type{}, errorAt(call.Loc(), "name error: %s is not defined", call.Callee)
	}
	if sym.typeName.Kind != ast.TypeFunction || sym.typeName.Return == nil {
		return ast.Type{}, errorAt(call.Loc(), "type error: %s is not callable", call.Callee)
	}
	if len(call.Args) != len(sym.typeName.Params) {
		return ast.Type{}, errorAt(call.Loc(), "type error: %s expects %d args, got %d", call.Callee, len(sym.typeName.Params), len(call.Args))
	}
	for i, arg := range call.Args {
		argType, err := c.checkExpression(arg)
		if err != nil {
			return ast.Type{}, err
		}
		if !assignable(argType, sym.typeName.Params[i]) {
			return ast.Type{}, errorAt(arg.Loc(), "type error: arg %d to %s is %s, got %s", i+1, call.Callee, sym.typeName.Params[i].String(), argType.String())
		}
	}
	return *sym.typeName.Return, nil
}

func (c *Checker) checkModuleCall(call *ast.Call) (ast.Type, error) {
	parts := strings.Split(call.Callee, ".")
	if len(parts) != 2 {
		return ast.Type{}, errorAt(call.Loc(), "name error: unsupported qualified call %s", call.Callee)
	}
	module, name := parts[0], parts[1]
	if !c.imports[module] {
		return ast.Type{}, errorAt(call.Loc(), "name error: module %s is not imported", module)
	}
	switch module + "." + name {
	case "math.sqrt":
		if len(call.Args) != 1 {
			return ast.Type{}, errorAt(call.Loc(), "type error: math.sqrt expects 1 arg, got %d", len(call.Args))
		}
		argType, err := c.checkExpression(call.Args[0])
		if err != nil {
			return ast.Type{}, err
		}
		if !isNumeric(argType) {
			return ast.Type{}, errorAt(call.Args[0].Loc(), "type error: math.sqrt needs numeric arg, got %s", argType.String())
		}
		return ast.Basic(ast.TypeFloat), nil
	case "random.int":
		if len(call.Args) != 2 {
			return ast.Type{}, errorAt(call.Loc(), "type error: random.int expects 2 args, got %d", len(call.Args))
		}
		for _, arg := range call.Args {
			argType, err := c.checkExpression(arg)
			if err != nil {
				return ast.Type{}, err
			}
			if !argType.Equal(ast.Basic(ast.TypeInt)) {
				return ast.Type{}, errorAt(arg.Loc(), "type error: random.int args must be int, got %s", argType.String())
			}
		}
		return ast.Basic(ast.TypeInt), nil
	}
	return ast.Type{}, errorAt(call.Loc(), "name error: unknown library function %s", call.Callee)
}

func (c *Checker) checkArrayLiteral(array *ast.ArrayLiteral) (ast.Type, error) {
	if len(array.Elements) == 0 {
		return ast.Type{}, errorAt(array.Loc(), "type error: empty arrays need an explicit type")
	}
	firstType, err := c.checkExpression(array.Elements[0])
	if err != nil {
		return ast.Type{}, err
	}
	for _, element := range array.Elements[1:] {
		elementType, err := c.checkExpression(element)
		if err != nil {
			return ast.Type{}, err
		}
		if !assignable(elementType, firstType) || !assignable(firstType, elementType) {
			return ast.Type{}, errorAt(element.Loc(), "type error: arrays must be homogeneous, got %s and %s", firstType.String(), elementType.String())
		}
	}
	return ast.ArrayOf(firstType), nil
}

func (c *Checker) checkNestedBlock(statements []ast.Statement) error {
	c.pushScope()
	err := c.checkBlock(statements)
	c.popScope()
	return err
}

func (c *Checker) checkBlock(statements []ast.Statement) error {
	for _, statement := range statements {
		if err := c.checkStatement(statement); err != nil {
			return err
		}
	}
	return nil
}

func (c *Checker) pushScope() {
	c.scopes = append(c.scopes, map[string]symbol{})
}

func (c *Checker) popScope() {
	c.scopes = c.scopes[:len(c.scopes)-1]
}

func (c *Checker) currentScopeHas(name string) bool {
	_, ok := c.scopes[len(c.scopes)-1][name]
	return ok
}

func (c *Checker) define(name string, typeName ast.Type, mutable bool, location ast.Location) error {
	if c.currentScopeHas(name) {
		return errorAt(location, "name error: %s is already defined", name)
	}
	c.scopes[len(c.scopes)-1][name] = symbol{typeName: typeName, mutable: mutable}
	return nil
}

func (c *Checker) resolve(name string) (symbol, bool) {
	for i := len(c.scopes) - 1; i >= 0; i-- {
		if sym, ok := c.scopes[i][name]; ok {
			return sym, true
		}
	}
	return symbol{}, false
}

func validateType(typeName ast.Type, location ast.Location) error {
	switch typeName.Kind {
	case ast.TypeVoid, ast.TypeInt, ast.TypeFloat, ast.TypeBool, ast.TypeString:
		return nil
	case ast.TypeArray:
		if typeName.Elem == nil {
			return errorAt(location, "type error: array type needs element type")
		}
		return validateType(*typeName.Elem, location)
	case ast.TypeFunction:
		for _, param := range typeName.Params {
			if err := validateType(param, location); err != nil {
				return err
			}
		}
		if typeName.Return == nil {
			return errorAt(location, "type error: function type needs return type")
		}
		return validateType(*typeName.Return, location)
	}
	return errorAt(location, "type error: unknown type %s", typeName.String())
}

func assignable(source ast.Type, target ast.Type) bool {
	if source.Kind == ast.TypeNull {
		return target.Nullable
	}
	if source.Equal(target) {
		return true
	}
	if target.Nullable {
		nonNullableTarget := target
		nonNullableTarget.Nullable = false
		if source.Equal(nonNullableTarget) {
			return true
		}
	}
	if source.Kind == ast.TypeInt && target.Kind == ast.TypeFloat && !source.Nullable && !target.Nullable {
		return true
	}
	return false
}

func comparable(left ast.Type, right ast.Type) bool {
	if left.Kind == ast.TypeNull {
		return right.Nullable
	}
	if right.Kind == ast.TypeNull {
		return left.Nullable
	}
	return assignable(left, right) || assignable(right, left)
}

func isNumeric(typeName ast.Type) bool {
	return !typeName.Nullable && (typeName.Kind == ast.TypeInt || typeName.Kind == ast.TypeFloat)
}

func containsKind(types []ast.Type, target ast.TypeKind) bool {
	for _, typeName := range types {
		if typeName.Kind == target {
			return true
		}
	}
	return false
}

func blockReturns(statements []ast.Statement) bool {
	for _, statement := range statements {
		switch s := statement.(type) {
		case *ast.Return:
			return true
		case *ast.If:
			if len(s.Else) > 0 && blockReturns(s.Then) && blockReturns(s.Else) {
				return true
			}
		}
	}
	return false
}

func rootName(expression ast.Expression) (string, bool) {
	switch e := expression.(type) {
	case *ast.Name:
		return e.Identifier, true
	case *ast.Index:
		return rootName(e.Target)
	default:
		return "", false
	}
}

func errorAt(location ast.Location, format string, args ...any) error {
	return fmt.Errorf("%s:%d: %s", location.Filename, location.Line, fmt.Sprintf(format, args...))
}
