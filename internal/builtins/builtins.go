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
	registerStruct(Struct{
		Name: "FileReadResult",
		Fields: []ast.StructField{
			field("ok", ast.Basic(ast.TypeBool)),
			field("value", ast.Basic(ast.TypeString)),
			field("error", ast.Basic(ast.TypeString)),
		},
		Draft: true,
	})
	registerStruct(Struct{
		Name: "FileActionResult",
		Fields: []ast.StructField{
			field("ok", ast.Basic(ast.TypeBool)),
			field("value", ast.Basic(ast.TypeBool)),
			field("error", ast.Basic(ast.TypeString)),
		},
		Draft: true,
	})
	registerStruct(Struct{
		Name: "ProcessResult",
		Fields: []ast.StructField{
			field("ok", ast.Basic(ast.TypeBool)),
			field("status", ast.Basic(ast.TypeInt)),
			field("stdout", ast.Basic(ast.TypeString)),
			field("stderr", ast.Basic(ast.TypeString)),
			field("error", ast.Basic(ast.TypeString)),
		},
		Draft: true,
	})
	registerStruct(Struct{
		Name: "ProcessOutputResult",
		Fields: []ast.StructField{
			field("ok", ast.Basic(ast.TypeBool)),
			field("value", ast.Basic(ast.TypeString)),
			field("status", ast.Basic(ast.TypeInt)),
			field("error", ast.Basic(ast.TypeString)),
		},
		Draft: true,
	})
	registerStruct(Struct{
		Name: "JsonResult",
		Fields: []ast.StructField{
			field("ok", ast.Basic(ast.TypeBool)),
			field("value", ast.Basic(ast.TypeString)),
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
		Name:   "try_read",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Struct("FileReadResult"),
		Draft:  true,
		CName:  "__walk_file_try_read",
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
		Name:   "try_write",
		Params: []ast.Type{ast.Basic(ast.TypeString), ast.Basic(ast.TypeString)},
		Return: ast.Struct("FileActionResult"),
		Draft:  true,
		CName:  "__walk_file_try_write",
	})
	register(Function{
		Module: "file",
		Name:   "append",
		Params: []ast.Type{ast.Basic(ast.TypeString), ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_file_append",
	})
	register(Function{
		Module: "file",
		Name:   "try_append",
		Params: []ast.Type{ast.Basic(ast.TypeString), ast.Basic(ast.TypeString)},
		Return: ast.Struct("FileActionResult"),
		Draft:  true,
		CName:  "__walk_file_try_append",
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
		Module: "dir",
		Name:   "list",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.ArrayOf(ast.Basic(ast.TypeString)),
		Draft:  true,
		CName:  "__walk_dir_list",
	})
	register(Function{
		Module: "dir",
		Name:   "make",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_dir_make",
	})
	register(Function{
		Module: "dir",
		Name:   "delete",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_dir_delete",
	})
	register(Function{
		Module: "path",
		Name:   "join",
		Params: []ast.Type{ast.Basic(ast.TypeString), ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeString),
		Draft:  true,
		CName:  "__walk_path_join",
	})
	register(Function{
		Module: "path",
		Name:   "base",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeString),
		Draft:  true,
		CName:  "__walk_path_base",
	})
	register(Function{
		Module: "path",
		Name:   "ext",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeString),
		Draft:  true,
		CName:  "__walk_path_ext",
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
		Name:   "chdir",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_process_chdir",
	})
	register(Function{
		Module: "process",
		Name:   "run",
		Params: []ast.Type{ast.Basic(ast.TypeString), ast.ArrayOf(ast.Basic(ast.TypeString))},
		Return: ast.Struct("ProcessResult"),
		Draft:  true,
		CName:  "__walk_process_run",
	})
	register(Function{
		Module: "process",
		Name:   "output",
		Params: []ast.Type{ast.Basic(ast.TypeString), ast.ArrayOf(ast.Basic(ast.TypeString))},
		Return: ast.Struct("ProcessOutputResult"),
		Draft:  true,
		CName:  "__walk_process_output",
	})
	register(Function{
		Module: "process",
		Name:   "run_shell",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Struct("ProcessResult"),
		Draft:  true,
		CName:  "__walk_process_run_shell",
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
	register(Function{
		Module: "json",
		Name:   "parse",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Struct("JsonResult"),
		Draft:  true,
		CName:  "__walk_json_parse",
	})
	register(Function{
		Module: "json",
		Name:   "stringify",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeString),
		Draft:  true,
		CName:  "__walk_json_stringify",
	})
	register(Function{
		Module: "json",
		Name:   "read",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Struct("JsonResult"),
		Draft:  true,
		CName:  "__walk_json_read",
	})
	register(Function{
		Module: "json",
		Name:   "write",
		Params: []ast.Type{ast.Basic(ast.TypeString), ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_json_write",
	})
	register(Function{
		Module: "term",
		Name:   "is_tty",
		Return: ast.Basic(ast.TypeBool),
		Draft:  true,
		CName:  "__walk_term_is_tty",
	})
	register(Function{
		Module: "term",
		Name:   "color",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_term_color",
	})
	register(Function{
		Module: "term",
		Name:   "background",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_term_background",
	})
	register(Function{
		Module: "term",
		Name:   "style",
		Params: []ast.Type{ast.Basic(ast.TypeString)},
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_term_style",
	})
	register(Function{
		Module: "term",
		Name:   "reset",
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_term_reset",
	})
	register(Function{
		Module: "term",
		Name:   "clear",
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_term_clear",
	})
	register(Function{
		Module: "term",
		Name:   "move",
		Params: []ast.Type{ast.Basic(ast.TypeInt), ast.Basic(ast.TypeInt)},
		Return: ast.Basic(ast.TypeVoid),
		Effect: true,
		Draft:  true,
		CName:  "__walk_term_move",
	})
	register(Function{
		Module: "term",
		Name:   "width",
		Return: ast.Basic(ast.TypeInt),
		Draft:  true,
		CName:  "__walk_term_width",
	})
	register(Function{
		Module: "term",
		Name:   "height",
		Return: ast.Basic(ast.TypeInt),
		Draft:  true,
		CName:  "__walk_term_height",
	})
	register(Function{
		Module: "term",
		Name:   "read_key",
		Return: ast.Struct("IOReadResult"),
		Draft:  true,
		CName:  "__walk_term_read_key",
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
