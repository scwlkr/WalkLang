#!/usr/bin/env sh
set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
walk_bin="${WALK_BIN:-walk}"
work_dir="$(mktemp -d "${TMPDIR:-/tmp}/walklang-stress-v1.XXXXXX")"

cleanup() {
    rm -rf "$work_dir"
}
trap cleanup EXIT

cd "$repo_root"

if ! command -v "$walk_bin" >/dev/null 2>&1; then
    echo "walk command not found. Run scripts/install-local.sh first." >&2
    exit 1
fi

expect_output() {
    command="$1"
    want="$2"
    output_file="$work_dir/output.txt"
    sh -c "$command" >"$output_file"
    if [ "$(cat "$output_file")" != "$want" ]; then
        echo "unexpected output for: $command" >&2
        echo "want:" >&2
        printf '%s\n' "$want" >&2
        echo "got:" >&2
        cat "$output_file" >&2
        exit 1
    fi
}

expect_failure() {
    command="$1"
    if sh -c "$command" >"$work_dir/failure.out" 2>"$work_dir/failure.err"; then
        echo "expected failure for: $command" >&2
        exit 1
    fi
}

go test ./cmd/walk -run TestV18CompatibilitySuite
go test ./...

"$walk_bin" check --warnings=error examples/v1.walk
"$walk_bin" emit-c examples/v1.walk -o "$work_dir/v1.c"
grep -q "math_extra__cube" "$work_dir/v1.c"

"$walk_bin" build examples/v1.walk -o "$work_dir/v1" --release
expect_output "$work_dir/v1" "27
8
4
3
true"

"$walk_bin" build tests/pass/stdlib.walk -o "$work_dir/stdlib"
expect_output "$work_dir/stdlib" "3
8
4
a
l
true
false
walklang
3
2
true
false
true
7
fixed
true"

"$walk_bin" build examples/v0.walk -o "$work_dir/v0"
expect_output "$work_dir/v0" "11
12
13
distance is 5"

"$walk_bin" build examples/hello.walk -o "$work_dir/hello"
expect_output "$work_dir/hello" "3
hello from WalkLang
true"

cat >"$work_dir/input.walk" <<'WALK'
var: prompt = 'Name? '
var: name = in: prompt
var: blank = in:
var: crlf = in:
out: name
out: blank
out: crlf
out: in:
WALK
"$walk_bin" build "$work_dir/input.walk" -o "$work_dir/input"
printf '  Walker \n\nLine\r\nFinal' | "$work_dir/input" >"$work_dir/input.out"
printf 'Name?   Walker \n\nLine\nFinal\n' >"$work_dir/input.expected"
cmp "$work_dir/input.expected" "$work_dir/input.out"

cat >"$work_dir/input_eof.walk" <<'WALK'
var: name = in:
out: name
WALK
"$walk_bin" build "$work_dir/input_eof.walk" -o "$work_dir/input-eof"
expect_failure "$work_dir/input-eof"
grep -q "walk runtime error: input reached EOF" "$work_dir/failure.err"

"$walk_bin" test examples/v0_1_tests.walk >"$work_dir/tests.out"
grep -q "ok 2 tests" "$work_dir/tests.out"

"$walk_bin" test tests/pass/walk_tests.walk >"$work_dir/stdlib-tests.out"
grep -q "ok 2 tests" "$work_dir/stdlib-tests.out"

cat >"$work_dir/shadow.walk" <<'WALK'
var: x = 1
if: true
    var: x = 2
    out: x
WALK
"$walk_bin" check "$work_dir/shadow.walk" >"$work_dir/shadow.out" 2>"$work_dir/shadow.err"
grep -q "warning: x shadows outer name" "$work_dir/shadow.err"
grep -q "rename this binding or assign to the existing name" "$work_dir/shadow.err"
expect_failure "$walk_bin check --warnings=error $work_dir/shadow.walk"

cat >"$work_dir/type_error.walk" <<'WALK'
var: age int = 'old'
WALK
expect_failure "$walk_bin check $work_dir/type_error.walk"
grep -q "type error: age is int, got string" "$work_dir/failure.err"
grep -q "var: age int = 'old'" "$work_dir/failure.err"
grep -q "string cannot initialize int" "$work_dir/failure.err"

cat >"$work_dir/math_extra.walk" <<'WALK'
func: cube(x int) int
    return: * x x x

exp: cube
WALK
cat >"$work_dir/private_import.walk" <<'WALK'
imp: math_extra
out: math_extra.hidden(3)
WALK
expect_failure "$walk_bin check $work_dir/private_import.walk"

cat >"$work_dir/messy.walk" <<'WALK'
if:true
  out:math_extra.cube(3)
out:> time.now() 0
WALK
"$walk_bin" fmt "$work_dir/messy.walk" >"$work_dir/formatted.walk"
cat >"$work_dir/expected-formatted.walk" <<'WALK'
if: true
    out: math_extra.cube(3)
out: > time.now() 0
WALK
cmp "$work_dir/expected-formatted.walk" "$work_dir/formatted.walk"

project_dir="$work_dir/hello_project"
"$walk_bin" init "$project_dir" >/dev/null
(
    cd "$project_dir"
    "$walk_bin" check --warnings=error >/dev/null
    "$walk_bin" build >/dev/null
    expect_output "$project_dir/build/hello_project" "27"
    "$walk_bin" test >"$work_dir/project-tests.out"
    grep -q "ok 1 tests" "$work_dir/project-tests.out"
    printf 'out:+ 1 2\n' >src/messy.walk
    "$walk_bin" fmt >/dev/null
    printf 'out: + 1 2\n' >"$work_dir/project-formatted.walk"
    cmp "$work_dir/project-formatted.walk" src/messy.walk
    "$walk_bin" clean >/dev/null
    test ! -d build
)

echo "v1.8 stress ok"
