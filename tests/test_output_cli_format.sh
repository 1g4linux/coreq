#!/usr/bin/env sh

set -eu

q_bin=$1
missing_cache=/tmp/coreq-missing-format-cache.coreq

run_capture() {
  LC_ALL=C "$q_bin" --cache-file "$missing_cache" "$@" 2>&1 || true
}

assert_contains() {
  haystack=$1
  needle=$2
  if ! printf '%s\n' "$haystack" | grep -F -- "$needle" >/dev/null 2>&1; then
    printf 'expected output to contain: %s\n' "$needle" >&2
    printf '%s\n' "$haystack" >&2
    exit 1
  fi
}

assert_not_contains() {
  haystack=$1
  needle=$2
  if printf '%s\n' "$haystack" | grep -F -- "$needle" >/dev/null 2>&1; then
    printf 'expected output to NOT contain: %s\n' "$needle" >&2
    printf '%s\n' "$haystack" >&2
    exit 1
  fi
}

# Invalid format string must fail at formatter parse time.
out=$(run_capture --format "<name" "cat/pkg")
assert_contains "$out" "problems while parsing --format"
assert_contains "$out" "without closing '>'"
assert_not_contains "$out" "cannot open database file"

# Valid format string must parse and then continue until cache open fails.
out=$(run_capture --format "<category>/<name>\\n" "cat/pkg")
assert_not_contains "$out" "problems while parsing --format"
assert_contains "$out" "cannot open database file"

# --only-names has stronger formatting semantics than --format.
# The invalid custom format must not be parsed in this mode.
out=$(run_capture --only-names --format "<name" "cat/pkg")
assert_not_contains "$out" "problems while parsing --format"
assert_contains "$out" "cannot open database file"

