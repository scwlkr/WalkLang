package ast

import "strings"

type Location struct {
	Filename string
	Line     int
	Column   int
}

type TypeKind string

const (
	TypeInvalid  TypeKind = ""
	TypeVoid     TypeKind = "void"
	TypeInt      TypeKind = "int"
	TypeFloat    TypeKind = "float"
	TypeBool     TypeKind = "bool"
	TypeString   TypeKind = "string"
	TypeNull     TypeKind = "null"
	TypeArray    TypeKind = "array"
	TypeFunction TypeKind = "func"
	TypeStruct   TypeKind = "struct"
)

type Type struct {
	Kind     TypeKind
	Name     string
	Nullable bool
	Elem     *Type
	Params   []Type
	Return   *Type
}

func (t Type) String() string {
	var out string
	switch t.Kind {
	case TypeArray:
		if t.Elem == nil {
			out = "array[?]"
		} else {
			out = "array[" + t.Elem.String() + "]"
		}
	case TypeFunction:
		parts := make([]string, 0, len(t.Params))
		for _, param := range t.Params {
			parts = append(parts, param.String())
		}
		returnType := "void"
		if t.Return != nil {
			returnType = t.Return.String()
		}
		out = "func(" + strings.Join(parts, ", ") + ") " + returnType
	case TypeStruct:
		out = t.Name
	default:
		out = string(t.Kind)
	}
	if t.Nullable {
		out += "?"
	}
	return out
}

func (t Type) Equal(other Type) bool {
	if t.Kind != other.Kind || t.Nullable != other.Nullable || t.Name != other.Name {
		return false
	}
	switch t.Kind {
	case TypeArray:
		return t.Elem != nil && other.Elem != nil && t.Elem.Equal(*other.Elem)
	case TypeFunction:
		if len(t.Params) != len(other.Params) {
			return false
		}
		for i := range t.Params {
			if !t.Params[i].Equal(other.Params[i]) {
				return false
			}
		}
		if t.Return == nil || other.Return == nil {
			return t.Return == other.Return
		}
		return t.Return.Equal(*other.Return)
	default:
		return true
	}
}

func Basic(kind TypeKind) Type {
	return Type{Kind: kind}
}

func ArrayOf(elem Type) Type {
	return Type{Kind: TypeArray, Elem: &elem}
}

func FuncType(params []Type, ret Type) Type {
	return Type{Kind: TypeFunction, Params: params, Return: &ret}
}

func Struct(name string) Type {
	return Type{Kind: TypeStruct, Name: name}
}

type Program struct {
	Statements []Statement
}

type Statement interface {
	Loc() Location
	statementNode()
}

type Expression interface {
	Loc() Location
	ExprType() Type
	SetExprType(Type)
	expressionNode()
}

type Param struct {
	Name string
	Type Type
}

type VarDecl struct {
	Location   Location
	Name       string
	Annotation Type
	Value      Expression
	Mutable    bool
}

func (s *VarDecl) Loc() Location  { return s.Location }
func (s *VarDecl) statementNode() {}

type Assignment struct {
	Location Location
	Target   Expression
	Value    Expression
}

func (s *Assignment) Loc() Location  { return s.Location }
func (s *Assignment) statementNode() {}

type Out struct {
	Location Location
	Value    Expression
}

func (s *Out) Loc() Location  { return s.Location }
func (s *Out) statementNode() {}

type TestDecl struct {
	Location Location
	Name     string
	Body     []Statement
}

func (s *TestDecl) Loc() Location  { return s.Location }
func (s *TestDecl) statementNode() {}

type Assert struct {
	Location Location
	Value    Expression
}

func (s *Assert) Loc() Location  { return s.Location }
func (s *Assert) statementNode() {}

type Import struct {
	Location Location
	Module   string
}

func (s *Import) Loc() Location  { return s.Location }
func (s *Import) statementNode() {}

type Export struct {
	Location Location
	Name     string
}

func (s *Export) Loc() Location  { return s.Location }
func (s *Export) statementNode() {}

type FuncDecl struct {
	Location   Location
	Name       string
	Receiver   string
	Params     []Param
	ReturnType Type
	Body       []Statement
}

func (s *FuncDecl) Loc() Location  { return s.Location }
func (s *FuncDecl) statementNode() {}

type StructField struct {
	Location Location
	Name     string
	Type     Type
}

type StructDecl struct {
	Location Location
	Name     string
	Fields   []StructField
}

func (s *StructDecl) Loc() Location  { return s.Location }
func (s *StructDecl) statementNode() {}

type Return struct {
	Location Location
	Value    Expression
}

func (s *Return) Loc() Location  { return s.Location }
func (s *Return) statementNode() {}

type If struct {
	Location Location
	Cond     Expression
	Then     []Statement
	Else     []Statement
}

func (s *If) Loc() Location  { return s.Location }
func (s *If) statementNode() {}

type While struct {
	Location Location
	Cond     Expression
	Body     []Statement
}

func (s *While) Loc() Location  { return s.Location }
func (s *While) statementNode() {}

type Repeat struct {
	Location Location
	Count    Expression
	Body     []Statement
}

func (s *Repeat) Loc() Location  { return s.Location }
func (s *Repeat) statementNode() {}

type For struct {
	Location Location
	Name     string
	Iterable Expression
	Body     []Statement
}

func (s *For) Loc() Location  { return s.Location }
func (s *For) statementNode() {}

type Break struct {
	Location Location
}

func (s *Break) Loc() Location  { return s.Location }
func (s *Break) statementNode() {}

type Continue struct {
	Location Location
}

func (s *Continue) Loc() Location  { return s.Location }
func (s *Continue) statementNode() {}

type ExprBase struct {
	Location Location
	Type     Type
}

func (e *ExprBase) Loc() Location      { return e.Location }
func (e *ExprBase) ExprType() Type     { return e.Type }
func (e *ExprBase) SetExprType(t Type) { e.Type = t }

type LiteralKind string

const (
	LiteralInt    LiteralKind = "int"
	LiteralFloat  LiteralKind = "float"
	LiteralBool   LiteralKind = "bool"
	LiteralString LiteralKind = "string"
	LiteralNull   LiteralKind = "null"
)

type Literal struct {
	ExprBase
	Kind  LiteralKind
	Value string
}

func (e *Literal) expressionNode() {}

type Name struct {
	ExprBase
	Identifier string
}

func (e *Name) expressionNode() {}

type Prefix struct {
	ExprBase
	Operator string
	Args     []Expression
}

func (e *Prefix) expressionNode() {}

type Call struct {
	ExprBase
	Callee   string
	Receiver Expression
	Method   string
	Args     []Expression
}

func (e *Call) expressionNode() {}

type ArrayLiteral struct {
	ExprBase
	Elements []Expression
}

func (e *ArrayLiteral) expressionNode() {}

type Index struct {
	ExprBase
	Target Expression
	Index  Expression
}

func (e *Index) expressionNode() {}

type FieldAccess struct {
	ExprBase
	Target Expression
	Field  string
}

func (e *FieldAccess) expressionNode() {}
