#!/usr/bin/env sh
set -eu

coreqcache_cc="$1"

if rg -n 'all_coreqcaches[[:space:]]*=[[:space:]]*new[[:space:]]+CachesList' "$coreqcache_cc" >/dev/null 2>&1; then
  echo "coreqcache.cc still heap-allocates static all_coreqcaches container"
  exit 1
fi
