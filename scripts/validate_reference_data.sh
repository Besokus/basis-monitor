#!/bin/sh
set -eu

usage() {
    echo "Usage:" >&2
    echo "  $0 <staging_root>" >&2
    echo "  $0 <future_metadata_dir> <index_metadata_dir> <future_eod_dir> <index_eod_dir>" >&2
    exit 1
}

[ "$#" -eq 1 ] || [ "$#" -eq 4 ] || usage

if [ "$#" -eq 1 ]; then
    ROOT="${1%/}"
    FUTURE_METADATA_DIR="$ROOT/all_instruments/Future"
    INDEX_METADATA_DIR="$ROOT/all_instruments/INDX"
    FUTURE_EOD_DIR="$ROOT/eod_price/Future"
    INDEX_EOD_DIR="$ROOT/eod_price/INDX"
else
    FUTURE_METADATA_DIR="$1"
    INDEX_METADATA_DIR="$2"
    FUTURE_EOD_DIR="$3"
    INDEX_EOD_DIR="$4"
fi

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

    [ -d "$dir" ] || {
        echo "Missing directory: $dir" >&2
        exit 1
    }

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

validate_dir "$FUTURE_METADATA_DIR" order_book_id exchange underlying_order_book_id product maturity_date
validate_dir "$INDEX_METADATA_DIR" order_book_id symbol
validate_dir "$FUTURE_EOD_DIR" trade_date underlying_symbol order_book_id close total_turnover
validate_dir "$INDEX_EOD_DIR" trade_date order_book_id close

echo "Reference data validation complete."
