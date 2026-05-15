# CTP 实盘接入计划（2026-04-08）

## 1. 目标

基于当前 `basis_monitor` 的 CTP 行情链路，完成从“旧版/测试版 MD 库”向“正式版实盘 API + 正式版看穿式采集库”的可控切换，确保：

- CTP 实盘行情可以稳定完成连接、登录、订阅、首笔行情接收
- 接入过程可分阶段验证、可回滚、可定位问题
- 不影响后续 `XTP` 指数行情与双行情架构推进

本计划只覆盖 **CTP 行情接入**，不覆盖交易下单链路。

## 2. 当前现状

### 2.1 当前程序实际加载情况

`basis_monitor` 当前链接的是项目内旧版 CTP MD 库：

- `vendor/ctp/include/ThostFtdcMdApi.h`
- `vendor/ctp/include/ThostFtdcUserApiDataType.h`
- `vendor/ctp/include/ThostFtdcUserApiStruct.h`
- `vendor/ctp/lib/linux/thostmduserapi_se.so`

当前 CMake 仅链接了：

- `thostmduserapi_se.so`

没有链接看穿式采集库。

### 2.2 当前现场症状

现场测试时已出现：

- TCP 端口可达
- CTP 程序进入握手阶段
- 报错 `Decrypt handshake data failed`

这类症状通常说明：

- 网络不是第一阻塞点
- 当前加载的 MD so 与目标柜台环境不匹配，或
- 正式环境要求的看穿式采集能力未正确接入

## 3. 已确认的正式版材料

### 3.1 正式版看穿式采集库

工作区中可见：

- `v6.7.0_CTP_api_clientdatacollectdll_linux64/DataCollect.h`
- `v6.7.0_CTP_api_clientdatacollectdll_linux64/LinuxDataCollect.so`

这说明 Linux 侧正式版采集库材料已具备。

### 3.2 正式版实盘 API

你指定的实盘 API 为：

- `v6.7.11_P4_20251120_traderapi`

当前工作区内已可见 Linux 版正式材料：

- `v6.7.11_P4_20251120_traderapi/.../v6.7.11_20250617_api_traderapi_se_linux64/ThostFtdcMdApi.h`
- `v6.7.11_P4_20251120_traderapi/.../v6.7.11_20250617_api_traderapi_se_linux64/ThostFtdcTraderApi.h`
- `v6.7.11_P4_20251120_traderapi/.../v6.7.11_20250617_api_traderapi_se_linux64/ThostFtdcUserApiDataType.h`
- `v6.7.11_P4_20251120_traderapi/.../v6.7.11_20250617_api_traderapi_se_linux64/ThostFtdcUserApiStruct.h`
- `v6.7.11_P4_20251120_traderapi/.../v6.7.11_20250617_api_traderapi_se_linux64/thostmduserapi_se.so`
- `v6.7.11_P4_20251120_traderapi/.../v6.7.11_20250617_api_traderapi_se_linux64/thosttraderapi_se.so`

这意味着：Linux 侧正式版 API/MD so 材料已经具备，计划可以直接进入替换实施阶段。

## 4. 设计判断

### 4.1 为什么当前会失败

从代码和运行现象看，当前失败更像是“库版本/采集接入”问题，而不是配置文本问题：

- 程序能正确读取 `FrontMdAddr/BrokerID/UserID/Password`
- 程序能发起连接并进入握手阶段
- 端口层连通
- 但在握手解密阶段失败

因此，优先级最高的不是继续改 `ctp.ini`，而是先把 CTP 运行时库栈切换到正式环境匹配版本。

### 4.2 为什么要分阶段接入

当前系统已经同时承载：

- 本地参考数据筛选
- CTP 期货行情
- XTP 指数行情
- 监控计算与落盘

如果一次性同时替换：

- CTP MD so
- 看穿式采集库
- 头文件
- CMake 依赖
- 认证字段逻辑

那么问题定位会非常困难。

因此建议采用“最小可验证增量”方式推进。

## 5. 分阶段接入计划

### 阶段 A：建立正式版 CTP 运行时基线

目标：

- 将 `basis_monitor` 的 CTP MD 依赖从旧版 vendor 切换到正式版目标版本
- 不动主业务逻辑

动作：

1. 在 `basis_monitor/vendor/ctp/` 下建立正式版材料归档策略，避免直接覆盖旧文件后失去对比基线。
2. 保留当前旧版材料作为 `backup` 或 `legacy` 参考。
3. 引入正式版 Linux 侧：
   - `ThostFtdcMdApi.h`
   - `ThostFtdcUserApiDataType.h`
   - `ThostFtdcUserApiStruct.h`
   - `thostmduserapi_se.so`
4. 暂不修改业务逻辑，仅完成头文件和 so 的切换。

验收：

- 程序可重新编译
- 运行时不再出现旧 so 路径
- 进入连接/登录阶段时错误形态发生变化，或直接连通成功

回滚：

- 恢复旧 vendor/ctp 即可

### 阶段 B：接入正式版看穿式采集库

目标：

- 将 `LinuxDataCollect.so` 与 `DataCollect.h` 正式纳入 `basis_monitor`

动作：

1. 在 `vendor/ctp` 下增加采集库目录，例如：
   - `vendor/ctp/include/DataCollect.h`
   - `vendor/ctp/lib/linux/LinuxDataCollect.so`
2. 在 CMake 中增加对 `LinuxDataCollect.so` 的链接与运行时 RPATH。
3. 参考 `6.6.5_demo` 的 Linux 工程，把采集库纳入构建链。
4. 第一阶段只完成“链接成功 + 可加载”，不急着大改业务代码。

验收：

- `ldd basis_monitor` 能看到 `LinuxDataCollect.so`
- 运行时能正确加载采集库
- 握手/登录问题不再因缺采集库而失败

回滚：

- 保持采集库链接改动可单独回退

### 阶段 C：补齐看穿式采集初始化流程

目标：

- 让程序不仅“链接采集库”，而且按正式柜台要求完成看穿式采集初始化

动作：

1. 梳理 `6.6.5_demo` 中采集库调用点。
2. 确认哪些调用是交易 API 必需、哪些对 MD 登录也有前置要求。
3. 将最小必要的采集初始化迁移到 `basis_monitor` 的启动链路。
4. 增加启动日志，明确打印：
   - 采集库版本
   - 采集初始化结果
   - 是否已上报系统信息

验收：

- 能看到采集初始化成功日志
- CTP 登录链路进入明确回调阶段

### 阶段 D：实盘最小连通验证

目标：

- 用正式版库栈完成最小实盘验证

动作：

1. 仅启用 CTP：
   - `EnableCtpMarketData=true`
   - `EnableXtpMarketData=false`
2. 只验证：
   - `OnFrontConnected`
   - `OnRspUserLogin`
   - `OnRspSubMarketData`
   - `MARKET_DATA_OK`
3. 不联动 XTP，不做双会话联调。

验收：

- 登录成功
- 订阅成功
- 收到首笔期货行情

### 阶段 E：回归双行情模式

目标：

- 在 CTP 正式接入稳定后，再重新启用 `CTP + XTP`

动作：

1. 恢复：
   - `EnableXtpMarketData=true`
2. 验证：
   - `CTP` 正常进期货计算链
   - `XTP` 正常产出 `INDEX_TICK`
   - 两边互不阻塞

## 6. 建议的代码改造顺序

### 优先级 P0

1. CMake 支持切换正式版 MD so 与 `LinuxDataCollect.so`
2. vendor 目录结构整理
3. 增加运行时库加载可观测日志
4. 最小 CTP 单链路连通验证

### 优先级 P1

1. 补齐采集初始化逻辑
2. 增加更明确的登录阶段日志
3. 形成“正式版 / 旧版”切换开关

### 优先级 P2

1. 恢复双行情联调
2. 再推进 `XTP` 指数价进入计算链

## 7. 风险与控制点

### 风险 1：仅替换头文件不替换 so

影响：

- 编译可过，运行期仍失败

控制：

- 头文件与 so 必须成套替换

### 风险 2：仅链接采集库但未执行初始化

影响：

- 仍可能握手失败或登录失败

控制：

- 采集库接入要分“链接成功”和“初始化成功”两层验收

### 风险 3：一次性联动双行情

影响：

- 问题根因难以定位

控制：

- 严格按“CTP 单链路成功后，再回归双行情”的顺序推进

## 8. 建议的验收标准

### 阶段 A/B 验收

- `ldd` 输出指向正式版 so
- `LinuxDataCollect.so` 已被链接和加载

### 阶段 C 验收

- 日志中出现采集初始化成功信息

### 阶段 D 验收

- `OnFrontConnected`
- `OnRspUserLogin` 且 `ErrorID [0]`
- `OnRspSubMarketData`
- `MARKET_DATA_OK`

### 阶段 E 验收

- `CTP` 与 `XTP` 同时运行
- `CTP` 正常进监控链
- `XTP` 正常产出指数 tick

## 9. 当前建议的执行结论

当前最稳妥的路线是：

1. 不再继续用旧版 `vendor/ctp/lib/linux/thostmduserapi_se.so` 做实盘验证。
2. 直接切换到 `v6.7.11_P4_20251120_traderapi` 中已补齐的 Linux 版正式 API/MD so。
3. 把 `LinuxDataCollect.so` 纳入 `basis_monitor` 构建与运行时。
4. 先跑“只启用 CTP”的最小连通验证。
5. CTP 稳定后，再恢复双行情联调。

## 10. 待确认事项

当前计划已具备实施前提，后续只需按阶段执行替换与验证。

建议实施顺序：

1. 在 `vendor/ctp` 中备份旧版头文件和 `thostmduserapi_se.so`
2. 将正式版 Linux API/MD so 与头文件切换到 `vendor/ctp`
3. 在 CMake 中增加 `LinuxDataCollect.so` 链接和运行时路径
4. 增加采集库加载与初始化日志
5. 先做 CTP 单链路连通验证
