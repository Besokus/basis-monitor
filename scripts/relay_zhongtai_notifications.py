#!/usr/bin/env python3

import argparse
import csv
import json
import os
import pathlib
import re
import sys
import time
import urllib.error
import urllib.request
import configparser
import datetime as dt
from typing import Dict, List, Optional


def log(message: str, *, err: bool = False) -> None:
    timestamp = dt.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    stream = sys.stderr if err else sys.stdout
    print(f"[{timestamp}] {message}", file=stream)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Relay Zhongtai alert events and text reports through WeCom from your own server."
    )
    parser.add_argument("--input-root", required=True, help="Local directory that stores pulled Zhongtai outputs.")
    parser.add_argument("--state-file", required=True, help="JSON state file used for incremental relay.")
    parser.add_argument("--webhook", default="", help="WeCom robot webhook. Falls back to WECOM_ROBOT_WEBHOOK env if omitted.")
    parser.add_argument("--ctp-ini", default="", help="Optional ctp.ini path. If provided, WeComRobotWebhook is read from it when --webhook is omitted.")
    parser.add_argument("--skip-alerts", action="store_true", help="Skip relaying alert_events.csv.")
    parser.add_argument("--skip-reports", action="store_true", help="Skip relaying report text files.")
    return parser.parse_args()


def load_wecom_webhook_from_ctp_ini(ctp_ini_path: pathlib.Path) -> str:
    if not ctp_ini_path.exists():
        return ""

    parser = configparser.ConfigParser()
    parser.read(ctp_ini_path, encoding="utf-8")
    if not parser.has_section("config"):
        return ""
    return parser.get("config", "WeComRobotWebhook", fallback="").strip()


def load_state(path: pathlib.Path) -> Dict[str, object]:
    default_state = {
        "alert_trade_date": "",
        "report_trade_date": "",
        "sent_alert_keys": [],
        "sent_reports": [],
        "gave_up_reports": [],
        "report_receipts": {},
        "send_attempts": {},
    }
    if not path.exists():
        return default_state
    try:
        raw_state = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return default_state

    if not isinstance(raw_state, dict):
        return default_state

    state = dict(default_state)
    state["alert_trade_date"] = str(raw_state.get("alert_trade_date", "") or "")
    state["report_trade_date"] = str(raw_state.get("report_trade_date", "") or "")

    sent_alert_keys = raw_state.get("sent_alert_keys", [])
    if isinstance(sent_alert_keys, list):
        state["sent_alert_keys"] = [str(item) for item in sent_alert_keys if item]

    sent_reports = raw_state.get("sent_reports", [])
    if isinstance(sent_reports, list):
        state["sent_reports"] = [str(item) for item in sent_reports if item]

    gave_up_reports = raw_state.get("gave_up_reports", [])
    if isinstance(gave_up_reports, list):
        state["gave_up_reports"] = [str(item) for item in gave_up_reports if item]

    report_receipts = raw_state.get("report_receipts", {})
    if isinstance(report_receipts, dict):
        normalized_receipts: Dict[str, Dict[str, str]] = {}
        for report_name, receipt in report_receipts.items():
            if not report_name:
                continue
            if isinstance(receipt, dict):
                normalized_receipts[str(report_name)] = {
                    str(key): str(value) for key, value in receipt.items() if key
                }
            elif receipt:
                normalized_receipts[str(report_name)] = {"received_at": str(receipt)}
        state["report_receipts"] = normalized_receipts

    send_attempts = raw_state.get("send_attempts", {})
    if isinstance(send_attempts, dict):
        normalized_attempts: Dict[str, int] = {}
        for report_name, count in send_attempts.items():
            if not report_name:
                continue
            try:
                normalized_attempts[str(report_name)] = int(count)
            except (TypeError, ValueError):
                normalized_attempts[str(report_name)] = 0
        state["send_attempts"] = normalized_attempts

    return state


def save_state(path: pathlib.Path, state: Dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(state, ensure_ascii=False, indent=2, sort_keys=True), encoding="utf-8")


def load_alert_rows(csv_path: pathlib.Path) -> List[Dict[str, str]]:
    if not csv_path.exists():
        return []
    with csv_path.open("r", encoding="utf-8", newline="") as handle:
        return list(csv.DictReader(handle))


def alert_trade_date(row: Dict[str, str]) -> str:
    timestamp = (row.get("timestamp") or "").strip()
    if len(timestamp) >= 10:
        return timestamp[:10]
    return ""


def alert_key(row: Dict[str, str]) -> str:
    return "|".join(
        [
            (row.get("timestamp") or "").strip(),
            (row.get("contract") or "").strip(),
            (row.get("index_code") or "").strip(),
            (row.get("transition") or "").strip(),
            (row.get("annual_rate") or "").strip(),
        ]
    )


def reset_daily_alert_state_if_needed(rows: List[Dict[str, str]], state: Dict[str, object]) -> None:
    current_trade_date = latest_alert_trade_date(rows)
    if not current_trade_date:
        return

    if state.get("alert_trade_date") != current_trade_date:
        state["alert_trade_date"] = current_trade_date
        state["sent_alert_keys"] = []


def latest_alert_trade_date(rows: List[Dict[str, str]]) -> str:
    row_trade_dates = [trade_date for trade_date in (alert_trade_date(row) for row in rows) if trade_date]
    if not row_trade_dates:
        return ""
    return max(row_trade_dates)


def report_trade_date(report_path: pathlib.Path) -> str:
    name = report_path.name
    if len(name) >= 10:
        return name[:10]
    return ""


def current_trade_date() -> str:
    return dt.datetime.now().strftime("%Y-%m-%d")


def reset_daily_report_state_if_needed(report_paths: List[pathlib.Path], state: Dict[str, object]) -> None:
    trade_dates = [trade_date for trade_date in (report_trade_date(path) for path in report_paths) if trade_date]
    if not trade_dates:
        return

    current_trade_date = max(trade_dates)
    if state.get("report_trade_date") != current_trade_date:
        state["report_trade_date"] = current_trade_date
        state["sent_reports"] = []
        state["gave_up_reports"] = []
        state["report_receipts"] = {}
        state["send_attempts"] = {}


def record_report_receipts(report_paths: List[pathlib.Path], state: Dict[str, object]) -> None:
    receipts = state.get("report_receipts", {})
    if not isinstance(receipts, dict):
        receipts = {}
    state["report_receipts"] = receipts

    now_text = dt.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    for report_path in report_paths:
        if report_path.name in receipts:
            continue
        receipts[report_path.name] = {
            "received_at": now_text,
            "trade_date": report_trade_date(report_path),
        }
        log(f"[REPORT_RECEIPT] {report_path.name} received_at={now_text}")


def extract_product_code(contract: str, fallback: str) -> str:
    prefix_chars = []
    for char in contract:
        if char.isalpha():
            prefix_chars.append(char)
            continue
        break
    if prefix_chars:
        return "".join(prefix_chars).upper()
    return fallback


def extract_contract_month_key(contract: str) -> int:
    match = re.search(r"(\d+)$", contract)
    if not match:
        return sys.maxsize
    try:
        return int(match.group(1))
    except ValueError:
        return sys.maxsize


def ordered_report_products(grouped: Dict[str, List[Dict[str, object]]]) -> List[str]:
    preferred = ["IF", "IC", "IM"]
    ordered = [product for product in preferred if product in grouped]
    ordered.extend(sorted(product for product in grouped.keys() if product not in set(preferred)))
    return ordered


def format_decimal(value: str, digits: int = 4) -> str:
    try:
        return f"{float(value):.{digits}f}"
    except (TypeError, ValueError):
        return value


def format_percent(value: str, digits: int = 4) -> str:
    formatted = format_decimal(value, digits)
    if not formatted:
        return formatted
    return f"{formatted}%"


def format_rate_value(value: Optional[float]) -> str:
    if value is None:
        return "N/A"
    color = "warning" if value < 0 else "comment"
    return f"<font color=\"{color}\">{value:.4f}%</font>"


def build_alert_markdown(row: Dict[str, str]) -> str:
    transition = row.get("transition", "")
    title = (
        "年化基差率已转为负值，请及时关注。"
        if transition == "EnteredNegative"
        else "年化基差率仍为负值，请及时关注。"
    )
    contract = (row.get("contract") or "").strip()
    product = extract_product_code(contract, (row.get("product_group") or "").strip())
    annual_rate = format_percent((row.get("annual_rate") or "").strip())
    basis = format_decimal((row.get("basis") or "").strip())
    index_price = format_decimal((row.get("index_price") or "").strip())
    future_price = format_decimal((row.get("future_price") or "").strip())
    remaining_days = (row.get("remaining_days") or "").strip()
    return (
        f"### {title}\n"
        f">品种：<font color=\"comment\">{product}</font>\n"
        f">合约：<font color=\"comment\">{contract}</font>\n"
        f">指数：<font color=\"comment\">{row.get('index_name', '')}</font>\n"
        f">年化基差率：<font color=\"warning\">{annual_rate}</font>\n"
        f">基差：<font color=\"comment\">{basis}</font>\n"
        f">指数价：<font color=\"comment\">{index_price}</font>\n"
        f">期货价：<font color=\"comment\">{future_price}</font>\n"
        f">剩余天数：<font color=\"comment\">{remaining_days}</font>\n"
        f">时间：<font color=\"comment\">{row.get('timestamp', '')}</font>\n"
    )


def post_wecom_json(webhook: str, payload: Dict[str, object],
                    max_retries: int = 3, retry_delay_seconds: float = 2.0,
                    timeout: int = 8) -> None:
    last_error: Optional[Exception] = None
    for attempt in range(1, max_retries + 1):
        try:
            request = urllib.request.Request(
                webhook,
                data=json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8"),
                headers={"Content-Type": "application/json"},
                method="POST",
            )
            with urllib.request.urlopen(request, timeout=timeout) as response:
                body = response.read().decode("utf-8")
            body_json = json.loads(body)
            if body_json.get("errcode") != 0:
                raise RuntimeError(f"WeCom returned error: {body_json}")
            return
        except (urllib.error.URLError, json.JSONDecodeError, RuntimeError) as exc:
            last_error = exc
            if attempt < max_retries:
                log(f"[WECOM_RETRY] attempt={attempt}/{max_retries} reason={exc}", err=True)
                time.sleep(retry_delay_seconds)
    raise RuntimeError(f"WeCom send failed after {max_retries} attempts: {last_error}") from last_error


def relay_alerts(webhook: str, csv_path: pathlib.Path, state: Dict[str, object],
                 max_retries: int = 3, retry_delay_seconds: float = 2.0,
                 timeout: int = 8) -> int:
    rows = load_alert_rows(csv_path)
    reset_daily_alert_state_if_needed(rows, state)
    current_trade_date = state.get("alert_trade_date", "")
    if current_trade_date:
        rows = [row for row in rows if alert_trade_date(row) == current_trade_date]
    sent_alert_keys = set(state.get("sent_alert_keys", []))
    relayed = 0
    for row in rows:
        transition = row.get("transition", "")
        if transition not in {"EnteredNegative", "RepeatedNegative"}:
            continue
        unique_key = alert_key(row)
        if not unique_key or unique_key in sent_alert_keys:
            continue
        try:
            post_wecom_json(
                webhook,
                {
                    "msgtype": "markdown",
                    "markdown": {"content": build_alert_markdown(row)},
                },
                max_retries=max_retries,
                retry_delay_seconds=retry_delay_seconds,
                timeout=timeout,
            )
        except RuntimeError as exc:
            log(f"[WECOM_ALERT_FAILED] key={unique_key} reason={exc}", err=True)
            continue
        sent_alert_keys.add(unique_key)
        relayed += 1
        log(f"[WECOM_ALERT_SENT] key={unique_key}")

    state["sent_alert_keys"] = sorted(sent_alert_keys)
    return relayed


def parse_report_title(report_text: str, fallback_name: str) -> str:
    del report_text
    match = re.match(r"(?P<date>\d{4}-\d{2}-\d{2})_(?P<moment>\d{4})_latest_basis\.txt$", fallback_name)
    if not match:
        return "最新基差表"

    trading_date = match.group("date")
    moment = match.group("moment")
    formatted_moment = f"{moment[:2]}:{moment[2:]}" if len(moment) == 4 else moment
    return f"{trading_date} {formatted_moment}最新基差表"


def parse_report_contract_line(line: str) -> Optional[Dict[str, object]]:
    parts = [part.strip() for part in line.split(" | ")]
    if not parts:
        return None

    contract = parts[0]
    if not contract:
        return None

    row: Dict[str, object] = {
        "contract": contract,
        "product": extract_product_code(contract, ""),
        "has_tick": True,
        "index_name": "",
        "annual_rate": None,
        "basis": None,
        "remaining_days": None,
    }

    for part in parts[1:]:
        if part == "暂无行情":
            row["has_tick"] = False
            continue
        if "=" not in part:
            continue
        key, value = part.split("=", 1)
        key = key.strip()
        value = value.strip()
        if key == "index":
            row["index_name"] = value
        elif key == "basis":
            try:
                row["basis"] = float(value)
            except ValueError:
                row["basis"] = None
        elif key == "remaining_days":
            try:
                row["remaining_days"] = int(float(value))
            except ValueError:
                row["remaining_days"] = None
        elif key == "annual_rate":
            if value.endswith("%"):
                value = value[:-1]
            try:
                row["annual_rate"] = float(value)
            except ValueError:
                row["annual_rate"] = None

    return row


def parse_report_rows(report_text: str) -> List[Dict[str, object]]:
    rows: List[Dict[str, object]] = []
    for raw_line in report_text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith("[Basis Monitor]") or line.startswith("report_generated_at=") or line.startswith("data_as_of=") or line.startswith("market_data_status="):
            continue
        if line.startswith("[GROUP]") or line.startswith("=") or line == "(no contracts)":
            continue
        parsed = parse_report_contract_line(line)
        if parsed is not None:
            rows.append(parsed)
    return rows


def summarize_extreme(rows: List[Dict[str, object]], reverse: bool) -> str:
    tick_rows = [row for row in rows if row.get("has_tick") and row.get("annual_rate") is not None]
    if not tick_rows:
        return "N/A"
    chosen = sorted(tick_rows, key=lambda item: float(item["annual_rate"]), reverse=reverse)[0]
    rate = float(chosen["annual_rate"])
    color = "warning" if rate < 0 else "comment"
    return f"<font color=\"{color}\">{chosen['contract']} {rate:.4f}%</font>"


def build_report_markdown(report_path: pathlib.Path, report_text: str) -> str:
    title = parse_report_title(report_text, report_path.name)
    rows = parse_report_rows(report_text)
    monitored_count = len(rows)
    tick_rows = [row for row in rows if row.get("has_tick")]
    negative_count = sum(1 for row in tick_rows if row.get("annual_rate") is not None and float(row["annual_rate"]) < 0)
    lowest = summarize_extreme(rows, reverse=False)
    highest = summarize_extreme(rows, reverse=True)

    grouped: Dict[str, List[Dict[str, object]]] = {}
    for row in rows:
        product = str(row.get("product") or "")
        grouped.setdefault(product, []).append(row)

    for product_rows in grouped.values():
        product_rows.sort(key=lambda item: (extract_contract_month_key(str(item["contract"])), str(item["contract"])))

    sections = [
        f"### {title}",
        "盘中监控摘要如下：",
        f">监控合约数：<font color=\"comment\">{monitored_count}</font>",
        f">已有行情：<font color=\"comment\">{len(tick_rows)}</font>",
        f">负年化基差：<font color=\"warning\">{negative_count}</font>",
        f">最低年化基差率：{lowest}",
        f">最高年化基差率：{highest}",
    ]

    for product in ordered_report_products(grouped):
        sections.append("")
        sections.append(f"**{product}**")
        for row in grouped[product]:
            contract = str(row["contract"])
            if not row.get("has_tick"):
                sections.append(f">{contract}：<font color=\"comment\">暂无行情</font>")
                continue
            annual_rate = format_rate_value(row.get("annual_rate"))
            basis = "N/A" if row.get("basis") is None else f"{float(row['basis']):.4f}"
            remaining_days = "N/A" if row.get("remaining_days") is None else f"{int(row['remaining_days'])}天"
            sections.append(f">{contract}：{annual_rate} | 基差 {basis} | 剩余 {remaining_days}")

    return "\n".join(sections)


def relay_reports(webhook: str, report_dir: pathlib.Path, state: Dict[str, object],
                  max_retries: int = 3, retry_delay_seconds: float = 2.0,
                  timeout: int = 8, max_report_send_attempts: int = 10) -> int:
    if not report_dir.exists():
        state.setdefault("sent_reports", [])
        state.setdefault("gave_up_reports", [])
        state.setdefault("report_receipts", {})
        state.setdefault("send_attempts", {})
        return 0

    today_trade_date = current_trade_date()
    report_paths = sorted(
        path for path in report_dir.glob("*_latest_basis.txt")
        if report_trade_date(path) == today_trade_date
    )
    reset_daily_report_state_if_needed(report_paths, state)
    record_report_receipts(report_paths, state)

    sent_reports = set(state.get("sent_reports", []))
    gave_up_reports = set(state.get("gave_up_reports", []))
    send_attempts: Dict[str, int] = dict(state.get("send_attempts", {}))
    relayed = 0
    for report_path in report_paths:
        if report_path.name in sent_reports or report_path.name in gave_up_reports:
            continue
        attempts = send_attempts.get(report_path.name, 0)
        if attempts >= max_report_send_attempts:
            log(f"[WECOM_REPORT_GAVEUP] {report_path.name} attempts={attempts}", err=True)
            gave_up_reports.add(report_path.name)
            continue
        report_text = report_path.read_text(encoding="utf-8", errors="replace").strip()
        if not report_text:
            continue
        payload = {
            "msgtype": "markdown",
            "markdown": {
                "content": build_report_markdown(report_path, report_text),
            },
        }
        try:
            post_wecom_json(webhook, payload,
                            max_retries=max_retries,
                            retry_delay_seconds=retry_delay_seconds,
                            timeout=timeout)
        except RuntimeError as exc:
            send_attempts[report_path.name] = attempts + 1
            log(f"[WECOM_RELAY_FAILED] {report_path.name} attempts={send_attempts[report_path.name]} reason={exc}",
                err=True)
            continue
        sent_reports.add(report_path.name)
        send_attempts.pop(report_path.name, None)
        relayed += 1
        log(f"[WECOM_REPORT_SENT] {report_path.name}")

    state["sent_reports"] = sorted(sent_reports)
    state["gave_up_reports"] = sorted(gave_up_reports)
    state["send_attempts"] = send_attempts
    return relayed


def load_relay_config() -> Dict[str, object]:
    config_root = pathlib.Path(os.environ.get("CONFIG_ROOT", "config"))
    if not config_root.is_absolute():
        config_root = pathlib.Path.cwd() / config_root
    alert_json_path = config_root / "alert.json"
    try:
        config = json.loads(alert_json_path.read_text(encoding="utf-8"))
    except (FileNotFoundError, json.JSONDecodeError):
        return {}
    if not isinstance(config, dict):
        return {}
    return config


def main() -> int:
    args = parse_args()
    webhook = args.webhook or ""
    if not webhook and args.ctp_ini:
        webhook = load_wecom_webhook_from_ctp_ini(pathlib.Path(args.ctp_ini))
    if not webhook:
        webhook = os.environ.get("WECOM_ROBOT_WEBHOOK", "")
    if not webhook:
        log("Missing webhook. Use --webhook or WECOM_ROBOT_WEBHOOK.", err=True)
        return 2

    relay_config = load_relay_config()
    wecom_max_retries = int(relay_config.get("wecom_max_send_retries", 3))
    wecom_retry_delay = float(relay_config.get("wecom_send_retry_delay_seconds", 2.0))
    wecom_timeout = int(relay_config.get("wecom_request_timeout_seconds", 8))
    max_report_attempts = int(relay_config.get("max_report_send_attempts", 10))

    input_root = pathlib.Path(args.input_root)
    state_path = pathlib.Path(args.state_file)
    alert_csv_path = input_root / "alert_events.csv"
    report_dir = input_root / "reports"

    state = load_state(state_path)
    relayed_alerts = 0
    relayed_reports = 0

    if not args.skip_alerts:
        try:
            relayed_alerts = relay_alerts(
                webhook, alert_csv_path, state,
                max_retries=wecom_max_retries,
                retry_delay_seconds=wecom_retry_delay,
                timeout=wecom_timeout,
            )
        except Exception as exc:
            log(f"[RELAY_ALERTS_CRASH] {exc}", err=True)

    if not args.skip_reports:
        try:
            relayed_reports = relay_reports(
                webhook, report_dir, state,
                max_retries=wecom_max_retries,
                retry_delay_seconds=wecom_retry_delay,
                timeout=wecom_timeout,
                max_report_send_attempts=max_report_attempts,
            )
        except Exception as exc:
            log(f"[RELAY_REPORTS_CRASH] {exc}", err=True)

    save_state(state_path, state)
    log(f"Relayed alerts: {relayed_alerts}")
    log(f"Relayed reports: {relayed_reports}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
