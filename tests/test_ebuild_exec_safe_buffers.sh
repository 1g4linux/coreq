#!/usr/bin/env sh
set -eu

ebuild_exec_cc="$1"

if rg -n 'new[[:space:]]+char\[[^]]+\]|std::strcpy\(' "$ebuild_exec_cc" >/dev/null 2>&1; then
  echo "ebuild_exec.cc still uses manual char[] buffers or strcpy"
  exit 1
fi
