#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
EMSDK_DIR="${1:-${EMSDK_DIR:-$SCRIPT_DIR/toolchains/emsdk}}"

case "$EMSDK_DIR" in
  ""|"/")
    printf 'refusing to remove unsafe path: %s\n' "$EMSDK_DIR" >&2
    exit 1
    ;;
esac

if [ ! -e "$EMSDK_DIR" ]; then
  printf 'emsdk already absent at %s\n' "$EMSDK_DIR"
  exit 0
fi

rm -rf "$EMSDK_DIR"
printf 'removed emsdk at %s\n' "$EMSDK_DIR"
