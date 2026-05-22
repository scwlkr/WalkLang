package checker

import (
	"fmt"
	"strings"

	"walklang/internal/ast"
	"walklang/internal/diagnostic"
)

type symbol struct {
	typeName ast.Type
	mutable  bool
}

type StructDef struct {
	Location ast.Location
	Name     string
	Fields   []ast.StructField
	FieldMap map[string]ast.StructField
}

type Module struct {
	Name    string
	Program *ast.Program
	Exports map[string]ast.Type
}

type Options struct {
	Modules map[string]*Module
	Structs map[string]StructDef
}

type Warning struct {
	Location ast.Location
	Message  string
}

func (w Warning) String() string {
	return fmt.Sprintf("%s:%d:%d: warning: %s", w.Location.Filename, w.Location.Line, w.Location.Column, w.Message)
}

type Checker struct {
	scopes        []map[string]symbol
	imports       map[string]bool
	modules       map[string]*Module
	structs       map[string]StructDef
	warnings      []Warning
	currentReturn ast.Type
	inFunction    bool
	loopDepth     int
	blockDepth    int
}

func Check(program *ast.Program) error {
	_, err := CheckWithOptions(program, Options{})
	return err
}

func CheckWithOptions(program *ast.Program, options Options) ([]Warning, error) {
	structs := options.Structs
	if structs == nil {
		var err error
		structs, err = StructDefinitions(program)
		if err != nil {
			return nil, err
		}
	}
	c := Checker{
		scopes:  []map[string]symbol{{}},
		imports: map[string]bool{},
		modules: options.Modules,
		structs: structs,
	}
	if c.modules == nil {
		c.modules = map[string]*Module{}
	}
	err := c.check(program)
	return c.warnings, err
}

func (c *Checker) check(program *ast.Program) error {
	if err := c.checkStructs(); err != nil {
		return err
	}
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

func StructDefinitions(program *ast.Program) (map[string]StructDef, error) {
	structs := map[string]StructDef{}
	for _, statement := range program.Statements {
		decl, ok := statement.(*ast.StructDecl)
		if !ok {
			continue
		}
		if _, exists := structs[decl.Name]; exists {
			return nil, errorAt(decl.Location, "type error: struct %s is already defined", decl.Name)
		}
		def := StructDef{
			Location: decl.Location,
			Name:     decl.Name,
			Fields:   decl.Fields,
			FieldMap: map[string]ast.StructField{},
		}
		for _, field := range decl.Fields {
			if _, exists := def.FieldMap[field.Name]; exists {
				return nil, errorAt(field.Location, "type error: struct %s already has field %s", decl.Name, field.Name)
			}
			def.FieldMap[field.Name] = field
		}
		structs[decl.Name] = def
	}
	return structs, nil
}

func (c *Checker) checkStructs() error {
	for _, def := range c.structs {
		for _, field := range def.Fields {
			if err := c.validateType(field.Type, field.Location); err != nil {
				return err
			}
		}
	}
	visiting := map[string]bool{}
	visited := map[string]bool{}
	var visit func(string) error
	visit = func(name string) error {
		if visited[name] {
			return nil
		}
		if visiting[name] {
			return errorAt(c.structs[name].Location, "type error: struct %s cannot contain itself", name)
		}
		visiting[name] = true
		for _, field := range c.structs[name].Fields {
			for _, dep := range structTypeDependencies(field.Type) {
				if _, ok := c.structs[dep]; ok {
					if err := visit(dep); err != nil {
						return err
					}
				}
			}
		}
		visiting[name] = false
		visited[name] = true
		return nil
	}
	for name := range c.structs {
		if err := visit(name); err != nil {
			return err
		}
	}
	return nil
}

func structTypeDependencies(typeName ast.Type) []string {
	switch typeName.Kind {
	case ast.TypeStruct:
		return []string{typeName.Name}
	case ast.TypeArray:
		if typeName.Elem == nil {
			return nil
		}
		return structTypeDependencies(*typeName.Elem)
	case ast.TypeFunction:
		var deps []string
		for _, param := range typeName.Params {
			deps = append(deps, structTypeDependencies(param)...)
		}
		if typeName.Return != nil {
			deps = append(deps, structTypeDependencies(*typeName.Return)...)
		}
		return deps
	default:
		return nil
	}
}

func (c *Checker) checkStatement(statement ast.Statement) error {
	switch s := statement.(type) {
	case *ast.Import:
		if !IsBuiltinModule(s.Module) {
			if _, ok := c.modules[s.Module]; !ok {
				return errorAt(s.Location, "module error: module %s is not available", s.Module)
			}
		}
		c.imports[s.Module] = true
		return nil
	case *ast.Export:
		if _, ok := c.resolve(s.Name); !ok {
			return errorAt(s.Location, "name error: %s is not defined", s.Name)
		}
		return nil
	case *ast.FuncDecl:
		return c.checkFuncDecl(s)
	case *ast.StructDecl:
		if c.blockDepth > 0 {
			return errorAt(s.Location, "syntax error: struct declarations must be top level")
		}
		return nil
	case *ast.VarDecl:
		return c.checkVarDecl(s)
	case *ast.Assignment:
		return c.checkAssignment(s)
	case *ast.Out:
		valueType, err := c.checkExpression(s.Value)
		if err != nil {
			return err
		}
		if valueType.Kind == ast.TypeArray || valueType.Kind == ast.TypeFunction || valueType.Kind == ast.TypeVoid || valueType.Kind == ast.TypeStruct {
			return errorAt(s.Location, "type error: cannot output %s", valueType.String())
		}
		return nil
	case *ast.TestDecl:
		return c.checkNestedBlock(s.Body)
	case *ast.Assert:
		valueType, err := c.checkExpression(s.Value)
		if err != nil {
			return err
		}
		if !valueType.Equal(ast.Basic(ast.TypeBool)) {
			return errorAt(s.Value.Loc(), "type error: assert needs bool, got %s", valueType.String())
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
	if err := c.validateType(fn.ReturnType, fn.Location); err != nil {
		return err
	}
	c.pushScope()
	previousReturn := c.currentReturn
	previousInFunction := c.inFunction
	previousBlockDepth := c.blockDepth
	c.currentReturn = fn.ReturnType
	c.inFunction = true
	c.blockDepth++
	for _, param := range fn.Params {
		if err := c.validateType(param.Type, fn.Location); err != nil {
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
	c.blockDepth = previousBlockDepth
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
		if err := c.validateType(statement.Annotation, statement.Location); err != nil {
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
		if field, ok := statement.Target.(*ast.FieldAccess); ok {
			return errorAt(statement.Location, "type error: field %s is %s, got %s", field.Field, targetType.String(), valueType.String())
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
	case *ast.FieldAccess:
		if name, ok := rootName(t.Target); ok {
			if sym, exists := c.resolve(name); exists && !sym.mutable {
				return ast.Type{}, errorAt(t.Loc(), "type error: %s is const and cannot be reassigned", name)
			}
		}
		targetType, err := c.checkExpression(t.Target)
		if err != nil {
			return ast.Type{}, err
		}
		field, err := c.lookupField(targetType, t.Field, t.Loc())
		if err != nil {
			return ast.Type{}, err
		}
		t.SetExprType(field.Type)
		return field.Type, nil
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
	case *ast.FieldAccess:
		targetType, err := c.checkExpression(e.Target)
		if err != nil {
			return ast.Type{}, err
		}
		field, err := c.lookupField(targetType, e.Field, e.Loc())
		if err != nil {
			return ast.Type{}, err
		}
		result = field.Type
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
	if def, ok := c.structs[call.Callee]; ok {
		return c.checkStructConstructor(call, def)
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

func (c *Checker) checkStructConstructor(call *ast.Call, def StructDef) (ast.Type, error) {
	if len(call.Args) != len(def.Fields) {
		return ast.Type{}, errorAt(call.Loc(), "type error: %s expects %d field values, got %d", def.Name, len(def.Fields), len(call.Args))
	}
	for i, arg := range call.Args {
		argType, err := c.checkExpression(arg)
		if err != nil {
			return ast.Type{}, err
		}
		field := def.Fields[i]
		if !assignable(argType, field.Type) {
			return ast.Type{}, errorAt(arg.Loc(), "type error: field %s is %s, got %s", field.Name, field.Type.String(), argType.String())
		}
	}
	return ast.Struct(def.Name), nil
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
	if userModule, ok := c.modules[module]; ok {
		fnType, ok := userModule.Exports[name]
		if !ok {
			return ast.Type{}, errorAt(call.Loc(), "name error: module %s does not export %s", module, name)
		}
		return c.checkFunctionCall(call, fnType)
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
	case "math.pow":
		if len(call.Args) != 2 {
			return ast.Type{}, errorAt(call.Loc(), "type error: math.pow expects 2 args, got %d", len(call.Args))
		}
		for _, arg := range call.Args {
			argType, err := c.checkExpression(arg)
			if err != nil {
				return ast.Type{}, err
			}
			if !isNumeric(argType) {
				return ast.Type{}, errorAt(arg.Loc(), "type error: math.pow needs numeric args, got %s", argType.String())
			}
		}
		return ast.Basic(ast.TypeFloat), nil
	case "string.len":
		if len(call.Args) != 1 {
			return ast.Type{}, errorAt(call.Loc(), "type error: string.len expects 1 arg, got %d", len(call.Args))
		}
		argType, err := c.checkExpression(call.Args[0])
		if err != nil {
			return ast.Type{}, err
		}
		if argType.Kind != ast.TypeString {
			return ast.Type{}, errorAt(call.Args[0].Loc(), "type error: string.len needs string arg, got %s", argType.String())
		}
		return ast.Basic(ast.TypeInt), nil
	case "array.len":
		if len(call.Args) != 1 {
			return ast.Type{}, errorAt(call.Loc(), "type error: array.len expects 1 arg, got %d", len(call.Args))
		}
		argType, err := c.checkExpression(call.Args[0])
		if err != nil {
			return ast.Type{}, err
		}
		if argType.Kind != ast.TypeArray {
			return ast.Type{}, errorAt(call.Args[0].Loc(), "type error: array.len needs array arg, got %s", argType.String())
		}
		return ast.Basic(ast.TypeInt), nil
	case "time.now":
		if len(call.Args) != 0 {
			return ast.Type{}, errorAt(call.Loc(), "type error: time.now expects 0 args, got %d", len(call.Args))
		}
		return ast.Basic(ast.TypeInt), nil
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
	case "testing.assert":
		if len(call.Args) != 1 {
			return ast.Type{}, errorAt(call.Loc(), "type error: testing.assert expects 1 arg, got %d", len(call.Args))
		}
		argType, err := c.checkExpression(call.Args[0])
		if err != nil {
			return ast.Type{}, err
		}
		if !argType.Equal(ast.Basic(ast.TypeBool)) {
			return ast.Type{}, errorAt(call.Args[0].Loc(), "type error: testing.assert needs bool arg, got %s", argType.String())
		}
		return ast.Basic(ast.TypeBool), nil
	}
	return ast.Type{}, errorAt(call.Loc(), "name error: unknown library function %s", call.Callee)
}

func (c *Checker) checkFunctionCall(call *ast.Call, fnType ast.Type) (ast.Type, error) {
	if fnType.Kind != ast.TypeFunction || fnType.Return == nil {
		return ast.Type{}, errorAt(call.Loc(), "type error: %s is not callable", call.Callee)
	}
	if len(call.Args) != len(fnType.Params) {
		return ast.Type{}, errorAt(call.Loc(), "type error: %s expects %d args, got %d", call.Callee, len(fnType.Params), len(call.Args))
	}
	for i, arg := range call.Args {
		argType, err := c.checkExpression(arg)
		if err != nil {
			return ast.Type{}, err
		}
		if !assignable(argType, fnType.Params[i]) {
			return ast.Type{}, errorAt(arg.Loc(), "type error: arg %d to %s is %s, got %s", i+1, call.Callee, fnType.Params[i].String(), argType.String())
		}
	}
	return *fnType.Return, nil
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

func (c *Checker) lookupField(targetType ast.Type, fieldName string, location ast.Location) (ast.StructField, error) {
	if targetType.Kind != ast.TypeStruct {
		return ast.StructField{}, errorAt(location, "type error: field access needs struct, got %s", targetType.String())
	}
	def, ok := c.structs[targetType.Name]
	if !ok {
		return ast.StructField{}, errorAt(location, "type error: unknown type %s", targetType.String())
	}
	field, ok := def.FieldMap[fieldName]
	if !ok {
		return ast.StructField{}, errorAt(location, "type error: %s has no field %s", targetType.String(), fieldName)
	}
	return field, nil
}

func (c *Checker) checkNestedBlock(statements []ast.Statement) error {
	c.pushScope()
	c.blockDepth++
	err := c.checkBlock(statements)
	c.blockDepth--
	c.popScope()
	return err
}

func (c *Checker) checkBlock(statements []ast.Statement) error {
	unreachable := false
	for _, statement := range statements {
		if unreachable {
			c.warnings = append(c.warnings, Warning{Location: statement.Loc(), Message: "unreachable statement"})
		}
		if err := c.checkStatement(statement); err != nil {
			return err
		}
		if statementTerminates(statement) {
			unreachable = true
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
	if _, ok := c.structs[name]; ok {
		return errorAt(location, "name error: %s is already defined as struct", name)
	}
	if c.outerScopeHas(name) {
		c.warnings = append(c.warnings, Warning{Location: location, Message: fmt.Sprintf("%s shadows outer name", name)})
	}
	c.scopes[len(c.scopes)-1][name] = symbol{typeName: typeName, mutable: mutable}
	return nil
}

func (c *Checker) outerScopeHas(name string) bool {
	for i := len(c.scopes) - 2; i >= 0; i-- {
		if _, ok := c.scopes[i][name]; ok {
			return true
		}
	}
	return false
}

func (c *Checker) resolve(name string) (symbol, bool) {
	for i := len(c.scopes) - 1; i >= 0; i-- {
		if sym, ok := c.scopes[i][name]; ok {
			return sym, true
		}
	}
	return symbol{}, false
}

func (c *Checker) validateType(typeName ast.Type, location ast.Location) error {
	switch typeName.Kind {
	case ast.TypeVoid, ast.TypeInt, ast.TypeFloat, ast.TypeBool, ast.TypeString:
		return nil
	case ast.TypeStruct:
		if typeName.Nullable {
			return errorAt(location, "type error: struct type %s cannot be nullable", typeName.Name)
		}
		if _, ok := c.structs[typeName.Name]; !ok {
			return errorAt(location, "type error: unknown type %s", typeName.String())
		}
		return nil
	case ast.TypeArray:
		if typeName.Elem == nil {
			return errorAt(location, "type error: array type needs element type")
		}
		return c.validateType(*typeName.Elem, location)
	case ast.TypeFunction:
		for _, param := range typeName.Params {
			if err := c.validateType(param, location); err != nil {
				return err
			}
		}
		if typeName.Return == nil {
			return errorAt(location, "type error: function type needs return type")
		}
		return c.validateType(*typeName.Return, location)
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
	if left.Kind == ast.TypeStruct || right.Kind == ast.TypeStruct {
		return false
	}
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

func statementTerminates(statement ast.Statement) bool {
	switch s := statement.(type) {
	case *ast.Return, *ast.Break, *ast.Continue:
		return true
	case *ast.If:
		return len(s.Else) > 0 && blockTerminates(s.Then) && blockTerminates(s.Else)
	default:
		return false
	}
}

func blockTerminates(statements []ast.Statement) bool {
	for _, statement := range statements {
		if statementTerminates(statement) {
			return true
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
	case *ast.FieldAccess:
		return rootName(e.Target)
	default:
		return "", false
	}
}

func errorAt(location ast.Location, format string, args ...any) error {
	return diagnostic.Errorf(location, format, args...)
}

func IsBuiltinModule(name string) bool {
	switch name {
	case "math", "string", "array", "time", "random", "testing":
		return true
	default:
		return false
	}
}

func ExportedFunctions(program *ast.Program) (map[string]ast.Type, error) {
	functions := map[string]ast.Type{}
	for _, statement := range program.Statements {
		fn, ok := statement.(*ast.FuncDecl)
		if !ok {
			continue
		}
		functions[fn.Name] = functionType(fn)
	}

	exports := map[string]ast.Type{}
	for _, statement := range program.Statements {
		exp, ok := statement.(*ast.Export)
		if !ok {
			continue
		}
		fnType, ok := functions[exp.Name]
		if !ok {
			return nil, errorAt(exp.Location, "module error: %s is not an exported function", exp.Name)
		}
		exports[exp.Name] = fnType
	}
	return exports, nil
}
