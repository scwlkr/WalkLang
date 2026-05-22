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

"$walk_bin" build examples/v0.walk -o "$work_dir/v0"
expect_output "$work_dir/v0" "11
12
13
distance is 5"

"$walk_bin" build examples/hello.walk -o "$work_dir/hello"
expect_output "$work_dir/hello" "3
hello from WalkLang
true"

"$walk_bin" test examples/v0_1_tests.walk >"$work_dir/tests.out"
grep -q "ok 2 tests" "$work_dir/tests.out"

cat >"$work_dir/shadow.walk" <<'WALK'
var: x = 1
if: true
    var: x = 2
    out: x
WALK
expect_failure "$walk_bin check --warnings=error $work_dir/shadow.walk"

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

echo "v1.2 stress ok"
