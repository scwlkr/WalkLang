package builtins

import "walklang/internal/ast"

type Function struct {
	Module string
	Name   string
	Params []ast.Type
	Return ast.Type
	Effect bool
	Draft  bool
	CName  string
}

type Struct struct {
	Name   string
	Fields []ast.StructField
	Draft  bool
}

func (f Function) QualifiedName() string {
	return f.Module + "." + f.Name
}

var functions = map[string]Function{}
var modules = map[string]bool{}
var structs []Struct

func init() {
	registerStruct(Struct{
		Name: "IOReadResult",
		Fields: []ast.StructField{
			field("ok", ast.Basic(ast.TypeBool)),
			field("value", ast.Basic(ast.TypeString)),
			field("error", ast.Basic(ast.TypeString)),
		},
		Draft: true,
	})
	registerStruct(Struct{
		Name: "ParseIntResult",
		Fields: []ast.StructField{
			field("ok", ast.Basic(ast.TypeBool)),
			field("value", ast.Basic(ast.TypeInt)),
			field("error", ast.Basic(ast.TypeString)),
		},
		Draft: true,
	})
	registerStruct(Struct{
		Name: "ParseFloatResult",
		Fields: []ast.StructField{
			field("ok", ast.Basic(ast.TypeBool)),
			field("value", ast.Basic(ast.TypeFloat)),
			field("error", ast.Basic(ast.TypeString)),
		},
		Draft: true,
	})
	registerStruct(Struct{
		Name: "ParseBoolResult",
		Fields: []ast.StructField{
			field("ok", ast.Basic(ast.TypeBool)),
			field("value", ast.Basic(ast.TypeBool)),
			field("error", ast.Basic(ast.TypeString)),
		},
		Draft: true,
	})
	register(Function{
		Module: "io",
		Name:   "write",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_io_write",
	})
	register(Function{
		Module: "io",
		Name:   "write_line",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_io_write_line",
	})
	register(Function{
		Module: "io",
		Name:   "error_line",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_io_error_line",
	})
	register(Function{
		Module: "io",
		Name:   "read_line",
		Return: ast.Struct("IOReadResult"),
		Draft:  true,
		CName:  "__walk_io_read_line",
	})
	register(Function{
		Module: "io",
		Name:   "read_all",
		Return: ast.Struct("IOReadResult"),
		Draft:  true,
		CName:  "__walk_io_read_all",
	})
	register(Function{
		Module: "file",
		Name:   "read",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeString),
		Draft:  true,
		CName:  "__walk_file_read",
	})
	register(Function{
		Module: "file",
		Name:   "write",
		Params: []ast.Type{ast.Basic(ast.TypeString), ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_file_write",
	})
	register(Function{
		Module: "file",
		Name:   "exists",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeBool),
		Draft:  true,
		CName:  "__walk_file_exists",
	})
	register(Function{
		Module: "process",
		Name:   "args",
		Return: ast.ArrayOf(ast.Basic(ast.TypeString)),
		Draft:  true,
		CName:  "__walk_process_args",
	})
	register(Function{
		Module: "process",
		Name:   "arg_count",
		Return: ast.Basic(ast.TypeInt),
		Draft:  true,
		CName:  "__walk_process_arg_count",
	})
	register(Function{
		Module: "process",
		Name:   "env",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: nullableString(),
		Draft:  true,
		CName:  "__walk_process_env",
	})
	register(Function{
		Module: "process",
		Name:   "cwd",
		Return: ast.Basic(ast.TypeString),
		Draft:  true,
		CName:  "__walk_process_cwd",
	})
	register(Function{
		Module: "process",
		Name:   "exit",
		Params: []ast.Type{ast.Basic(ast.TypeInt)},
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_process_exit",
	})
	register(Function{
		Module: "parse",
		Name:   "int",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Struct("ParseIntResult"),
		Draft:  true,
		CName:  "__walk_parse_int",
	})
	register(Function{
		Module: "parse",
		Name:   "float",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Struct("ParseFloatResult"),
		Draft:  true,
		CName:  "__walk_parse_float",
	})
	register(Function{
		Module: "parse",
		Name:   "bool",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Struct("ParseBoolResult"),
		Draft:  true,
		CName:  "__walk_parse_bool",
	})
}

func register(fn Function) {
	functions[fn.QualifiedName()] = fn
	modules[fn.Module] = true
}

func registerStruct(def Struct) {
	structs = append(structs, def)
}

func Lookup(module string, name string) (Function, bool) {
	fn, ok := functions[module+"."+name]
	return fn, ok
}

func LookupQualified(qualified string) (Function, bool) {
	fn, ok := functions[qualified]
	return fn, ok
}

func IsModule(name string) bool {
	return modules[name]
}

func Structs() []Struct {
	result := make([]Struct, len(structs))
	copy(result, structs)
	return result
}

func StructDecls() []*ast.StructDecl {
	result := make([]*ast.StructDecl, 0, len(structs))
	for _, def := range structs {
		fields := make([]ast.StructField, len(def.Fields))
		copy(fields, def.Fields)
		result = append(result, &ast.StructDecl{Name: def.Name, Fields: fields})
	}
	return result
}

func field(name string, typeName ast.Type) ast.StructField {
	return ast.StructField{Name: name, Type: typeName}
}

func nullableString() ast.Type {
	typeName := ast.Basic(ast.TypeString)
	typeName.Nullable = true
	return typeName
}
