#!/bin/sh
set -eu

timestamp() {
    date '+%Y-%m-%d %H:%M:%S'
}

log() {
    printf '[%s] %s\n' "$(timestamp)" "$*"
}

usage() {
    echo "Usage: $0 <user@host> <remote_project_dir> <local_output_root>" >&2
    echo "Usage: $0 <remote_project_dir> <local_output_root>" >&2
    echo "Example: $0 zhongtai@10.101.5.62 /list/10.101.5.62/basis-monitor-zhongtai/basis_monitor ./relay_spool" >&2
    echo "Example: $0 /list/10.101.5.62/basis-monitor-zhongtai/basis_monitor ./relay_spool" >&2
    exit 1
}

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
. "$SCRIPT_DIR/sftp_config.sh"
load_sftp_config "$SCRIPT_DIR" "$PROJECT_DIR"

[ "$#" -eq 2 ] || [ "$#" -eq 3 ] || usage

if [ "$#" -eq 2 ]; then
    REMOTE_TARGET="$(build_sftp_target "")"
    REMOTE_PROJECT_DIR="${1%/}"
    LOCAL_OUTPUT_ROOT="${2%/}"
else
    REMOTE_TARGET="$(build_sftp_target "$1")"
    REMOTE_PROJECT_DIR="${2%/}"
    LOCAL_OUTPUT_ROOT="${3%/}"
fi

LOCAL_REPORT_DIR="$LOCAL_OUTPUT_ROOT/reports"
LOCAL_LOG_DIR="$LOCAL_OUTPUT_ROOT/logs"
PULL_REMOTE_REPORT_IMAGES="${PULL_REMOTE_REPORT_IMAGES:-0}"
PULL_REPORT_TEXTS="${PULL_REPORT_TEXTS:-1}"

mkdir -p "$LOCAL_OUTPUT_ROOT" "$LOCAL_REPORT_DIR" "$LOCAL_LOG_DIR"

BATCH_FILE="$(mktemp)"
cleanup() {
    rm -f "$BATCH_FILE"
}
trap cleanup EXIT INT TERM

{
    printf 'lcd "%s"\n' "$LOCAL_OUTPUT_ROOT"
    printf -- '-get -p "%s/data/output/alert_events.csv" "alert_events.csv"\n' "$REMOTE_PROJECT_DIR"
    printf -- '-get -p "%s/data/output/basis_results.csv" "basis_results.csv"\n' "$REMOTE_PROJECT_DIR"
    printf 'lcd "%s"\n' "$LOCAL_LOG_DIR"
    printf -- '-get -p "%s/logs/runtime.log" "runtime.log"\n' "$REMOTE_PROJECT_DIR"
    printf -- '-get -p "%s/logs/alert.log" "alert.log"\n' "$REMOTE_PROJECT_DIR"
    if [ "$PULL_REPORT_TEXTS" = "1" ] || [ "$PULL_REMOTE_REPORT_IMAGES" = "1" ]; then
        printf 'lcd "%s"\n' "$LOCAL_REPORT_DIR"
        printf 'cd "%s/data/output/reports"\n' "$REMOTE_PROJECT_DIR"
    fi
    if [ "$PULL_REPORT_TEXTS" = "1" ]; then
        printf -- 'mget -p *.txt\n'
    fi
    if [ "$PULL_REMOTE_REPORT_IMAGES" = "1" ]; then
        printf -- '-mget -p *.png\n'
    fi
} > "$BATCH_FILE"

if [ -n "$SFTP_IDENTITY_FILE" ]; then
    log "Downloading through SFTP: target=$REMOTE_TARGET port=$SFTP_PORT identity=$SFTP_IDENTITY_FILE"
    log "SFTP batch start: remote_project_dir=$REMOTE_PROJECT_DIR local_output_root=$LOCAL_OUTPUT_ROOT"
    sftp -P "$SFTP_PORT" -i "$SFTP_IDENTITY_FILE" -b "$BATCH_FILE" "$REMOTE_TARGET"
else
    log "Downloading through SFTP: target=$REMOTE_TARGET port=$SFTP_PORT"
    log "SFTP batch start: remote_project_dir=$REMOTE_PROJECT_DIR local_output_root=$LOCAL_OUTPUT_ROOT"
    sftp -P "$SFTP_PORT" -b "$BATCH_FILE" "$REMOTE_TARGET"
fi

log "Pulled Zhongtai outputs into $LOCAL_OUTPUT_ROOT"
