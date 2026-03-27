# Basis Monitor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a standalone `basis_monitor` project that safely migrates the proven CTP market-data receiving path from `6.6.5_demo`, stores live futures ticks, computes intraday annualized basis against locally supplied spot prices, and emits alerts when annualized basis turns negative.

**Architecture:** Keep the existing `6.6.5_demo` untouched as the fallback baseline. Build a new Linux-first project under repo-root `basis_monitor` as a sibling of the inner `6.7.11apidemo` directory, with clear module boundaries: config, logging, domain, storage, monitor, and CTP market-data integration. First prove the new project can independently reach `OnRtnDepthMarketData`, then layer on persistence, basis calculation, alert deduplication, and runtime scripts.

**Tech Stack:** C++17, CMake, existing local CTP market-data headers/libs, existing Linux compatibility helpers, STL-only test executables via CTest, PowerShell/shell verification scripts.

---

## File Structure

### New Project Root
- Create: `basis_monitor/CMakeLists.txt`
- Create: `basis_monitor/README.md`
- Create: `basis_monitor/app/main.cpp`

### Config Module
- Create: `basis_monitor/include/basis_monitor/config/app_config.h`
- Create: `basis_monitor/include/basis_monitor/config/config_loader.h`
- Create: `basis_monitor/src/config/config_loader.cpp`

Responsibility:
- Load `ctp.ini`
- Load `contracts.json`
- Load `spot_price.json`
- Load `alert.json`
- Resolve runtime paths relative to the new project instead of `6.6.5_demo`

### Logging Module
- Create: `basis_monitor/include/basis_monitor/logging/logger.h`
- Create: `basis_monitor/src/logging/logger.cpp`

Responsibility:
- Console + file logging
- Stable tags: `[MARKET_DATA_OK]`, `[MD_TICK]`, `[BASIS_MONITOR]`, `[ALERT]`, `[RECOVERY]`
- Separate normal runtime log and alert log

### Domain Module
- Create: `basis_monitor/include/basis_monitor/domain/market_tick.h`
- Create: `basis_monitor/include/basis_monitor/domain/contract_definition.h`
- Create: `basis_monitor/include/basis_monitor/domain/basis_result.h`
- Create: `basis_monitor/include/basis_monitor/domain/alert_event.h`

Responsibility:
- Internal data contracts only
- No CTP headers in domain types

### Monitor Module
- Create: `basis_monitor/include/basis_monitor/monitor/basis_calculator.h`
- Create: `basis_monitor/include/basis_monitor/monitor/alert_engine.h`
- Create: `basis_monitor/include/basis_monitor/monitor/basis_monitor_service.h`
- Create: `basis_monitor/src/monitor/basis_calculator.cpp`
- Create: `basis_monitor/src/monitor/alert_engine.cpp`
- Create: `basis_monitor/src/monitor/basis_monitor_service.cpp`

Responsibility:
- Compute `basis` and `annual_rate`
- Match tick instrument to configured expiry
- Emit negative-basis alerts with deduplication and recovery logs

### Storage Module
- Create: `basis_monitor/include/basis_monitor/storage/tick_store.h`
- Create: `basis_monitor/include/basis_monitor/storage/basis_result_store.h`
- Create: `basis_monitor/include/basis_monitor/storage/alert_store.h`
- Create: `basis_monitor/src/storage/tick_store.cpp`
- Create: `basis_monitor/src/storage/basis_result_store.cpp`
- Create: `basis_monitor/src/storage/alert_store.cpp`

Responsibility:
- Append CSV rows
- Ensure headers are written once
- Keep runtime files under `basis_monitor/data` and `basis_monitor/logs`

### CTP Module
- Create: `basis_monitor/include/basis_monitor/ctp/md_listener.h`
- Create: `basis_monitor/include/basis_monitor/ctp/md_api_session.h`
- Create: `basis_monitor/include/basis_monitor/ctp/md_spi_bridge.h`
- Create: `basis_monitor/src/ctp/md_api_session.cpp`
- Create: `basis_monitor/src/ctp/md_spi_bridge.cpp`
- Create: `basis_monitor/include/basis_monitor/platform/linux_compat.h`

Responsibility:
- Rebuild the minimal market-data-only path from the demo
- Keep CTP callback code isolated from basis business logic
- Retain event synchronization behavior that already worked in the demo

### Runtime Assets
- Create: `basis_monitor/config/ctp.ini`
- Create: `basis_monitor/config/contracts.json`
- Create: `basis_monitor/config/spot_price.json`
- Create: `basis_monitor/config/alert.json`
- Create: `basis_monitor/resources/error.xml`
- Create: `basis_monitor/resources/error.dtd`
- Create: `basis_monitor/runtime/flow/.gitkeep`
- Create: `basis_monitor/data/.gitkeep`
- Create: `basis_monitor/logs/.gitkeep`
- Create: `basis_monitor/scripts/run.sh`
- Create: `basis_monitor/scripts/start.sh`

Responsibility:
- Runtime data belongs to the new project only
- No accidental reads/writes back into `6.6.5_demo`

### Vendor Assets
- Create: `basis_monitor/vendor/ctp/include/ThostFtdcMdApi.h`
- Create: `basis_monitor/vendor/ctp/include/ThostFtdcUserApiDataType.h`
- Create: `basis_monitor/vendor/ctp/include/ThostFtdcUserApiStruct.h`
- Create: `basis_monitor/vendor/ctp/lib/linux/thostmduserapi_se.so`

Responsibility:
- Copy only the market-data side first
- Do not migrate trader-side files unless a real dependency appears

### Tests
- Create: `basis_monitor/tests/test_basis_calculator.cpp`
- Create: `basis_monitor/tests/test_alert_engine.cpp`
- Create: `basis_monitor/tests/test_config_loader.cpp`
- Create: `basis_monitor/tests/test_tick_store.cpp`
- Create: `basis_monitor/tests/test_basis_monitor_service.cpp`
- Create: `basis_monitor/tests/fixtures/config/ctp.ini`
- Create: `basis_monitor/tests/fixtures/config/contracts.json`
- Create: `basis_monitor/tests/fixtures/config/spot_price.json`
- Create: `basis_monitor/tests/fixtures/config/alert.json`
- Create: `basis_monitor/tests/verify_market_data_smoke.ps1`

Responsibility:
- Fast local unit coverage for calculation, config, storage, and alert dedupe
- Manual/smoke verification for live CTP market-data path

## Execution Notes
- Linux-first runtime is the target for `basis_monitor`; keep logic portable where cheap, but do not expand scope to Windows runtime packaging in phase 1.
- The current repo has no JSON dependency. Implement only the minimal fixed-schema JSON parsing needed for the four config files, and lock it down with fixture tests.
- Keep `6.7.11apidemo/6.6.5_demo` unchanged during migration except for reading it as reference.
- Do not copy all of `main.h`. Re-express only the minimal MD login/subscription/callback path in focused files.

### Task 1: Scaffold the Standalone Project and Build Graph

**Files:**
- Create: `basis_monitor/CMakeLists.txt`
- Create: `basis_monitor/README.md`
- Create: `basis_monitor/app/main.cpp`
- Create: `basis_monitor/tests/test_basis_calculator.cpp`

- [ ] **Step 1: Create the directory skeleton and placeholder files**

Create the directories listed in the File Structure section and add placeholder source files that compile but do not yet implement business logic.

- [ ] **Step 2: Add a minimal CMake target layout**

```cmake
add_executable(basis_monitor app/main.cpp)
target_include_directories(basis_monitor PRIVATE include vendor/ctp/include)

add_executable(test_basis_calculator tests/test_basis_calculator.cpp)
add_test(NAME test_basis_calculator COMMAND test_basis_calculator)
```

- [ ] **Step 3: Add a failing calculator smoke test**

```cpp
#include "basis_monitor/monitor/basis_calculator.h"

int main() {
    auto result = CalculateAnnualizedBasis(6300.0, 6190.0, 23);
    return result.valid ? 0 : 1;
}
```

- [ ] **Step 4: Verify the new project configures and the test target fails for the expected reason**

Run: `cmake -S basis_monitor -B basis_monitor/build`

Expected: configure succeeds.

Run: `cmake --build basis_monitor/build --target test_basis_calculator`

Expected: build fails because `basis_calculator.h` and implementation do not exist yet.

- [ ] **Step 5: Commit**

```bash
git add basis_monitor docs/superpowers/plans/2026-03-27-basis-monitor-implementation-plan.md
git commit -m "chore: scaffold basis monitor project"
```

### Task 2: Implement Domain Types and Basis Calculation

**Files:**
- Create: `basis_monitor/include/basis_monitor/domain/basis_result.h`
- Create: `basis_monitor/include/basis_monitor/monitor/basis_calculator.h`
- Create: `basis_monitor/src/monitor/basis_calculator.cpp`
- Modify: `basis_monitor/tests/test_basis_calculator.cpp`

- [ ] **Step 1: Write the failing basis-calculation tests**

```cpp
int main() {
    {
        auto result = CalculateAnnualizedBasis(6300.0, 6190.0, 23);
        assert(result.valid);
        assert(std::abs(result.basis - 110.0) < 0.001);
        assert(std::abs(result.annual_rate - 27.71) < 0.02);
    }
    {
        auto result = CalculateAnnualizedBasis(6300.0, 6400.0, 23);
        assert(result.valid);
        assert(result.annual_rate < 0.0);
    }
    {
        auto result = CalculateAnnualizedBasis(6300.0, 6230.0, 0);
        assert(!result.valid);
    }
    return 0;
}
```

- [ ] **Step 2: Run the test target and confirm it fails**

Run: `cmake --build basis_monitor/build --target test_basis_calculator`

Expected: build fails or test fails because calculator API is still missing.

- [ ] **Step 3: Implement the minimal calculator**

```cpp
struct BasisResult {
    bool valid = false;
    double basis = 0.0;
    double annual_rate = 0.0;
    int remaining_days = 0;
};

BasisResult CalculateAnnualizedBasis(double spot_price, double future_price, int remaining_days);
```

Implementation rules:
- if `remaining_days <= 0`, return `valid = false`
- if `spot_price <= 0`, return `valid = false`
- otherwise compute:
  - `basis = spot_price - future_price`
  - `annual_rate = basis / spot_price * (365.0 / remaining_days) * 100.0`

- [ ] **Step 4: Run the calculator test and confirm it passes**

Run: `ctest --test-dir basis_monitor/build -R test_basis_calculator --output-on-failure`

Expected: `1/1 tests passed`

- [ ] **Step 5: Commit**

```bash
git add basis_monitor/include/basis_monitor/domain/basis_result.h basis_monitor/include/basis_monitor/monitor/basis_calculator.h basis_monitor/src/monitor/basis_calculator.cpp basis_monitor/tests/test_basis_calculator.cpp
git commit -m "feat: add basis calculation core"
```

### Task 3: Implement Alert State Tracking and Deduplication

**Files:**
- Create: `basis_monitor/include/basis_monitor/domain/alert_event.h`
- Create: `basis_monitor/include/basis_monitor/monitor/alert_engine.h`
- Create: `basis_monitor/src/monitor/alert_engine.cpp`
- Create: `basis_monitor/tests/test_alert_engine.cpp`

- [ ] **Step 1: Write the failing alert-engine tests**

```cpp
int main() {
    AlertEngine engine;
    auto first = engine.Evaluate("IC2604", -1.25);
    assert(first.transition == AlertTransition::EnteredNegative);

    auto repeat = engine.Evaluate("IC2604", -0.50);
    assert(repeat.transition == AlertTransition::None);

    auto recovery = engine.Evaluate("IC2604", 0.80);
    assert(recovery.transition == AlertTransition::Recovered);
    return 0;
}
```

- [ ] **Step 2: Run the alert-engine target and confirm it fails**

Run: `cmake --build basis_monitor/build --target test_alert_engine`

Expected: build fails because alert engine types do not exist yet.

- [ ] **Step 3: Implement the minimal alert engine**

```cpp
enum class AlertTransition {
    None,
    EnteredNegative,
    Recovered,
};

class AlertEngine {
public:
    AlertEvent Evaluate(const std::string& instrument_id, double annual_rate);
private:
    std::unordered_map<std::string, bool> negative_state_;
};
```

Behavior:
- First time `annual_rate < 0` for a contract: emit `EnteredNegative`
- While still `< 0`: emit `None`
- First non-negative tick after a negative period: emit `Recovered`

- [ ] **Step 4: Run the alert-engine test and confirm it passes**

Run: `ctest --test-dir basis_monitor/build -R test_alert_engine --output-on-failure`

Expected: `1/1 tests passed`

- [ ] **Step 5: Commit**

```bash
git add basis_monitor/include/basis_monitor/domain/alert_event.h basis_monitor/include/basis_monitor/monitor/alert_engine.h basis_monitor/src/monitor/alert_engine.cpp basis_monitor/tests/test_alert_engine.cpp
git commit -m "feat: add negative basis alert dedupe"
```

### Task 4: Implement Config Models and Fixed-Schema Loaders

**Files:**
- Create: `basis_monitor/include/basis_monitor/config/app_config.h`
- Create: `basis_monitor/include/basis_monitor/config/config_loader.h`
- Create: `basis_monitor/src/config/config_loader.cpp`
- Create: `basis_monitor/tests/test_config_loader.cpp`
- Create: `basis_monitor/tests/fixtures/config/ctp.ini`
- Create: `basis_monitor/tests/fixtures/config/contracts.json`
- Create: `basis_monitor/tests/fixtures/config/spot_price.json`
- Create: `basis_monitor/tests/fixtures/config/alert.json`

- [ ] **Step 1: Write the failing config-loader tests**

Test cases:
- `ctp.ini` loads `FrontMdAddr`, `BrokerID`, `UserID`, `Password`
- `contracts.json` loads enabled IC contracts and expiry dates
- `spot_price.json` loads spot price and update timestamp
- `alert.json` loads duplicate suppression flags

```cpp
int main() {
    auto config = LoadAppConfig("tests/fixtures/config");
    assert(config.ctp.front_md_addr == "tcp://119.188.3.11:16113");
    assert(config.contracts.size() == 2);
    assert(config.spot.current_spot_price == 6300.0);
    assert(config.alert.enable_terminal_alert);
    return 0;
}
```

- [ ] **Step 2: Run the config-loader target and confirm it fails**

Run: `cmake --build basis_monitor/build --target test_config_loader`

Expected: build fails because config types and loader do not exist yet.

- [ ] **Step 3: Implement config models and loaders**

Implementation notes:
- Reuse the INI-reading approach from `6.7.11apidemo/6.6.5_demo/getconfig.cpp` for `ctp.ini`
- Do not call `_getch()` or `exit(-1)` inside the new loader
- Return rich error status instead of terminating the process
- Keep JSON parsing limited to the fixed shapes required by:
  - `contracts.json`
  - `spot_price.json`
  - `alert.json`

- [ ] **Step 4: Run the config-loader test and confirm it passes**

Run: `ctest --test-dir basis_monitor/build -R test_config_loader --output-on-failure`

Expected: `1/1 tests passed`

- [ ] **Step 5: Commit**

```bash
git add basis_monitor/include/basis_monitor/config/app_config.h basis_monitor/include/basis_monitor/config/config_loader.h basis_monitor/src/config/config_loader.cpp basis_monitor/tests/test_config_loader.cpp basis_monitor/tests/fixtures/config
git commit -m "feat: add basis monitor config loading"
```

### Task 5: Implement Logger and CSV Storage

**Files:**
- Create: `basis_monitor/include/basis_monitor/logging/logger.h`
- Create: `basis_monitor/src/logging/logger.cpp`
- Create: `basis_monitor/include/basis_monitor/domain/market_tick.h`
- Create: `basis_monitor/include/basis_monitor/domain/contract_definition.h`
- Create: `basis_monitor/include/basis_monitor/storage/tick_store.h`
- Create: `basis_monitor/include/basis_monitor/storage/basis_result_store.h`
- Create: `basis_monitor/include/basis_monitor/storage/alert_store.h`
- Create: `basis_monitor/src/storage/tick_store.cpp`
- Create: `basis_monitor/src/storage/basis_result_store.cpp`
- Create: `basis_monitor/src/storage/alert_store.cpp`
- Create: `basis_monitor/tests/test_tick_store.cpp`

- [ ] **Step 1: Write the failing storage test**

```cpp
int main() {
    TickStore store("tmp_ticks.csv");
    MarketTick tick{};
    tick.instrument_id = "IC2606";
    tick.last_price = 6080.0;
    store.Append(tick);
    assert(FileContains("tmp_ticks.csv", "IC2606"));
    return 0;
}
```

- [ ] **Step 2: Run the storage target and confirm it fails**

Run: `cmake --build basis_monitor/build --target test_tick_store`

Expected: build fails because store and domain types do not exist yet.

- [ ] **Step 3: Implement logger and stores**

Implementation rules:
- `Logger` writes to stdout and a runtime log file
- `AlertStore` writes to a separate alert log file
- CSV writers write header row once
- Keep filenames date-based, for example:
  - `data/ticks_YYYYMMDD.csv`
  - `data/basis_monitor_YYYYMMDD.csv`
  - `logs/alerts_YYYYMMDD.log`

- [ ] **Step 4: Run the storage test and confirm it passes**

Run: `ctest --test-dir basis_monitor/build -R test_tick_store --output-on-failure`

Expected: `1/1 tests passed`

- [ ] **Step 5: Commit**

```bash
git add basis_monitor/include/basis_monitor/logging/logger.h basis_monitor/src/logging/logger.cpp basis_monitor/include/basis_monitor/domain/market_tick.h basis_monitor/include/basis_monitor/domain/contract_definition.h basis_monitor/include/basis_monitor/storage basis_monitor/src/storage basis_monitor/tests/test_tick_store.cpp
git commit -m "feat: add logging and csv persistence"
```

### Task 6: Migrate the Minimal CTP Market-Data Path

**Files:**
- Create: `basis_monitor/include/basis_monitor/ctp/md_listener.h`
- Create: `basis_monitor/include/basis_monitor/ctp/md_api_session.h`
- Create: `basis_monitor/include/basis_monitor/ctp/md_spi_bridge.h`
- Create: `basis_monitor/src/ctp/md_api_session.cpp`
- Create: `basis_monitor/src/ctp/md_spi_bridge.cpp`
- Create: `basis_monitor/include/basis_monitor/platform/linux_compat.h`
- Create: `basis_monitor/vendor/ctp/include/ThostFtdcMdApi.h`
- Create: `basis_monitor/vendor/ctp/include/ThostFtdcUserApiDataType.h`
- Create: `basis_monitor/vendor/ctp/include/ThostFtdcUserApiStruct.h`
- Create: `basis_monitor/vendor/ctp/lib/linux/thostmduserapi_se.so`
- Create: `basis_monitor/tests/verify_market_data_smoke.ps1`

- [ ] **Step 1: Write the market-data smoke verification script**

The script should fail until the new project has all of these:
- a dedicated market-data session class
- a dedicated SPI bridge
- a first-tick success log `[MARKET_DATA_OK]`
- live tick log `[MD_TICK]`
- event-based wait for the first tick in the new app

- [ ] **Step 2: Copy only the MD-relevant vendor assets**

Copy from:
- `6.7.11apidemo/6.6.5_demo/ThostFtdcMdApi.h`
- `6.7.11apidemo/6.6.5_demo/ThostFtdcUserApiDataType.h`
- `6.7.11apidemo/6.6.5_demo/ThostFtdcUserApiStruct.h`
- `6.7.11apidemo/6.6.5_demo/thostmduserapi_se.so`

Do not copy trader API headers or trader `.so` files in this task.

- [ ] **Step 3: Port the Linux compatibility helpers**

Start from:
- `6.7.11apidemo/6.6.5_demo/linux_compat.h`

Keep only the pieces needed by the new project:
- `HANDLE`
- `CreateEvent`
- `SetEvent`
- `ResetEvent`
- `WaitForSingleObject`

- [ ] **Step 4: Implement the CTP session and SPI bridge**

Port behavior from:
- `6.7.11apidemo/6.6.5_demo/main.cpp`
- `6.7.11apidemo/6.6.5_demo/main.h`

Required behavior:
- register front
- connect
- login
- subscribe configured instruments
- convert `CThostFtdcDepthMarketDataField` into `MarketTick`
- log `[MARKET_DATA_OK]` on first tick
- log `[MD_TICK]` on every tick
- signal an event so the app can wait for first data

- [ ] **Step 5: Build and run the smoke script**

Run: `cmake --build basis_monitor/build --target basis_monitor`

Expected: build succeeds.

Run: `powershell -ExecutionPolicy Bypass -File basis_monitor/tests/verify_market_data_smoke.ps1`

Expected: script reports the required market-data path is present.

- [ ] **Step 6: Manual runtime verification with real credentials**

Run from Linux/WSL:
`sh basis_monitor/scripts/run.sh`

Expected terminal lines:
- `[MARKET_DATA_OK]`
- `[MD_TICK]`

If these do not appear, stop and fix this task before writing any basis-monitor logic.

- [ ] **Step 7: Commit**

```bash
git add basis_monitor/include/basis_monitor/ctp basis_monitor/src/ctp basis_monitor/include/basis_monitor/platform/linux_compat.h basis_monitor/vendor/ctp basis_monitor/tests/verify_market_data_smoke.ps1
git commit -m "feat: migrate standalone ctp market data path"
```

### Task 7: Implement the Basis Monitor Service

**Files:**
- Create: `basis_monitor/include/basis_monitor/monitor/basis_monitor_service.h`
- Create: `basis_monitor/src/monitor/basis_monitor_service.cpp`
- Create: `basis_monitor/tests/test_basis_monitor_service.cpp`

- [ ] **Step 1: Write the failing service tests**

Test cases:
- known contract + valid spot price -> writes a valid basis result
- negative annualized basis -> emits `EnteredNegative`
- repeated negative basis -> emits no duplicate alert
- non-negative after negative -> emits `Recovered`
- unknown contract -> ignored with a warning log, no crash

```cpp
int main() {
    BasisMonitorService service(/* fake stores + fake logger + fake alert engine */);
    MarketTick tick{};
    tick.instrument_id = "IC2606";
    tick.last_price = 6400.0;
    auto outcome = service.OnTick(tick);
    assert(outcome.has_result);
    assert(outcome.alert.transition == AlertTransition::EnteredNegative);
    return 0;
}
```

- [ ] **Step 2: Run the service target and confirm it fails**

Run: `cmake --build basis_monitor/build --target test_basis_monitor_service`

Expected: build fails because service wiring does not exist yet.

- [ ] **Step 3: Implement the service**

Processing pipeline:
1. receive `MarketTick`
2. append tick to `TickStore`
3. load or read cached spot price
4. find contract expiry
5. compute basis result
6. append basis result
7. evaluate alert transition
8. if alert or recovery, append alert record and log it

- [ ] **Step 4: Run the service test and confirm it passes**

Run: `ctest --test-dir basis_monitor/build -R test_basis_monitor_service --output-on-failure`

Expected: `1/1 tests passed`

- [ ] **Step 5: Commit**

```bash
git add basis_monitor/include/basis_monitor/monitor/basis_monitor_service.h basis_monitor/src/monitor/basis_monitor_service.cpp basis_monitor/tests/test_basis_monitor_service.cpp
git commit -m "feat: add intraday basis monitor service"
```

### Task 8: Wire the App, Runtime Assets, and Scripts

**Files:**
- Modify: `basis_monitor/app/main.cpp`
- Create: `basis_monitor/config/ctp.ini`
- Create: `basis_monitor/config/contracts.json`
- Create: `basis_monitor/config/spot_price.json`
- Create: `basis_monitor/config/alert.json`
- Create: `basis_monitor/resources/error.xml`
- Create: `basis_monitor/resources/error.dtd`
- Create: `basis_monitor/runtime/flow/.gitkeep`
- Create: `basis_monitor/data/.gitkeep`
- Create: `basis_monitor/logs/.gitkeep`
- Create: `basis_monitor/scripts/run.sh`
- Create: `basis_monitor/scripts/start.sh`
- Modify: `basis_monitor/README.md`

- [ ] **Step 1: Write a failing end-to-end app smoke check**

The app should exit non-zero with a clear config error when any required config file is missing.

- [ ] **Step 2: Implement app wiring**

`app/main.cpp` should:
- initialize runtime directories
- initialize logger
- load config
- create stores
- create `BasisMonitorService`
- create `MdApiSession`
- subscribe configured enabled contracts
- wait for first tick
- keep process alive for ongoing intraday monitoring

- [ ] **Step 3: Add sample runtime files**

Sample contents:
- `contracts.json` includes enabled IC contracts and expiry dates
- `spot_price.json` includes the current CSI 500 spot proxy
- `alert.json` enables terminal + file alerts
- `ctp.ini` contains placeholder credential keys only

- [ ] **Step 4: Add Linux runtime scripts**

Required script behavior:
- build in Linux filesystem, not `/mnt/c/.../build`
- run `basis_monitor` from its own runtime root
- keep config/data/log/flow under `basis_monitor`

- [ ] **Step 5: Build and run the full test suite**

Run: `cmake --build basis_monitor/build`

Expected: build succeeds.

Run: `ctest --test-dir basis_monitor/build --output-on-failure`

Expected: all unit tests pass.

- [ ] **Step 6: Commit**

```bash
git add basis_monitor/app/main.cpp basis_monitor/config basis_monitor/resources basis_monitor/runtime basis_monitor/data basis_monitor/logs basis_monitor/scripts basis_monitor/README.md
git commit -m "feat: wire standalone basis monitor runtime"
```

### Task 9: End-to-End Verification and Migration Safety Checks

**Files:**
- Modify: `basis_monitor/README.md`
- Verify: `6.7.11apidemo/6.6.5_demo` remains unchanged and runnable

- [ ] **Step 1: Rebuild from a clean build directory**

Run:
- `cmake -S basis_monitor -B basis_monitor/build`
- `cmake --build basis_monitor/build`
- `ctest --test-dir basis_monitor/build --output-on-failure`

Expected: configure, build, and tests all succeed.

- [ ] **Step 2: Validate runtime isolation**

Manual checks:
- logs land under `basis_monitor/logs`
- tick and basis CSV files land under `basis_monitor/data`
- flow files land under `basis_monitor/runtime/flow`
- no new runtime files are written into `6.7.11apidemo/6.6.5_demo`

- [ ] **Step 3: Validate live monitoring behavior**

With a negative annualized basis scenario:
- terminal prints `[ALERT]`
- alert log is written
- repeated negative ticks do not spam duplicate alerts

After recovery:
- terminal prints `[RECOVERY]`
- recovery is logged once

- [ ] **Step 4: Validate baseline safety**

Re-run the existing demo manually:
- `sh 6.7.11apidemo/6.6.5_demo/run.sh`

Expected: the old demo still behaves exactly as before the migration.

- [ ] **Step 5: Document operations**

Update `README.md` with:
- required config files
- how to update spot price during the day
- how to read alert logs
- known phase-1 limitations

- [ ] **Step 6: Commit**

```bash
git add basis_monitor/README.md
git commit -m "docs: add basis monitor verification and operations guide"
```

## Done Criteria
- `basis_monitor` is a separate buildable project under `basis_monitor`
- New project reaches real `OnRtnDepthMarketData` with its own runtime layout
- Tick data is stored
- Basis results are stored
- Negative annualized basis emits deduplicated alerts and recovery logs
- `6.6.5_demo` remains runnable as the fallback baseline


