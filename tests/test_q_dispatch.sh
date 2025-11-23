#!/usr/bin/env sh

set -eu

q_bin=$1

run_capture() {
  LC_ALL=C "$q_bin" "$@" 2>&1 || true
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

# Unknown first token must fall back to coreq behavior.
out=$(run_capture acl --help)
assert_contains "$out" "Usage: q [options] EXPRESSION"
assert_not_contains "$out" "Unknown subcommand:"

# Unknown first token without --help must still not go through subcommand error path.
out=$(run_capture acl)
assert_not_contains "$out" "Unknown subcommand:"

# Another unknown token should behave the same.
out=$(run_capture zzxxyy --help)
assert_contains "$out" "Usage: q [options] EXPRESSION"
assert_not_contains "$out" "Unknown subcommand:"

# Known subcommand should keep subcommand behavior.
out=$(run_capture has --help)
assert_contains "$out" "Usage: q has <variable> <value>"

# Known subcommand alias should keep subcommand behavior.
out=$(run_capture a --help)
assert_contains "$out" "Usage: q has <variable> <value>"

# list subcommand and alias must still route to coreq behavior.
out=$(run_capture list --help)
assert_contains "$out" "Usage: q [options] EXPRESSION"
assert_not_contains "$out" "Unknown subcommand:"

out=$(run_capture l --help)
assert_contains "$out" "Usage: q [options] EXPRESSION"
assert_not_contains "$out" "Unknown subcommand:"

# Known subcommand error path should remain intact.
out=$(run_capture has)
assert_contains "$out" "Usage: q has <variable> <value>"

# Plain help should remain coreq help.
out=$(run_capture --help)
assert_contains "$out" "Usage: q [options] EXPRESSION"
