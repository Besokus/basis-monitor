# 面试故事库：Basis Monitor

说明：

- 本文档按 STAR 思路整理，但为了便于面试复述，额外加入了“为什么难”“技术决策”“验证证据”“后续演进”。
- 仅整理与当前仓库和开发过程有直接证据关系的故事。
- 与当前项目无关的方向，例如 DeerFlow、MoneyPrinterTurbo、视频生成链路、artifacts 回传等，不纳入本文档，避免混淆项目边界。

## 故事 1：把单行情源监控改造成 `CTP + XTP` 双链路联合架构

### Story Title
为什么要把 `CTP / XTP` 二选一改成双链路职责分离

### Situation
项目原本更接近单行情源模式：`CTP` 和 `XTP` 是互斥的，代码和配置都偏向“只能选一个 provider”。但业务目标已经变成：

- `CTP` 获取股指期货实盘价
- `XTP` 获取指数实盘价
- 用两路数据共同参与年化基差监控

### Task
在尽量不推翻现有监控主链的前提下，把系统演进为“双链路接入 + 联合计算可扩展”的结构。

### Challenge
- 现有代码默认所有 tick 都是同一类数据
- `CTP` 与 `XTP` 的订阅对象完全不同
- 如果处理不好，会变成“两个接口都接上了，但业务链还是单路在跑”

### Why it was hard
- 不是简单加一个接口，而是要重新定义：
  - tick 的语义
  - 订阅对象来源
  - 会话启动方式
  - 计算链入口
- 同时又不能把已有 `CTP` 主链稳定性打散

### Action
- 给 `MarketTick` 增加 `provider` 和 `instrument_type`
- 增加双会话工厂，让 `CTP + XTP` 能同时启动
- 监听器按 `Future/Index` 分流
- `CTP` 订阅 Top4 期货，`XTP` 订阅对应指数
- 后续再把两者在计算层合流

### Key technical decision
不让两个接口订阅同一种对象，也不在配置层硬拼，而是：

- `CTP` 专注期货
- `XTP` 专注指数
- 用同一批监控合约结果驱动两边订阅

### Validation / Evidence
- 双会话工厂：[src/market_data/market_data_session_factory.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\src\market_data\market_data_session_factory.cpp)
- Tick 分流：[app/main.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\app\main.cpp)
- 单测：
  - `test_market_data_session_factory`
  - `test_xtp_market_data_session`
  - `test_md_spi_bridge`

### Result
- 代码层已经具备 `CTP + XTP` 双链路能力
- 为后续联合计算和健康状态拆分打下基础

### Lessons learned
- 多数据源改造最容易犯的错误，是“把接入做完了，但没有让业务链真正使用”
- 先把 tick 语义补齐，再谈联合计算，会更稳

### Interview follow-up angles
- 为什么不保持单 provider
- 为什么 `CTP` 和 `XTP` 不都订同一批合约
- 双链路异常时如何降级

## 故事 2：把年化基差计算从“昨收指数”升级成“实时联合计算”

### Story Title
如何把“接入两路行情”真正变成“用两路数据算结果”

### Situation
项目最初虽然能做基差监控，但指数价格来自参考 CSV 的昨收，不是实时指数价。这样做简单，但盘中告警精度不够。

### Task
在不大改现有告警和落盘主链的情况下，把计算输入升级成：

- `CTP` 期货实时价
- `XTP` 指数实时价

### Challenge
- 如果任一 tick 都触发重算，系统行为会更复杂
- 如果 `XTP` 没首包或 stale，还要避免静默错算
- 落盘和报表也必须跟着统一口径

### Why it was hard
- 这是“接入层”问题和“业务口径”问题的组合
- 稍不注意就会出现：
  - 计算用实时指数
  - 但报表和落盘还是旧口径

### Action
- 引入实时指数缓存
- `XTP` 指数 tick 只更新缓存
- `CTP` 期货 tick 触发计算
- 计算和落盘统一使用本次实际采用的指数价
- 快照也同步保存该指数价

### Key technical decision
采用“`CTP` 触发计算，`XTP` 提供最新指数缓存”的模型，而不是任一 tick 都重算。

### Validation / Evidence
- 计算入口：[src/monitor/basis_monitor_service.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\src\monitor\basis_monitor_service.cpp)
- 落盘一致性：[src/storage/basis_result_store.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\src\storage\basis_result_store.cpp)
- 快照一致性：[src/report/latest_basis_snapshot_store.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\src\report\latest_basis_snapshot_store.cpp)
- 回归测试：
  - `test_basis_monitor_service`
  - `test_basis_result_store`
  - `test_latest_basis_snapshot_store`

### Result
- 联合计算已经真正进入运行时主链
- 系统从“接了双行情但结果还是单路口径”升级为“结果、落盘、快照口径一致”

### Lessons learned
- 业务计算升级不能只改公式，还要看上下游是否同时跟上
- “实际使用了什么价格”本身就应该成为可追溯信息

### Interview follow-up angles
- 为什么选择 `CTP` 触发计算
- `XTP` 先动、`CTP` 不动时怎么处理
- 如何验证落盘和报表没偏离

## 故事 3：在“告警必须准确”和“系统要及时启动”之间做权衡

### Story Title
如何做“及时启动、严格告警”的平衡

### Situation
双行情接入后，必须回答一个很实际的问题：

- `XTP` 还没首包时，能不能先算
- `XTP` stale 时，能不能继续用旧指数价告警

### Task
在保证系统可用性的同时，明确联合计算与告警的准入门槛。

### Challenge
- 如果严格双首包，系统启动时效性差
- 如果放任使用旧指数价，告警准确性会受损

### Why it was hard
- 这是典型的工程取舍题，没有天然唯一正确答案
- 必须把业务偏好转成明确的运行时行为

### Action
- 沉淀决策文档
- 代码层实现两种显式跳过状态：
  - `WAITING_FOR_LIVE_INDEX`
  - `STALE_LIVE_INDEX`
- 只要 `XTP` 已启用，计算和告警都要求新鲜实时指数价

### Key technical decision
“启动偏时效性，计算与告警偏准确性”。

### Validation / Evidence
- 决策文档：[docs/decision_2026-04-08_live-index-accuracy-vs-timeliness.md](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\docs\decision_2026-04-08_live-index-accuracy-vs-timeliness.md)
- 代码实现：[src/monitor/basis_monitor_service.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\src\monitor\basis_monitor_service.cpp)
- 日志表现：[app/main.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\app\main.cpp)

### Result
- 告警准确性门槛变成显式系统行为，而不是靠操作人员理解
- 为后续现场排障提供了明确状态信号

### Lessons learned
- 涉及业务准确性的取舍，一定要单独沉淀决策，而不是只留在代码里

### Interview follow-up angles
- 为什么不做双首包硬阻断
- 为什么不直接回退昨收
- 如果系统更强调连续性，会怎么改

## 故事 4：在受限环境中设计“本地编译 + 中泰运行”的部署链

### Story Title
如何在“中泰不可上传源码”的约束下完成交付

### Situation
中泰服务器有明确约束：

- 只能上传编译好的可执行文件和相关数据
- 不可以上传源代码

### Task
让项目在这个限制下仍然可部署、可运行、可回滚、可审计。

### Challenge
- 不能把“编译解决问题”留给目标机
- 动态库路径和 cwd 必须稳定
- 运行脚本、上传脚本、数据同步脚本必须协同

### Why it was hard
- 这不是单纯的代码问题，而是工程交付问题
- 一旦路径或库目录有一处漂移，现场就会出现“启动成功但实际跑不起来”

### Action
- 固化正式版 CTP runtime 和采集库 vendor 路径
- 维护 `push_prebuilt_runtime_to_zhongtai.sh`
- 维护 `start_prebuilt.sh`
- 明确本地和中泰的职责边界

### Key technical decision
把“构建环境”和“运行环境”彻底拆开，中泰只保留运行所需最小集合。

### Validation / Evidence
- 预编译上传脚本：[scripts/push_prebuilt_runtime_to_zhongtai.sh](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\scripts\push_prebuilt_runtime_to_zhongtai.sh)
- 后台启动脚本：[start_prebuilt.sh](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\start_prebuilt.sh)
- README 部署说明：[README.md](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\README.md)

### Result
- 构建和运行边界清晰
- 本地服务器可以控制版本和依赖
- 中泰服务器只负责运行与产出结果

### Lessons learned
- 受限环境下，脚本和目录布局本身就是架构的一部分

### Interview follow-up angles
- 为什么不在目标机编译
- 如何管理动态库
- 如何验证跑的是最新预编译包

## 故事 5：定位并修复正式版 CTP 接入中的运行时问题

### Story Title
如何把“能编译”变成“能实盘登录并拿到首包”

### Situation
项目从旧测试版 CTP 库切到正式版 Linux API 和采集库时，出现了：

- 握手失败
- 动态库版本冲突
- 相对路径找库失败

### Task
让程序在正式实盘库下真正完成：

- connect
- login
- subscribe
- first tick

### Challenge
- 问题可能来自前置地址、库版本、运行时加载路径、采集库、部署脚本
- 单看配置很容易误判

### Why it was hard
- 这是典型的“运行时环境问题”，表面现象相似，根因却很多
- 需要同时查构建链、运行脚本、vendor 路径、现场日志

### Action
- 检查正式版头文件和 so 是否一致
- 固化项目内正式版库目录
- 修正运行时 `LD_LIBRARY_PATH`
- 增强运行时日志，打印 `MdApiVersion` 和 `DataCollectVersion`
- 用现场日志确认 `OnFrontConnected`、`OnRspUserLogin`、`OnRspSubMarketData`、`MARKET_DATA_OK`

### Key technical decision
通过项目内 vendor 目录锁定正式版 runtime，而不是继续引用外部散落的 SDK 目录。

### Validation / Evidence
- 运行时版本日志：[src/ctp/md_api_session.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\src\ctp\md_api_session.cpp)
- CTP 回调日志：[src/ctp/md_spi_bridge.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\src\ctp\md_spi_bridge.cpp)
- 开发过程已记录过现场成功日志，当前仓库代码与脚本已与该路径对齐

### Result
- `CTP` 实盘收数链路已达到可验证可复现状态

### Lessons learned
- 现场接入问题要优先看“实际加载了什么库”，而不是只看配置文本

### Interview follow-up angles
- 如何快速区分网络问题和库问题
- 为什么要打印版本日志
- 如何保证预编译包在目标机加载正确库

## 当前系统仍不完善之处，以及如果继续做会如何演进

### 1. `XTP` 现场连续收数证据还不够完整
- 现状：代码链路已齐，首包与连续 tick 仍需交易时段验证
- 演进：补连续日志、首包时间、断流恢复统计

### 2. 报表可信度表达仍偏向 `future` 通道
- 现状：报表头部没有体现 `index` 通道健康度
- 演进：不一定要在报表里直接打印，但后台至少要保留双通道审计信息

### 3. 启动成功的语义仍偏宽松
- 现状：双链路模式下，`CTP` ready 即可进入运行态
- 演进：增加更显式的 degraded mode 或启动阶段状态划分

### 4. `CTP` 启动等待仍缺少真实可配置超时
- 现状：`INFINITE` 等待不利于卡死场景定位
- 演进：拆成连接、登录、订阅三个阶段的超时和失败分类

### 5. 文档和过程记录需要持续收口
- 现状：README 与部分过程文档仍有旧口径残留
- 演进：以本轮整理的知识资产为主，后续每次关键决策或阶段变更都同步更新
