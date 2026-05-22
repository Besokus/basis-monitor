#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
PID_FILE="$PROJECT_DIR/runtime/basis_monitor.pid"

if [ ! -f "$PID_FILE" ]; then
    echo "basis_monitor is not running (pid file missing)"
    exit 0
fi

PID="$(cat "$PID_FILE" 2>/dev/null || true)"
if [ -z "${PID:-}" ]; then
    rm -f "$PID_FILE"
    echo "basis_monitor is not running (empty pid file removed)"
    exit 0
fi

if [ ! -d "/proc/$PID" ]; then
    rm -f "$PID_FILE"
    echo "basis_monitor is not running (stale pid file removed)"
    exit 0
fi

CMDLINE="$(tr '\0' ' ' < "/proc/$PID/cmdline" 2>/dev/null || true)"
case "$CMDLINE" in
    *basis_monitor*)
        ;;
    *)
        echo "pid $PID does not look like basis_monitor, refusing to stop"
        exit 1
        ;;
esac

kill "$PID"

COUNT=0
while kill -0 "$PID" 2>/dev/null; do
    COUNT=$((COUNT + 1))
    if [ "$COUNT" -ge 10 ]; then
        kill -9 "$PID" 2>/dev/null || true
        break
    fi
    sleep 1
done

rm -f "$PID_FILE"
echo "basis_monitor stopped"
