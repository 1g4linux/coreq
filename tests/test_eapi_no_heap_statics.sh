#!/usr/bin/env sh
set -eu

eapi_cc="$1"

if rg -n 'new[[:space:]]+EapiMap|new[[:space:]]+WordVec|delete[[:space:]]+eapi_map|delete[[:space:]]+eapi_vec' "$eapi_cc" >/dev/null 2>&1; then
  echo "eapi.cc still uses raw new/delete for static EAPI storage"
  exit 1
fi
