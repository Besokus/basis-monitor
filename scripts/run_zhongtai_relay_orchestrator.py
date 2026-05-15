#!/usr/bin/env python3

import argparse
import configparser
import datetime as dt
import json
import pathlib
import subprocess
import sys
import time
from typing import List


REPORT_WINDOW_MINUTES = 5


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
                                  window_minutes: int = REPORT_WINDOW_MINUTES) -> bool:
    try:
        report_start = parse_report_time(report_time)
    except (ValueError, TypeError):
        return False

    current_minutes = now.hour * 60 + now.minute
    return report_start <= current_minutes <= report_start + window_minutes


def pull_outputs(script_dir: pathlib.Path, remote_target: str, remote_project_dir: str, spool_dir: pathlib.Path) -> None:
    subprocess.run(
        [
            "sh",
            str(script_dir / "pull_zhongtai_outputs.sh"),
            remote_target,
            remote_project_dir,
            str(spool_dir),
        ],
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
    report_window_interval = min(5, pull_interval)

    print(f"[ORCHESTRATOR] started pull_interval={pull_interval}s "
          f"report_times={report_relay_times} report_window={report_window_minutes}min",
          file=sys.stderr)

    while True:
        now = dt.datetime.now()

        try:
            pull_outputs(script_dir, args.remote_target, args.remote_project_dir, spool_dir)
        except subprocess.CalledProcessError as exc:
            print(f"[ORCHESTRATOR] {now:%Y-%m-%d %H:%M:%S} pull_outputs failed: {exc}", file=sys.stderr)
            if args.once:
                return 1
            time.sleep(pull_interval)
            continue

        try:
            relay_alerts(script_dir, spool_dir, state_file, ctp_ini_path)
        except subprocess.CalledProcessError as exc:
            print(f"[ORCHESTRATOR] {now:%Y-%m-%d %H:%M:%S} relay_alerts failed: {exc}", file=sys.stderr)

        in_report_window = any(
            is_time_within_report_window(now, rt, report_window_minutes)
            for rt in report_relay_times
        )
        if in_report_window:
            try:
                relay_report_text(script_dir, spool_dir, state_file, ctp_ini_path)
            except subprocess.CalledProcessError as exc:
                print(f"[ORCHESTRATOR] {now:%Y-%m-%d %H:%M:%S} relay_report_text failed: {exc}",
                      file=sys.stderr)
            sleep_sec = report_window_interval
        else:
            sleep_sec = pull_interval

        print(f"[ORCHESTRATOR] {now:%Y-%m-%d %H:%M:%S} cycle done pull_ok alerts_ok "
              f"report_window={in_report_window} next_in={sleep_sec}s",
              file=sys.stderr)

        if args.once:
            return 0

        time.sleep(sleep_sec)


if __name__ == "__main__":
    raise SystemExit(main())
