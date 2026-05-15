# Basis Monitor

`basis_monitor` is a standalone intraday basis monitoring application for stock-index futures.
At startup it reads reference CSV datasets, selects the following groups by latest available turnover, subscribes those contracts through the configured market-data provider, computes annualized basis against the latest available index close, and raises alerts when annualized basis falls below the configured threshold.

## Current Implementation Status

For the current execution phase, completed items, pending tasks, and field-validation checkpoints, see:

- `docs/implementation_status_2026-04-08.md`
- `docs/ctp_live_integration_plan_2026-04-08.md`
- `docs/code_review_and_phase_2026-04-08.md`

- `hs300` -> `IF` top-4 real month contracts
- `zz500` -> `IC` top-4 real month contracts
- `zz1000` -> `IM` top-4 real month contracts

## Data inputs

The application can read reference data from either:

- the directories configured in `config/ctp.ini`
- or the local fallback directories under `basis_monitor/data/` when those config entries are empty

The four required reference directories are:

- `all_instruments/Future/`
- `all_instruments/INDX/`
- `eod_price/Future/`
- `eod_price/INDX/`

File discovery rule:

- all four directories use the latest available CSV file

Required CSV fields:

- Future metadata: `order_book_id`, `exchange`, `underlying_symbol`, `underlying_order_book_id`, `product`, `maturity_date`
- Index metadata: `order_book_id`, `symbol`
- Future EOD: `trade_date`, `underlying_symbol`, `order_book_id`, `close`, `total_turnover`
- Index EOD: `trade_date`, `order_book_id`, `close`

## Selection rules

Only real month contracts for the following product groups are monitored:

- `IF` (reported as `hs300`)
- `IC` (reported as `zz500`)
- `IM` (reported as `zz1000`)

Rules:

- the future metadata row must be `exchange == CFFEX`
- the future metadata row must be `product == Index`
- `order_book_id` must match a real month contract such as `IC2606`
- continuous contracts such as `88`, `888`, `889`, `99`, `88A2`, and `88A3` are excluded by pattern
- contracts missing `maturity_date` or index mapping are skipped with a warning
- each product group keeps up to 4 contracts sorted by latest available `total_turnover` descending

## Calculation

Per tick:

- `basis = index_close_yesterday - future_last_price`
- `annual_rate = basis / index_close_yesterday * (365 / remaining_days) * 100`

Special cases:

- if `remaining_days <= 0`, annualized basis is reported as `0%` and no alert is triggered
- if `index_close_yesterday <= 0`, the tick is skipped as invalid baseline data

## Alerting

`config/alert.json` controls:

- `negative_threshold`
- `repeat_interval_minutes`

Behavior:

- first crossing below `negative_threshold` triggers an alert immediately
- if the contract remains below threshold, reminder alerts are sent every `repeat_interval_minutes`
- the same interval also acts as the alert-state hold window, so a brief move back above threshold does not immediately clear the alert state
- returning above threshold is silent and does not send a recovery notification

## Runtime outputs

Runtime files:

- `logs/runtime.log`
- `logs/alert.log`
- `data/output/basis_results.csv`
- `data/output/alert_events.csv`
- `data/output/reports/YYYY-MM-DD_1130_latest_basis.txt`
- `data/output/reports/YYYY-MM-DD_1500_latest_basis.txt`
- `data/output/reports/YYYY-MM-DD_1130_latest_basis.png` when `GenerateLocalReportImage=true`
- `data/output/reports/YYYY-MM-DD_1500_latest_basis.png` when `GenerateLocalReportImage=true`

Terminal behavior:

- startup logs selected contracts per business group
- startup derives `CtpInstrumentIDs` from the selected Top-4 monitored futures contracts
- startup derives `XtpIndexInstrumentIDs` from the selected monitored contracts' mapped indices
- live monitoring output is rendered as grouped snapshots for `hs300 / zz500 / zz1000`
- negative-threshold transitions and reminder repeats are written through `LogAlert(...)`
- 11:30 and 15:00 generate the latest-basis report from the in-memory snapshot cache
- contracts without any tick yet still appear in scheduled reports as `N/A`

## WeCom behavior

Important `ctp.ini` fields:

- `MarketDataProvider`
- `ReferenceFutureMetadataDir`
- `ReferenceIndexMetadataDir`
- `ReferenceFutureEodDir`
- `ReferenceIndexEodDir`
- `EnableWeComAlert`
- `EnableWeComReport`
- `WeComRobotWebhook`

WeCom delivery rules:

- when `EnableWeComAlert=true`, realtime negative-basis alerts are pushed as markdown messages
- when `EnableWeComReport=true`, 11:30 and 15:00 reports are rendered as PNG tables and pushed as image messages
- report images are grouped by `hs300 / zz500 / zz1000`
- rows inside each group are sorted by `remaining_days` from near to far
- the `WARNING` column shows the actual annualized basis value when it is below threshold and highlights that cell in red
- when `WeComRobotWebhook` is empty, WeCom delivery is disabled and the process keeps running
- if WeCom delivery fails, the failure is logged but market-data monitoring continues

## Dual-server deployment

Recommended production split:

- the Zhongtai server runs `basis_monitor` only
- your own server compiles binaries, pushes reference CSV through SFTP, pulls Zhongtai outputs, and relays WeCom notifications

Server responsibilities:

- Zhongtai server
  - keeps local staging under `/list/10.101.5.62/basis-monitor-zhongtai/data/staging`
  - runs the prebuilt binary only
  - writes `logs/` and `data/output/*.csv`
  - does not send WeCom directly
- your own server
  - builds `basis_monitor/bin/basis_monitor`
  - locally filters reference CSV down to today's monitored contracts plus their mapped indices
  - pushes the filtered staging CSV to Zhongtai with SFTP
  - optionally pushes refreshed runtime bundle
  - pulls `alert_events.csv` and `basis_results.csv` back from Zhongtai
  - renders report PNGs locally
  - sends WeCom notifications outward

The tracked `config/ctp.ini` sample is now aligned to Zhongtai local mode:

- `Reference*Dir` points to Zhongtai local staging
- `EnableWeComAlert=false`
- `EnableWeComReport=false`
- `GenerateLocalReportImage=false`
- `WeComRobotWebhook=` is left empty

Your own server should supply the actual webhook to the relay script instead of enabling direct delivery on Zhongtai.

## Configuration

Required startup config:

- `config/ctp.ini`
- `config/alert.json` (optional; defaults are used when missing)
- `config/sftp.conf` (optional for local relay/upload scripts; copy from `config/sftp.conf.example`)

Provider config:

- `MarketDataProvider=ctp|xtp`
- default is `ctp`
- when `MarketDataProvider=xtp`, the following fields are parsed:
  - `XtpServerIp`
  - `XtpServerPort`
  - `XtpUser`
  - `XtpPassword`
  - `XtpClientId`
  - `XtpProtocol`
  - `XtpExchangeId`
  - `XtpLocalIp`
  - `XtpConfigFile`
  - `XtpFirstTickTimeoutMs`
- `XTP` runtime subscriptions now prefer indices derived from the selected Top-4 monitored contracts
- `IndexInstrumentID` in `xtp.ini` is now a fallback only, used when runtime-monitored contracts cannot derive index subscriptions
- phase 2 keeps `ctp` as the stable fallback baseline while `xtp` can be enabled for real-provider validation
- `xtp` startup currently assumes the configured exchange and instruments are supported by the server and should be validated on the Zhongtai environment before switching away from `ctp`

Legacy-only inputs:

- `config/contracts.json`
- `config/spot_price.json`

The formal workflow no longer depends on the legacy files above. If they are absent, startup still succeeds. If they exist but are invalid paths, startup fails fast.

## Linux build and run

Build manually:

```sh
cmake -S basis_monitor -B basis_monitor/build
cmake --build basis_monitor/build
```

Run the helper script:

```sh
sh basis_monitor/run.sh
```

The helper script builds into `${HOME}/.cache/basis_monitor/build` and then launches `basis_monitor` from the project directory so relative `config/` and `data/` paths continue to work.

Background run on Linux:

```sh
sh basis_monitor/start.sh
```

- `start.sh` builds first, then launches `basis_monitor` in the background
- runtime/application logs are written to `basis_monitor/logs/runtime.log`
- `basis_monitor/logs/nohup.out` is reserved for launcher-level startup failures
- the pid is recorded in `basis_monitor/runtime/basis_monitor.pid`

Stop the background process:

```sh
sh basis_monitor/stop.sh
```

Prebuilt deployment on Linux:

- use this mode when the binary is compiled on another server and the Zhongtai server only runs the executable
- place the executable at `basis_monitor/bin/basis_monitor`
- keep the current runtime layout:
  - `basis_monitor/config/`
  - `basis_monitor/scripts/`
  - `basis_monitor/vendor/ctp/live/lib/linux/thostmduserapi_se.so`
  - `basis_monitor/vendor/ctp/data_collect/LinuxDataCollect.so`
  - `XTPXQuoteAPI_1.0.15_20260113/lib/centos/onload-8.1.2.26/libxtpxquoteapi.so`

Start the prebuilt binary in background:

```sh
sh basis_monitor/start_prebuilt.sh
```

Stop the prebuilt background process:

```sh
sh basis_monitor/stop_prebuilt.sh
```

The prebuilt startup script:

- does not run `cmake`
- exports both CTP and XTP runtime library paths
- expects to be launched from the existing project layout
- writes application logs to `basis_monitor/logs/runtime.log`
- keeps `basis_monitor/logs/nohup.out` for launcher-level startup failures
- records the pid in `basis_monitor/runtime/basis_monitor.pid`

Linux runtime requirements for image reports:

- `python3`
- the bundled scripts under `basis_monitor/scripts/`
- no extra Python packages are required

Deployment helpers:

- Zhongtai deployment guide: `basis_monitor/deploy/README-zhongtai.md`
- Dual-server split guide: `basis_monitor/deploy/README-dual-server.md`
- systemd service template: `basis_monitor/deploy/systemd/basis-monitor.service`

## Reference-data staging on Zhongtai server

Recommended production flow:

1. sync reference CSV files from your server to a local staging directory on the Zhongtai server
2. validate the latest files locally
3. point `ReferenceFutureMetadataDir`, `ReferenceIndexMetadataDir`, `ReferenceFutureEodDir`, and `ReferenceIndexEodDir` to that staging directory
4. start `basis_monitor`

Example staging layout:

```text
/data/basis_monitor/reference/
  all_instruments/Future/
  all_instruments/INDX/
  eod_price/Future/
  eod_price/INDX/
```

Helper script:

```sh
sh basis_monitor/scripts/sync_reference_data.sh rsync user@host:/data/riceQuantData /data/basis_monitor/reference
```

If the source directories do not match the default `all_instruments/*` and `eod_price/*` layout, pass the four source directories explicitly:

```sh
sh basis_monitor/scripts/sync_reference_data.sh \
  rsync \
  user@host:/data/riceQuantData/all_instruments/Future \
  user@host:/data/riceQuantData/all_instruments/INDX \
  user@host:/data/riceQuantData/future_eod_price \
  user@host:/data/riceQuantData/index_eod_price \
  /data/basis_monitor/reference
```

The helper script:

- syncs the four required directories into the local staging root
- validates that the latest CSV exists and is non-empty
- validates the required headers before the monitor process starts

Production SFTP workflow:

- optional SFTP connection config on your own server:

```sh
cp basis_monitor/config/sftp.conf.example basis_monitor/config/sftp.conf
```

Fill in:

- `SFTP_HOST`
- `SFTP_USER`
- `SFTP_PORT`
- `SFTP_IDENTITY_FILE`

- push reference CSV from your own server to Zhongtai:

```sh
sh basis_monitor/scripts/push_reference_data_to_zhongtai.sh \
  /list/10.101.5.62/basis-monitor-zhongtai/data/staging
```

The push helper now:

- loads the latest four source CSV snapshots from your own server
- applies the same `IF/IC/IM` Top4 selection rule used by `basis_monitor`
- writes a temporary filtered staging set that contains only today's monitored contracts and the required index rows
- uploads that filtered staging set to Zhongtai

- validate Zhongtai staging locally on the Zhongtai server:

```sh
sh basis_monitor/scripts/validate_reference_data.sh \
  /list/10.101.5.62/basis-monitor-zhongtai/data/staging
```

- optionally push the prebuilt runtime bundle from your own server:

```sh
sh basis_monitor/scripts/push_prebuilt_runtime_to_zhongtai.sh \
  /list/10.101.5.62/basis-monitor-zhongtai
```

- pull Zhongtai outputs back to your own server:

```sh
sh basis_monitor/scripts/pull_zhongtai_outputs.sh \
  /list/10.101.5.62/basis-monitor-zhongtai/basis_monitor \
  ./relay_spool
```

- generate report PNGs locally from pulled `basis_results.csv`:

```sh
python3 basis_monitor/scripts/generate_report_image_from_basis_results.py \
  --basis-results ./relay_spool/basis_results.csv \
  --output ./relay_spool/reports/$(date +%F)_1130_latest_basis.png \
  --moment 1130 \
  --negative-threshold 0.0
```

- relay alerts and locally generated report PNGs from your own server:

```sh
python3 basis_monitor/scripts/relay_zhongtai_notifications.py \
  --input-root ./relay_spool \
  --state-file ./relay_state.json \
  --webhook "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=..."
```

## Validation

Run the full test suite:

```sh
ctest --test-dir basis_monitor/build --output-on-failure
```

Migration sanity check:

```powershell
.\basis_monitor\tests\verify_market_data_migration.ps1
```

Manual runtime verification checklist:

- `hs300 / zz500 / zz1000` selection logs appear at startup
- each group shows up to 4 contracts
- `MarketDataProvider = ctp|xtp` is logged at startup
- the subscribed instrument list matches the derived selection
- `logs/runtime.log` is written
- `logs/alert.log` is written
- `data/output/basis_results.csv` is written
- `data/output/alert_events.csv` is written
- `data/output/reports/` contains text reports for 11:30 and 15:00
- PNG reports are present locally only when `GenerateLocalReportImage=true`, or after your own server renders them from pulled `basis_results.csv`
- if WeCom alert is enabled, negative-basis alerts arrive in the robot as markdown
- if WeCom report is enabled, 11:30 and 15:00 report images arrive in the robot
