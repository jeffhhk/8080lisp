#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
EMSDK_DIR="${1:-${EMSDK_DIR:-$SCRIPT_DIR/toolchains/emsdk}}"
EMSDK_VERSION="${EMSDK_VERSION:-latest}"

if [ -x "$EMSDK_DIR/upstream/emscripten/emcc" ]; then
  printf 'emsdk already installed at %s\n' "$EMSDK_DIR"
  exit 0
fi

mkdir -p "$(dirname -- "$EMSDK_DIR")"

if [ -d "$EMSDK_DIR" ] && [ ! -d "$EMSDK_DIR/.git" ]; then
  printf 'refusing to reuse non-git directory: %s\n' "$EMSDK_DIR" >&2
  exit 1
fi

if [ ! -d "$EMSDK_DIR/.git" ]; then
  git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
fi

"$EMSDK_DIR/emsdk" install "$EMSDK_VERSION"
"$EMSDK_DIR/emsdk" activate --embedded "$EMSDK_VERSION"
"$EMSDK_DIR/upstream/emscripten/emcc" --version
