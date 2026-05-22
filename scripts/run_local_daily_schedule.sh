#!/bin/sh
set -eu

usage() {
    echo "Usage: $0 start|stop" >&2
    exit 1
}

[ "$#" -eq 1 ] || usage

ACTION="$1"
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"

REMOTE_STAGING_ROOT="${LOCAL_SCHEDULE_REMOTE_STAGING_ROOT:-/list/10.101.5.62/basis-monitor-zhongtai/basis_monitor/data/staging}"

start_jobs() {
    echo "[LOCAL_SCHEDULE] pushing reference CSV to Zhongtai staging..."
    sh "$SCRIPT_DIR/push_reference_data_to_zhongtai.sh" "$REMOTE_STAGING_ROOT"

    echo "[LOCAL_SCHEDULE] starting local Zhongtai relay orchestrator..."
    sh "$SCRIPT_DIR/stop_zhongtai_relay_orchestrator.sh" >/dev/null 2>&1 || true
    sh "$SCRIPT_DIR/start_zhongtai_relay_orchestrator.sh"
}

stop_jobs() {
    echo "[LOCAL_SCHEDULE] stopping local Zhongtai relay orchestrator..."
    sh "$SCRIPT_DIR/stop_zhongtai_relay_orchestrator.sh"
}

case "$ACTION" in
    start)
        start_jobs
        ;;
    stop)
        stop_jobs
        ;;
    *)
        usage
        ;;
esac
