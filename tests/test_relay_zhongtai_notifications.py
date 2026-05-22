#!/usr/bin/env python3

import importlib.util
import pathlib
import tempfile
import unittest


def load_module():
    script_path = pathlib.Path(__file__).resolve().parents[1] / "scripts" / "relay_zhongtai_notifications.py"
    spec = importlib.util.spec_from_file_location("relay_zhongtai_notifications", script_path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


class RelayZhongtaiNotificationsTest(unittest.TestCase):
    def setUp(self):
        self.module = load_module()
        self.temp_dir = tempfile.TemporaryDirectory()
        temp_root = pathlib.Path(self.temp_dir.name)
        self.csv_path = temp_root / "alert_events.csv"
        self.report_dir = temp_root / "reports"
        self.report_dir.mkdir(parents=True, exist_ok=True)
        self.webhook = "https://example.invalid/wecom"
        self.sent_payloads = []

        def fake_post_wecom_json(webhook, payload):
            self.sent_payloads.append((webhook, payload))

        self.module.post_wecom_json = fake_post_wecom_json
        self.module.current_trade_date = lambda: "2026-04-10"

    def tearDown(self):
        self.temp_dir.cleanup()

    def write_rows(self, rows):
        self.csv_path.write_text(
            "timestamp,contract,product_group,index_code,index_name,index_price,future_price,basis,annual_rate,remaining_days,transition,reason\n"
            + "\n".join(rows)
            + ("\n" if rows else ""),
            encoding="utf-8",
        )

    def write_report(self, name, content):
        report_path = self.report_dir / name
        report_path.write_text(content, encoding="utf-8")
        return report_path

    def test_relay_alerts_skips_previously_sent_rows_by_alert_key(self):
        self.write_rows(
            [
                "2026-04-10 10:00:00.000,IC2604,IC,000905.XSHG,\u4e2d\u8bc1500,8024.6930,8023.4000,1.2930,-0.0305,7,EnteredNegative,annual rate below threshold",
                "2026-04-10 10:20:00.000,IC2604,IC,000905.XSHG,\u4e2d\u8bc1500,8024.6930,8023.4000,1.2930,-0.0280,7,RepeatedNegative,annual rate remains below threshold reminder",
            ]
        )

        state = self.module.load_state(pathlib.Path(self.temp_dir.name) / "relay_state.json")

        relayed = self.module.relay_alerts(self.webhook, self.csv_path, state)
        self.assertEqual(2, relayed)
        self.assertEqual(2, len(self.sent_payloads))

        first_message = self.sent_payloads[0][1]["markdown"]["content"]
        self.assertIn("\u54c1\u79cd", first_message)
        self.assertIn("\u5408\u7ea6", first_message)
        self.assertIn("\u6307\u6570", first_message)
        self.assertIn("\u5e74\u5316\u57fa\u5dee\u7387", first_message)
        self.assertIn("\u57fa\u5dee", first_message)
        self.assertIn("\u6307\u6570\u4ef7", first_message)
        self.assertIn("\u671f\u8d27\u4ef7", first_message)
        self.assertIn("\u5269\u4f59\u5929\u6570", first_message)
        self.assertIn("\u65f6\u95f4", first_message)
        self.assertNotIn("\u7c7b\u578b", first_message)
        self.assertNotIn("\u539f\u56e0", first_message)

        relayed_again = self.module.relay_alerts(self.webhook, self.csv_path, state)
        self.assertEqual(0, relayed_again)
        self.assertEqual(2, len(self.sent_payloads))

        self.write_rows(
            [
                "2026-04-10 10:00:00.000,IC2604,IC,000905.XSHG,\u4e2d\u8bc1500,8024.6930,8023.4000,1.2930,-0.0305,7,EnteredNegative,annual rate below threshold",
                "2026-04-10 10:20:00.000,IC2604,IC,000905.XSHG,\u4e2d\u8bc1500,8024.6930,8023.4000,1.2930,-0.0280,7,RepeatedNegative,annual rate remains below threshold reminder",
                "2026-04-10 10:40:00.000,IC2604,IC,000905.XSHG,\u4e2d\u8bc1500,8026.0000,8024.1000,1.9000,-0.0210,7,RepeatedNegative,annual rate remains below threshold reminder",
            ]
        )

        relayed_new = self.module.relay_alerts(self.webhook, self.csv_path, state)
        self.assertEqual(1, relayed_new)
        self.assertEqual(3, len(self.sent_payloads))

    def test_relay_alerts_resets_sent_keys_when_trade_date_changes(self):
        self.write_rows(
            [
                "2026-04-10 14:59:00.000,IF2604,IF,000300.XSHG,\u6caa\u6df1300,4633.6121,4638.0000,-4.3879,-0.0100,1,EnteredNegative,annual rate below threshold",
            ]
        )

        state = self.module.load_state(pathlib.Path(self.temp_dir.name) / "relay_state.json")
        first_day_relayed = self.module.relay_alerts(self.webhook, self.csv_path, state)
        self.assertEqual(1, first_day_relayed)
        self.assertEqual("2026-04-10", state.get("alert_trade_date"))

        self.sent_payloads.clear()
        self.write_rows(
            [
                "2026-04-11 09:31:00.000,IF2604,IF,000300.XSHG,\u6caa\u6df1300,4638.0000,4641.0000,-3.0000,-0.0080,1,EnteredNegative,annual rate below threshold",
            ]
        )

        second_day_relayed = self.module.relay_alerts(self.webhook, self.csv_path, state)
        self.assertEqual(1, second_day_relayed)
        self.assertEqual(1, len(self.sent_payloads))
        self.assertEqual("2026-04-11", state.get("alert_trade_date"))

    def test_relay_alerts_only_sends_latest_trade_date_rows_when_csv_contains_multiple_days(self):
        self.write_rows(
            [
                "2026-04-13 09:29:00.222,IC2604,IC,000905.XSHG,\u4e2d\u8bc1500,7917.92,7923,-5.0806,-5.85513,4,EnteredNegative,annual rate below threshold",
                "2026-04-14 09:29:00.386,IC2604,IC,000905.XSHG,\u4e2d\u8bc1500,7900.00,7930,-30.0000,-30.1573,4,EnteredNegative,annual rate below threshold",
            ]
        )
        state = {
            "alert_trade_date": "2026-04-13",
            "report_trade_date": "",
            "sent_alert_keys": [
                "2026-04-13 09:29:00.222|IC2604|000905.XSHG|EnteredNegative|-5.85513",
            ],
            "sent_reports": [],
        }

        relayed = self.module.relay_alerts(self.webhook, self.csv_path, state)
        self.assertEqual(1, relayed)
        self.assertEqual(1, len(self.sent_payloads))
        payload_text = self.sent_payloads[0][1]["markdown"]["content"]
        self.assertIn("2026-04-14 09:29:00.386", payload_text)
        self.assertNotIn("2026-04-13 09:29:00.222", payload_text)
        self.assertEqual("2026-04-14", state.get("alert_trade_date"))

    def test_relay_reports_formats_summary_and_sorts_by_contract_month(self):
        report_path = self.write_report(
            "2026-04-10_1500_latest_basis.txt",
            "\n".join(
                [
                    "[Basis Monitor] 15:00 最新基差表",
                    "report_generated_at=2026-04-10 15:00:01.000",
                    "data_as_of=2026-04-10 15:00:00.000",
                    "market_data_status=OK",
                    "===============================",
                    "[GROUP] hs300",
                    "IF2606 | index=沪深300 | index_price=4633.6121 | future=4625.0000 | basis=8.6121 | remaining_days=82 | annual_rate=0.8250%",
                    "IF2604 | index=沪深300 | index_price=4633.6121 | future=4692.0000 | basis=-58.3879 | remaining_days=16 | annual_rate=-28.9899%",
                    "[GROUP] zz500",
                    "IC2606 | index=中证500 | index_price=8004.2200 | future=7953.6938 | basis=50.5262 | remaining_days=82 | annual_rate=2.9525%",
                    "IC2604 | index=中证500 | future=N/A | 暂无行情",
                    "IC2609 | index=中证500 | index_price=8004.2200 | future=7790.0000 | basis=214.3262 | remaining_days=170 | annual_rate=6.0411%",
                    "IC2605 | index=中证500 | index_price=8004.2200 | future=8050.4938 | basis=-46.2738 | remaining_days=44 | annual_rate=-5.0393%",
                    "[GROUP] zz1000",
                    "IM2606 | index=中证1000 | index_price=8123.1100 | future=8150.5500 | basis=-27.4400 | remaining_days=82 | annual_rate=-1.4800%",
                    "===============================",
                ]
            ),
        )

        state = self.module.load_state(pathlib.Path(self.temp_dir.name) / "relay_state.json")
        relayed = self.module.relay_reports(self.webhook, self.report_dir, state)
        self.assertEqual(1, relayed)

        message = self.sent_payloads[0][1]["markdown"]["content"]
        self.assertIn("2026-04-10 15:00最新基差表", message)
        self.assertIn("监控合约数", message)
        self.assertIn("已有行情", message)
        self.assertIn("负年化基差", message)
        self.assertIn("最低年化基差率", message)
        self.assertIn("最高年化基差率", message)
        self.assertIn("**IC**", message)
        self.assertIn("**IF**", message)
        self.assertIn("**IM**", message)
        self.assertTrue(message.index("**IF**") < message.index("**IC**"))
        self.assertTrue(message.index("**IC**") < message.index("**IM**"))
        ic_section = message.split("**IC**", 1)[1].split("**IM**", 1)[0]
        if_section = message.split("**IF**", 1)[1].split("**IC**", 1)[0]
        self.assertTrue(ic_section.index("IC2604") < ic_section.index("IC2605"))
        self.assertTrue(ic_section.index("IC2605") < ic_section.index("IC2606"))
        self.assertTrue(ic_section.index("IC2606") < ic_section.index("IC2609"))
        self.assertTrue(if_section.index("IF2604") < if_section.index("IF2606"))
        self.assertIn("暂无行情", message)
        self.assertEqual("2026-04-10", state.get("report_trade_date"))
        self.assertIn(report_path.name, state.get("report_receipts", {}))
        self.assertTrue(state["report_receipts"][report_path.name]["received_at"])

        relayed_again = self.module.relay_reports(self.webhook, self.report_dir, state)
        self.assertEqual(0, relayed_again)

    def test_relay_reports_only_sends_current_trade_date_reports(self):
        self.write_report(
            "2026-04-09_1500_latest_basis.txt",
            "\n".join(
                [
                    "[Basis Monitor] 15:00 report",
                    "[GROUP] hs300",
                    "IF2604 | index=HS300 | index_price=4600.0000 | future=4590.0000 | basis=10.0000 | remaining_days=10 | annual_rate=1.0000%",
                ]
            ),
        )
        self.write_report(
            "2026-04-10_1500_latest_basis.txt",
            "\n".join(
                [
                    "[Basis Monitor] 15:00 report",
                    "[GROUP] hs300",
                    "IF2604 | index=HS300 | index_price=4610.0000 | future=4600.0000 | basis=10.0000 | remaining_days=9 | annual_rate=1.1000%",
                ]
            ),
        )

        state = self.module.load_state(pathlib.Path(self.temp_dir.name) / "relay_state.json")
        relayed = self.module.relay_reports(self.webhook, self.report_dir, state)
        self.assertEqual(1, relayed)
        self.assertEqual(1, len(self.sent_payloads))
        sent_message = self.sent_payloads[0][1]["markdown"]["content"]
        self.assertIn("2026-04-10 15:00", sent_message)
        self.assertNotIn("2026-04-09 15:00", sent_message)
        self.assertEqual("2026-04-10", state.get("report_trade_date"))


if __name__ == "__main__":
    unittest.main()
