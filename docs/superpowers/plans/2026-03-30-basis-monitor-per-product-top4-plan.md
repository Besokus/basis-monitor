# Basis Monitor Per-Product Top4 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a CSV-driven intraday basis monitor that selects `IC / IF / IH / IM` top-4 contracts by yesterday turnover for each product, subscribes those contracts through CTP, computes annualized basis against yesterday's index close, and alerts when annualized basis turns negative.

**Architecture:** Keep the existing CTP market-data, calculation, alert-dedup, storage, and logging foundations, but replace the old manual contract/spot workflow with a new data-selection layer that reads four local CSV datasets. The app will construct a monitored-contract set from daily files, subscribe the resulting instrument list, compute grouped results in real time, and write grouped operational outputs for trading desk use.

**Tech Stack:** C++17, CMake, STL file I/O and parsing, existing CTP MD library, CTest, PowerShell/shell smoke verification on Linux.

---

## File Structure

### Runtime Config
- Modify: `basis_monitor/include/basis_monitor/config/app_config.h`
- Modify: `basis_monitor/src/config/config_loader.cpp`
- Modify: `basis_monitor/tests/test_config_loader.cpp`

Responsibility:
- Reduce startup config to CTP + alert settings
- Stop requiring manual `contracts.json` and `spot_price.json` for the formal workflow
- Keep compatibility defaults where practical

### Data Selection Layer
- Create: `basis_monitor/include/basis_monitor/data/reference_data_types.h`
- Create: `basis_monitor/include/basis_monitor/data/reference_data_loader.h`
- Create: `basis_monitor/include/basis_monitor/data/contract_selector.h`
- Create: `basis_monitor/src/data/reference_data_loader.cpp`
- Create: `basis_monitor/src/data/contract_selector.cpp`
- Create: `basis_monitor/tests/test_reference_data_loader.cpp`
- Create: `basis_monitor/tests/test_contract_selector.cpp`
- Create: `basis_monitor/tests/fixtures/data/all_instruments_future.csv`
- Create: `basis_monitor/tests/fixtures/data/all_instruments_indx.csv`
- Create: `basis_monitor/tests/fixtures/data/eod_future.csv`
- Create: `basis_monitor/tests/fixtures/data/eod_indx.csv`

Responsibility:
- Read the latest metadata files and the latest available previous-trading-day EOD files from the four required datasets
- Parse only the fields needed for contract selection and basis calculation
- Select per-product top-4 real month contracts for `IC / IF / IH / IM`

### Domain Models
- Create: `basis_monitor/include/basis_monitor/domain/monitored_contract.h`
- Modify: `basis_monitor/include/basis_monitor/domain/alert_event.h`

Responsibility:
- Represent the final monitoring contract with product group, mapped index, index name, yesterday close, and maturity date
- Carry enough alert context for grouped output and persistence

### Monitor Service
- Modify: `basis_monitor/include/basis_monitor/monitor/basis_monitor_service.h`
- Modify: `basis_monitor/src/monitor/basis_monitor_service.cpp`
- Modify: `basis_monitor/tests/test_basis_monitor_service.cpp`

Responsibility:
- Replace the old `spot + static contracts` model with `MonitoredContract`
- Compute basis using yesterday index close
- Return enough information for grouped display, storage, and alert handling

### App Wiring
- Modify: `basis_monitor/app/main.cpp`
- Modify: `basis_monitor/CMakeLists.txt`

Responsibility:
- Build the monitored-contract set from CSV files at startup
- Subscribe all selected contracts through CTP
- Display grouped per-product results
- Connect storage and `LogAlert(...)` output into the runtime path

### Storage and Logging Integration
- Modify: `basis_monitor/src/storage/tick_store.cpp`
- Modify: `basis_monitor/src/storage/basis_result_store.cpp`
- Modify: `basis_monitor/src/storage/alert_store.cpp`
- Modify: `basis_monitor/tests/test_basis_result_store.cpp`
- Modify: `basis_monitor/tests/test_alert_store.cpp`
- Modify: `basis_monitor/include/basis_monitor/logging/logger.h`
- Modify: `basis_monitor/src/logging/logger.cpp`
- Modify: `basis_monitor/tests/test_logger.cpp`

Responsibility:
- Persist grouped monitoring outputs using the new monitored-contract metadata
- Ensure alerts land in alert log and runtime log

### CTP Robustness
- Modify: `basis_monitor/include/basis_monitor/ctp/md_spi_bridge.h`
- Modify: `basis_monitor/src/ctp/md_spi_bridge.cpp`
- Modify: `basis_monitor/src/ctp/md_api_session.cpp`
- Modify: `basis_monitor/tests/verify_market_data_migration.ps1`

Responsibility:
- Fail fast when request return codes are negative
- Aggregate `OnRspSubMarketData` results across all subscribed contracts instead of trusting only the last callback

### Documentation and Operations
- Modify: `basis_monitor/README.md`

Responsibility:
- Document CSV locations, per-product top-4 logic, grouped output, and Linux verification

## Task 1: Reframe Startup Config Around CTP and Alerts

**Files:**
- Modify: `basis_monitor/include/basis_monitor/config/app_config.h`
- Modify: `basis_monitor/src/config/config_loader.cpp`
- Modify: `basis_monitor/tests/test_config_loader.cpp`

- [ ] **Step 1: Extend config-loader tests to express the new startup contract**

Add tests for:
- loading CTP + alert config without requiring `contracts.json`
- loading CTP + alert config without requiring `spot_price.json`
- preserving defaults if legacy files are absent

- [ ] **Step 2: Run the config-loader test to verify failure**

Run: `ctest --test-dir basis_monitor/build -R test_config_loader --output-on-failure`
Expected: FAIL because the loader still requires legacy files.

- [ ] **Step 3: Implement the minimal config change**

Rules:
- keep `ctp.ini` and `alert.json`
- make `contracts.json` and `spot_price.json` optional or legacy-only
- do not break existing `AlertConfig`

- [ ] **Step 4: Re-run the targeted config-loader test**

Run: `ctest --test-dir basis_monitor/build -R test_config_loader --output-on-failure`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add basis_monitor/include/basis_monitor/config/app_config.h basis_monitor/src/config/config_loader.cpp basis_monitor/tests/test_config_loader.cpp
git commit -m "refactor: drop legacy contract spot startup requirement"
```

## Task 2: Implement CSV Reference Data Loading

**Files:**
- Create: `basis_monitor/include/basis_monitor/data/reference_data_types.h`
- Create: `basis_monitor/include/basis_monitor/data/reference_data_loader.h`
- Create: `basis_monitor/src/data/reference_data_loader.cpp`
- Create: `basis_monitor/tests/test_reference_data_loader.cpp`
- Create: `basis_monitor/tests/fixtures/data/all_instruments_future.csv`
- Create: `basis_monitor/tests/fixtures/data/all_instruments_indx.csv`
- Create: `basis_monitor/tests/fixtures/data/eod_future.csv`
- Create: `basis_monitor/tests/fixtures/data/eod_indx.csv`
- Modify: `basis_monitor/CMakeLists.txt`

- [ ] **Step 1: Write the failing reference-data loader test**

Cover:
- parsing future metadata rows including `maturity_date` and `underlying_order_book_id`
- parsing index metadata rows including index names
- parsing future EOD rows including `total_turnover`
- parsing index EOD rows including `close`
- resolving “yesterday” EOD files rather than an arbitrary latest file when multiple dates exist

- [ ] **Step 2: Run the new test target to verify failure**

Run: `cmake --build basis_monitor/build --target test_reference_data_loader`
Expected: FAIL because the loader types do not exist yet.

- [ ] **Step 3: Implement the minimal data record types and loader**

Rules:
- parse only required columns
- keep CSV handling STL-only
- keep file-discovery and row-parsing in one focused module
- metadata files may use latest available date, but `eod_price/Future` and `eod_price/INDX` must resolve to the latest available previous-trading-day dataset

- [ ] **Step 4: Re-run the targeted loader test**

Run: `ctest --test-dir basis_monitor/build -R test_reference_data_loader --output-on-failure`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add basis_monitor/include/basis_monitor/data basis_monitor/src/data/reference_data_loader.cpp basis_monitor/tests/test_reference_data_loader.cpp basis_monitor/tests/fixtures/data basis_monitor/CMakeLists.txt
git commit -m "feat: add basis monitor csv reference data loader"
```

## Task 3: Implement Per-Product Top4 Contract Selection

**Files:**
- Create: `basis_monitor/include/basis_monitor/data/contract_selector.h`
- Create: `basis_monitor/src/data/contract_selector.cpp`
- Create: `basis_monitor/include/basis_monitor/domain/monitored_contract.h`
- Create: `basis_monitor/tests/test_contract_selector.cpp`
- Modify: `basis_monitor/CMakeLists.txt`

- [ ] **Step 1: Write the failing selector test**

Cover:
- only `IC / IF / IH / IM` contracts are considered
- only real month contracts are considered
- continuous contracts such as `88/888/889/99/88A2/88A3` are excluded
- each product returns up to 4 contracts sorted by yesterday `total_turnover`
- mapped index code, index name, and yesterday close are attached
- contracts missing `maturity_date` or missing index mapping are skipped with a warning result rather than entering the monitored set

- [ ] **Step 2: Run the selector test to verify failure**

Run: `cmake --build basis_monitor/build --target test_contract_selector`
Expected: FAIL because the selector does not exist yet.

- [ ] **Step 3: Implement the minimal selector**

Rules:
- split by product group first
- take top-4 inside each group
- tolerate groups with fewer than 4 valid contracts
- explicitly surface skipped rows caused by missing maturity or missing index mapping so the app can warn

- [ ] **Step 4: Re-run the selector test**

Run: `ctest --test-dir basis_monitor/build -R test_contract_selector --output-on-failure`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add basis_monitor/include/basis_monitor/data/contract_selector.h basis_monitor/src/data/contract_selector.cpp basis_monitor/include/basis_monitor/domain/monitored_contract.h basis_monitor/tests/test_contract_selector.cpp basis_monitor/CMakeLists.txt
git commit -m "feat: add per-product top4 contract selection"
```

## Task 4: Adapt the Monitor Service to Monitored Contracts

**Files:**
- Modify: `basis_monitor/include/basis_monitor/monitor/basis_monitor_service.h`
- Modify: `basis_monitor/src/monitor/basis_monitor_service.cpp`
- Modify: `basis_monitor/tests/test_basis_monitor_service.cpp`
- Modify: `basis_monitor/include/basis_monitor/domain/alert_event.h`

- [ ] **Step 1: Extend the service test first**

Cover:
- monitored contracts are found by instrument id
- basis uses yesterday index close rather than `spot_price.json`
- expired contracts produce `0%` and no alert
- invalid `index_close_yesterday <= 0` skips calculation and produces a warning-friendly outcome
- negative annualized basis still enters alert state
- recovery still works

- [ ] **Step 2: Run the service test to verify failure**

Run: `ctest --test-dir basis_monitor/build -R test_basis_monitor_service --output-on-failure`
Expected: FAIL because the service still expects `SpotPriceConfig` + static contracts.

- [ ] **Step 3: Implement the minimal service refactor**

Rules:
- replace the old `spot_` + `contracts_` model with `MonitoredContract`
- return enough metadata for output and persistence
- keep alert-dedup behavior unchanged
- preserve a distinct “invalid baseline data” path so the app can warn without crashing

- [ ] **Step 4: Re-run the service test**

Run: `ctest --test-dir basis_monitor/build -R test_basis_monitor_service --output-on-failure`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add basis_monitor/include/basis_monitor/monitor/basis_monitor_service.h basis_monitor/src/monitor/basis_monitor_service.cpp basis_monitor/tests/test_basis_monitor_service.cpp basis_monitor/include/basis_monitor/domain/alert_event.h
git commit -m "refactor: use monitored contracts in basis monitor service"
```

## Task 5: Wire CSV Selection and Subscription Into the App

**Files:**
- Modify: `basis_monitor/app/main.cpp`
- Modify: `basis_monitor/CMakeLists.txt`

- [ ] **Step 1: Write a failing startup smoke check**

Express these expectations:
- app discovers latest metadata files and latest available previous-trading-day EOD files
- app builds a monitored-contract set
- app subscribes only the selected instruments
- app can start even if a product group has fewer than 4 valid contracts, while warning
- app fails startup with a clear error if yesterday `eod_price/Future` data cannot be found
- app logs warnings when candidate contracts are skipped due to missing maturity date or missing index mapping

- [ ] **Step 2: Run the smoke check or targeted test to verify failure**

Expected: FAIL because the app still uses the old static config path.

- [ ] **Step 3: Implement app startup wiring**

`app/main.cpp` should:
- load CTP + alert config
- locate latest metadata files and previous-trading-day EOD files
- build `MonitoredContract` lists for `IC / IF / IH / IM`
- flatten the selected instrument ids into the subscription list
- initialize the session using that derived list
- fail fast if yesterday futures EOD data is unavailable

- [ ] **Step 4: Re-run the startup check**

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add basis_monitor/app/main.cpp basis_monitor/CMakeLists.txt
git commit -m "feat: wire csv-driven per-product subscriptions"
```

## Task 6: Connect Grouped Output, Storage, and Alerts

**Files:**
- Modify: `basis_monitor/app/main.cpp`
- Modify: `basis_monitor/src/storage/basis_result_store.cpp`
- Modify: `basis_monitor/src/storage/alert_store.cpp`
- Modify: `basis_monitor/tests/test_basis_result_store.cpp`
- Modify: `basis_monitor/tests/test_alert_store.cpp`
- Modify: `basis_monitor/tests/test_logger.cpp`

- [ ] **Step 1: Write failing tests for grouped result persistence and alert persistence**

Cover:
- basis result rows include `product_group`, `index_code`, `index_name`
- alert rows include enough context to identify the product and contract
- grouped terminal output uses per-product sections

- [ ] **Step 2: Run the targeted tests to verify failure**

Run: `ctest --test-dir basis_monitor/build -R "test_basis_result_store|test_alert_store|test_logger" --output-on-failure`
Expected: FAIL because the stores and output still reflect the older schema.

- [ ] **Step 3: Implement grouped output and persistence**

Rules:
- `LogAlert(...)` handles alert/recovery output
- `basis_result_store` stores product/index metadata
- `alert_store` stores product/index metadata and transition reason
- terminal output is grouped by `IC / IF / IH / IM`

- [ ] **Step 4: Re-run the targeted tests**

Run: `ctest --test-dir basis_monitor/build -R "test_basis_result_store|test_alert_store|test_logger" --output-on-failure`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add basis_monitor/app/main.cpp basis_monitor/src/storage/basis_result_store.cpp basis_monitor/src/storage/alert_store.cpp basis_monitor/tests/test_basis_result_store.cpp basis_monitor/tests/test_alert_store.cpp basis_monitor/tests/test_logger.cpp
git commit -m "feat: add grouped output and alert persistence"
```

## Task 7: Harden CTP Startup and Subscription Handling

**Files:**
- Modify: `basis_monitor/include/basis_monitor/ctp/md_spi_bridge.h`
- Modify: `basis_monitor/src/ctp/md_spi_bridge.cpp`
- Modify: `basis_monitor/src/ctp/md_api_session.cpp`
- Modify: `basis_monitor/tests/verify_market_data_migration.ps1`

- [ ] **Step 1: Write a focused failing check for request/subscribe robustness**

Express:
- negative return codes from `ReqUserLogin` or `SubscribeMarketData` do not lead to infinite waits
- subscription success is aggregated across all subscribed contracts rather than trusting only the last callback

- [ ] **Step 2: Run the check to verify failure**

Expected: FAIL because startup still assumes request return codes are fine and the last subscription callback is authoritative.

- [ ] **Step 3: Implement the minimal robustness fix**

Rules:
- record request return status explicitly
- fail fast on negative request returns
- aggregate subscription errors across all callbacks

- [ ] **Step 4: Re-run the check**

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add basis_monitor/include/basis_monitor/ctp/md_spi_bridge.h basis_monitor/src/ctp/md_spi_bridge.cpp basis_monitor/src/ctp/md_api_session.cpp basis_monitor/tests/verify_market_data_migration.ps1
git commit -m "fix: harden ctp subscription startup flow"
```

## Task 8: Linux Validation and Operations Documentation

**Files:**
- Modify: `basis_monitor/README.md`

- [ ] **Step 1: Update README for the new workflow**

Document:
- required CSV directories
- latest-file discovery behavior
- per-product top-4 selection rules
- grouped output format
- alert behavior
- Linux build/run steps

- [ ] **Step 2: Run a clean Linux build**

Run:
- `cmake -S basis_monitor -B basis_monitor/build`
- `cmake --build basis_monitor/build`

Expected: configure and build succeed.

- [ ] **Step 3: Run the full test suite**

Run: `ctest --test-dir basis_monitor/build --output-on-failure`
Expected: all tests pass.

- [ ] **Step 4: Manual runtime verification with real data**

Run:
- `sh basis_monitor/run.sh`

Verify:
- selection output shows `IC / IF / IH / IM` groups
- each group contains up to 4 contracts
- subscribed instruments match the selected list
- runtime log is written
- alert log is written
- basis result file is written

- [ ] **Step 5: Commit**

```bash
git add basis_monitor/README.md
git commit -m "docs: add per-product top4 operations guide"
```

## Done Criteria
- The app automatically selects `IC / IF / IH / IM` top-4 contracts by yesterday turnover
- The app subscribes only those selected contracts through CTP
- The app computes basis and annualized basis using yesterday index close and contract maturity date
- Output is grouped by `IC / IF / IH / IM`
- Negative annualized basis produces deduplicated alerts and recovery logs
- Runtime log, alert log, and basis result outputs are all written
- Linux build, tests, and real-data smoke verification pass
