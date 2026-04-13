# 任务计划：Basis Monitor 双行情接入改造

## 目标

将当前 `CTP / XTP` 二选一的行情接入模式改造为可同时接入：

- `CTP`：期货实时行情
- `XTP`：股指指数实时行情

第一阶段目标仅为：

- 系统可以同时启动 `CTP + XTP`
- `CTP` 继续订阅本地筛出的 `Top4` 期货合约
- `XTP` 独立订阅指数标的
- 日志中可以确认 `XTP` 已成功接收到数据

暂不在这一阶段改造：

- 年化基差计算逻辑
- 告警逻辑
- 企业微信通知逻辑

## 当前阶段

阶段 1：代码现状审查与缺口确认 `complete`

阶段 2：双行情接入改造方案拆解 `complete`

阶段 3：建立文件化上下文管理 `complete`

阶段 4：双行情接入第一阶段实现 `pending`

阶段 5：XTP 收数现场验证支持 `pending`

## 第一阶段代码级实施清单

### 4.1 配置结构与加载

文件：

- `.worktrees/basis-monitor/basis_monitor/include/basis_monitor/config/app_config.h`
- `.worktrees/basis-monitor/basis_monitor/src/config/config_loader.cpp`
- `.worktrees/basis-monitor/basis_monitor/tests/test_config_loader.cpp`

动作：

1. 在 `XtpConfig` 中新增独立指数订阅列表字段，例如 `std::vector<std::string> index_instruments`
2. 在 `xtp.ini` 中增加指数订阅配置读取，例如 `IndexInstrumentID=...`
3. 保持 `EnableCtpMarketData` 与 `EnableXtpMarketData` 同时可为 `true`
4. 不再将 `MarketDataProvider` 视为双行情模式的控制开关，仅作兼容保留

验收：

- `test_config_loader` 可覆盖“双启用 + XTP 独立指数订阅”场景

### 4.2 Tick 模型扩展

文件：

- `.worktrees/basis-monitor/basis_monitor/include/basis_monitor/domain/market_tick.h`
- `.worktrees/basis-monitor/basis_monitor/src/ctp/md_spi_bridge.cpp`
- `.worktrees/basis-monitor/basis_monitor/src/market_data/xtp_market_data_session.cpp`
- 相关 tick/bridge 测试

动作：

1. 为 `MarketTick` 增加 `provider` 字段
2. 为 `MarketTick` 增加 `instrument_type` 字段，至少区分 `Future` / `Index`
3. CTP 侧生成 `provider=CTP, instrument_type=Future`
4. XTP 侧生成 `provider=XTP, instrument_type=Index`

验收：

- 监听器可在日志中区分两类数据来源

### 4.3 XTP 独立指数订阅

文件：

- `.worktrees/basis-monitor/basis_monitor/src/market_data/xtp_market_data_session.cpp`
- `.worktrees/basis-monitor/basis_monitor/tests/test_xtp_market_data_session.cpp`

动作：

1. `SubscribeCurrentInstruments()` 改为读取 `config.xtp.index_instruments`
2. 不再依赖 `config.ctp.instruments`
3. 增加空订阅列表保护和清晰错误日志

验收：

- XTP session 的订阅列表与 CTP 期货订阅列表完全解耦

### 4.4 双会话编排器

文件：

- `.worktrees/basis-monitor/basis_monitor/include/basis_monitor/market_data/market_data_session.h`
- `.worktrees/basis-monitor/basis_monitor/include/basis_monitor/market_data/market_data_session_factory.h`
- `.worktrees/basis-monitor/basis_monitor/src/market_data/market_data_session_factory.cpp`
- `.worktrees/basis-monitor/basis_monitor/tests/test_market_data_session_factory.cpp`

动作：

1. 新增 `DualMarketDataSession`
2. 内部持有 `CtpMarketDataSession` 与 `XtpMarketDataSession`
3. `Start()` 同时启动两路
4. `Stop()` 同时停止两路
5. `WaitForFirstMarketData()` 第一阶段优先对 CTP 严格，对 XTP 记录状态但不阻断主链路
6. 删除当前“双启用即报错”的分支

验收：

- 双启用配置下 `CreateMarketDataSession()` 返回复合 session

### 4.5 主流程接双行情

文件：

- `.worktrees/basis-monitor/basis_monitor/app/main.cpp`

动作：

1. 启动阶段分别打印：
   - CTP 启用状态
   - XTP 启用状态
   - CTP 期货订阅列表
   - XTP 指数订阅列表
2. `runtime_config` 中仅 CTP 订阅列表由 Top4 结果驱动
3. XTP 订阅列表来自配置，不混入 Top4 期货合约代码
4. 首阶段参考数据目录仍可沿用 `ctp` 侧配置，不先扩散到双配置选择逻辑

验收：

- 启动日志可清楚确认双路接入状态

### 4.6 监听器分流与最小缓存

文件：

- `.worktrees/basis-monitor/basis_monitor/app/main.cpp`
- 需要时新增：`.worktrees/basis-monitor/basis_monitor/include/basis_monitor/monitor/index_tick_cache.h`
- 需要时新增对应 `src/` 实现

动作：

1. `TerminalMdListener::OnTick(...)` 先根据 `instrument_type` 分流
2. 期货 tick：
   - 继续走现有 `BasisMonitorService::OnTick(...)`
3. 指数 tick：
   - 先只做日志
   - 更新最新指数缓存
   - 不进入年化基差计算

验收：

- 日志中可明确看到 XTP 指数更新
- 现有期货监控链不受影响

### 4.7 健康检查拆分

文件：

- `.worktrees/basis-monitor/basis_monitor/include/basis_monitor/monitor/market_data_health_tracker.h`
- `.worktrees/basis-monitor/basis_monitor/src/monitor/market_data_health_tracker.cpp`
- `.worktrees/basis-monitor/basis_monitor/app/main.cpp`

动作：

1. 至少拆成 CTP / XTP 两套路健康状态
2. `main.cpp` 中分别打印 stale / recovered

验收：

- 能知道是哪一路行情断流

### 4.8 配置样例与 README

文件：

- `.worktrees/basis-monitor/basis_monitor/config/ctp.ini`
- 新增或整理 `.worktrees/basis-monitor/basis_monitor/config/xtp.ini`
- `.worktrees/basis-monitor/basis_monitor/README.md`

动作：

1. 给出双启用样例
2. 给出“只验证 XTP 收数”的样例
3. 明确第一阶段不接入计算，仅验证通道

验收：

- 现场只看配置和日志即可验证 XTP 收数

## 已确认缺口

1. 当前 `CreateMarketDataSession(...)` 在 `CTP + XTP` 同时启用时直接报错，双会话未实现。
2. 当前 `XTP` 订阅列表仍复用 `config.ctp.instruments`，没有独立指数订阅集合。
3. 当前 `MarketTick` 没有来源/资产类型语义，监听器无法区分期货 tick 与指数 tick。
4. 当前主流程只构造单 session，并默认所有 tick 都进入期货计算链。
5. 当前参考数据目录构造只使用 `config.ctp`，`xtp.ini` 中的参考数据目录尚未接入主流程。
6. 当前年化基差中的指数价格仍来自静态 `index_close_yesterday`，未使用 XTP 实时指数价。

## 第一阶段改造顺序

1. 配置结构收口
2. XTP 独立订阅标的
3. Tick 事件模型加来源标识
4. 双会话编排器
5. 主流程接双会话
6. 监听器分流
7. 健康检查拆分
8. 配置样例和 README

## 风险

1. XTP 指数代码格式与当前合约/指数映射可能不一致，需要现场校验。
2. 双会话启动后，首包等待与健康检查策略要避免互相拖死。
3. 第一阶段若过早把 XTP 接入计算链，会扩大调试范围，不利于先验证收数。

## 决策

1. 第一阶段先实现“同时接入并可观测”，不接入实时指数到计算。
2. 先让 XTP 作为独立指数行情通道稳定收数，再进入第二阶段计算改造。
3. 第一批实现以最小改动为主，不在第一阶段改造 `BasisMonitorService` 的计算公式。
## 2026-04-08 updated status

- Stage 4 now includes runtime live-index integration, not just dual-session startup.
- Completed:
  - CTP runtime Top4 future subscription derivation
  - XTP runtime index subscription derivation from the same monitored contracts
  - live index cache inside `BasisMonitorService`
  - basis result and snapshot persistence switched to runtime index price
- Next focus:
  1. split CTP/XTP health tracking
  2. verify XTP first tick and continuous index ticks in the field
  3. review alert/report semantics under more frequently changing live index prices
