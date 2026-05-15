# 架构决策文档：Basis Monitor

本文档用于记录当前系统已经落地或已经形成共识的架构决策、边界、约束与技术债。目标不是复述实现细节，而是帮助后续开发者快速理解：

- 系统为什么要这样拆
- 关键约束来自哪里
- 哪些地方已经稳定
- 哪些地方只是阶段性选择
- 后续继续演进时哪些点最可能成为重构触发器

## 决策 1：采用 `CTP` 与 `XTP` 双行情职责分离

### Title
双行情分工：`CTP` 负责期货，`XTP` 负责指数

### Status
Validated

### Context
系统的目标是对股指期货做盘中年化基差监控。要得到联合计算结果，至少需要：

- 期货实时价
- 指数实时价

项目早期是单行情源模式，代码和配置更偏向“`CTP` 或 `XTP` 二选一”。随着目标调整，需要在同一进程中同时处理期货与指数两个数据源。

### Problem
单行情源模式无法同时满足以下要求：

- 使用 `CTP` 获取期货实盘行情
- 使用 `XTP` 获取股指指数行情
- 用两路实时数据参与同一套年化基差计算

### Decision
采用双行情职责分离：

- `CTP` 只负责期货实时行情
- `XTP` 只负责指数实时行情
- 两者在监听器和计算层合流，而不是在配置层做互斥切换

### Why this decision
- 更贴近各接口的实际可用性和业务分工
- 可以保留已有 `CTP` 期货主链稳定性
- 为“期货价与指数价分别来自不同柜台”的场景提供清晰边界

### Alternatives considered
- 继续保持单行情源模式，只用 `CTP`
- 继续保持单行情源模式，只用 `XTP`
- 两路同时接入，但都订同一种标的，由后处理阶段再区分

### Trade-offs
- 优点：职责清晰，后续计算链和健康检查可以按通道拆开
- 代价：启动流程、健康检查、日志、调试复杂度都明显上升

### Risks / Limitations
- 双链路中任意一边异常都可能影响联合计算
- 需要处理两类 tick 的时序不同步问题
- `XTP` 现场收数稳定性仍需交易时段验证

### Evidence
- 双会话工厂：[src/market_data/market_data_session_factory.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\src\market_data\market_data_session_factory.cpp)
- Tick 语义区分：[include/basis_monitor/domain/market_tick.h](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\include\basis_monitor\domain\market_tick.h)
- 监听器分流：[app/main.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\app\main.cpp)

### Impact on future development
- 后续任何关于联合计算、健康监控、报表可信度的改造，都应以“future/index 双通道”作为基本假设
- 如果未来再接第三类行情源，应沿用“职责单一 + 运行时合流”的模式

## 决策 2：用同一份 staging 参考数据驱动 Top4 监控池

### Title
统一参考数据源：中泰本机 staging CSV 驱动监控合约选择

### Status
Validated

### Context
系统既要在本地服务器做参考数据清洗与上传，又要在中泰服务器上稳定运行。运行时必须基于同一套参考数据确定：

- 当天监控哪些期货合约
- 每个合约映射哪个指数
- 对应的昨收、到期日、分组信息

### Problem
如果监控池选择逻辑分散在多处，容易出现：

- 本地上传的是一套数据
- 中泰运行时又按另一套规则选合约
- `CTP` 和 `XTP` 订阅集合不一致

### Decision
统一使用中泰服务器上的 staging CSV 作为运行时参考数据源，由程序启动时本地读取这批 CSV，再选出当天 Top4 监控合约。

### Why this decision
- 让“上传数据”和“运行时订阅”之间有明确因果链
- 保证 `CTP` 和 `XTP` 订阅派生的输入一致
- 便于回溯：出现结果异常时，可以直接定位到当天 staging 文件

### Alternatives considered
- 直接在 `ctp.ini` / `xtp.ini` 静态写死订阅合约
- 在本地服务器先算出 Top4，然后把结果单独发给中泰
- 中泰运行时只读全量 CSV，不做二次筛选

### Trade-offs
- 优点：数据来源单一，现场问题更容易定位
- 代价：程序仍依赖参考 CSV 目录存在且结构正确

### Risks / Limitations
- 当前主流程只读取 `ctp.ini` 中的 `Reference*Dir`
- `xtp.ini` 里虽然也有 `Reference*Dir` 字段，但目前不参与主流程，容易造成运维误解

### Evidence
- 参考数据目录构造：[app/main.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\app\main.cpp)
- Top4 选择：[src/data/contract_selector.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\src\data\contract_selector.cpp)
- 中泰上传脚本：[scripts/push_reference_data_to_zhongtai.sh](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\scripts\push_reference_data_to_zhongtai.sh)

### Impact on future development
- 如果后续要支持多个运行环境，应优先把“参考数据目录”抽成共享配置，而不是让 `ctp.ini` / `xtp.ini` 各管一套

## 决策 3：运行时动态派生订阅列表，而不是依赖静态配置

### Title
订阅列表运行时派生：`CTP` 订 Top4 合约，`XTP` 订对应指数

### Status
Validated

### Context
`ctp.ini` 里可以写 `InstrumentID`，`xtp.ini` 里也可以写 `IndexInstrumentID`。但当天真正应该监控哪些合约，会随参考数据变化而变化。

### Problem
如果继续使用静态配置：

- `CTP` 可能订到不再属于当天 Top4 的合约
- `XTP` 可能订到和当前 Top4 无关的指数
- 联合计算会失去一致性

### Decision
- `CTP` 运行时订阅列表直接由当天 Top4 监控合约派生
- `XTP` 运行时订阅列表由这批监控合约的 `index_code` 去重派生
- `xtp.ini` 的 `IndexInstrumentID` 只保留为兜底 fallback

### Why this decision
- 保证两条链路围绕同一批监控对象运转
- 避免人工维护两套静态订阅列表
- 为后续联合计算和审计提供一致基础

### Alternatives considered
- `CTP` 与 `XTP` 都继续使用静态配置
- `CTP` 动态，`XTP` 静态
- 用独立映射文件驱动 `XTP` 订阅

### Trade-offs
- 优点：订阅层与监控对象自动对齐
- 代价：启动流程比过去更依赖参考数据质量

### Risks / Limitations
- 指数代码格式必须与 `XTP` 柜台实际接受的格式一致
- 当前派生逻辑只做去重，不额外校验指数代码是否可订

### Evidence
- `CTP` 运行时订阅覆盖：[app/main.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\app\main.cpp)
- `XTP` 指数订阅派生：[src/data/subscription_instrument_builder.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\src\data\subscription_instrument_builder.cpp)
- `XTP` 订阅实现：[src/market_data/xtp_market_data_session.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\src\market_data\xtp_market_data_session.cpp)

### Impact on future development
- 后续如果支持更多指数或新的产品组，优先扩展“合约到指数”的映射逻辑，而不是回退到手工静态配置

## 决策 4：用 `CTP` 期货 tick 作为联合计算触发器

### Title
联合计算触发策略：`CTP` 触发，`XTP` 提供实时指数缓存

### Status
Accepted

### Context
联合计算引入后，系统需要决定：

- 是任一 tick 到来都重算
- 还是只在期货 tick 到来时重算

### Problem
如果 `XTP` 和 `CTP` 任一更新都触发重算，结果会更“实时”，但也会带来：

- 高频重复重算
- 同一笔期货价配多笔指数价反复刷新
- 告警、落盘和报表波动频率难以控制

### Decision
采用：

- `XTP` tick 到来时只更新缓存
- `CTP` tick 到来时才触发一次完整计算、落盘和告警评估

### Why this decision
- 期货价格是当前监控主链的核心输入
- 改动较小，能平滑承接现有 `BasisMonitorService`
- 更利于维持当前落盘、告警、报表节奏

### Alternatives considered
- 任一 tick 到来都重算
- 定时聚合重算，例如每 500ms / 1s 统一出结果

### Trade-offs
- 优点：时序更稳，行为更可解释
- 代价：指数先动、期货未更新时，结果不会立即刷新

### Risks / Limitations
- 在指数波动较快、期货成交较稀疏时，结果的刷新节奏取决于期货 tick

### Evidence
- `XTP` 只更新缓存：[src/monitor/basis_monitor_service.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\src\monitor\basis_monitor_service.cpp)
- 监听器分流：[app/main.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\app\main.cpp)

### Impact on future development
- 如果后续业务方要求“指数变动也要立即反映到结果”，需要重新评估这一触发策略，并同步收紧去重与节流逻辑

## 决策 5：优先保证告警准确性，但不阻断系统启动

### Title
策略选择：及时启动、严格计算与告警

### Status
Validated

### Context
联合计算后，系统需要平衡两个目标：

- 程序尽快启动并进入运行态
- 年化基差告警必须尽量准确

### Problem
如果要求 `CTP` 和 `XTP` 都拿到首包后才允许启动，系统时效性会下降；如果允许 `XTP` stale 时继续用旧指数价计算，又会伤害告警准确性。

### Decision
采用“启动偏时效性、计算与告警偏准确性”的策略：

- 启动不强制等待 `XTP` 首包
- 但只要 `XTP` 已启用，计算与告警必须依赖新鲜的实时指数价
- 没有实时指数价时，记录 `WAITING_FOR_LIVE_INDEX`
- 实时指数价 stale 时，记录 `STALE_LIVE_INDEX`
- 上述两种情况都暂停对应合约的计算与告警

### Why this decision
- 保持 `CTP` 主链的可观测性
- 避免把“系统启动成功”和“告警准确可信”混为一谈
- 让错误以显式跳过的形式暴露，而不是悄悄继续算

### Alternatives considered
- 严格双首包后才允许进入运行态
- 启动后允许继续使用旧指数缓存
- 没有实时指数时回退到昨收继续告警

### Trade-offs
- 优点：更符合“告警要准确”的业务要求
- 代价：在 `XTP` 缺席或短时断流时，会出现结果暂停更新

### Risks / Limitations
- 当前“启动成功”的语义仍偏宽松，可能导致运维误判
- 报表头部还没有显示 `index` 通道状态

### Evidence
- 决策记录：[docs/decision_2026-04-08_live-index-accuracy-vs-timeliness.md](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\docs\decision_2026-04-08_live-index-accuracy-vs-timeliness.md)
- 计算门槛实现：[src/monitor/basis_monitor_service.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\src\monitor\basis_monitor_service.cpp)
- 日志行为：[app/main.cpp](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\app\main.cpp)

### Impact on future development
- 后续如果要做“降级模式”展示，应直接围绕这条决策扩展，而不是改回隐式 fallback

## 决策 6：中泰服务器只运行预编译产物，不上传源代码

### Title
部署边界：中泰只跑预编译运行包

### Status
Validated

### Context
项目运行在中泰服务器，但工作约束明确：

- 只能上传编译好的可执行文件和相关数据文件
- 不允许上传源代码

### Problem
如果继续沿用“目标机编译 + 目标机运行”的方式：

- 会违反部署约束
- 会让动态库、依赖路径、编译环境差异带来更多不确定性

### Decision
采用双服务器分工：

- 本地服务器负责编译、预处理参考数据、SFTP 上传
- 中泰服务器只接收预编译二进制、运行库、配置和数据文件，并负责实际运行

### Why this decision
- 符合实际环境约束
- 明确了编译与运行责任边界
- 更利于固定动态库版本与运行目录结构

### Alternatives considered
- 中泰直接编译运行
- 上传源码后在中泰二次构建
- 仅上传单个二进制，不上传运行库

### Trade-offs
- 优点：运行环境更可控，可回滚性更好
- 代价：需要维护预编译上传脚本和运行脚本

### Risks / Limitations
- 预编译包与中泰运行目录的动态库路径必须保持一致
- 脚本和目录布局一旦漂移，现场容易出现“库找不到”或 cwd 错误

### Evidence
- 预编译上传脚本：[scripts/push_prebuilt_runtime_to_zhongtai.sh](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\scripts\push_prebuilt_runtime_to_zhongtai.sh)
- 中泰后台启动脚本：[start_prebuilt.sh](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\start_prebuilt.sh)
- AGENTS 约束：[AGENTS.md](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\AGENTS.md)

### Impact on future development
- 任何新增运行依赖，都必须同步纳入预编译上传与启动脚本校验

## 决策 7：正式版 CTP API 和采集库固化到项目内 vendor 目录

### Title
运行时库管理：项目内固化正式版 CTP runtime 与数据采集库

### Status
Validated

### Context
`CTP` 实盘接入需要：

- 正式版 Linux `thostmduserapi_se.so`
- 对应头文件
- `LinuxDataCollect.so`

同时，旧测试版路径和新正式版路径并存时，运行时极易加载错误版本。

### Problem
如果继续依赖外部 SDK 原始目录：

- 路径容易漂移
- 运行时容易误用旧版 so
- 中泰预编译部署也缺少稳定引用路径

### Decision
把正式版运行所需文件固化到项目内：

- `vendor/ctp/live/include`
- `vendor/ctp/live/lib/linux`
- `vendor/ctp/data_collect`

并清理旧测试库目录。

### Why this decision
- 降低路径不一致和运行时误加载风险
- 让预编译上传脚本、运行脚本、构建脚本都引用同一套正式版文件
- 方便回滚和代码审查

### Alternatives considered
- 继续直接引用外部原始 SDK 目录
- 只复制 so，不复制头文件
- 通过系统级安装路径提供 CTP 运行库

### Trade-offs
- 优点：依赖闭环更清晰
- 代价：仓库中需要维护一套 vendor runtime 目录

### Risks / Limitations
- 后续升级 CTP 版本时，要同步更新 vendor 内容和验证流程
- 当前仍是正常动态链接，不是 `dlopen` 式纯运行时加载

### Evidence
- 构建配置：[CMakeLists.txt](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\CMakeLists.txt)
- 运行脚本：[run.sh](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\run.sh) [start.sh](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\start.sh) [start_prebuilt.sh](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\start_prebuilt.sh)

### Impact on future development
- 未来如果引入 TraderApi 或升级到新版本，看穿式和 MdApi 也应继续沿用“项目内稳定 vendor”策略

## 决策 8：本地先清洗参考数据，再通过 SFTP 同步到中泰

### Title
数据同步策略：本地筛选 Top4 相关参考数据后再上传

### Status
Validated

### Context
中泰服务器不适合持有全量原始参考数据，也不希望把全市场 CSV 全量同步过去。项目需要一个低风险、可验证的数据同步方案。

### Problem
如果直接把全量数据上传到中泰：

- 传输量大
- 目录噪声高
- 运维排障难度大

### Decision
本地服务器先基于参考数据构建“当天监控合约及其对应指数”的精简 staging，再通过 SFTP 上传到中泰。

### Why this decision
- 与运行时实际订阅集合更一致
- 降低网络与存储开销
- 中泰只保留运行所需数据，符合最小暴露原则

### Alternatives considered
- 全量上传，再在中泰本机过滤
- 直接在 shell 里边传边筛
- 让程序启动时再在中泰上扫描更大目录

### Trade-offs
- 优点：上传更轻，数据更聚焦
- 代价：本地服务器多了一步预处理

### Risks / Limitations
- 本地预处理逻辑与运行时选池规则必须保持一致
- 当前 staging 仍然是“参考数据子集”，不是最终运行结果缓存

### Evidence
- 预处理工具与脚本：[scripts/push_reference_data_to_zhongtai.sh](c:\Users\yp636\Documents\workdir\6.7.11apidemo\.worktrees\basis-monitor\basis_monitor\scripts\push_reference_data_to_zhongtai.sh)
- 相关代码与测试可见现有实现与提交历史

### Impact on future development
- 如果未来监控品种扩大，优先扩展本地预处理逻辑，而不是恢复全量上传

## 已知限制、技术债与潜在重构点

### 1. `xtp.ini` 的参考数据目录字段当前不参与主流程
- 现状：加载了，但不生效
- 影响：容易让运维误以为“测 XTP 就该改 `xtp.ini`”
- 可能演进：把参考数据目录提升为共享配置，或明确删除 `xtp.ini` 中这几项

### 2. 双链路首包策略与“系统已启动”语义仍有偏差
- 现状：`CTP` ready 即可进入运行态，`XTP` 首包失败不会阻断启动
- 影响：从运维视角看，“启动成功”不等于“联合计算已完全可用”
- 可能演进：加入更显式的 degraded mode 或双状态启动日志

### 3. 报表元数据只反映 `future` 通道状态
- 现状：报表头部没有体现 `index` 通道健康度
- 影响：当 `XTP` stale 时，报表可信度表达不足
- 可能演进：不一定要在报表里直接打印字段，但后台至少应保留可追溯状态

### 4. `CTP` 启动阶段仍使用 `INFINITE` 等待
- 现状：连接、登录、订阅等待不可配置
- 影响：卡死场景下不够可控
- 可能演进：改为分阶段可配置超时，并记录阶段性诊断日志

### 5. 告警状态机没有显式恢复通知
- 现状：只有进入负值和持续负值提醒，没有恢复事件
- 影响：状态闭环不完整，但不影响“负值告警准确性”
- 可能演进：若业务希望更完整的状态表达，再单独补恢复通知

### 6. README 与部分过程文档尚未完全追上代码现状
- 现状：部分文档仍描述“昨收指数价”或旧阶段行为
- 影响：新同学阅读时容易把旧状态当现状
- 可能演进：继续做文档收口，优先保证“运行方式、口径、边界”一致
