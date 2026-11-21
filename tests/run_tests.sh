#!/bin/sh
set -eu

mkdir -p build/tests

fail() {
    printf 'FAILED: %s\n' "$1" >&2
    exit 1
}

expect_output() {
    label=$1
    expected=$2
    shift 2
    set +e
    actual=$("$@" 2>&1)
    status=$?
    set -e
    [ "$status" -eq 0 ] || fail "$label exited with $status: $actual"
    [ "$actual" = "$expected" ] || fail "$label output mismatch: $actual"
}

expect_failure() {
    label=$1
    expected=$2
    shift 2
    set +e
    actual=$("$@" 2>&1)
    status=$?
    set -e
    [ "$status" -ne 0 ] || fail "$label unexpectedly succeeded"
    case "$actual" in
        *"$expected"*) ;;
        *) fail "$label missing expected error '$expected': $actual" ;;
    esac
}

compile_and_check() {
    name=$1
    source=$2
    expected=$3
    bin/roman-urdu compile "$source" "build/tests/$name.ll"
    clang -Wno-override-module "build/tests/$name.ll" -o "build/tests/$name"
    expect_output "compiler $name" "$expected" "build/tests/$name"
}

expected_demo='y bara hai
1 se 5 tak jama:
15'
expected_factorial='120'
expected_comparisons='x aur y barabar hain
x aur z barabar nahi hain
z x se bara hay
x z se chota hay
z y se bara ya barabar hay
x y se chota ya barabar hay'

expect_output "interpreter demo" "$expected_demo" \
    bin/roman-urdu interpret examples/demo.ru
expect_output "interpreter factorial" "$expected_factorial" \
    bin/roman-urdu interpret examples/factorial.ru
expect_output "interpreter comparisons" "$expected_comparisons" \
    bin/roman-urdu interpret examples/comparisons.ru
expect_output "interpreter short circuit" "short circuit ok" \
    bin/roman-urdu interpret tests/programs/short_circuit.ru

compile_and_check demo examples/demo.ru "$expected_demo"
compile_and_check factorial examples/factorial.ru "$expected_factorial"
compile_and_check comparisons examples/comparisons.ru "$expected_comparisons"

bin/roman-urdu build tests/programs/short_circuit.ru build/tests/short-circuit
expect_output "AOT build command" "short circuit ok" build/tests/short-circuit

expect_failure "interpreter use before declaration" "variable used before declaration x" \
    bin/roman-urdu interpret tests/programs/use_before_declare.ru
expect_failure "compiler use before declaration" "variable used before declaration x" \
    bin/roman-urdu compile tests/programs/use_before_declare.ru build/tests/invalid.ll
expect_failure "assignment before declaration" "variable assigned before declaration x" \
    bin/roman-urdu interpret tests/programs/assignment_before_declare.ru
expect_failure "duplicate declaration" "variable already declared x" \
    bin/roman-urdu compile tests/programs/duplicate_declaration.ru build/tests/invalid.ll
expect_failure "nested declaration" "variable declarations are only allowed at top level x" \
    bin/roman-urdu interpret tests/programs/nested_declaration.ru
expect_failure "syntax error" "syntax error" \
    bin/roman-urdu interpret tests/programs/syntax_error.ru
expect_failure "integer overflow" "integer literal is out of range" \
    bin/roman-urdu interpret tests/programs/integer_overflow.ru
expect_failure "interpreter division by zero" "runtime error: division by zero" \
    bin/roman-urdu interpret tests/programs/division_zero.ru

bin/roman-urdu build tests/programs/division_zero.ru build/tests/division-zero
expect_failure "native division by zero" "runtime error: division by zero" \
    build/tests/division-zero

expect_failure "interpreter modulo by zero" "runtime error: modulo by zero" \
    bin/roman-urdu interpret tests/programs/modulo_zero.ru
bin/roman-urdu build tests/programs/modulo_zero.ru build/tests/modulo-zero
expect_failure "native modulo by zero" "runtime error: modulo by zero" \
    build/tests/modulo-zero

printf 'All interpreter, compiler, semantic, and runtime tests passed.\n'
