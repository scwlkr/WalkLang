#!/usr/bin/env sh
set -eu

repo_root="$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)"
manifest="$repo_root/tests/conformance/manifest.tsv"
expected_dir="$repo_root/tests/conformance/expected"
tmp_root="$repo_root/tests/conformance/tmp"

usage() {
    echo "usage: WALK_REF=<path> [WALK_CANDIDATE=<path>] tests/conformance/run.sh --record|--verify|--parse" >&2
}

if [ "$#" -ne 1 ]; then
    usage
    exit 2
fi

action="$1"
case "$action" in
    --record|--verify|--parse)
        ;;
    *)
        usage
        exit 2
        ;;
esac

if [ "${WALK_REF:-}" = "" ]; then
    echo "WALK_REF is required" >&2
    exit 2
fi

resolve_tool() {
    tool="$1"
    case "$tool" in
        */*)
            tool_dir="$(CDPATH= cd -- "$(dirname -- "$tool")" && pwd)"
            printf '%s/%s\n' "$tool_dir" "$(basename -- "$tool")"
            ;;
        *)
            command -v "$tool"
            ;;
    esac
}

walk_ref="$(resolve_tool "$WALK_REF")"
walk_candidate=""
if [ "${WALK_CANDIDATE:-}" != "" ]; then
    walk_candidate="$(resolve_tool "$WALK_CANDIDATE")"
fi
if [ "$action" = "--parse" ] && [ "$walk_candidate" = "" ]; then
    echo "WALK_CANDIDATE is required for --parse" >&2
    exit 2
fi

mkdir -p "$expected_dir" "$tmp_root"
work_dir="$(mktemp -d "$tmp_root/run.XXXXXX")"

cleanup() {
    rm -rf "$work_dir"
}
trap cleanup EXIT

tab="$(printf '\t')"

expected_base() {
    printf '%s/%s' "$expected_dir" "$1"
}

actual_base() {
    printf '%s/%s/%s' "$work_dir" "$1" "$2"
}

status_class() {
    if [ "$1" -eq 0 ]; then
        printf '0\n'
    else
        printf 'failure\n'
    fi
}

stdin_command() {
    key="$1"
    case "$key" in
        -)
            return 1
            ;;
        compat/stable/input)
            printf '  Walker \n\nLine\r\nFinal'
            return 0
            ;;
        *)
            echo "unknown stdin fixture: $key" >&2
            exit 2
            ;;
    esac
}

run_case() {
    compiler="$1"
    label="$2"
    id="$3"
    mode="$4"
    source="$5"
    cwd="$6"
    stdin_key="$7"

    base="$(actual_base "$label" "$id")"
    mkdir -p "$(dirname -- "$base")"
    stdout_file="$base.stdout"
    stderr_file="$base.stderr"
    status_file="$base.status"
    c_file="$base.c"
    raw_stdout="$base.stdout.raw"
    rm -f "$stdout_file" "$stderr_file" "$status_file" "$c_file" "$raw_stdout"

    set +e
    case "$mode" in
        check)
            (cd "$repo_root/$cwd" && "$compiler" check --warnings=error "$source") >"$stdout_file" 2>"$stderr_file"
            code=$?
            ;;
        run)
            if [ "$stdin_key" = "-" ]; then
                (cd "$repo_root/$cwd" && "$compiler" run --warnings=error "$source") >"$stdout_file" 2>"$stderr_file"
                code=$?
            else
                (cd "$repo_root/$cwd" && stdin_command "$stdin_key" | "$compiler" run --warnings=error "$source") >"$stdout_file" 2>"$stderr_file"
                code=$?
            fi
            ;;
        test)
            (cd "$repo_root/$cwd" && "$compiler" test --warnings=error "$source") >"$stdout_file" 2>"$stderr_file"
            code=$?
            ;;
        emit-c)
            (cd "$repo_root/$cwd" && "$compiler" emit-c --warnings=error "$source" -o "$c_file") >"$raw_stdout" 2>"$stderr_file"
            code=$?
            : >"$stdout_file"
            ;;
        project-test)
            (cd "$repo_root/$cwd" && "$compiler" test --warnings=error) >"$stdout_file" 2>"$stderr_file"
            code=$?
            ;;
        walktop-fixture)
            exe="$work_dir/$label/walktop-fixture"
            (cd "$repo_root" && "$compiler" build --mode release --warnings=error tools/walktop/src/main.walk -o "$exe" >/dev/null && NO_COLOR=1 "$exe" --once --fixture "$source") >"$stdout_file" 2>"$stderr_file"
            code=$?
            ;;
        *)
            echo "unknown conformance mode: $mode" >&2
            exit 2
            ;;
    esac
    set -e

    status_class "$code" >"$status_file"
}

run_parse_case() {
    compiler="$1"
    label="$2"
    id="$3"
    source="$4"
    cwd="$5"
    parse_only="$6"

    base="$(actual_base "$label" "$id")"
    mkdir -p "$(dirname -- "$base")"
    stdout_file="$base.stdout"
    stderr_file="$base.stderr"
    status_file="$base.status"
    rm -f "$stdout_file" "$stderr_file" "$status_file"

    set +e
    if [ "$parse_only" = "yes" ]; then
        (cd "$repo_root/$cwd" && "$compiler" check --parse-only "$source") >"$stdout_file" 2>"$stderr_file"
        code=$?
    else
        (cd "$repo_root/$cwd" && "$compiler" check --warnings=error "$source") >"$stdout_file" 2>"$stderr_file"
        code=$?
    fi
    set -e

    status_class "$code" >"$status_file"
}

validate_kind_status() {
    kind="$1"
    id="$2"
    status_file="$3"
    status="$(cat "$status_file")"
    case "$kind" in
        fail)
            if [ "$status" = "0" ]; then
                echo "expected failure but command passed: $id" >&2
                exit 1
            fi
            ;;
        *)
            if [ "$status" != "0" ]; then
                echo "expected success but command failed: $id" >&2
                exit 1
            fi
            ;;
    esac
}

record_case() {
    id="$1"
    kind="$2"
    mode="$3"
    source="$4"
    cwd="$5"
    stdin_key="$6"

    run_case "$walk_ref" reference "$id" "$mode" "$source" "$cwd" "$stdin_key"
    base="$(actual_base reference "$id")"
    validate_kind_status "$kind" "$id" "$base.status"

    expected="$(expected_base "$id")"
    mkdir -p "$(dirname -- "$expected")"
    cp "$base.stdout" "$expected.stdout"
    cp "$base.stderr" "$expected.stderr"
    cp "$base.status" "$expected.status"
    if [ "$mode" = "emit-c" ]; then
        cp "$base.c" "$expected.c"
    else
        rm -f "$expected.c"
    fi
}

compare_file() {
    cmp_expected="$1"
    cmp_actual="$2"
    cmp_label="$3"
    if [ ! -f "$cmp_expected" ]; then
        echo "missing expected artifact: $cmp_expected" >&2
        exit 1
    fi
    if ! diff -u "$cmp_expected" "$cmp_actual"; then
        echo "conformance mismatch: $cmp_label" >&2
        exit 1
    fi
}

verify_case_for() {
    compiler="$1"
    label="$2"
    id="$3"
    kind="$4"
    mode="$5"
    source="$6"
    cwd="$7"
    stdin_key="$8"

    run_case "$compiler" "$label" "$id" "$mode" "$source" "$cwd" "$stdin_key"
    actual_prefix="$(actual_base "$label" "$id")"
    validate_kind_status "$kind" "$id" "$actual_prefix.status"

    expected_prefix="$(expected_base "$id")"
    compare_file "$expected_prefix.status" "$actual_prefix.status" "$label $id status"
    compare_file "$expected_prefix.stdout" "$actual_prefix.stdout" "$label $id stdout"
    compare_file "$expected_prefix.stderr" "$actual_prefix.stderr" "$label $id stderr"
    if [ "$mode" = "emit-c" ]; then
        compare_file "$expected_prefix.c" "$actual_prefix.c" "$label $id generated C"
    fi
}

is_parse_case() {
    id="$1"
    kind="$2"
    case "$kind" in
        pass)
            return 0
            ;;
        fail)
            expected_stderr="$(expected_base "$id").stderr"
            if [ -f "$expected_stderr" ] && grep -q 'syntax error:' "$expected_stderr"; then
                return 0
            fi
            return 1
            ;;
        *)
            return 1
            ;;
    esac
}

verify_parse_case_for() {
    compiler="$1"
    label="$2"
    id="$3"
    kind="$4"
    source="$5"
    cwd="$6"
    parse_only="$7"

    run_parse_case "$compiler" "$label" "$id" "$source" "$cwd" "$parse_only"
    actual_prefix="$(actual_base "$label" "$id")"
    validate_kind_status "$kind" "$id" "$actual_prefix.status"
}

manifest_sources_for_kind() {
    kind="$1"
    awk -F '\t' -v kind="$kind" 'NF && $1 !~ /^#/ && $2 == kind { print $4 }' "$manifest" | sort -u
}

check_coverage() {
    expected="$work_dir/coverage.expected"
    actual="$work_dir/coverage.actual"

    find "$repo_root/tests/pass" -type f -name '*.walk' | sed "s#^$repo_root/##" | sort >"$expected"
    manifest_sources_for_kind pass >"$actual"
    if ! diff -u "$expected" "$actual"; then
        echo "pass fixture manifest coverage mismatch" >&2
        exit 1
    fi

    find "$repo_root/tests/fail" -type f -name '*.walk' |
        sed "s#^$repo_root/##" |
        grep -v '^tests/fail/private_math\.walk$' |
        sort >"$expected"
    manifest_sources_for_kind fail >"$actual"
    if ! diff -u "$expected" "$actual"; then
        echo "fail fixture manifest coverage mismatch" >&2
        exit 1
    fi

    find "$repo_root/tests/compat" -type f -name '*.walk' | sed "s#^$repo_root/##" | sort >"$expected"
    manifest_sources_for_kind compat >"$actual"
    if ! diff -u "$expected" "$actual"; then
        echo "compat fixture manifest coverage mismatch" >&2
        exit 1
    fi

    find "$repo_root/tests/snapshots" -type f -name '*.c' |
        sed "s#^$repo_root/tests/snapshots/#tests/pass/#" |
        sed 's#\.c$#.walk#' |
        sort >"$expected"
    manifest_sources_for_kind snapshot >"$actual"
    if ! diff -u "$expected" "$actual"; then
        echo "snapshot fixture manifest coverage mismatch" >&2
        exit 1
    fi

    {
        find "$repo_root/tools/walktop/src" "$repo_root/tools/walktop/tests" -type f -name '*.walk'
        find "$repo_root/tools/walktop/testdata" -mindepth 1 -maxdepth 1 -type d
    } | sed "s#^$repo_root/##" | sort >"$expected"
    manifest_sources_for_kind walktop >"$actual"
    if ! diff -u "$expected" "$actual"; then
        echo "walktop fixture manifest coverage mismatch" >&2
        exit 1
    fi
}

pass_ok=0
fail_ok=0
native_ok=0
compat_ok=0
snapshot_ok=0
walktop_ok=0

check_coverage

while IFS="$tab" read -r id kind mode source cwd stdin_key native; do
    case "$id" in
        ""|\#*)
            continue
            ;;
    esac
    if [ "${native:-}" = "" ]; then
        echo "manifest row has missing fields: $id" >&2
        exit 1
    fi

    if [ "$action" = "--parse" ]; then
        if ! is_parse_case "$id" "$kind"; then
            continue
        fi
        verify_parse_case_for "$walk_ref" reference "$id" "$kind" "$source" "$cwd" no
        verify_parse_case_for "$walk_candidate" candidate "$id" "$kind" "$source" "$cwd" yes
        case "$kind" in
            pass)
                pass_ok=$((pass_ok + 1))
                ;;
            fail)
                fail_ok=$((fail_ok + 1))
                ;;
        esac
        continue
    fi

    case "$action" in
        --record)
            record_case "$id" "$kind" "$mode" "$source" "$cwd" "$stdin_key"
            ;;
        --verify)
            verify_case_for "$walk_ref" reference "$id" "$kind" "$mode" "$source" "$cwd" "$stdin_key"
            if [ "$walk_candidate" != "" ]; then
                verify_case_for "$walk_candidate" candidate "$id" "$kind" "$mode" "$source" "$cwd" "$stdin_key"
            fi
            ;;
    esac

    case "$kind" in
        pass)
            pass_ok=$((pass_ok + 1))
            ;;
        fail)
            fail_ok=$((fail_ok + 1))
            ;;
        compat)
            compat_ok=$((compat_ok + 1))
            ;;
        snapshot)
            snapshot_ok=$((snapshot_ok + 1))
            ;;
        walktop)
            walktop_ok=$((walktop_ok + 1))
            ;;
        *)
            echo "unknown manifest kind: $kind" >&2
            exit 1
            ;;
    esac
    if [ "$native" = "yes" ]; then
        native_ok=$((native_ok + 1))
    fi
done <"$manifest"

if [ "$action" = "--parse" ]; then
    echo "conformance parse: $pass_ok pass fixtures ok"
    echo "conformance parse: $fail_ok syntax fail fixtures ok"
    echo "conformance parse: ok"
    exit 0
fi

echo "conformance: $pass_ok pass fixtures ok"
echo "conformance: $fail_ok fail fixtures ok"
echo "conformance: $native_ok native executions ok"
echo "conformance: $compat_ok compat fixtures ok"
echo "conformance: $snapshot_ok snapshot fixtures ok"
echo "conformance: $walktop_ok walktop fixtures ok"
echo "conformance: ok"
