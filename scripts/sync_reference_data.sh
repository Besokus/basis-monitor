#!/bin/sh
set -eu

usage() {
    echo "Usage:" >&2
    echo "  $0 <rsync|scp> <source_root> <dest_root>" >&2
    echo "  $0 <rsync|scp> <future_metadata_src> <index_metadata_src> <future_eod_src> <index_eod_src> <dest_root>" >&2
    echo "Examples:" >&2
    echo "  $0 rsync user@host:/data/riceQuantData /data/basis_monitor/reference" >&2
    echo "  $0 rsync user@host:/data/riceQuantData/all_instruments/Future user@host:/data/riceQuantData/all_instruments/INDX user@host:/data/riceQuantData/future_eod_price user@host:/data/riceQuantData/index_eod_price /data/basis_monitor/reference" >&2
    exit 1
}

[ "$#" -eq 3 ] || [ "$#" -eq 6 ] || usage

MODE="$1"

if [ "$#" -eq 3 ]; then
    SOURCE_FUTURE_METADATA_DIR="${2%/}/all_instruments/Future"
    SOURCE_INDEX_METADATA_DIR="${2%/}/all_instruments/INDX"
    SOURCE_FUTURE_EOD_DIR="${2%/}/eod_price/Future"
    SOURCE_INDEX_EOD_DIR="${2%/}/eod_price/INDX"
    DEST_ROOT="$3"
else
    SOURCE_FUTURE_METADATA_DIR="$2"
    SOURCE_INDEX_METADATA_DIR="$3"
    SOURCE_FUTURE_EOD_DIR="$4"
    SOURCE_INDEX_EOD_DIR="$5"
    DEST_ROOT="$6"
fi

DEST_FUTURE_METADATA_DIR="${DEST_ROOT%/}/all_instruments/Future"
DEST_INDEX_METADATA_DIR="${DEST_ROOT%/}/all_instruments/INDX"
DEST_FUTURE_EOD_DIR="${DEST_ROOT%/}/eod_price/Future"
DEST_INDEX_EOD_DIR="${DEST_ROOT%/}/eod_price/INDX"

sync_one() {
    source_path="$1"
    dest_path="$2"

    mkdir -p "$dest_path"

    case "$MODE" in
        rsync)
            rsync -a "$source_path"/ "$dest_path"/
            ;;
        scp)
            scp -p "$source_path"/*.csv "$dest_path"/
            ;;
        *)
            echo "Unsupported sync mode: $MODE" >&2
            exit 1
            ;;
    esac
}

require_column() {
    header="$1"
    column="$2"
    echo "$header" | grep -q "$column" || {
        echo "Missing required column '$column' in header: $header" >&2
        exit 1
    }
}

validate_dir() {
    dir="$1"
    shift

    latest_file="$(find "$dir" -maxdepth 1 -type f -name '*.csv' | sort | tail -n 1)"
    [ -n "${latest_file:-}" ] || {
        echo "No CSV files found in $dir" >&2
        exit 1
    }
    [ -s "$latest_file" ] || {
        echo "Latest CSV is empty: $latest_file" >&2
        exit 1
    }

    header="$(head -n 1 "$latest_file")"
    for required in "$@"; do
        require_column "$header" "$required"
    done

    echo "Validated $latest_file"
}

sync_one "$SOURCE_FUTURE_METADATA_DIR" "$DEST_FUTURE_METADATA_DIR"
sync_one "$SOURCE_INDEX_METADATA_DIR" "$DEST_INDEX_METADATA_DIR"
sync_one "$SOURCE_FUTURE_EOD_DIR" "$DEST_FUTURE_EOD_DIR"
sync_one "$SOURCE_INDEX_EOD_DIR" "$DEST_INDEX_EOD_DIR"

validate_dir "$DEST_FUTURE_METADATA_DIR" order_book_id exchange underlying_order_book_id product maturity_date
validate_dir "$DEST_INDEX_METADATA_DIR" order_book_id symbol
validate_dir "$DEST_FUTURE_EOD_DIR" trade_date underlying_symbol order_book_id close total_turnover
validate_dir "$DEST_INDEX_EOD_DIR" trade_date order_book_id close

echo "Reference data sync complete: $DEST_ROOT"
