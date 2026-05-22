#!/bin/sh
set -eu

usage() {
    echo "Usage: $0 <user@host> <remote_root> [<local_project_dir> <local_xtp_sdk_root>]" >&2
    echo "Usage: $0 <remote_root> [<local_project_dir> <local_xtp_sdk_root>]" >&2
    echo "Example: $0 zhongtai@10.101.5.62 /list/10.101.5.62/basis-monitor-zhongtai" >&2
    echo "Example: $0 /list/10.101.5.62/basis-monitor-zhongtai" >&2
    exit 1
}

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
DEFAULT_PROJECT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
. "$SCRIPT_DIR/sftp_config.sh"
load_sftp_config "$SCRIPT_DIR" "$DEFAULT_PROJECT_DIR"

[ "$#" -eq 1 ] || [ "$#" -eq 2 ] || [ "$#" -eq 3 ] || [ "$#" -eq 4 ] || usage

REMOTE_TARGET=""
REMOTE_ROOT=""

if [ "$#" -eq 1 ] || [ "$#" -eq 3 ]; then
    REMOTE_ROOT="${1%/}"
else
    REMOTE_TARGET="$1"
    REMOTE_ROOT="${2%/}"
fi

REMOTE_TARGET="$(build_sftp_target "$REMOTE_TARGET")"

DEFAULT_XTP_SDK_ROOT="$(CDPATH= cd -- "$DEFAULT_PROJECT_DIR/.." && pwd)/XTPXQuoteAPI_1.0.15_20260113"

if [ "$#" -eq 1 ] || [ "$#" -eq 2 ]; then
    LOCAL_PROJECT_DIR="$DEFAULT_PROJECT_DIR"
    LOCAL_XTP_SDK_ROOT="$DEFAULT_XTP_SDK_ROOT"
else
    if [ "$#" -eq 3 ]; then
        LOCAL_PROJECT_DIR="${2%/}"
        LOCAL_XTP_SDK_ROOT="${3%/}"
    else
        LOCAL_PROJECT_DIR="${3%/}"
        LOCAL_XTP_SDK_ROOT="${4%/}"
    fi
fi

LOCAL_BINARY="$LOCAL_PROJECT_DIR/bin/basis_monitor"
LOCAL_CTP_LIB="$LOCAL_PROJECT_DIR/vendor/ctp/live/lib/linux/thostmduserapi_se.so"
LOCAL_CTP_DATA_COLLECT_LIB="$LOCAL_PROJECT_DIR/vendor/ctp/data_collect/LinuxDataCollect.so"
LOCAL_CTP_DATA_COLLECT_HEADER="$LOCAL_PROJECT_DIR/vendor/ctp/data_collect/DataCollect.h"
LOCAL_XTP_LIB="$LOCAL_XTP_SDK_ROOT/lib/centos/onload-8.1.2.26/libxtpxquoteapi.so"

require_file() {
    path="$1"
    [ -f "$path" ] || {
        echo "Missing file: $path" >&2
        exit 1
    }
}

append_config_put_commands() {
    local_config_dir="$1"
    remote_project_dir="$2"

    find "$local_config_dir" -maxdepth 1 -type f | sort | while IFS= read -r config_path; do
        [ -n "$config_path" ] || continue
        config_name="$(basename "$config_path")"
        printf 'put -p "%s" "%s/config/%s"\n' "$config_path" "$remote_project_dir" "$config_name"
    done
}

require_file "$LOCAL_BINARY"
require_file "$LOCAL_CTP_LIB"
require_file "$LOCAL_CTP_DATA_COLLECT_LIB"
require_file "$LOCAL_CTP_DATA_COLLECT_HEADER"
require_file "$LOCAL_XTP_LIB"
require_file "$LOCAL_PROJECT_DIR/config/ctp.ini"
require_file "$LOCAL_PROJECT_DIR/config/alert.json"
require_file "$LOCAL_PROJECT_DIR/start_prebuilt.sh"
require_file "$LOCAL_PROJECT_DIR/stop_prebuilt.sh"

BATCH_FILE="$(mktemp)"
cleanup() {
    rm -f "$BATCH_FILE"
}
trap cleanup EXIT INT TERM

REMOTE_PROJECT_DIR="$REMOTE_ROOT/basis_monitor"
REMOTE_XTP_LIB_DIR="$REMOTE_ROOT/XTPXQuoteAPI_1.0.15_20260113/lib/centos/onload-8.1.2.26"

{
    printf -- "-mkdir \"%s\"\n" "$REMOTE_ROOT"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_PROJECT_DIR"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_PROJECT_DIR/bin"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_PROJECT_DIR/config"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_PROJECT_DIR/scripts"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_PROJECT_DIR/vendor"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_PROJECT_DIR/vendor/ctp"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_PROJECT_DIR/vendor/ctp/live"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_PROJECT_DIR/vendor/ctp/live/lib"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_PROJECT_DIR/vendor/ctp/live/lib/linux"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_PROJECT_DIR/vendor/ctp/data_collect"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_ROOT/XTPXQuoteAPI_1.0.15_20260113"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_ROOT/XTPXQuoteAPI_1.0.15_20260113/lib"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_ROOT/XTPXQuoteAPI_1.0.15_20260113/lib/centos"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_ROOT/XTPXQuoteAPI_1.0.15_20260113/lib/centos/onload-8.1.2.26"

    printf 'put -p "%s" "%s/bin/basis_monitor"\n' "$LOCAL_BINARY" "$REMOTE_PROJECT_DIR"
    append_config_put_commands "$LOCAL_PROJECT_DIR/config" "$REMOTE_PROJECT_DIR"
    printf 'put -p "%s" "%s/start_prebuilt.sh"\n' "$LOCAL_PROJECT_DIR/start_prebuilt.sh" "$REMOTE_PROJECT_DIR"
    printf 'put -p "%s" "%s/stop_prebuilt.sh"\n' "$LOCAL_PROJECT_DIR/stop_prebuilt.sh" "$REMOTE_PROJECT_DIR"
    printf 'put -p "%s" "%s/vendor/ctp/live/lib/linux/thostmduserapi_se.so"\n' "$LOCAL_CTP_LIB" "$REMOTE_PROJECT_DIR"
    printf 'put -p "%s" "%s/vendor/ctp/data_collect/LinuxDataCollect.so"\n' "$LOCAL_CTP_DATA_COLLECT_LIB" "$REMOTE_PROJECT_DIR"
    printf 'put -p "%s" "%s/vendor/ctp/data_collect/DataCollect.h"\n' "$LOCAL_CTP_DATA_COLLECT_HEADER" "$REMOTE_PROJECT_DIR"
    printf 'put -p "%s" "%s/libxtpxquoteapi.so"\n' "$LOCAL_XTP_LIB" "$REMOTE_XTP_LIB_DIR"
} > "$BATCH_FILE"

if [ -n "$SFTP_IDENTITY_FILE" ]; then
    echo "Uploading runtime through SFTP: target=$REMOTE_TARGET port=$SFTP_PORT identity=$SFTP_IDENTITY_FILE"
    sftp -P "$SFTP_PORT" -i "$SFTP_IDENTITY_FILE" -b "$BATCH_FILE" "$REMOTE_TARGET"
else
    echo "Uploading runtime through SFTP: target=$REMOTE_TARGET port=$SFTP_PORT"
    sftp -P "$SFTP_PORT" -b "$BATCH_FILE" "$REMOTE_TARGET"
fi

echo "Prebuilt runtime push complete."
echo "Remote project dir: $REMOTE_PROJECT_DIR"
