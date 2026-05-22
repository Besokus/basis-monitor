#!/bin/sh
set -eu

usage() {
    echo "Usage: $0 <user@host> <remote_root>" >&2
    echo "Example: $0 zhongtai@10.101.5.62 /list/10.101.5.62/basis-monitor-zhongtai" >&2
    exit 1
}

[ "$#" -eq 2 ] || usage

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"

REMOTE_TARGET="$1"
REMOTE_ROOT="$2"
REMOTE_STAGING_ROOT="${REMOTE_ROOT%/}/data/staging"

cd "$PROJECT_DIR"

sh run.sh
sh scripts/push_reference_data_to_zhongtai.sh "$REMOTE_TARGET" "$REMOTE_STAGING_ROOT"
sh scripts/push_prebuilt_runtime_to_zhongtai.sh "$REMOTE_TARGET" "$REMOTE_ROOT"

echo "Prepared Zhongtai runtime at $REMOTE_ROOT"
