#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
ROOT_DIR="$(CDPATH= cd -- "$PROJECT_DIR/.." && pwd)"
RUNTIME_ROOT="${HOME}/.cache/basis_monitor"
BUILD_DIR="${RUNTIME_ROOT}/build"
PID_FILE="$PROJECT_DIR/runtime/basis_monitor.pid"
NOHUP_LOG="$PROJECT_DIR/logs/nohup.out"
CTP_LIB_DIR="$PROJECT_DIR/vendor/ctp/live/lib/linux"
CTP_DATA_COLLECT_DIR="$PROJECT_DIR/vendor/ctp/data_collect"
XTP_LIB_DIR="$ROOT_DIR/XTPXQuoteAPI_1.0.15_20260113/lib/centos/onload-8.1.2.26"

mkdir -p "$BUILD_DIR" "$PROJECT_DIR/logs" "$PROJECT_DIR/runtime"

: > "$NOHUP_LOG"

record_startup_issue() {
    message="$1"
    printf '%s\n' "$message" | tee -a "$NOHUP_LOG" >&2
}

if [ -f "$PID_FILE" ]; then
    EXISTING_PID="$(cat "$PID_FILE" 2>/dev/null || true)"
    if [ -n "${EXISTING_PID:-}" ] && kill -0 "$EXISTING_PID" 2>/dev/null; then
        record_startup_issue "basis_monitor is already running with pid $EXISTING_PID"
        exit 1
    fi
    rm -f "$PID_FILE"
fi

CMAKE_BIN="${CMAKE_BIN:-cmake}"
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"

export LD_LIBRARY_PATH="$CTP_LIB_DIR:$CTP_DATA_COLLECT_DIR:$XTP_LIB_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

"$CMAKE_BIN" -S "$PROJECT_DIR" -B "$BUILD_DIR"
"$CMAKE_BIN" --build "$BUILD_DIR" -j"$JOBS"

cd "$PROJECT_DIR"

nohup "$BUILD_DIR/basis_monitor" > /dev/null 2>&1 &
PID=$!
echo "$PID" > "$PID_FILE"

sleep 1

if ! kill -0 "$PID" 2>/dev/null; then
    rm -f "$PID_FILE"
    record_startup_issue "basis_monitor failed to stay running, check logs/runtime.log for application logs"
    exit 1
fi

echo "basis_monitor started in background"
echo "pid: $PID"
echo "runtime log: $PROJECT_DIR/logs/runtime.log"
echo "alert log: $PROJECT_DIR/logs/alert.log"
