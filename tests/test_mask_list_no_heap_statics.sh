#!/usr/bin/env sh
set -eu

mask_list_cc="$1"

if rg -n 'static[[:space:]]+string\*[[:space:]]+unique|unique[[:space:]]*=[[:space:]]*new[[:space:]]+string\("#a"\)' "$mask_list_cc" >/dev/null 2>&1; then
  echo "mask_list.cc still uses heap-backed static unique comment sentinel storage"
  exit 1
fi
