#!/usr/bin/env bash
set -euo pipefail

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
LOCK="$ROOT/PORTMASTER_SUBMODULES.lock"

if ! command -v git >/dev/null 2>&1; then
  echo "git is required" >&2
  exit 1
fi

while IFS=$'\t' read -r rel sha url; do
  case "$rel" in ''|'#'*) continue ;; esac
  dst="$ROOT/$rel"
  echo "==> $rel @ $sha"

  if [ -d "$dst/.git" ] || [ -f "$dst/.git" ]; then
    git -C "$dst" remote get-url origin >/dev/null 2>&1 || git -C "$dst" remote add origin "$url"
  else
    rm -rf "$dst"
    mkdir -p "$dst"
    git -C "$dst" init -q
    git -C "$dst" remote add origin "$url"
  fi

  if ! git -C "$dst" cat-file -e "$sha^{commit}" 2>/dev/null; then
    git -C "$dst" fetch -q --depth 1 origin "$sha"
  fi
  git -C "$dst" checkout -q --detach "$sha"
  # Some top-level dependencies (notably dynarmic) have their own pinned
  # submodules. Restore them recursively from that dependency's metadata.
  git -C "$dst" submodule update --init --recursive --depth 1

done < "$LOCK"

echo "All locked EKA2L1 submodules are present."
