#!/usr/bin/env sh
set -eu

ebuild_exec_cc="$1"

if rg -n 'new[[:space:]]+EbuildExecSettings|delete[[:space:]]+settings' "$ebuild_exec_cc" >/dev/null 2>&1; then
  echo "ebuild_exec.cc still uses raw new/delete for static EbuildExec::settings storage"
  exit 1
fi
