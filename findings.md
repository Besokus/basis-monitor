# 发现记录：Basis Monitor 双行情接入

## 当前代码行为

### 行情接入

- `src/market_data/market_data_session_factory.cpp`
  - 当前先读 `EnableCtpMarketData` / `EnableXtpMarketData`
  - 若两者同时为 `true`，直接抛错：`Dual market data mode (CTP + XTP) is not yet implemented`
  - 说明双行情模式尚未落地

### 配置加载

- `src/config/config_loader.cpp`
  - 已支持从 `ctp.ini` 加载 `EnableCtpMarketData`
  - 已支持从 `xtp.ini` 加载 `EnableXtpMarketData`
  - 已支持从 `xtp.ini` 读取 XTP 连接参数
  - 说明“配置拆分”已开始，但仅是参数拆分，不是流程拆分

### XTP 订阅

- `src/market_data/xtp_market_data_session.cpp`
  - `SubscribeCurrentInstruments()` 仍读取 `config_.ctp.instruments`
  - 说明 XTP 当前仍被当作“期货行情替代源”，而不是独立指数通道

### 主流程

- `app/main.cpp`
  - 参考数据目录只从 `config.ctp` 构造
  - 主流程只启动一个 `session`
  - `TerminalMdListener::OnTick(...)` 默认把所有 tick 当作期货监控标的处理

### 计算链

- `src/monitor/basis_monitor_service.cpp`
  - 指数价格使用 `contract.index_close_yesterday`
  - 当前计算公式仍是“昨收指数价 + 实时期货价”
  - 尚未接入 XTP 实时指数价

## SFTP 相关独立工作

- 已把本地上传链路改造成“本地先清洗，再通过 SFTP 上传”
- 已新增 `config/sftp.conf` 方案管理：
  - `SFTP_HOST`
  - `SFTP_USER`
  - `SFTP_PORT`
  - `SFTP_IDENTITY_FILE`

## 当前建议

- 第一阶段只做：
  - 双 session
  - XTP 独立指数订阅
  - 日志确认 XTP 收数
- 第二阶段再做：
  - 实时指数缓存
  - `CTP期货价 + XTP指数价` 计算改造
## 2026-04-07 补充

- `MarketTick` 已新增 `provider` 与 `instrument_type` 字段。
- `CTP` 产生的 tick 现在会标记为 `provider=CTP`、`instrument_type=Future`。
- `XTP` 产生的 tick 现在会标记为 `provider=XTP`、`instrument_type=Index`。
- `CreateMarketDataSession(...)` 已支持 `EnableCtpMarketData=true` 且 `EnableXtpMarketData=true` 时返回复合双会话，不再直接抛出“未实现”异常。
- `main.cpp` 中监听器已把 `Index` tick 从期货基差计算链分流，当前只记录指数行情日志，不进入 `BasisMonitorService`。
- 双会话模式下，首包等待目前仍以 `CTP` 首包为主成功条件，`XTP` 首包暂未纳入主流程阻断条件。

## 2026-04-08 CTP 正式版材料复核

- `v6.7.11_P4_20251120_traderapi` 目录现已包含 Linux 版正式 API/MD so 与对应头文件。
- 当前 `basis_monitor` 仍链接旧版 `vendor/ctp/lib/linux/thostmduserapi_se.so`，尚未切换到正式版 Linux so。
- 当前 `basis_monitor` 仍未链接 `LinuxDataCollect.so`，因此看穿式采集链还未真正接入。
- CTP 现场报错 `Decrypt handshake data failed` 时，网络端口已确认可达，因此优先排查运行时库栈与采集初始化，而不是继续调整基础地址文本。
## 2026-04-08 runtime calculation findings

- `CTP` runtime subscription already comes from the selected Top4 futures, not the static `InstrumentID=IC2606`.
- `XTP` runtime subscription now comes from the same monitored Top4 contracts and currently derives `000300.XSHG,000852.XSHG,000905.XSHG`.
- Before this iteration, the runtime calculation still used `contract.index_close_yesterday` everywhere.
- After this iteration, the runtime path is:
  - `XTP index tick -> BasisMonitorService::OnIndexTick(...) -> cache by normalized index id`
  - `CTP future tick -> BasisMonitorService::OnTick(...) -> use cached live index price when available`
  - `MonitorUpdate.index_price -> basis result CSV + latest snapshot store`
- Remaining gap: health tracking is still future-tick-centric, so `XTP` freshness is not yet tracked independently.
