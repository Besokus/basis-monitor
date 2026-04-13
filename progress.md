# 进度日志

## 2026-04-07

### 已完成

- 审查当前 `CTP / XTP` 接入结构
- 确认当前仍是二选一模式
- 确认用户已拆分 `ctp.ini` / `xtp.ini`
- 梳理出双行情第一阶段改造目标
- 按用户要求启用 `planning-with-files-zh` 风格的文件化上下文管理

### 当前状态

- 已完成架构缺口审查
- 已整理改造清单和分阶段实施顺序
- 已进一步细化到代码级实施清单
- 第一批改造已完成：
  - `XTP` 独立指数订阅配置字段
  - `xtp.ini` 指数订阅读取
  - `XTP` 会话订阅列表与 `CTP` 解耦
- 针对性验证已通过：
  - `test_config_loader`
  - `test_xtp_market_data_session`

### 下一步

1. 改双会话工厂，去掉“双启用即报错”
2. 给 Tick 增加来源/类型标识
3. 改主流程和监听器分流，支持现场验证 XTP 收数
### 本轮新增
- 已实现双会话工厂，`CTP + XTP` 双启用时可正常创建复合会话。
- 已为 `MarketTick` 增加来源与品类标识，并在 `CTP/XTP` 两侧产出时完成打标。
- 已在 `main.cpp` 中将 `XTP Index` tick 从期货计算链中分流，仅做日志观测。
- 已完成本轮验证：
  - `test_market_data_session_factory`
  - `test_md_spi_bridge`
  - `test_xtp_market_data_session`
  - `basis_monitor` 可成功编译

### 更新后的下一步
1. 用已配置好的 `ctp.ini / xtp.ini` 在目标环境验证真实连接与登录日志。
2. 按需要继续拆分 `CTP / XTP` 健康检查与首包日志。
3. 等你确认 `XTP` 能稳定收数后，再进入实时指数价接入计算的第二阶段。
## 2026-04-08

### Documentation update

- Added a new implementation status document for current phase execution:
  - `.worktrees/basis-monitor/basis_monitor/docs/implementation_status_2026-04-08.md`
- Added a dedicated CTP live integration plan:
  - `.worktrees/basis-monitor/basis_monitor/docs/ctp_live_integration_plan_2026-04-08.md`
- Added a README entry point to the new status document:
  - `.worktrees/basis-monitor/basis_monitor/README.md`
- The new document reflects:
  - current dual-session integration status
  - completed code items and verified tests
  - pending field-validation tasks
  - decision points that need user confirmation

### 2026-04-08 plan refresh

- Re-read the updated `v6.7.11_P4_20251120_traderapi` directory.
- Confirmed that Linux formal API/MD so files are now present under the P4 package.
- Updated the CTP live integration plan from “material missing” to “ready for replacement implementation”.
### 2026-04-08 live-index calculation update

- Added a runtime live-index cache to `BasisMonitorService`.
- `XTP` index ticks now update the cache through `BasisMonitorService::OnIndexTick(...)`.
- `CTP` future ticks now calculate basis and annualized basis with the latest cached `XTP` index price when available.
- If the cache is still empty, the system falls back to `index_close_yesterday`.
- `basis_results.csv` now persists the runtime index price actually used in the calculation.
- Latest basis snapshots now persist the runtime index price actually used in the calculation.
- Verified by regression tests:
  - `test_basis_monitor_service`
  - `test_basis_result_store`
  - `test_latest_basis_snapshot_store`
  - `test_market_data_session_factory`
  - `test_xtp_market_data_session`
  - `test_md_spi_bridge`
### 2026-04-08 health-split update

- Added `MarketDataHealthRegistry` to maintain separate future/index health state.
- `TerminalMdListener` now records health for both `Future` and `Index` ticks.
- Main loop now logs:
  - `[MARKET_DATA_STALE] channel=[future|index] ...`
  - `[MARKET_DATA_RECOVERED] channel=[future|index] ...`
- Report metadata still follows the `future` channel so existing report semantics stay stable.
- Verified by regression tests:
  - `test_market_data_health_tracker`
  - `test_market_data_health_registry`
  - `test_basis_monitor_service`
  - `test_market_data_session_factory`
  - `test_xtp_market_data_session`

### 2026-04-08 live-index decision saved

- Saved a written decision record for the accuracy-vs-timeliness tradeoff:
  - `.worktrees/basis-monitor/basis_monitor/docs/decision_2026-04-08_live-index-accuracy-vs-timeliness.md`
- Decision adopted:
  - startup remains timely
  - but when `XTP` is enabled, annualized-basis calculation and alerts require a fresh live index price
  - waiting/stale live index now suspends calculation and alerting instead of silently reusing stale data
