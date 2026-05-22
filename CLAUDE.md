# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Language & Deployment Constraint

**Always answer in Chinese.**
**XTP runs on the Zhongtai server — only compiled binaries and data files may be uploaded, never source code.**

See `AGENTS.md` for detailed working rules (plan-before-change, minimal diffs, one-concern-at-a-time, etc.).

## Build & Test

```sh
# Configure and build (Linux only)
cmake -S . -B build
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure

# Run a single test binary
./build/test_basis_calculator
```

The CMake build requires C++17, vendor CTP headers/libs under `vendor/ctp/`, and XTP SDK at `../XTPXQuoteAPI_1.0.15_20260113/`. RPATH is baked into the binary so vendor `.so` files are found at runtime.

## Run (Linux)

| Script | Purpose |
|---|---|
| `run.sh` | Build + foreground run |
| `start.sh` | Build + background (pid at `runtime/basis_monitor.pid`) |
| `stop.sh` | Stop background process |
| `start_prebuilt.sh` | Background run using prebuilt `bin/basis_monitor` (no cmake) |
| `stop_prebuilt.sh` | Stop prebuilt process |

## Architecture

`app/main.cpp` is the only entry point. The CMake build produces one executable (`basis_monitor`) and 6 static libraries:

| Library | Purpose | Key source files |
|---|---|---|
| `basis_monitor_core` | Tick processing, basis calculation, alerting, health tracking | `src/monitor/` |
| `basis_monitor_market_data` | CTP/CTP SPI bridge, XTP sessions, config loading, logging | `src/ctp/`, `src/market_data/` |
| `basis_monitor_data` | Reference CSV loading, Top4 contract selection, subscription derivation | `src/data/` |
| `basis_monitor_storage` | CSV output: tick store, basis results, alerts, reports | `src/storage/` |
| `basis_monitor_report` | Scheduled 11:30/15:00 text reports, PNG image generation | `src/report/` |
| `basis_monitor_notify` | WeCom robot markdown/image delivery | `src/notify/` |

Headers live under `include/basis_monitor/` mirroring this structure (domain/, monitor/, market_data/, data/, storage/, report/, notify/, config/, ctp/).

### Startup flow (main.cpp)

1. Load config from `config/ctp.ini` + `config/alert.json`
2. Build reference data directories (config paths or `data/` fallback)
3. Load latest reference CSVs → select IF/IC/IM top-4 contracts by turnover
4. Derive CTP instrument IDs (futures) and XTP instrument IDs (indices) from selection
5. Create `MarketDataSession` (single or dual CTP+XTP) with a listener that routes ticks
6. Wait for first tick, then enter main loop:
   - Every tick: `BasisMonitorService` computes annualized basis, `AlertEngine` checks thresholds
   - Every second: health check on both Future/Index channels, scheduled report check

### Tick routing (dual-provider)

- **XTP ticks** (`instrument_type == Index`): cached in `BasisMonitorService::latest_index_prices_`, no calculation triggered
- **CTP ticks** (`instrument_type == Future`): trigger full computation — reads latest cached index price, calculates basis + annual rate, evaluates alert, writes CSV, updates terminal display

### Key domain types

- `MarketTick` (`include/basis_monitor/domain/market_tick.h`) — unified tick with `instrument_type` (Future/Index) and `provider` (Ctp/Xtp)
- `MonitoredContract` — selected contract with product_group, instrument_id, index_code, maturity_date, etc.
- `MonitorUpdate` — result of processing a tick: includes computed `BasisResult`, `AlertEvent`, and status flags (contract_found, invalid_baseline, waiting_for_live_index, stale_live_index)
- `IMarketDataSession` — abstract interface: `Start()`, `WaitForFirstMarketData()`, `Stop()`

### Index liveness gating

When XTP is enabled (`enable_xtp_market_data=true`), the service requires a fresh live index price for calculation. Three skip conditions:
- `WAITING_FOR_LIVE_INDEX` — no index tick received yet
- `STALE_LIVE_INDEX` — last index tick older than `market_data_stale_threshold_sec` (default 30s)
- `INVALID_INDEX_BASELINE` — index price ≤ 0

These skip calculation and alerting for the affected contract; the terminal/CSV shows the last computed state.

## Configuration

- `config/ctp.ini` — CTP connection, instruments, WeCom webhook, reference data dirs, health thresholds
- `config/alert.json` — `negative_threshold`, `repeat_interval_minutes`
- `config/xtp.ini` — XTP connection, index instruments (used as fallback when runtime derivation produces empty list)
- `config/sftp.conf` — optional, for relay scripts (copy from `config/sftp.conf.example`)

## Reference Data

Four CSV directories, each using the latest file:
- `all_instruments/Future/` — contract metadata
- `all_instruments/INDX/` — index metadata
- `eod_price/Future/` — futures EOD (close, total_turnover)
- `eod_price/INDX/` — index EOD (close)

Selection: CFFEX + Index product + real-month pattern (excludes 88/888/889/99/88A2/88A3), top 4 by total_turnover per product group (IF→hs300, IC→zz500, IM→zz1000).

## Dual-Server Deployment

- **Your server**: compiles, filters reference CSV to monitored subset, SFTP pushes to Zhongtai, pulls outputs back, renders PNGs, relays WeCom
- **Zhongtai server**: runs prebuilt binary only, reads local staging CSVs, writes logs + CSV outputs, does not send WeCom directly

Key scripts: `scripts/push_reference_data_to_zhongtai.sh`, `scripts/pull_zhongtai_outputs.sh`, `scripts/relay_zhongtai_notifications.py`
