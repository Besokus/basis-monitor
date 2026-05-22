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
CLEAN_ON_STOP="${REMOTE_SCHEDULE_CLEAN_ON_STOP:-false}"

ensure_dir() {
    dir="$1"
    mkdir -p "$dir"
}

clear_files_under_dir() {
    dir="$1"
    ensure_dir "$dir"
    find "$dir" -mindepth 1 -type f -exec rm -f -- {} +
}

start_jobs() {
    echo "[REMOTE_SCHEDULE] starting Zhongtai basis_monitor..."
    sh "$PROJECT_DIR/start_prebuilt.sh"
}

stop_jobs() {
    echo "[REMOTE_SCHEDULE] stopping Zhongtai basis_monitor..."
    sh "$PROJECT_DIR/stop_prebuilt.sh"

    case "$CLEAN_ON_STOP" in
        1|true|TRUE|yes|YES|on|ON)
            echo "[REMOTE_SCHEDULE] clearing runtime output files while keeping directories..."
            clear_files_under_dir "$PROJECT_DIR/logs"
            clear_files_under_dir "$PROJECT_DIR/data/output"
            ;;
        *)
            echo "[REMOTE_SCHEDULE] skip cleanup on stop (REMOTE_SCHEDULE_CLEAN_ON_STOP=$CLEAN_ON_STOP)"
            ;;
    esac
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
