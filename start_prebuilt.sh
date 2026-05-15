#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
ROOT_DIR="$(CDPATH= cd -- "$PROJECT_DIR/.." && pwd)"

BIN_PATH="$PROJECT_DIR/bin/basis_monitor"
PID_FILE="$PROJECT_DIR/runtime/basis_monitor.pid"
NOHUP_LOG="$PROJECT_DIR/logs/nohup.out"

CTP_LIB_DIR="$PROJECT_DIR/vendor/ctp/live/lib/linux"
CTP_DATA_COLLECT_DIR="$PROJECT_DIR/vendor/ctp/data_collect"
XTP_LIB_DIR="$ROOT_DIR/XTPXQuoteAPI_1.0.15_20260113/lib/centos/onload-8.1.2.26"

mkdir -p \
    "$PROJECT_DIR/logs" \
    "$PROJECT_DIR/runtime/flow" \
    "$PROJECT_DIR/data/output/reports"

: > "$NOHUP_LOG"

record_startup_issue() {
    message="$1"
    printf '%s\n' "$message" | tee -a "$NOHUP_LOG" >&2
}

if [ ! -x "$BIN_PATH" ]; then
    record_startup_issue "missing executable: $BIN_PATH"
    exit 1
fi

if [ ! -f "$PROJECT_DIR/config/ctp.ini" ]; then
    record_startup_issue "missing config: $PROJECT_DIR/config/ctp.ini"
    exit 1
fi

if [ ! -f "$CTP_LIB_DIR/thostmduserapi_se.so" ]; then
    record_startup_issue "missing CTP library: $CTP_LIB_DIR/thostmduserapi_se.so"
    exit 1
fi

if [ ! -f "$CTP_DATA_COLLECT_DIR/LinuxDataCollect.so" ]; then
    record_startup_issue "missing CTP data collect library: $CTP_DATA_COLLECT_DIR/LinuxDataCollect.so"
    exit 1
fi

if [ ! -f "$XTP_LIB_DIR/libxtpxquoteapi.so" ]; then
    record_startup_issue "missing XTP library: $XTP_LIB_DIR/libxtpxquoteapi.so"
    exit 1
fi

if [ -f "$PID_FILE" ]; then
    EXISTING_PID="$(cat "$PID_FILE" 2>/dev/null || true)"
    if [ -n "${EXISTING_PID:-}" ] && kill -0 "$EXISTING_PID" 2>/dev/null; then
        echo "basis_monitor already running with pid $EXISTING_PID"
        exit 1
    fi
    rm -f "$PID_FILE"
fi

export LD_LIBRARY_PATH="$CTP_LIB_DIR:$CTP_DATA_COLLECT_DIR:$XTP_LIB_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

cd "$PROJECT_DIR"

nohup "$BIN_PATH" > /dev/null 2>&1 &
PID=$!
echo "$PID" > "$PID_FILE"

sleep 1

if ! kill -0 "$PID" 2>/dev/null; then
    rm -f "$PID_FILE"
    record_startup_issue "basis_monitor failed to stay running, check logs/runtime.log for application logs"
    exit 1
fi

echo "basis_monitor started"
echo "pid: $PID"
echo "runtime log: $PROJECT_DIR/logs/runtime.log"
echo "alert log: $PROJECT_DIR/logs/alert.log"
