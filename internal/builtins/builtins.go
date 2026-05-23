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

func (f Function) QualifiedName() string {
	return f.Module + "." + f.Name
}

var functions = map[string]Function{}
var modules = map[string]bool{}

func init() {
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
}

func register(fn Function) {
	functions[fn.QualifiedName()] = fn
	modules[fn.Module] = true
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

func nullableString() ast.Type {
	typeName := ast.Basic(ast.TypeString)
	typeName.Nullable = true
	return typeName
}
