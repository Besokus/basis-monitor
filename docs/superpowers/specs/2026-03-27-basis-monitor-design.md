# Basis Monitor 设计文档

## 概要
在仓库根目录下创建一个全新的独立项目 `basis_monitor`，并使其与内层 `6.7.11apidemo` 目录同级，复用 `6.7.11apidemo/6.6.5_demo` 中已经验证可用的 CTP 行情接收能力，同时将职责拆分为清晰的模块：行情接入、配置加载、数据存储、年化基差计算与告警。第一阶段先通过“CTP 实时期货行情 + 本地提供的现货价格”打通完整闭环，随后实时计算盘中年化基差，并在年化基差为负时发出告警。

## 目标
- 构建一个独立的 `basis_monitor` 项目，而不是继续在 `6.6.5_demo` 上叠加功能。
- 安全迁移所有与行情相关的核心能力，包括连接、登录、订阅、接收 Tick，以及运行时依赖资源。
- 保留并迁移以下关键支撑能力：
  - configuration loading
  - logging
  - runtime flow directory usage
  - market-data event synchronization
  - error-code resource loading where applicable
- 保存接收到的行情数据与计算后的监控结果，便于盘中观察和复盘。
- 按已确认的公式实时计算年化基差：
  - `basis = spot - future`
  - `annual_rate = basis / spot * (365 / remaining_days) * 100`
- 当 `annual_rate < 0` 时向交易组发出告警。
- 通过合理的文件夹划分组织代码，避免后续仍然需要在单个大型头文件中继续堆积逻辑。

## 非目标
- 第一阶段不修改 `6.6.5_demo` 内的交易逻辑，仅将其保留为参考实现。
- 第一阶段不实现直接下单或自动执行对冲。
- 第一阶段不接入真实的外部现货/指数数据源。
- 第一阶段不覆盖所有品种，初始范围限定为 IC 合约。
- 第一阶段不引入终端和日志之外的外部告警通道。

## 可复用的现有资产

### 直接相关的源码文件
- `6.7.11apidemo/6.6.5_demo/main.cpp`
  - 行情登录与订阅的入口流程
- `6.7.11apidemo/6.6.5_demo/main.h`
  - 当前的 `CSimpleMdHandler`
  - 行情登录与订阅的请求/响应流程
  - 行情回调 `OnRtnDepthMarketData`
  - 行情同步事件
- `6.7.11apidemo/6.6.5_demo/getconfig.cpp`
- `6.7.11apidemo/6.6.5_demo/getconfig.h`
- `6.7.11apidemo/6.6.5_demo/define.h`
- `6.7.11apidemo/6.6.5_demo/linux_compat.h`

### 运行时/配置资源
- `6.7.11apidemo/6.6.5_demo/config.ini`
- `6.7.11apidemo/6.6.5_demo/flow/`
- `6.7.11apidemo/6.6.5_demo/error.xml`
- `6.7.11apidemo/6.6.5_demo/error.dtd`

### CTP vendor 资源
- `6.7.11apidemo/6.6.5_demo/ThostFtdcMdApi.h`
- `6.7.11apidemo/6.6.5_demo/ThostFtdcUserApiDataType.h`
- `6.7.11apidemo/6.6.5_demo/ThostFtdcUserApiStruct.h`
- `6.7.11apidemo/6.6.5_demo/thostmduserapi_se.so`

## 迁移原则
- 不直接将 `main.h` 整体复制到新项目中。
- 只抽取新监控项目真正需要的最小行情功能闭环。
- 将配置、日志、运行时行为拆到职责单一的独立模块中。
- 保持原始 demo 可运行，作为参考实现和回退基线。
- 迁移时保留现有运行时假设，尤其包括：
  - config file discovery
  - flow path expectations
  - shared-library lookup
  - log file creation
- 让新项目可以独立构建、独立运行、独立验证。

## 建议的项目结构

### 顶层目录
- `basis_monitor/`（与 `6.7.11apidemo/` 同级）

### 源码结构
- `basis_monitor/CMakeLists.txt`
- `basis_monitor/README.md`
- `basis_monitor/app/main.cpp`
- `basis_monitor/include/basis_monitor/config/`
- `basis_monitor/include/basis_monitor/logging/`
- `basis_monitor/include/basis_monitor/domain/`
- `basis_monitor/include/basis_monitor/ctp/`
- `basis_monitor/include/basis_monitor/monitor/`
- `basis_monitor/include/basis_monitor/storage/`
- `basis_monitor/src/config/`
- `basis_monitor/src/logging/`
- `basis_monitor/src/ctp/`
- `basis_monitor/src/monitor/`
- `basis_monitor/src/storage/`

### 配置与运行时结构
- `basis_monitor/config/ctp.ini`
- `basis_monitor/config/contracts.json`
- `basis_monitor/config/spot_price.json`
- `basis_monitor/config/alert.json`
- `basis_monitor/resources/error.xml`
- `basis_monitor/resources/error.dtd`
- `basis_monitor/runtime/flow/`
- `basis_monitor/data/`
- `basis_monitor/logs/`
- `basis_monitor/scripts/run.sh`
- `basis_monitor/scripts/start.sh`

### Vendor 结构
- `basis_monitor/vendor/ctp/include/`
- `basis_monitor/vendor/ctp/lib/linux/`

### 测试结构
- `basis_monitor/tests/`

## 模块职责

### `config`
- 从 `ctp.ini` 加载 CTP 连接参数和 front 地址。
- 从 `spot_price.json` 加载现货价格。
- 从 `contracts.json` 加载监控合约及其到期日。
- 从 `alert.json` 加载告警策略。

### `logging`
- 用监控项目自己的 logger 替代对 `LOG(...)` 宏的直接耦合。
- 同时支持终端输出和文件日志。
- 保留关键标记格式，便于筛查：
  - `[MARKET_DATA_OK]`
  - `[MD_TICK]`
  - `[ALERT]`

### `domain`
- 定义内部领域对象：
  - market tick
  - contract definition
  - basis calculation result
  - alert event

### `ctp`
- 管理 CTP 行情 API 生命周期。
- 封装以下职责：
  - API creation
  - front registration
  - login
  - subscription
  - market-data callback conversion
- 保留当前 demo 已经验证可用的事件同步机制。

### `monitor`
- 计算 basis 与 annualized basis。
- 将收到的期货 Tick 与配置中的合约及到期日匹配。
- 评估是否满足告警条件。
- 做告警去重，避免在持续负值期间向交易组连续刷屏。

### `storage`
- 将原始/标准化后的 Tick 持久化为 CSV 或 JSONL。
- 将基差监控结果持久化为 CSV 或 JSONL。
- 将告警事件写入独立的 alert log。

## 第一阶段功能流程
1. 启动 `basis_monitor`。
2. 加载配置文件：
   - `ctp.ini`
   - `contracts.json`
   - `spot_price.json`
   - `alert.json`
3. 初始化 logger 与运行时目录。
4. 使用监控项目的 runtime flow path 创建行情 API。
5. 连接 `FrontMdAddr`。
6. 使用 `BrokerID`、`UserID`、`Password` 登录。
7. 订阅配置中的 IC 合约。
8. 每当收到一笔实时行情 Tick：
   - 标准化关键价格字段
   - 保存 Tick 数据
   - 读取或缓存当前 spot price
   - 查找该合约对应的 expiry date
   - 计算 basis 和 annualized basis
   - 保存计算后的监控结果
   - 如果 `annual_rate < 0`，发出 alert

## 公式
- `remaining_days = (expiry_date - today).days`
- 当 `remaining_days <= 0` 时，视为该合约已到期，不再进行年化基差监控。
- `basis = spot_price - future_price`
- `annual_rate = basis / spot_price * (365 / remaining_days) * 100`

## 第一阶段数据输入

### Futures price
- 来自 CTP 的实时行情 Tick。
- 期货价格的主字段使用 `LastPrice`。

### Spot price
- 来自本地配置文件。
- 第一阶段默认由运营或交易支持通过文件更新该值，而不是通过远端 API 获取。

### Expiry date
- 来自本地合约定义文件。
- 第一阶段不通过合约代码自动推导到期日，而是使用显式配置。

## 存储要求

### Tick 存储
至少保存以下字段：
- timestamp
- instrument id
- update time
- update millisec
- last price
- bid price 1
- bid volume 1
- ask price 1
- ask volume 1
- volume

### Basis monitor 结果存储
至少保存以下字段：
- timestamp
- contract code
- spot price
- future price
- expiry date
- remaining days
- basis
- annualized basis rate
- negative-basis flag

### Alert 存储
至少保存以下字段：
- timestamp
- contract code
- spot price
- future price
- basis
- annualized basis rate
- alert reason
- alert status transition

## 告警要求
- 当 `annual_rate < 0` 时触发。
- 第一阶段的告警渠道：
  - terminal output
  - alert log file
- 需要防止重复刷屏：
  - 进入负值状态时发一次 alert
  - 离开负值状态时发一次 recovery log

## 配置文件

### `ctp.ini`
必需字段：
- `FrontMdAddr`
- `BrokerID`
- `UserID`
- `Password`

### `contracts.json`
每个条目包含：
- contract code
- expiry date
- enabled flag

### `spot_price.json`
包含：
- index symbol 或 label
- current spot price
- update timestamp

### `alert.json`
包含：
- enable alerting
- enable terminal alert
- enable file alert
- duplicate suppression policy

## 日志要求
- 所有运行行为都应可追踪。
- 普通运行日志与告警日志要分离。
- 行情与监控标记需便于 grep：
  - `[MARKET_DATA_OK]`
  - `[MD_TICK]`
  - `[BASIS_MONITOR]`
  - `[ALERT]`
  - `[RECOVERY]`

## 迁移安全要求
- 在增加业务计算之前，先验证新项目确实能成功进入 `OnRtnDepthMarketData`。
- 验证新运行目录下的配置发现逻辑正常工作。
- 验证 CTP `.so` 能从新的 vendor/runtime 布局正常加载。
- 验证日志文件写入的是新项目目录，而不是旧 demo 目录。
- 验证 flow 文件写入到 `basis_monitor/runtime/flow` 下。

## 测试计划

### 第一阶段验证
- 新的独立项目能够成功构建。
- 使用迁移后的配置成功完成行情登录。
- 成功订阅配置中的 IC 合约。
- 新项目回调中能收到真实 Tick。
- Tick 数据能够落盘。
- 对配置了有效到期日的合约能够计算 annualized basis。
- 当 annualized basis 为负时，能够看到终端告警并写入 alert log。
- 在连续负值期间不会重复刷出大量相同告警。

### 回归保护
- 参考现有行情验证方式，补充文本型验证脚本。
- 在迁移过程中始终保留 `6.6.5_demo` 可运行，作为对照基线。

## 风险
- 第一阶段的 spot price 来自本地文件，可能滞后于真实指数。
- 如果合约配置或到期日配置错误，年化基差结果会失真。
- 如果直接复制过多 `main.h` 内容，会把当前单体式结构再次带入新项目。
- 当前 demo 混合了多种职责，抽取时必须非常小心，避免遗漏隐藏依赖。

## 仓库演进建议
- 第一阶段建议先将 `basis_monitor` 保持在当前仓库中，作为仓库根目录下、与 `6.7.11apidemo/` 同级的独立模块开发。
- 如果只是想在另一个目录继续开发，同时保留完整 Git 历史，应使用 `git worktree`，而不是直接复制文件夹。
- 如果未来要将 `basis_monitor` 拆成独立仓库并保留该模块自己的提交历史，优先使用 `git subtree split --prefix basis_monitor ...`，也可在需要更细粒度历史重写时使用 `git filter-repo --path basis_monitor ...`。
- 如果只是仓库内调整目录位置，应使用 `git mv`，避免后续 blame、diff 和历史追踪变差。
- 直接把 `basis_monitor` 文件夹复制到别处不会携带 Git 历史，只会复制当前文件内容。

## 建议
将新的独立项目 `basis_monitor` 分两步安全落地：
1. 先在新项目中抽取并验证“仅行情接收”的闭环。
2. 再在已验证的行情通路上叠加本地现货价加载、基差计算、结果存储和负基差告警。


