#!/usr/bin/env python3

import argparse
import datetime as real_dt
import importlib.util
import pathlib
import types
import unittest


def load_module():
    script_path = pathlib.Path(__file__).resolve().parents[1] / "scripts" / "run_zhongtai_relay_orchestrator.py"
    spec = importlib.util.spec_from_file_location("run_zhongtai_relay_orchestrator", script_path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


class RunZhongtaiRelayOrchestratorTest(unittest.TestCase):
    def setUp(self):
        self.module = load_module()

    def test_is_time_within_report_window_is_inclusive(self):
        now = real_dt.datetime(2026, 4, 17, 11, 30, 0)
        self.assertTrue(self.module.is_time_within_report_window(now, "11:30"))
        self.assertTrue(self.module.is_time_within_report_window(now.replace(minute=35), "11:30"))
        self.assertFalse(self.module.is_time_within_report_window(now.replace(minute=36), "11:30"))

        now = now.replace(hour=15, minute=0)
        self.assertTrue(self.module.is_time_within_report_window(now, "15:00"))
        self.assertTrue(self.module.is_time_within_report_window(now.replace(minute=5), "15:00"))
        self.assertFalse(self.module.is_time_within_report_window(now.replace(minute=6), "15:00"))

    def test_main_triggers_report_relay_within_window(self):
        calls = []

        def fake_pull_outputs(*args, **kwargs):
            calls.append("pull")

        def fake_relay_alerts(*args, **kwargs):
            calls.append("alerts")

        def fake_relay_report_text(*args, **kwargs):
            calls.append("report")

        fixed_now = real_dt.datetime(2026, 4, 17, 15, 4, 0)

        class FakeDatetime(real_dt.datetime):
            @classmethod
            def now(cls, tz=None):
                del tz
                return fixed_now

        self.module.pull_outputs = fake_pull_outputs
        self.module.relay_alerts = fake_relay_alerts
        self.module.relay_report_text = fake_relay_report_text
        self.module.load_alert_json = lambda path: {
            "relay_pull_interval_seconds": 10,
            "report_relay_times": ["11:30", "15:00"],
        }
        self.module.dt.datetime = FakeDatetime
        self.module.parse_args = lambda: argparse.Namespace(
            remote_target="zhongtai@10.101.5.62",
            remote_project_dir="/list/10.101.5.62/basis-monitor-zhongtai/basis_monitor",
            config_root="config",
            spool_dir="relay_spool",
            state_file="relay_state.json",
            once=True,
        )

        exit_code = self.module.main()

        self.assertEqual(0, exit_code)
        self.assertEqual(["pull", "alerts", "report"], calls)

    def test_main_triggers_report_relay_outside_window_too(self):
        calls = []

        def fake_pull_outputs(*args, **kwargs):
            calls.append("pull")

        def fake_relay_alerts(*args, **kwargs):
            calls.append("alerts")

        def fake_relay_report_text(*args, **kwargs):
            calls.append("report")

        fixed_now = real_dt.datetime(2026, 4, 17, 15, 6, 0)

        class FakeDatetime(real_dt.datetime):
            @classmethod
            def now(cls, tz=None):
                del tz
                return fixed_now

        self.module.pull_outputs = fake_pull_outputs
        self.module.relay_alerts = fake_relay_alerts
        self.module.relay_report_text = fake_relay_report_text
        self.module.load_alert_json = lambda path: {
            "relay_pull_interval_seconds": 10,
            "report_relay_times": ["11:30", "15:00"],
        }
        self.module.dt.datetime = FakeDatetime
        self.module.parse_args = lambda: argparse.Namespace(
            remote_target="zhongtai@10.101.5.62",
            remote_project_dir="/list/10.101.5.62/basis-monitor-zhongtai/basis_monitor",
            config_root="config",
            spool_dir="relay_spool",
            state_file="relay_state.json",
            once=True,
        )

        exit_code = self.module.main()

        self.assertEqual(0, exit_code)
        self.assertEqual(["pull", "alerts", "report"], calls)


if __name__ == "__main__":
    unittest.main()
