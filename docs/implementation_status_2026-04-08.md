# Basis Monitor 开发实施文档（2026-04-08）

## 1. 当前阶段与目标

当前处于 **阶段 1 收口 + 阶段 2 联调准备**：

- 阶段 1 目标：完成 `CTP + XTP` 同时接入能力（不再二选一）、打通双行情会话、支持区分期货与指数 tick。
- 阶段 2 目标：先在现场验证 `XTP` 稳定收数，再推进“`CTP` 期货价 + `XTP` 指数价”进入年化基差计算链。

本阶段明确不改：

- 年化基差核心公式
- 告警规则与提醒节奏
- 企业微信推送链路

## 2. 已完成实施项（代码已落地）

### 2.1 双行情会话能力

- `CreateMarketDataSession(...)` 已支持双启用，不再在 `EnableCtpMarketData=true` 且 `EnableXtpMarketData=true` 时直接报错。
- 新增复合会话编排，双会话可同时 `Start()` / `Stop()`。

### 2.2 XTP 独立指数订阅

- `XtpConfig` 增加独立订阅字段：`index_instruments`。
- 从 `xtp.ini` 读取 `IndexInstrumentID`。
- XTP 订阅来源改为 `config.xtp.index_instruments`，不再复用 `config.ctp.instruments`。

### 2.3 Tick 语义补齐

- `MarketTick` 新增：
  - `provider`
  - `instrument_type`（`Future/Index/Unknown`）
- CTP tick 标记为 `Future`，XTP tick 标记为 `Index`。

### 2.4 主流程最小分流

- `main.cpp` 监听器已将 `Index` tick 从期货基差计算链分流。
- 当前 `Index` tick 只记录观测日志（`[INDEX_TICK]`），不进入 `BasisMonitorService` 计算。

### 2.5 启动可观测性增强

- 启动日志新增：
  - `EnableCtpMarketData`
  - `EnableXtpMarketData`
  - `CtpInstrumentIDs`
  - `XtpIndexInstrumentIDs`

## 3. 已完成验证

已在当前开发环境完成编译与测试验证：

- `test_market_data_session_factory`
- `test_md_spi_bridge`
- `test_xtp_market_data_session`
- `basis_monitor` 主程序编译通过

## 4. 未完成项（下一阶段）

### 4.1 现场连通性验收（优先级 P0）

目标：确认 `ctp.ini / xtp.ini` 已配置条件下，现场可稳定连接并收数。

验收日志建议：

- CTP 登录成功与订阅成功
- XTP 登录成功与订阅成功
- 出现连续 `INDEX_TICK` 日志
- 期货 tick 仍正常进入监控链

### 4.2 健康检查拆分（优先级 P1）

当前健康检查仍以单链路视角为主，建议拆分为：

- `CTP` 健康状态
- `XTP` 健康状态

### 4.3 计算链升级准备（优先级 P1）

在 XTP 现场稳定后，进入下一阶段：

- 引入实时指数缓存
- 将计算链切换为：`CTP future_last_price` + `XTP index_last_price`

## 5. 运行与验收建议

建议按以下顺序进行现场验证：

1. 使用已配置好的 `ctp.ini / xtp.ini` 启动 `basis_monitor`。
2. 先验证双会话均完成连接、登录、订阅。
3. 观察 `INDEX_TICK` 连续性（频率、延迟、断流恢复）。
4. 同时确认期货主链路无回归（告警、落盘保持原行为）。

## 6. 待确认决策（需你确认）

1. 双会话模式下首包策略：是否继续保持“以 CTP 首包为主门槛，XTP 首包仅记录日志不阻断启动”？
2. XTP 指数代码规范：当前配置值是否固定为 `000300/000905/000852`，还是需要支持带交易所后缀形式？
3. 阶段 2 切换点：是否以“连续 2 个交易日 XTP 稳定收数”为准，再接入计算链？

## 7. 相关补充文档

- `docs/ctp_live_integration_plan_2026-04-08.md`
  - 说明正式版实盘 API 与看穿式采集库接入计划
  - 说明当前 CTP 握手失败问题的判断依据
  - 说明建议的分阶段替换与验收顺序
  - 已更新为“正式版 Linux API/MD so 已具备，可直接进入替换实施”
## 2026-04-08 runtime live-index integration

This iteration moved the system from "dual feeds connected but only CTP participates in calculation"
to "CTP futures calculate against the latest cached XTP index price".

Completed in this iteration:

- `BasisMonitorService` now owns a runtime index-price cache keyed by normalized index instrument id.
- `TerminalMdListener` now forwards `Index` ticks into `BasisMonitorService::OnIndexTick(...)` before logging and returning.
- `BasisMonitorService::OnTick(...)` now prefers the latest cached live index price and falls back to `index_close_yesterday` only when live data is not available yet.
- `BasisResultStore` now writes the runtime index price actually used in the calculation.
- `LatestBasisSnapshotStore` now writes the runtime index price actually used in the latest calculation.

Validation completed:

- `test_basis_monitor_service`
- `test_basis_result_store`
- `test_latest_basis_snapshot_store`
- `test_market_data_session_factory`
- `test_xtp_market_data_session`
- `test_md_spi_bridge`

Current stage definition:

- `CTP` live market data access: completed and field-verified
- `XTP` runtime subscription and first-tick waiting path: completed in code, field verification still pending
- `CTP + XTP` joint annualized basis calculation: core runtime path completed
- remaining stabilization work: separate health tracking, field validation, and alert/report regression review
## 2026-04-08 health-state split

This iteration split runtime health monitoring into two channels:

- `future` for the CTP-driven calculation chain
- `index` for the XTP-driven index stream

Completed in this iteration:

- Added `MarketDataHealthRegistry` as a lightweight router over two `MarketDataHealthTracker` instances.
- `TerminalMdListener` now records health for both future and index ticks.
- Main loop now logs stale/recovered events with an explicit `channel=[future|index]`.
- Scheduled report metadata continues to follow the `future` channel to preserve existing report semantics.

Validation completed:

- `test_market_data_health_tracker`
- `test_market_data_health_registry`
- `test_basis_monitor_service`
- `test_market_data_session_factory`
- `test_xtp_market_data_session`
