# Basis Monitor Gap Closure Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the remaining gaps so `basis_monitor` can continuously receive market data, compute intraday annualized basis, persist monitoring evidence, and emit separated operational and alert logs.

**Architecture:** Keep the existing `basis_monitor` market-data session and calculation path intact, and add only the missing production-facing pieces around it: storage writers, alert configuration, log separation, and end-to-end runtime wiring. Preserve the current standalone-project boundary and avoid touching `6.7.11apidemo/6.6.5_demo`.

**Tech Stack:** C++17, CMake, STL file I/O, existing CTP MD library, CTest, PowerShell/shell smoke verification.

---

## File Structure

### Config
- Modify: `basis_monitor/include/basis_monitor/config/app_config.h`
- Modify: `basis_monitor/include/basis_monitor/config/config_loader.h`
- Modify: `basis_monitor/src/config/config_loader.cpp`
- Create: `basis_monitor/config/alert.json`
- Create: `basis_monitor/tests/fixtures/config/alert.json`
- Modify: `basis_monitor/tests/test_config_loader.cpp`

Responsibility:
- Add alert-policy config
- Load three runtime concerns explicitly: operational logging, alert logging, storage behavior

### Storage
- Create: `basis_monitor/include/basis_monitor/storage/tick_store.h`
- Create: `basis_monitor/include/basis_monitor/storage/basis_result_store.h`
- Create: `basis_monitor/include/basis_monitor/storage/alert_store.h`
- Create: `basis_monitor/src/storage/tick_store.cpp`
- Create: `basis_monitor/src/storage/basis_result_store.cpp`
- Create: `basis_monitor/src/storage/alert_store.cpp`
- Create: `basis_monitor/tests/test_tick_store.cpp`
- Create: `basis_monitor/tests/test_basis_result_store.cpp`
- Create: `basis_monitor/tests/test_alert_store.cpp`

Responsibility:
- Persist raw ticks
- Persist basis-monitor results
- Persist alert/recovery events

### Logging
- Modify: `basis_monitor/include/basis_monitor/logging/logger.h`
- Modify: `basis_monitor/src/logging/logger.cpp`

Responsibility:
- Keep runtime log and alert log separate
- Preserve current terminal output behavior

### Monitor Wiring
- Modify: `basis_monitor/include/basis_monitor/domain/alert_event.h`
- Modify: `basis_monitor/include/basis_monitor/monitor/basis_monitor_service.h`
- Modify: `basis_monitor/src/monitor/basis_monitor_service.cpp`
- Modify: `basis_monitor/tests/test_basis_monitor_service.cpp`

Responsibility:
- Return enough data to persist result rows and alert rows
- Keep existing negative-entry and recovery semantics

### App Wiring
- Modify: `basis_monitor/CMakeLists.txt`
- Modify: `basis_monitor/app/main.cpp`
- Modify: `basis_monitor/README.md`
- Create: `basis_monitor/data/.gitkeep`
- Create: `basis_monitor/logs/.gitkeep`

Responsibility:
- Wire config, stores, runtime directories, and separated logs together
- Make the app produce the three outputs expected for operations

### Verification
- Modify: `basis_monitor/tests/verify_market_data_migration.ps1`

Responsibility:
- Check that required config and output paths now exist
- Check that separated logs and storage classes are wired in

## Task 1: Add Alert Policy Config and Runtime Output Settings

**Files:**
- Modify: `basis_monitor/include/basis_monitor/config/app_config.h`
- Modify: `basis_monitor/src/config/config_loader.cpp`
- Create: `basis_monitor/config/alert.json`
- Create: `basis_monitor/tests/fixtures/config/alert.json`
- Modify: `basis_monitor/tests/test_config_loader.cpp`

- [ ] **Step 1: Extend config models**

Add:
- `struct AlertConfig`
- booleans for terminal alert and file alert
- output file names or relative paths if you want them configurable

- [ ] **Step 2: Write the failing config-loader assertions**

Add assertions in `basis_monitor/tests/test_config_loader.cpp` for:
- alert config file loads
- terminal/file alert switches parse correctly

- [ ] **Step 3: Run config-loader test to verify failure**

Run: `ctest --test-dir basis_monitor/build -R test_config_loader --output-on-failure`
Expected: FAIL because alert config is not loaded yet.

- [ ] **Step 4: Implement minimal alert-config loading**

Load `alert.json` with fixed-schema parsing only. Keep the parser simple and aligned with the existing regex-based loader approach.

- [ ] **Step 5: Re-run config-loader test**

Run: `ctest --test-dir basis_monitor/build -R test_config_loader --output-on-failure`
Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add basis_monitor/include/basis_monitor/config/app_config.h basis_monitor/src/config/config_loader.cpp basis_monitor/config/alert.json basis_monitor/tests/fixtures/config/alert.json basis_monitor/tests/test_config_loader.cpp
git commit -m "feat: add basis monitor alert config"
```

## Task 2: Implement Tick, Result, and Alert Persistence

**Files:**
- Create: `basis_monitor/include/basis_monitor/storage/tick_store.h`
- Create: `basis_monitor/include/basis_monitor/storage/basis_result_store.h`
- Create: `basis_monitor/include/basis_monitor/storage/alert_store.h`
- Create: `basis_monitor/src/storage/tick_store.cpp`
- Create: `basis_monitor/src/storage/basis_result_store.cpp`
- Create: `basis_monitor/src/storage/alert_store.cpp`
- Create: `basis_monitor/tests/test_tick_store.cpp`
- Create: `basis_monitor/tests/test_basis_result_store.cpp`
- Create: `basis_monitor/tests/test_alert_store.cpp`

- [ ] **Step 1: Write the failing tick-store test**

Validate:
- file is created
- header is written once
- appended row contains `instrument_id` and `last_price`

- [ ] **Step 2: Write the failing basis-result-store test**

Validate:
- file is created
- header is written once
- row contains contract, spot, future, basis, annual rate, remaining days

- [ ] **Step 3: Write the failing alert-store test**

Validate:
- file is created
- row contains contract, annual rate, transition, reason

- [ ] **Step 4: Run store tests to verify failure**

Run: `cmake --build basis_monitor/build --target test_tick_store test_basis_result_store test_alert_store`
Expected: FAIL because store classes do not exist yet.

- [ ] **Step 5: Implement minimal CSV/append stores**

Rules:
- Use append mode
- Write header row only when file is new or empty
- Keep format simple CSV
- Use runtime-relative outputs under `data/`

- [ ] **Step 6: Re-run store tests**

Run: `ctest --test-dir basis_monitor/build -R "test_tick_store|test_basis_result_store|test_alert_store" --output-on-failure`
Expected: PASS

- [ ] **Step 7: Commit**

```bash
git add basis_monitor/include/basis_monitor/storage basis_monitor/src/storage basis_monitor/tests/test_tick_store.cpp basis_monitor/tests/test_basis_result_store.cpp basis_monitor/tests/test_alert_store.cpp
git commit -m "feat: add basis monitor persistence"
```

## Task 3: Split Runtime Log and Alert Log

**Files:**
- Modify: `basis_monitor/include/basis_monitor/logging/logger.h`
- Modify: `basis_monitor/src/logging/logger.cpp`

- [ ] **Step 1: Write the failing logger test or smoke check**

Add a focused test or simple executable check that:
- runtime log receives normal messages
- alert log receives alert/recovery messages
- terminal output is preserved

- [ ] **Step 2: Run it to verify failure**

Run the targeted logger check.
Expected: FAIL because only one log file exists today.

- [ ] **Step 3: Implement dual-log behavior**

Expose APIs such as:
- `InitializeLogger(runtime_log_path, alert_log_path)`
- `Log(...)`
- `LogAlert(...)`

Behavior:
- `Log` writes terminal + runtime log
- `LogAlert` writes terminal + runtime log + alert log

- [ ] **Step 4: Re-run logger verification**

Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add basis_monitor/include/basis_monitor/logging/logger.h basis_monitor/src/logging/logger.cpp
git commit -m "feat: split runtime and alert logs"
```

## Task 4: Wire Stores into BasisMonitorService and App

**Files:**
- Modify: `basis_monitor/include/basis_monitor/domain/alert_event.h`
- Modify: `basis_monitor/include/basis_monitor/monitor/basis_monitor_service.h`
- Modify: `basis_monitor/src/monitor/basis_monitor_service.cpp`
- Modify: `basis_monitor/tests/test_basis_monitor_service.cpp`
- Modify: `basis_monitor/app/main.cpp`
- Modify: `basis_monitor/CMakeLists.txt`

- [ ] **Step 1: Extend service tests first**

Add assertions covering:
- every valid tick produces a basis result ready for persistence
- negative annual-rate transition produces an alert record
- recovery produces a recovery record

- [ ] **Step 2: Run service test to verify failure**

Run: `ctest --test-dir basis_monitor/build -R test_basis_monitor_service --output-on-failure`
Expected: FAIL because persistence-facing fields are missing.

- [ ] **Step 3: Implement minimal service contract changes**

Keep `BasisMonitorService` calculation-focused, but return enough metadata for the app layer to:
- append tick rows
- append basis result rows
- append alert rows

- [ ] **Step 4: Wire stores into `app/main.cpp`**

On each tick:
1. append raw tick
2. call monitor service
3. append basis result if valid
4. call `LogAlert` and append alert row on `EnteredNegative`
5. call `LogAlert` and append alert row on `Recovered`

- [ ] **Step 5: Re-run service tests**

Run: `ctest --test-dir basis_monitor/build -R test_basis_monitor_service --output-on-failure`
Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add basis_monitor/include/basis_monitor/domain/alert_event.h basis_monitor/include/basis_monitor/monitor/basis_monitor_service.h basis_monitor/src/monitor/basis_monitor_service.cpp basis_monitor/tests/test_basis_monitor_service.cpp basis_monitor/app/main.cpp basis_monitor/CMakeLists.txt
git commit -m "feat: wire monitoring outputs into basis monitor app"
```

## Task 5: Add Runtime Directories, Sample Files, and Smoke Checks

**Files:**
- Create: `basis_monitor/data/.gitkeep`
- Create: `basis_monitor/logs/.gitkeep`
- Modify: `basis_monitor/tests/verify_market_data_migration.ps1`
- Modify: `basis_monitor/README.md`

- [ ] **Step 1: Update migration smoke script**

Check for:
- `alert.json`
- storage headers/sources
- dual-log API usage
- `data/` and `logs/` runtime paths

- [ ] **Step 2: Update README**

Document:
- required config files
- meaning of the three outputs
- where to look for runtime log
- where to look for alert log
- where to look for basis/tick CSV files
- phase-1 limitation that spot price is still local-file driven

- [ ] **Step 3: Build and run full test suite**

Run:
- `cmake -S basis_monitor -B basis_monitor/build`
- `cmake --build basis_monitor/build`
- `ctest --test-dir basis_monitor/build --output-on-failure`

Expected:
- configure succeeds
- build succeeds
- all tests pass

- [ ] **Step 4: Run migration smoke check**

Run: `powershell -ExecutionPolicy Bypass -File basis_monitor/tests/verify_market_data_migration.ps1`
Expected: PASS

- [ ] **Step 5: Manual runtime verification**

Run from Linux/WSL:
- `sh basis_monitor/run.sh`

Expected outputs:
- runtime log under `basis_monitor/logs/`
- alert log under `basis_monitor/logs/`
- tick CSV under `basis_monitor/data/`
- basis-result CSV under `basis_monitor/data/`
- `[ALERT]` printed once on entering negative annual basis
- `[RECOVERY]` printed once on recovery

- [ ] **Step 6: Commit**

```bash
git add basis_monitor/data/.gitkeep basis_monitor/logs/.gitkeep basis_monitor/tests/verify_market_data_migration.ps1 basis_monitor/README.md
git commit -m "docs: add basis monitor runtime outputs and verification guide"
```

## Done Criteria
- The app can keep receiving configured futures ticks through the standalone `basis_monitor` session
- Every valid tick can produce a persisted basis-monitor result
- Negative annualized basis produces a deduplicated alert and recovery record
- Three operational outputs exist and are easy to inspect:
  - runtime log
  - alert log
  - monitoring data files (tick + basis result)
- The old `6.6.5_demo` remains untouched
