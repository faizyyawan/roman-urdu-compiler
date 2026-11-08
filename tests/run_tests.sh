#!/bin/sh
set -eu

mkdir -p build/tests

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

actual="$(bin/roman-urdu-interpreter examples/demo.ru)"
[ "$actual" = "$expected_demo" ] || {
    printf 'Phase 1 demo failed. Got:\n%s\n' "$actual" >&2
    exit 1
}

actual="$(bin/roman-urdu-interpreter examples/factorial.ru)"
[ "$actual" = "$expected_factorial" ] || {
    printf 'Phase 1 factorial failed. Got:\n%s\n' "$actual" >&2
    exit 1
}

actual="$(bin/roman-urdu-interpreter examples/comparisons.ru)"
[ "$actual" = "$expected_comparisons" ] || {
    printf 'Phase 1 comparisons failed. Got:\n%s\n' "$actual" >&2
    exit 1
}

bin/roman-urdu-compiler examples/demo.ru build/tests/demo.ll
clang build/tests/demo.ll -o build/tests/demo
actual="$(build/tests/demo)"
[ "$actual" = "$expected_demo" ] || {
    printf 'Phase 2 demo failed. Got:\n%s\n' "$actual" >&2
    exit 1
}

bin/roman-urdu-compiler examples/factorial.ru build/tests/factorial.ll
clang build/tests/factorial.ll -o build/tests/factorial
actual="$(build/tests/factorial)"
[ "$actual" = "$expected_factorial" ] || {
    printf 'Phase 2 factorial failed. Got:\n%s\n' "$actual" >&2
    exit 1
}

bin/roman-urdu-compiler examples/comparisons.ru build/tests/comparisons.ll
clang build/tests/comparisons.ll -o build/tests/comparisons
actual="$(build/tests/comparisons)"
[ "$actual" = "$expected_comparisons" ] || {
    printf 'Phase 2 comparisons failed. Got:\n%s\n' "$actual" >&2
    exit 1
}

printf 'All Phase 1 and Phase 2 tests passed.\n'

