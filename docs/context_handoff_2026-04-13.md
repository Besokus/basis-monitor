# Basis Monitor Context Handoff

更新时间：`2026-04-13`

本文档用于压缩当前会话上下文，方便后续继续开发、排障或交接。

## 1. 当前目标

项目当前运行模式是双机协作：
- 中泰服务器运行 `basis_monitor`
- 本地服务器负责上传参考数据、拉取输出、企业微信转发

当前关注重点：
- `CTP` 期货行情接收
- `XTP` 指数行情接收
- 告警 CSV 正确落盘并被本地 relay 正确解析
- 日内报表只发送当日 txt，不重放历史日期文件

## 2. 已完成的关键改动

### 2.1 告警链路
- `alert_events.csv` 已扩展为 12 列快照格式：
  - `timestamp,contract,product_group,index_code,index_name,index_price,future_price,basis,annual_rate,remaining_days,transition,reason`
- 本地 relay 直接读取扩展后的告警 CSV，不再补查 `basis_results.csv`
- 告警去重基于事件唯一键，不再按行号去重

相关文件：
- `include/basis_monitor/domain/alert_event.h`
- `src/storage/alert_store.cpp`
- `scripts/relay_zhongtai_notifications.py`

### 2.2 报表链路
- 中泰侧生成 `data/output/reports/*_latest_basis.txt`
- 本地 relay 拉取 txt 并解析为企业微信 markdown
- 报表分组顺序固定为 `IF -> IC -> IM`
- 组内按合约月份升序排序
- 本地 relay 现已限制只发送“当前交易日”报表

相关文件：
- `scripts/relay_zhongtai_notifications.py`
- `src/notify/wecom_message_formatter.cpp`

### 2.3 双机调度
- 本地：
  - `09:30` 上传参考 CSV 并启动 relay
  - `15:05` 停止 relay
- 中泰：
  - `09:31` 启动 `basis_monitor`
  - `15:05` 停止并清理 `logs` / `data/output` 下文件

相关脚本：
- `scripts/run_local_daily_schedule.sh`
- `scripts/run_remote_daily_schedule.sh`

## 3. 当前确认有效的运行判断方法

### 3.1 CTP 行情正常
查看 `logs/runtime.log`：
- 出现 `FrontMdAddr = ...`
- 出现 `<OnRspUserLogin>` 且无错误
- 出现 `<OnRspSubMarketData>`
- 出现 `[MARKET_DATA_OK]`
- 持续出现 `[MD_TICK]`

### 3.2 XTP 行情正常
查看 `logs/runtime.log`：
- 出现 `XtpIndexInstrumentIDs = ...`
- 持续出现 `[INDEX_TICK] provider=[xtp] ...`
- 不长期出现：
  - `WAITING_FOR_LIVE_INDEX`
  - `STALE_LIVE_INDEX`

### 3.3 联合计算正常
查看：
- `data/output/basis_results.csv` 是否持续追加
- `logs/runtime.log` 是否持续输出各组基差结果

## 4. 已知问题与结论

### 4.1 旧 8 列表头导致告警 CSV 解析错位

现象：
- 本地已拉到新告警数据
- 企业微信却没有正确发送当天新告警，或仍出现旧历史告警

根因：
- 旧 `alert_events.csv` 文件最初是 8 列表头
- 新版本程序在文件非空时不会重写表头
- 后续追加了 12 列数据，导致 `csv.DictReader` 按旧表头错位解析

结论：
- 删除旧 `alert_events.csv` 后，由当前程序重新创建文件，才能写出正确 12 列表头
- 日终清理和次日重建文件是防止问题复发的关键

### 4.2 报表会重发历史日期

现象：
- 本地 `relay_spool/reports` 中存在多日 txt
- relay 重启后可能把之前日期的报表再次发出

根因：
- 旧逻辑只按 `sent_reports` 去重，不限制报表日期必须等于当天

结论：
- 已修复为只发送 `current_trade_date()` 对应日期的 txt 报表

## 5. 当前部署路径口径

### 本地服务器
- 项目目录：
  - `/home/sdgy-yp/basis-monitor/basis_monitor`

### 中泰服务器
- 项目根目录：
  - `/list/10.101.5.62/basis-monitor-zhongtai/basis_monitor`

## 6. 最近关键提交

- `e6dea7f` `feat: 调整日内报表分组顺序为 IF/IC/IM`
- `3651063` `chore: 更新中泰环境 CTP 配置与本地告警转发参数`
- `ddd83a2` `fix: 限制 relay 仅发送当日日内基差报表`

## 7. 后续继续开发时优先检查

1. 中泰当天新建的 `alert_events.csv` 是否为 12 列表头
2. 本地 `relay_spool/alert_events.csv` 是否只包含当天新告警
3. 本地 `relay_state.json` 的 `alert_trade_date` 与当天是否一致
4. `logs/runtime.log` 中是否同时有 `MD_TICK` 和 `INDEX_TICK`
5. `relay_spool/reports` 中是否混有历史日期 txt
