#!/usr/bin/env sh
set -eu

corepkgsettings_cc="$1"

if rg -n 'emptystring[[:space:]]*=[[:space:]]*new[[:space:]]+string|delete[[:space:]]+emptystring' "$corepkgsettings_cc" >/dev/null 2>&1; then
  echo "corepkgsettings.cc still uses raw new/delete for static emptystring storage"
  exit 1
fi
