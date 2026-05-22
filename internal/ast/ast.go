package ast

type Location struct {
	Filename string
	Line     int
	Column   int
}

type TypeName string

const (
	TypeInt    TypeName = "int"
	TypeFloat  TypeName = "float"
	TypeBool   TypeName = "bool"
	TypeString TypeName = "string"
)

type Program struct {
	Statements []Statement
}

type Statement interface {
	Loc() Location
	statementNode()
}

type Expression interface {
	Loc() Location
	ExprType() TypeName
	SetExprType(TypeName)
	expressionNode()
}

type VarDecl struct {
	Location   Location
	Name       string
	Annotation TypeName
	Value      Expression
	Mutable    bool
}

func (s *VarDecl) Loc() Location  { return s.Location }
func (s *VarDecl) statementNode() {}

type Assignment struct {
	Location Location
	Name     string
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

type ExprBase struct {
	Location Location
	Type     TypeName
}

func (e *ExprBase) Loc() Location          { return e.Location }
func (e *ExprBase) ExprType() TypeName     { return e.Type }
func (e *ExprBase) SetExprType(t TypeName) { e.Type = t }

type LiteralKind string

const (
	LiteralInt    LiteralKind = "int"
	LiteralFloat  LiteralKind = "float"
	LiteralBool   LiteralKind = "bool"
	LiteralString LiteralKind = "string"
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
