package format

import "testing"

func TestV1FormatterNormalizesIndentationToFourSpaces(t *testing.T) {
	formatted, err := Format("if:true\n  out:'inside'\nout:'outside'\n", "main.walk")
	if err != nil {
		t.Fatal(err)
	}
	want := "if: true\n    out: 'inside'\nout: 'outside'\n"
	if formatted != want {
		t.Fatalf("want %q, got %q", want, formatted)
	}
}

func TestV1FormatterKeepsCallCalleeTight(t *testing.T) {
	formatted, err := Format("out:math_extra.cube(3)\nout:> time.now() 0\n", "main.walk")
	if err != nil {
		t.Fatal(err)
	}
	want := "out: math_extra.cube(3)\nout: > time.now() 0\n"
	if formatted != want {
		t.Fatalf("want %q, got %q", want, formatted)
	}
}

func TestV2FormatterNormalizesStructs(t *testing.T) {
	formatted, err := Format("struct:User\n  name string\n  age int\nvar:user=User('Walker',25)\nout:user.name\n", "main.walk")
	if err != nil {
		t.Fatal(err)
	}
	want := "struct: User\n    name string\n    age int\nvar: user = User('Walker', 25)\nout: user.name\n"
	if formatted != want {
		t.Fatalf("want %q, got %q", want, formatted)
	}
}

func TestV17FormatterNormalizesInputExpression(t *testing.T) {
	formatted, err := Format("var:name=in:'Name? '\nout:in:\n", "main.walk")
	if err != nil {
		t.Fatal(err)
	}
	want := "var: name = in: 'Name? '\nout: in:\n"
	if formatted != want {
		t.Fatalf("want %q, got %q", want, formatted)
	}
}

func TestV21FormatterKeepsMethodSyntaxTight(t *testing.T) {
	formatted, err := Format("func:User.is_adult(self User)bool\n  return:>= self.age 18\nout:user.is_adult()\n", "main.walk")
	if err != nil {
		t.Fatal(err)
	}
	want := "func: User.is_adult(self User) bool\n    return: >= self.age 18\nout: user.is_adult()\n"
	if formatted != want {
		t.Fatalf("want %q, got %q", want, formatted)
	}
}
