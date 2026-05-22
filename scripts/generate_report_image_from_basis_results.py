#!/usr/bin/env python3

import argparse
import csv
import datetime as dt
import json
import pathlib
import subprocess
import sys
import tempfile
from typing import Dict, List


GROUP_ORDER = ["hs300", "zz500", "zz1000"]
COLUMN_NAMES = ["CONTRACT", "PRICE", "CHG", "CHG%", "BASIS", "ANNUAL", "DTE", "WARNING"]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate a basis report PNG from basis_results.csv on your own server.")
    parser.add_argument("--basis-results", required=True, help="Path to pulled basis_results.csv")
    parser.add_argument("--output", required=True, help="Output PNG path")
    parser.add_argument("--moment", required=True, choices=["1130", "1500"], help="Report moment")
    parser.add_argument("--negative-threshold", type=float, default=0.0, help="Alert threshold used for WARNING column")
    parser.add_argument("--trading-date", default="", help="Trading date in YYYY-MM-DD. Defaults to latest date in CSV.")
    parser.add_argument("--market-data-status", default="OK", choices=["OK", "STALE"], help="Status text shown in subtitle")
    parser.add_argument("--render-script", default="", help="Optional explicit path to render_basis_report_image.py")
    return parser.parse_args()


def parse_timestamp(text: str) -> dt.datetime:
    return dt.datetime.fromisoformat(text.replace(" ", "T"))


def title_for_moment(moment: str) -> str:
    if moment == "1130":
        return "BASIS MONITOR 11:30 REPORT"
    return "BASIS MONITOR 15:00 REPORT"


def cutoff_for_moment(trading_date: str, moment: str) -> dt.datetime:
    suffix = "11:30:59" if moment == "1130" else "15:00:59"
    return dt.datetime.fromisoformat(f"{trading_date}T{suffix}")


def report_group_for_product(product_group: str) -> str:
    mapping = {
        "IF": "hs300",
        "IC": "zz500",
        "IM": "zz1000",
    }
    return mapping.get(product_group, product_group.lower())


def format_fixed(value: float, decimals: int) -> str:
    return f"{value:.{decimals}f}"


def format_percent(value: float, decimals: int) -> str:
    return f"{value:.{decimals}f}%"


def load_rows(csv_path: pathlib.Path) -> List[Dict[str, str]]:
    if not csv_path.exists():
        raise FileNotFoundError(f"Missing basis results csv: {csv_path}")
    with csv_path.open("r", encoding="utf-8", newline="") as handle:
        return list(csv.DictReader(handle))


def latest_trading_date(rows: List[Dict[str, str]]) -> str:
    if not rows:
        raise RuntimeError("basis_results.csv is empty")
    return max(row["timestamp"][:10] for row in rows if row.get("timestamp"))


def build_rows(rows: List[Dict[str, str]], trading_date: str, moment: str, negative_threshold: float) -> Dict[str, List[Dict[str, object]]]:
    cutoff = cutoff_for_moment(trading_date, moment)
    latest_by_contract: Dict[str, Dict[str, str]] = {}

    for row in rows:
        timestamp_text = row.get("timestamp", "")
        if not timestamp_text or not timestamp_text.startswith(trading_date):
            continue
        timestamp = parse_timestamp(timestamp_text)
        if timestamp > cutoff:
            continue

        contract = row.get("contract", "")
        if not contract:
            continue
        previous = latest_by_contract.get(contract)
        if previous is None or previous.get("timestamp", "") < timestamp_text:
            latest_by_contract[contract] = row

    grouped_rows: Dict[str, List[Dict[str, object]]] = {group: [] for group in GROUP_ORDER}
    for row in latest_by_contract.values():
        report_group = row.get("report_group") or report_group_for_product(row.get("product_group", ""))
        if report_group not in grouped_rows:
            grouped_rows[report_group] = []

        future_price = float(row["future_price"])
        future_close_yesterday = float(row.get("future_close_yesterday") or 0.0)
        annual_rate = float(row["annual_rate"])
        remaining_days = int(float(row["remaining_days"]))
        change_text = "N/A"
        change_percent_text = "N/A"
        if future_close_yesterday > 0.0:
            change = future_price - future_close_yesterday
            change_percent = (change / future_close_yesterday) * 100.0
            change_text = format_fixed(change, 1)
            change_percent_text = format_percent(change_percent, 2)

        warning_negative = annual_rate < negative_threshold
        grouped_rows[report_group].append(
            {
                "instrument_id": row["contract"],
                "remaining_days": remaining_days,
                "price_text": format_fixed(future_price, 1),
                "change_text": change_text,
                "change_percent_text": change_percent_text,
                "basis_text": format_fixed(float(row["basis"]), 2),
                "annual_rate_text": format_percent(annual_rate, 2),
                "remaining_days_text": str(remaining_days),
                "warning_text": format_percent(annual_rate, 2) if warning_negative else "-",
                "warning_negative": warning_negative,
            }
        )

    for group_name in grouped_rows:
        grouped_rows[group_name].sort(key=lambda item: (item["remaining_days"], item["instrument_id"]))

    return grouped_rows


def render_document(document: Dict[str, object], output_path: pathlib.Path, render_script: pathlib.Path) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".json", delete=False) as temp_file:
        temp_path = pathlib.Path(temp_file.name)
        json.dump(document, temp_file, ensure_ascii=False, separators=(",", ":"))

    try:
        subprocess.run(
            [sys.executable, str(render_script), str(temp_path), str(output_path)],
            check=True,
        )
    finally:
        temp_path.unlink(missing_ok=True)


def main() -> int:
    args = parse_args()
    basis_results_path = pathlib.Path(args.basis_results)
    output_path = pathlib.Path(args.output)
    render_script = pathlib.Path(args.render_script) if args.render_script else pathlib.Path(__file__).with_name("render_basis_report_image.py")

    rows = load_rows(basis_results_path)
    trading_date = args.trading_date or latest_trading_date(rows)
    grouped_rows = build_rows(rows, trading_date, args.moment, args.negative_threshold)

    latest_rows = [row for group_rows in grouped_rows.values() for row in group_rows]
    data_as_of = "N/A"
    if latest_rows:
        data_as_of = max(
            row["timestamp"]
            for row in rows
            if row.get("timestamp", "").startswith(trading_date)
            and parse_timestamp(row["timestamp"]) <= cutoff_for_moment(trading_date, args.moment)
        )

    generated_at = dt.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    document = {
        "title": title_for_moment(args.moment),
        "subtitle": f"DATE {trading_date} | STAT {generated_at} | DATA {data_as_of} | STATUS {args.market_data_status}",
        "columns": COLUMN_NAMES,
        "groups": [{"name": group_name, "rows": grouped_rows.get(group_name, [])} for group_name in GROUP_ORDER],
    }

    render_document(document, output_path, render_script)
    print(f"Generated report image: {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
