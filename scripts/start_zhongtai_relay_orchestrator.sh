#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"

. "$SCRIPT_DIR/sftp_config.sh"
load_sftp_config "$SCRIPT_DIR" "$PROJECT_DIR"

PYTHON_BIN="${PYTHON_BIN:-python3}"
REMOTE_TARGET="${RELAY_REMOTE_TARGET:-$(build_sftp_target "")}"
REMOTE_PROJECT_DIR="${RELAY_REMOTE_PROJECT_DIR:-/list/10.101.5.62/basis-monitor-zhongtai/basis_monitor}"
CONFIG_ROOT="${RELAY_CONFIG_ROOT:-$PROJECT_DIR/config}"
SPOOL_DIR="${RELAY_SPOOL_DIR:-$PROJECT_DIR/relay_spool}"
STATE_FILE="${RELAY_STATE_FILE:-$PROJECT_DIR/relay_state.json}"
PID_FILE="${RELAY_PID_FILE:-$PROJECT_DIR/runtime/zhongtai_relay_orchestrator.pid}"
LOG_FILE="${RELAY_LOG_FILE:-$PROJECT_DIR/logs/zhongtai_relay_orchestrator.log}"

mkdir -p "$PROJECT_DIR/runtime" "$PROJECT_DIR/logs" "$SPOOL_DIR"

if ! command -v "$PYTHON_BIN" >/dev/null 2>&1; then
    echo "Missing Python interpreter: $PYTHON_BIN" >&2
    exit 1
fi

if [ ! -f "$CONFIG_ROOT/ctp.ini" ]; then
    echo "Missing config file: $CONFIG_ROOT/ctp.ini" >&2
    exit 1
fi

if [ ! -f "$CONFIG_ROOT/alert.json" ]; then
    echo "Missing config file: $CONFIG_ROOT/alert.json" >&2
    exit 1
fi

if [ -f "$PID_FILE" ]; then
    EXISTING_PID="$(cat "$PID_FILE" 2>/dev/null || true)"
    if [ -n "${EXISTING_PID:-}" ] && kill -0 "$EXISTING_PID" 2>/dev/null; then
        echo "zhongtai relay orchestrator already running with pid $EXISTING_PID" >&2
        exit 1
    fi
    rm -f "$PID_FILE"
fi

cd "$PROJECT_DIR"

nohup "$PYTHON_BIN" "$SCRIPT_DIR/run_zhongtai_relay_orchestrator.py" \
    --remote-target "$REMOTE_TARGET" \
    --remote-project-dir "$REMOTE_PROJECT_DIR" \
    --config-root "$CONFIG_ROOT" \
    --spool-dir "$SPOOL_DIR" \
    --state-file "$STATE_FILE" \
    >> "$LOG_FILE" 2>&1 &

PID=$!
echo "$PID" > "$PID_FILE"

sleep 1

if ! kill -0 "$PID" 2>/dev/null; then
    rm -f "$PID_FILE"
    echo "zhongtai relay orchestrator failed to stay running, check $LOG_FILE" >&2
    exit 1
fi

echo "zhongtai relay orchestrator started"
echo "pid: $PID"
echo "log: $LOG_FILE"
echo "state: $STATE_FILE"
echo "spool: $SPOOL_DIR"
