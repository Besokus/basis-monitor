# Basis Monitor 分品种 Top4 日内监控设计文档

## 概要
`basis_monitor` 的正式目标调整为一个“盘前选池 + 盘中实时监控 + 负基差报警”的独立监控项目。系统每天启动时，基于服务器上已有的四类 CSV 数据，分别从 `IC`、`IF`、`IH`、`IM` 四类股指期货真实月合约中，按昨日成交额筛出各自 Top4 监控合约；随后通过 CTP 订阅这些合约的盘中实时行情，持续计算年化基差率，并在年化基差率为负时立即报警告知交易组。

第一版采用“指数昨日收盘价”作为盘中全天固定基准价，期货价格则来自 CTP 的实时行情 Tick。这一方案在当前数据条件下可以快速形成稳定、可运行、可验证的监控闭环。

## 当前项目阶段

### 已完成能力
- 独立 `basis_monitor` 项目骨架已经建立。
- CTP 行情链路已经打通，包含连接、登录、订阅与 `OnRtnDepthMarketData` 回调接收。
- 基差与年化基差率计算核心已实现。
- 负值报警状态机已实现，具备进入负值与恢复去重的能力。
- 告警配置、存储层和双日志能力的代码已开始落地。

### 未完成能力
- 尚未基于服务器 CSV 自动完成 `IC / IF / IH / IM` 各自 Top4 的选池。
- 尚未将 `all_instruments/Future`、`eod_price/Future`、`all_instruments/INDX`、`eod_price/INDX` 接入正式主流程。
- 尚未将“选池结果 + CTP 实时行情 + 存储 + 告警输出”完整接入 `app/main.cpp`。
- 尚未完成 Linux 环境下的真实构建与联调验收。

### 阶段判断
当前项目处于“监控引擎原型已完成，数据驱动选池和业务闭环尚未完成”的阶段。相对最终目标，已完成核心内核，正进入业务数据接入与监控主流程接线阶段。

## 目标
- 每日盘前分别从 `IC`、`IF`、`IH`、`IM` 中自动筛出昨日成交额 Top4 的真实月合约。
- 自动关联这些合约对应的指数代码、指数名称和到期日。
- 盘中通过 CTP 实时接收这些合约的行情。
- 按约定公式持续计算年化基差率。
- 当年化基差率为负时立即报警，并避免重复刷屏。
- 同时输出终端监控结果、运行日志、监控结果落盘和告警日志。

## 非目标
- 第一版不接入盘中实时指数行情源。
- 第一版不自动下单或联动交易。
- 第一版不监控商品期货或非股指期货品种。
- 第一版不通过外部 IM、短信或邮件发送告警，先以终端和日志为主。
- 第一版不覆盖历史回放和批量复盘分析，只覆盖“启动后实时监控”。

## 数据资产与用途

### 1. 期货主数据
- 路径模式：`basis_monitor/data/all_instruments/Future/<date>.csv`
- 当前样例：`basis_monitor/data/all_instruments/Future/2026-03-30.csv`

关键字段：
- `order_book_id`
- `underlying_symbol`
- `underlying_order_book_id`
- `maturity_date`
- `de_listed_date`
- `exchange`
- `contract_multiplier`
- `product`

用途：
- 确认一个期货合约是否是股指真实月合约
- 获取合约到期日
- 获取该合约对应的指数代码
- 获取展示和校验所需的主数据

### 2. 指数主数据
- 路径模式：`basis_monitor/data/all_instruments/INDX/<date>.csv`
- 当前样例：`basis_monitor/data/all_instruments/INDX/2026-03-30.csv`

关键字段：
- `order_book_id`
- `symbol`
- `exchange`
- `abbrev_symbol`

用途：
- 将指数代码转换成可读名称
- 作为输出展示和监控说明的元数据来源

### 3. 期货日行情
- 路径模式：`basis_monitor/data/eod_price/Future/<date>.csv`
- 当前样例：`basis_monitor/data/eod_price/Future/2026-03-27.csv`

关键字段：
- `trade_date`
- `order_book_id`
- `underlying_symbol`
- `close`
- `total_turnover`
- `volume`
- `settlement`
- `open_interest`

用途：
- 作为每个品种“昨日成交额 Top4”筛选的正式来源
- 用于盘前确定今日监控合约集合

### 4. 指数日行情
- 路径模式：`basis_monitor/data/eod_price/INDX/<date>.csv`
- 当前样例：`basis_monitor/data/eod_price/INDX/2026-03-27.csv`

关键字段：
- `trade_date`
- `order_book_id`
- `close`
- `total_turnover`
- `volume`

用途：
- 提供对应指数的昨日收盘价
- 第一版将其作为盘中全天固定的指数基准价

### 5. CTP 实时行情
- 来源：`OnRtnDepthMarketData`
- 主字段：`LastPrice`

用途：
- 盘中实时期货价格输入
- 触发每一轮基差与年化基差率计算

## 监控标的选择规则

### 监控品种
系统只监控以下四类股指期货：
- `IC`
- `IF`
- `IH`
- `IM`

### 真实月合约识别规则
候选合约必须同时满足：
- `exchange == CFFEX`
- `product == Index`
- `order_book_id` 匹配真实月合约形式，例如 `IC2606`、`IF2604`
- `maturity_date` 不为 `0000-00-00`

必须排除：
- 主力连续、次主力连续、指数连续等派生合约，例如：
  - `IC88`
  - `IC888`
  - `IC889`
  - `IC99`
  - `IC88A2`
  - `IC88A3`
  - 以及 `IF/IH/IM` 的对应连续合约

### 排名规则
- 从昨日期货日行情中筛出上述真实月合约
- 先按品种拆分成 `IC`、`IF`、`IH`、`IM` 四组
- 每组按 `total_turnover` 降序排序
- 每个品种取 Top4
- 今日监控清单是四组结果的并集，理论上最多 16 个合约

这里的“Top4”是分品种 Top4，不是全市场总榜 Top4。

## 指数映射规则
每个入选期货合约通过 `all_instruments/Future.underlying_order_book_id` 映射到对应指数。

典型映射：
- `IC -> 000905.XSHG`（中证500）
- `IF -> 000300.XSHG`（沪深300）
- `IH -> 000016.XSHG`（上证50）
- `IM -> 000852.XSHG`（中证1000）

系统再从 `all_instruments/INDX` 中读取指数名称，从 `eod_price/INDX` 中读取该指数的昨日收盘价。

## 计算口径

### 基差
- `basis = index_close_yesterday - future_last_price`

### 年化基差率
- `annual_rate = basis / index_close_yesterday * (365 / remaining_days) * 100`

### 剩余天数
- `remaining_days = maturity_date - today`

### 特殊情况
- 当 `remaining_days <= 0` 时：
  - 视为该合约已到期
  - 年化基差率显示为 `0%`
  - 不触发告警
- 当 `index_close_yesterday <= 0` 时：
  - 视为指数基准数据无效
  - 跳过计算并输出警告

## 盘中处理流程
1. 启动程序。
2. 读取最新可用的 `all_instruments/Future` 和 `all_instruments/INDX` 文件。
3. 读取最新可用的昨日 `eod_price/Future` 和 `eod_price/INDX` 文件。
4. 从昨日 `eod_price/Future` 中筛出股指真实月合约，并按 `IC / IF / IH / IM` 分组。
5. 每组按成交额排序后取 Top4，构建今日监控清单。
6. 为每个入选合约补齐：
   - 到期日
   - 对应指数代码
   - 指数名称
   - 指数昨日收盘价
7. 使用 CTP 订阅这些监控合约。
8. 每当收到一笔实时 Tick：
   - 读取 `LastPrice`
   - 查出该合约对应的指数昨日收盘价与到期日
   - 计算基差与年化基差率
   - 写入监控结果
   - 若年化基差率为负，则触发告警状态机

## 输出要求

### 终端输出
终端输出应按品种分块展示，每个品种展示其 Top4 合约。例如：

```text
===============================
📊 中证500 年化基差率（昨日成交额Top4合约）
======================================================================
IC2503 | 指数6300.0 | 期货6230.0 | 基差70.0 | 剩余-5天 | 年化基差率 0%
IC2504 | 指数6300.0 | 期货6190.0 | 基差110.0 | 剩余23天 | 年化基差率 27.79%
IC2506 | 指数6300.0 | 期货6080.0 | 基差220.0 | 剩余86天 | 年化基差率 14.83%
IC2509 | 指数6300.0 | 期货5980.0 | 基差320.0 | 剩余177天 | 年化基差率 10.49%
=======================================
```

实际运行时应至少展示 4 个分组：
- 沪深300（`IF`）
- 上证50（`IH`）
- 中证500（`IC`）
- 中证1000（`IM`）

### 日志
需要保留并明确分离：
- 运行日志
  - 启动、加载数据、选池、连接、订阅、异常
- 告警日志
  - 进入负值告警
  - 恢复告警

### 监控结果落盘
至少应包含：
- timestamp
- contract_code
- product_group
- index_code
- index_name
- index_price
- future_price
- basis
- remaining_days
- annual_rate
- negative_flag

## 告警要求
- 当 `annual_rate < 0` 时触发告警。
- 同一合约连续处于负值状态时，不应重复刷屏。
- 从负值恢复到非负值时，记录一条恢复日志。
- 第一版告警渠道：
  - terminal output
  - alert log file

## 错误处理
- 若昨日 `eod_price/Future` 文件缺失，程序应启动失败并提示“无法构建监控清单”。
- 若某个品种可用真实月合约少于 4 个，则展示实际数量，并记录警告。
- 若某个入选合约缺少到期日或缺少指数映射，跳过该合约并记录警告。
- 若 `CTP` 订阅请求返回负值，不应无限等待，应立即失败并记录原因。
- 若 `OnRspSubMarketData` 中某个合约报错，不应仅看最后一个回调结果，应正确聚合所有订阅结果。

## 测试与验收

### 数据侧验收
- 能正确读取 4 类 CSV。
- 能正确识别真实月合约与连续合约。
- 能正确筛出 `IC / IF / IH / IM` 各自成交额 Top4。
- 能正确为每个入选合约补齐：
  - 到期日
  - 对应指数
  - 指数名称
  - 指数昨日收盘价

### 运行侧验收
- 启动时只订阅各品种 Top4 合约。
- 能按 `IC / IF / IH / IM` 四个板块输出监控结果。
- 能在终端持续输出监控结果。
- 能在年化基差率为负时报警。
- 能在恢复时输出 recovery。
- 能写出运行日志、监控结果和告警日志。

### 环境侧验收
- 在 Linux 环境中可成功构建。
- 在真实 CSV 和真实 CTP 环境下联调成功。

## 风险与限制
- 第一版使用“指数昨日收盘价”作为全天固定基准，因此属于“准实时监控”，不是严格意义上的指数实时基差监控。
- 如果服务器日行情 CSV 更新延迟，则选池结果和指数基准可能滞后。
- 若股指真实月合约映射或到期日数据异常，将直接影响年化基差率准确性。
- 当前项目在 CTP 订阅请求返回值处理、订阅错误聚合上仍需补强，这在上线前必须修复。

## 对旧方案的替代说明
本设计文档替代旧版“通过 `contracts.json` 手工配置监控合约 + 通过 `spot_price.json` 手工提供指数价格”的第一阶段方案。旧方案可保留作为临时调试手段，但不再作为正式业务流程。

## 建议
将后续开发分为两步推进：
1. 先完成“CSV 分品种 Top4 选池 + 基础映射 + 接入现有监控引擎”的闭环。
2. 再完成“终端分组展示样式、存储落盘、告警日志、Linux 联调验收”的交付收尾。
