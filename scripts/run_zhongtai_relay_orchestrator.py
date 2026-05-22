#!/usr/bin/env python3

import argparse
import configparser
import datetime as dt
import json
import os
import pathlib
import subprocess
import sys
import time
from typing import List


REPORT_WINDOW_MINUTES = 5
REPORT_PULL_LEAD_MINUTES = 5


def log(message: str, *, err: bool = False) -> None:
    timestamp = dt.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    stream = sys.stderr if err else sys.stdout
    print(f"[{timestamp}] {message}", file=stream)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Pull Zhongtai outputs, generate local reports, and relay notifications.")
    parser.add_argument("--remote-target", required=True, help="SFTP target such as zhongtai@10.101.5.62")
    parser.add_argument("--remote-project-dir", required=True, help="Remote basis_monitor directory on Zhongtai")
    parser.add_argument("--config-root", default="config", help="Directory containing ctp.ini and alert.json")
    parser.add_argument("--spool-dir", default="relay_spool", help="Local spool directory for pulled Zhongtai outputs")
    parser.add_argument("--state-file", default="relay_state.json", help="Local relay state file")
    parser.add_argument("--once", action="store_true", help="Run one pull/relay cycle and exit")
    return parser.parse_args()


def load_ctp_ini(path: pathlib.Path) -> configparser.ConfigParser:
    parser = configparser.ConfigParser()
    parser.read(path, encoding="utf-8")
    return parser


def load_alert_json(path: pathlib.Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def report_moment_from_time(text: str) -> str:
    return "1130" if text.startswith("11:3") else "1500"


def parse_report_time(report_time: str) -> int:
    hour_text, minute_text = report_time.split(":", 1)
    return int(hour_text) * 60 + int(minute_text)


def is_time_within_report_window(now: dt.datetime, report_time: str,
                                  window_minutes: int = REPORT_WINDOW_MINUTES,
                                  lead_minutes: int = 0) -> bool:
    try:
        report_start = parse_report_time(report_time)
    except (ValueError, TypeError):
        return False

    current_minutes = now.hour * 60 + now.minute
    window_start = report_start - max(0, lead_minutes)
    window_end = report_start + max(0, window_minutes)
    return window_start <= current_minutes <= window_end


def pull_outputs(script_dir: pathlib.Path,
                 remote_target: str,
                 remote_project_dir: str,
                 spool_dir: pathlib.Path,
                 *,
                 include_report_texts: bool = True) -> None:
    env = dict(**os.environ, PULL_REPORT_TEXTS="1" if include_report_texts else "0")
    subprocess.run(
        [
            "sh",
            str(script_dir / "pull_zhongtai_outputs.sh"),
            remote_target,
            remote_project_dir,
            str(spool_dir),
        ],
        env=env,
        check=True,
    )


def relay_alerts(script_dir: pathlib.Path, spool_dir: pathlib.Path, state_file: pathlib.Path, ctp_ini_path: pathlib.Path) -> None:
    subprocess.run(
        [
            sys.executable,
            str(script_dir / "relay_zhongtai_notifications.py"),
            "--input-root",
            str(spool_dir),
            "--state-file",
            str(state_file),
            "--ctp-ini",
            str(ctp_ini_path),
            "--skip-reports",
        ],
        check=True,
    )


def relay_report_text(script_dir: pathlib.Path,
                      spool_dir: pathlib.Path,
                      state_file: pathlib.Path,
                      ctp_ini_path: pathlib.Path) -> None:
    subprocess.run(
        [
            sys.executable,
            str(script_dir / "relay_zhongtai_notifications.py"),
            "--input-root",
            str(spool_dir),
            "--state-file",
            str(state_file),
            "--ctp-ini",
            str(ctp_ini_path),
            "--skip-alerts",
        ],
        check=True,
    )


def main() -> int:
    args = parse_args()
    script_dir = pathlib.Path(__file__).resolve().parent
    config_root = pathlib.Path(args.config_root)
    if not config_root.is_absolute():
        config_root = pathlib.Path.cwd() / config_root
    ctp_ini_path = config_root / "ctp.ini"
    alert_json_path = config_root / "alert.json"
    spool_dir = pathlib.Path(args.spool_dir)
    if not spool_dir.is_absolute():
        spool_dir = pathlib.Path.cwd() / spool_dir
    state_file = pathlib.Path(args.state_file)
    if not state_file.is_absolute():
        state_file = pathlib.Path.cwd() / state_file

    alert_config = load_alert_json(alert_json_path)
    pull_interval = int(alert_config.get("relay_pull_interval_seconds", 10))
    if pull_interval <= 0:
        pull_interval = max(10, int(alert_config.get("repeat_interval_minutes", 20)) * 60)
    report_relay_times = alert_config.get("report_relay_times", ["11:30", "15:00"])
    report_window_minutes = int(alert_config.get("report_relay_window_minutes", REPORT_WINDOW_MINUTES))
    report_pull_lead_minutes = int(alert_config.get("report_pull_lead_minutes", REPORT_PULL_LEAD_MINUTES))
    report_window_interval = min(5, pull_interval)

    log(f"[ORCHESTRATOR] started pull_interval={pull_interval}s "
        f"report_times={report_relay_times} report_lead={report_pull_lead_minutes}min "
        f"report_window={report_window_minutes}min",
        err=True)

    while True:
        now = dt.datetime.now()
        in_report_window = any(
            is_time_within_report_window(now, rt, report_window_minutes, report_pull_lead_minutes)
            for rt in report_relay_times
        )

        try:
            pull_outputs(
                script_dir,
                args.remote_target,
                args.remote_project_dir,
                spool_dir,
                include_report_texts=in_report_window,
            )
        except subprocess.CalledProcessError as exc:
            log(f"[ORCHESTRATOR] pull_outputs failed: {exc}", err=True)
            if args.once:
                return 1
            time.sleep(pull_interval)
            continue

        try:
            relay_alerts(script_dir, spool_dir, state_file, ctp_ini_path)
        except subprocess.CalledProcessError as exc:
            log(f"[ORCHESTRATOR] relay_alerts failed: {exc}", err=True)

        if in_report_window:
            try:
                relay_report_text(script_dir, spool_dir, state_file, ctp_ini_path)
            except subprocess.CalledProcessError as exc:
                log(f"[ORCHESTRATOR] relay_report_text failed: {exc}", err=True)

        sleep_sec = report_window_interval if in_report_window else pull_interval

        log(f"[ORCHESTRATOR] cycle done pull_ok alerts_ok "
            f"report_window={in_report_window} next_in={sleep_sec}s",
            err=True)

        if args.once:
            return 0

        time.sleep(sleep_sec)


if __name__ == "__main__":
    raise SystemExit(main())
