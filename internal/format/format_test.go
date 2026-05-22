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
