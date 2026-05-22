#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
PID_FILE="${RELAY_PID_FILE:-$PROJECT_DIR/runtime/zhongtai_relay_orchestrator.pid}"

if [ ! -f "$PID_FILE" ]; then
    echo "zhongtai relay orchestrator is not running (pid file missing)"
    exit 0
fi

PID="$(cat "$PID_FILE" 2>/dev/null || true)"
if [ -z "${PID:-}" ]; then
    rm -f "$PID_FILE"
    echo "zhongtai relay orchestrator is not running (empty pid file removed)"
    exit 0
fi

if ! kill -0 "$PID" 2>/dev/null; then
    rm -f "$PID_FILE"
    echo "zhongtai relay orchestrator is not running (stale pid file removed)"
    exit 0
fi

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
echo "zhongtai relay orchestrator stopped"
