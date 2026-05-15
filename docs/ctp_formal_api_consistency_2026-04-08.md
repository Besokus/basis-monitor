# CTP 正式版接口一致性检查与实施补充

## 1. 结论

`v6.7.11_20250617_api_traderapi_linux64` 下的正式版 Linux API 与当前项目里使用的旧测试版 **不是完全一致**。

更准确的判断是：

- 主体行情接入流程仍然兼容
- 工厂函数签名已经升级
- 数据类型和结构体定义已经扩展
- 看穿式采集链仍需单独接入，不能只换一个 `thostmduserapi_se.so`

因此，后续实盘 CTP 接入必须按“头文件 + so + 采集库 + 初始化流程”一起规划，而不能做成只替换运行库的热切换。

## 2. 本次核对范围

对比对象：

- 当前项目旧版 vendor
  - `vendor/ctp/include/ThostFtdcMdApi.h`
  - `vendor/ctp/include/ThostFtdcUserApiDataType.h`
  - `vendor/ctp/include/ThostFtdcUserApiStruct.h`
  - `vendor/ctp/lib/linux/thostmduserapi_se.so`
- 正式版 Linux API
  - `v6.7.11_P4_20251120_traderapi/.../ThostFtdcMdApi.h`
  - `v6.7.11_P4_20251120_traderapi/.../ThostFtdcTraderApi.h`
  - `v6.7.11_P4_20251120_traderapi/.../ThostFtdcUserApiDataType.h`
  - `v6.7.11_P4_20251120_traderapi/.../ThostFtdcUserApiStruct.h`
  - `v6.7.11_P4_20251120_traderapi/.../thostmduserapi_se.so`
  - `v6.7.11_P4_20251120_traderapi/.../thosttraderapi_se.so`
- 正式版看穿式采集库
  - `v6.7.0_CTP_api_clientdatacollectdll_linux64/DataCollect.h`
  - `v6.7.0_CTP_api_clientdatacollectdll_linux64/LinuxDataCollect.so`
- 参考样例
  - `6.7.11apidemo/6.6.5_demo/CMakeLists.txt`
  - `6.7.11apidemo/6.6.5_demo/main.h`
  - `6.7.11apidemo/6.6.5_demo/traderApi.cpp`

## 3. 已确认的接口差异

### 3.1 MdApi 工厂函数签名已变

旧版：

```cpp
CreateFtdcMdApi(
    const char *pszFlowPath = "",
    const bool bIsUsingUdp = false,
    const bool bIsMulticast = false
)
```

正式版：

```cpp
CreateFtdcMdApi(
    const char *pszFlowPath = "",
    const bool bIsUsingUdp = false,
    const bool bIsMulticast = false,
    bool bIsProductionMode = true
)
```

这不是注释级变动，而是公开接口签名变化。当前项目代码仍按旧版三参数调用，后续切正式版时应显式改成四参数调用，并明确传生产模式。

### 3.2 TraderApi 工厂函数签名也已变

正式版新增：

```cpp
CreateFtdcTraderApi(
    const char *pszFlowPath = "",
    bool bIsProductionMode = true
)
```

虽然 `basis_monitor` 当前不走交易主链路，但这说明这套正式版是“生产/测评合并版”，不是旧测试版的简单 Linux 重打包。

### 3.3 用户数据类型与结构体已升级

核对结果显示：

- `ThostFtdcUserApiDataType.h` 大小和哈希均变化
- `ThostFtdcUserApiStruct.h` 大小和哈希均变化

这说明正式版不仅改了库文件，也新增了类型定义和结构体字段。

结合升级说明，可以确认至少包括：

- 登录响应增加 `LastLoginTime`
- 登录响应增加 `ReserveInfo`
- 新增 `ReqQryUserSession`
- 部分结构体新增字段，未适配终端可能出现行为差异

因此，正式版切换时不应采用“旧头文件 + 新 so”混搭模式。

## 4. 与当前项目的兼容性判断

当前 `basis_monitor` 在 CTP MD 侧只使用了最基础的一组接口：

- `CreateFtdcMdApi`
- `RegisterSpi`
- `RegisterFront`
- `Init`
- `ReqUserLogin`
- `SubscribeMarketData`

这意味着：

- 业务主流程改造量目前可控
- 不需要因为正式版升级就重写整个行情接入层

但同时也意味着：

- 必须成套替换正式版头文件和正式版 so
- 必须补上正式版看穿式采集链
- 必须增强启动日志，区分“库加载成功”“采集初始化成功”“前置连接成功”“登录回调成功”

## 5. 看穿式采集库的现状判断

当前项目 [`CMakeLists.txt`](/mnt/c/Users/yp636/Documents/workdir/6.7.11apidemo/.worktrees/basis-monitor/basis_monitor/CMakeLists.txt) 只链接了：

- `thostmduserapi_se.so`

尚未链接：

- `LinuxDataCollect.so`

这意味着当前工程还没有真正把正式柜台要求的看穿式采集链接入运行时。

而参考样例 [`6.6.5_demo/CMakeLists.txt`](/mnt/c/Users/yp636/Documents/workdir/6.7.11apidemo/6.7.11apidemo/6.6.5_demo/CMakeLists.txt) 在 Linux 下同时链接了：

- `thosttraderapi_se.so`
- `thostmduserapi_se.so`
- `LinuxDataCollect.so`

并且参考代码里明确使用了：

- `CTP_GetSystemInfo`
- `RegisterUserSystemInfo`
- `SubmitUserSystemInfo`

说明正式柜台接入不只是“能找到采集库”，还涉及终端系统信息采集和上报流程。

## 6. 对当前 CTP 问题的解释

现场已经确认：

- `112.65.172.131:51213` 端口可达
- 当前运行时实际加载的是旧版 `vendor/ctp/lib/linux/thostmduserapi_se.so`
- 程序报错为 `Decrypt handshake data failed`

结合本次接口核对，更合理的判断是：

1. 网络不是当前首要问题
2. 当前运行时库栈与正式柜台存在版本或模式不匹配
3. 看穿式采集链尚未接入，也可能是正式环境握手失败的重要原因

## 7. 对开发实施计划的补充

后续实盘 CTP 接入建议按下面顺序推进。

### P0. 正式版基线替换

1. 将 `vendor/ctp/include` 与 `vendor/ctp/lib/linux` 切换到正式版 Linux 材料
2. 不再保留旧头文件与新 so 混用
3. 把 `MdApiSession::Start()` 的 `CreateFtdcMdApi(...)` 调整为正式版签名，并显式传入生产模式

### P0. 采集库纳入构建与运行时

1. 在 CMake 中加入 `LinuxDataCollect.so`
2. 补充运行时 RPATH 或等价加载路径
3. 启动日志中增加采集库版本输出，建议调用 `CTP_GetDataCollectApiVersion()`

### P1. 采集初始化最小迁移

以 `6.6.5_demo` 为参考，梳理下列流程是否需要在当前项目中迁移：

- `CTP_GetSystemInfo`
- `RegisterUserSystemInfo`
- `SubmitUserSystemInfo`

这一阶段先做最小闭环，不扩展到交易业务逻辑。

### P1. 单链路验证

先只开 CTP：

- `EnableCtpMarketData=true`
- `EnableXtpMarketData=false`

验收日志目标：

- `OnFrontConnected`
- `OnRspUserLogin`
- `OnRspSubMarketData`
- `MARKET_DATA_OK`

### P2. 回归双行情

等 CTP 单链路稳定后，再恢复：

- `EnableXtpMarketData=true`

重新验证双会话模式。

## 8. 当前建议

当前最稳妥的判断是：

- 正式版接口和测试版不是完全一致
- 但差异仍然在可控范围内
- 下一步应该进入“成套替换正式版头文件与 so + 接入 LinuxDataCollect.so + 参考样例补最小采集初始化”的实施阶段

不建议继续在“旧 vendor so + 正式柜台”这个组合上反复排查配置文本。
