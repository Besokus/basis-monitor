#!/bin/sh
set -eu

usage() {
    echo "Usage:" >&2
    echo "  $0 <user@host> <remote_staging_root>" >&2
    echo "  $0 <remote_staging_root>" >&2
    echo "  $0 <user@host> <remote_staging_root> <future_metadata_src> <index_metadata_src> <future_eod_src> <index_eod_src>" >&2
    echo "  $0 <remote_staging_root> <future_metadata_src> <index_metadata_src> <future_eod_src> <index_eod_src>" >&2
    echo "Examples:" >&2
    echo "  $0 zhongtai@10.101.5.62 /list/10.101.5.62/basis-monitor-zhongtai/data/staging" >&2
    echo "  $0 /list/10.101.5.62/basis-monitor-zhongtai/data/staging" >&2
    echo "  $0 zhongtai@10.101.5.62 /list/10.101.5.62/basis-monitor-zhongtai/data/staging /data/disk1/share_data/riceQuantData/all_instruments/Future /data/disk1/share_data/riceQuantData/all_instruments/INDX /data/disk1/share_data/riceQuantData/future_eod_price /data/disk1/share_data/riceQuantData/index_eod_price" >&2
    exit 1
}

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
PROJECT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
. "$SCRIPT_DIR/sftp_config.sh"
load_sftp_config "$SCRIPT_DIR" "$PROJECT_DIR"

[ "$#" -eq 1 ] || [ "$#" -eq 2 ] || [ "$#" -eq 5 ] || [ "$#" -eq 6 ] || usage

REMOTE_TARGET=""
REMOTE_STAGING_ROOT=""

if [ "$#" -eq 1 ] || [ "$#" -eq 5 ]; then
    REMOTE_STAGING_ROOT="${1%/}"
else
    REMOTE_TARGET="$1"
    REMOTE_STAGING_ROOT="${2%/}"
fi

REMOTE_TARGET="$(build_sftp_target "$REMOTE_TARGET")"

if [ "$#" -eq 1 ] || [ "$#" -eq 2 ]; then
    SOURCE_FUTURE_METADATA_DIR="/data/disk1/share_data/riceQuantData/all_instruments/Future"
    SOURCE_INDEX_METADATA_DIR="/data/disk1/share_data/riceQuantData/all_instruments/INDX"
    SOURCE_FUTURE_EOD_DIR="/data/disk1/share_data/riceQuantData/future_eod_price"
    SOURCE_INDEX_EOD_DIR="/data/disk1/share_data/riceQuantData/index_eod_price"
else
    if [ "$#" -eq 5 ]; then
        SOURCE_FUTURE_METADATA_DIR="$2"
        SOURCE_INDEX_METADATA_DIR="$3"
        SOURCE_FUTURE_EOD_DIR="$4"
        SOURCE_INDEX_EOD_DIR="$5"
    else
        SOURCE_FUTURE_METADATA_DIR="$3"
        SOURCE_INDEX_METADATA_DIR="$4"
        SOURCE_FUTURE_EOD_DIR="$5"
        SOURCE_INDEX_EOD_DIR="$6"
    fi
fi

VALIDATE_SCRIPT="$SCRIPT_DIR/validate_reference_data.sh"
DEFAULT_BUILD_DIR="$PROJECT_DIR/build"

REMOTE_FUTURE_METADATA_DIR="$REMOTE_STAGING_ROOT/all_instruments/Future"
REMOTE_INDEX_METADATA_DIR="$REMOTE_STAGING_ROOT/all_instruments/INDX"
REMOTE_FUTURE_EOD_DIR="$REMOTE_STAGING_ROOT/eod_price/Future"
REMOTE_INDEX_EOD_DIR="$REMOTE_STAGING_ROOT/eod_price/INDX"
SFTP_PORT="${SFTP_PORT:-22}"
SFTP_IDENTITY_FILE="${SFTP_IDENTITY_FILE:-}"

require_local_dir() {
    dir="$1"
    [ -d "$dir" ] || {
        echo "Missing local source directory: $dir" >&2
        exit 1
    }
}

select_latest_csv() {
    dir="$1"
    latest_file="$(find "$dir" -maxdepth 1 -type f -name '*.csv' | sort | tail -n 1)"
    [ -n "${latest_file:-}" ] || {
        echo "No CSV files found in $dir" >&2
        exit 1
    }
    printf '%s\n' "$latest_file"
}

copy_latest_csv_to_dir() {
    source_dir="$1"
    target_dir="$2"
    label="$3"

    mkdir -p "$target_dir"
    latest_file="$(select_latest_csv "$source_dir")"
    echo "[REFERENCE_INPUT] $label=$latest_file"
    cp "$latest_file" "$target_dir/"
}

find_prepare_tool() {
    if [ -n "${BASIS_MONITOR_PREPARE_TOOL:-}" ]; then
        [ -x "$BASIS_MONITOR_PREPARE_TOOL" ] || {
            echo "Configured BASIS_MONITOR_PREPARE_TOOL is not executable: $BASIS_MONITOR_PREPARE_TOOL" >&2
            exit 1
        }
        echo "$BASIS_MONITOR_PREPARE_TOOL"
        return
    fi

    for candidate in \
        "$PROJECT_DIR/build/prepare_reference_subset" \
        "$PROJECT_DIR/bin/prepare_reference_subset" \
        "$HOME/.cache/basis_monitor/build/prepare_reference_subset"
    do
        if [ -f "$candidate" ] && [ ! -x "$candidate" ]; then
            chmod +x "$candidate" 2>/dev/null || true
        fi
        if [ -x "$candidate" ]; then
            echo "$candidate"
            return
        fi
    done

    mkdir -p "$DEFAULT_BUILD_DIR"
    CMAKE_BIN="${CMAKE_BIN:-cmake}"
    JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"
    "$CMAKE_BIN" -S "$PROJECT_DIR" -B "$DEFAULT_BUILD_DIR" >&2
    "$CMAKE_BIN" --build "$DEFAULT_BUILD_DIR" --target prepare_reference_subset -j"$JOBS" >&2

    [ -x "$DEFAULT_BUILD_DIR/prepare_reference_subset" ] || {
        echo "Failed to build prepare_reference_subset" >&2
        exit 1
    }
    echo "$DEFAULT_BUILD_DIR/prepare_reference_subset"
}

build_prepare_tool_in_default_dir() {
    mkdir -p "$DEFAULT_BUILD_DIR"
    rm -f "$DEFAULT_BUILD_DIR/prepare_reference_subset"
    CMAKE_BIN="${CMAKE_BIN:-cmake}"
    JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"

    cmake_log="$(mktemp)"
    if ! "$CMAKE_BIN" -S "$PROJECT_DIR" -B "$DEFAULT_BUILD_DIR" >"$cmake_log" 2>&1; then
        cat "$cmake_log" >&2
        if grep -Eq 'CMakeCache.txt directory .* is different|does not match the source' "$cmake_log"; then
            echo "Detected stale CMake cache under $DEFAULT_BUILD_DIR, cleaning cache files and retrying configure..." >&2
            rm -f "$DEFAULT_BUILD_DIR/CMakeCache.txt"
            rm -rf "$DEFAULT_BUILD_DIR/CMakeFiles"
            "$CMAKE_BIN" -S "$PROJECT_DIR" -B "$DEFAULT_BUILD_DIR" >&2
        else
            rm -f "$cmake_log"
            return 1
        fi
    fi
    rm -f "$cmake_log"

    "$CMAKE_BIN" --build "$DEFAULT_BUILD_DIR" --target prepare_reference_subset -j"$JOBS" >&2

    [ -x "$DEFAULT_BUILD_DIR/prepare_reference_subset" ] || {
        echo "Failed to build prepare_reference_subset" >&2
        exit 1
    }
}

run_prepare_tool_with_retry() {
    tool="$1"
    shift

    prepare_log="$(mktemp)"
    if "$tool" "$@" 2>"$prepare_log"; then
        rm -f "$prepare_log"
        return 0
    fi

    rc=$?
    cat "$prepare_log" >&2

    if grep -Eq 'GLIBCXX_[0-9.]|GLIBC_[0-9.]' "$prepare_log"; then
        echo "Detected incompatible runtime libraries for $tool, rebuilding prepare_reference_subset under $DEFAULT_BUILD_DIR..." >&2
        build_prepare_tool_in_default_dir
        tool="$DEFAULT_BUILD_DIR/prepare_reference_subset"
        rm -f "$prepare_log"
        "$tool" "$@"
        return $?
    fi

    rm -f "$prepare_log"
    return $rc
}

append_put_commands() {
    local_dir="$1"
    remote_dir="$2"
    batch_file="$3"

    csv_list="$(find "$local_dir" -maxdepth 1 -type f -name '*.csv' | sort)"
    [ -n "${csv_list:-}" ] || {
        echo "No CSV files found in local source directory: $local_dir" >&2
        exit 1
    }

    printf '%s\n' "$csv_list" | while IFS= read -r csv_path; do
        [ -n "$csv_path" ] || continue
        printf 'put -p "%s" "%s/"\n' "$csv_path" "$remote_dir" >> "$batch_file"
    done
}

append_remote_cleanup_commands() {
    remote_dir="$1"
    batch_file="$2"

    {
        printf -- "-cd \"%s\"\n" "$remote_dir"
        printf -- "-rm *.csv\n"
    } >> "$batch_file"
}

run_sftp_batch() {
    if [ -n "$SFTP_IDENTITY_FILE" ]; then
        echo "Uploading through SFTP: target=$REMOTE_TARGET port=$SFTP_PORT identity=$SFTP_IDENTITY_FILE"
        sftp -P "$SFTP_PORT" -i "$SFTP_IDENTITY_FILE" -b "$BATCH_FILE" "$REMOTE_TARGET"
    else
        echo "Uploading through SFTP: target=$REMOTE_TARGET port=$SFTP_PORT"
        sftp -P "$SFTP_PORT" -b "$BATCH_FILE" "$REMOTE_TARGET"
    fi
}

require_local_dir "$SOURCE_FUTURE_METADATA_DIR"
require_local_dir "$SOURCE_INDEX_METADATA_DIR"
require_local_dir "$SOURCE_FUTURE_EOD_DIR"
require_local_dir "$SOURCE_INDEX_EOD_DIR"

PREPARE_TOOL="$(find_prepare_tool)"
PREPARE_INPUT_ROOT="$(mktemp -d)"
FILTERED_ROOT="$(mktemp -d)"
BATCH_FILE="$(mktemp)"
cleanup() {
    rm -rf "$PREPARE_INPUT_ROOT"
    rm -rf "$FILTERED_ROOT"
    rm -f "$BATCH_FILE"
}
trap cleanup EXIT INT TERM

PREPARE_FUTURE_METADATA_DIR="$PREPARE_INPUT_ROOT/all_instruments/Future"
PREPARE_INDEX_METADATA_DIR="$PREPARE_INPUT_ROOT/all_instruments/INDX"
PREPARE_FUTURE_EOD_DIR="$PREPARE_INPUT_ROOT/eod_price/Future"
PREPARE_INDEX_EOD_DIR="$PREPARE_INPUT_ROOT/eod_price/INDX"

copy_latest_csv_to_dir "$SOURCE_FUTURE_METADATA_DIR" "$PREPARE_FUTURE_METADATA_DIR" "future_metadata"
copy_latest_csv_to_dir "$SOURCE_INDEX_METADATA_DIR" "$PREPARE_INDEX_METADATA_DIR" "index_metadata"
copy_latest_csv_to_dir "$SOURCE_FUTURE_EOD_DIR" "$PREPARE_FUTURE_EOD_DIR" "future_eod"
copy_latest_csv_to_dir "$SOURCE_INDEX_EOD_DIR" "$PREPARE_INDEX_EOD_DIR" "index_eod"

run_prepare_tool_with_retry "$PREPARE_TOOL" \
    "$PREPARE_FUTURE_METADATA_DIR" \
    "$PREPARE_INDEX_METADATA_DIR" \
    "$PREPARE_FUTURE_EOD_DIR" \
    "$PREPARE_INDEX_EOD_DIR" \
    "$FILTERED_ROOT"

FILTERED_FUTURE_METADATA_DIR="$FILTERED_ROOT/all_instruments/Future"
FILTERED_INDEX_METADATA_DIR="$FILTERED_ROOT/all_instruments/INDX"
FILTERED_FUTURE_EOD_DIR="$FILTERED_ROOT/eod_price/Future"
FILTERED_INDEX_EOD_DIR="$FILTERED_ROOT/eod_price/INDX"

{
    printf -- "-mkdir \"%s\"\n" "$REMOTE_STAGING_ROOT"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_STAGING_ROOT/all_instruments"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_STAGING_ROOT/eod_price"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_FUTURE_METADATA_DIR"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_INDEX_METADATA_DIR"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_FUTURE_EOD_DIR"
    printf -- "-mkdir \"%s\"\n" "$REMOTE_INDEX_EOD_DIR"
} > "$BATCH_FILE"

append_remote_cleanup_commands "$REMOTE_FUTURE_METADATA_DIR" "$BATCH_FILE"
append_remote_cleanup_commands "$REMOTE_INDEX_METADATA_DIR" "$BATCH_FILE"
append_remote_cleanup_commands "$REMOTE_FUTURE_EOD_DIR" "$BATCH_FILE"
append_remote_cleanup_commands "$REMOTE_INDEX_EOD_DIR" "$BATCH_FILE"

append_put_commands "$FILTERED_FUTURE_METADATA_DIR" "$REMOTE_FUTURE_METADATA_DIR" "$BATCH_FILE"
append_put_commands "$FILTERED_INDEX_METADATA_DIR" "$REMOTE_INDEX_METADATA_DIR" "$BATCH_FILE"
append_put_commands "$FILTERED_FUTURE_EOD_DIR" "$REMOTE_FUTURE_EOD_DIR" "$BATCH_FILE"
append_put_commands "$FILTERED_INDEX_EOD_DIR" "$REMOTE_INDEX_EOD_DIR" "$BATCH_FILE"

run_sftp_batch

if [ -x "$VALIDATE_SCRIPT" ]; then
    echo "Filtered staging validation:"
    sh "$VALIDATE_SCRIPT" \
        "$FILTERED_FUTURE_METADATA_DIR" \
        "$FILTERED_INDEX_METADATA_DIR" \
        "$FILTERED_FUTURE_EOD_DIR" \
        "$FILTERED_INDEX_EOD_DIR"
fi

echo "Reference CSV push complete."
echo "Local filtered staging was prepared and uploaded via temporary directory."
echo "Remote staging root: $REMOTE_STAGING_ROOT"
