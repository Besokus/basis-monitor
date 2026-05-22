#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
ROOT_DIR="$(CDPATH= cd -- "$PROJECT_DIR/.." && pwd)"
RUNTIME_ROOT="${HOME}/.cache/basis_monitor"
BUILD_DIR="${RUNTIME_ROOT}/build"
CTP_LIB_DIR="$PROJECT_DIR/vendor/ctp/live/lib/linux"
CTP_DATA_COLLECT_DIR="$PROJECT_DIR/vendor/ctp/data_collect"
XTP_LIB_DIR="$ROOT_DIR/XTPXQuoteAPI_1.0.15_20260113/lib/centos/onload-8.1.2.26"
BIN_DIR="$PROJECT_DIR/bin"
BIN_PATH="$BIN_DIR/basis_monitor"

mkdir -p "$BUILD_DIR" "$BIN_DIR"

CMAKE_BIN="${CMAKE_BIN:-cmake}"
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"
export LD_LIBRARY_PATH="$CTP_LIB_DIR:$CTP_DATA_COLLECT_DIR:$XTP_LIB_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

"$CMAKE_BIN" -S "$PROJECT_DIR" -B "$BUILD_DIR"
"$CMAKE_BIN" --build "$BUILD_DIR" -j"$JOBS"

cp "$BUILD_DIR/basis_monitor" "$BIN_PATH"
chmod +x "$BIN_PATH"
