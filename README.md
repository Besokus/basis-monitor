# Basis Monitor — 股指期货日内基差实时监控系统

面向量化私募的股指期货基差实时监控与告警系统。自动筛选 IF/IC/IM 主力合约，接入 CTP/XTP 双路行情，逐 tick 计算年化基差率，负基差时通过企业微信机器人实时推送告警，11:30/15:00 自动生成图文报告。

## 核心价值

- **全自动合约筛选**：启动时从参考数据中按成交额自动选取 IF/IC/IM 各自 Top4 真实月份合约，无需手动维护合约列表
- **双行情源冗余**：同时支持 CTP 和 XTP 行情接入，可灵活切换或并行使用，XTP 提供指数实时价提升基差计算精度
- **逐 tick 实时计算**：每笔期货行情到达即计算年化基差率，结果写入 CSV 并实时刷新终端监控面板
- **负基差智能告警**：年化基差率跌破阈值时立即通过企业微信推送，支持重复提醒和状态保持，避免频繁抖动
- **定时图文报告**：每日 11:30 和 15:00 自动生成基差快照报告（文本 + PNG 图片），按 hs300/zz500/zz1000 分组展示
- **双服务器安全部署**：核心监控程序以编译后的二进制形式在中泰服务器运行，源代码不离开本地；通过 SFTP 实现数据摆渡和通知中继
- **零外部依赖的 PNG 渲染**：内建 5×5 点阵字体 PNG 渲染器（纯 Python 标准库），无需 PIL/Pillow 等第三方包
- **完善的单元测试**：25 个单元测试覆盖核心计算、告警逻辑、存储、报告生成、行情接入等模块

## 技术栈

| 层级 | 技术 |
|---|---|
| 核心运行时 | C++17（GCC 7+ / Clang 5+） |
| 构建系统 | CMake 3.20+ |
| 行情接入 | CTP 正式版 MdApi（`thostmduserapi_se.so`）+ LinuxDataCollect |
| 指数行情 | XTP Quote SDK（`libxtpxquoteapi.so`），可选启用 |
| 数据存储 | CSV 文本格式（行情记录、基差结果、告警事件） |
| 报告渲染 | 内建点阵字体 PNG 渲染器（Python3 标准库，零额外依赖） |
| 消息推送 | 企业微信机器人 Webhook（Markdown + 图片） |
| 部署脚本 | Bash（构建/启停/数据摆渡/定时调度） |
| 运维编排 | Python3（SFTP 拉取、告警中继、报告生成、调度编排） |
| 进程管理 | nohup 后台运行 + PID 文件管理 |
| 版本控制 | Git |

## 项目架构

```
basis_monitor/
├── app/
│   ├── main.cpp                      # 唯一入口：启动→加载数据→连接行情→监控循环
│   └── prepare_reference_subset.cpp  # 参考数据子集过滤工具（按监控合约裁剪）
├── src/
│   ├── monitor/                      # 核心监控：基差计算、告警引擎、健康追踪
│   ├── market_data/                  # 行情会话：CTP/XTP 适配、会话工厂
│   ├── ctp/                          # CTP 桥接：MdApi SPI 回调、环境配置
│   ├── data/                         # 数据层：合约筛选、参考数据加载、订阅推导
│   ├── storage/                      # 存储层：Tick/基差/告警/报告 CSV 写入
│   ├── report/                       # 报告层：快照缓存、文本格式化、PNG 渲染
│   ├── notify/                       # 通知层：企业微信 Markdown/图片消息
│   ├── config/                       # 配置加载：INI/JSON 解析
│   └── logging/                      # 日志：分级文件日志
├── include/basis_monitor/            # 头文件（按模块镜像 src/ 结构）
├── config/
│   ├── ctp.ini                       # 主配置：CTP 连接、合约、Webhook、参考数据路径
│   ├── alert.json                    # 告警配置：阈值、重复间隔、中继参数
│   └── sftp.conf.example            # SFTP 配置模板
├── scripts/                          # 运维脚本（详见下方说明）
├── tests/                            # 25 个单元测试
├── vendor/ctp/                       # CTP SDK（header + .so）
├── deploy/                           # systemd 服务模板 + 部署文档
├── CMakeLists.txt                    # CMake 构建定义
├── run.sh                            # 构建 + 前台运行
├── start.sh / stop.sh                # 构建 + 后台启停
├── start_prebuilt.sh / stop_prebuilt.sh  # 预编译二进制后台启停
└── README.md
```

### 静态库划分

CMake 构建产出 **1 个可执行文件 + 6 个静态库 + 1 个辅助工具**：

| 库/目标 | 职责 | 核心源文件 |
|---|---|---|
| `basis_monitor_core` | Tick 处理、年化基差计算、负基差告警、行情健康检查 | `src/monitor/` |
| `basis_monitor_market_data` | CTP/XTP 行情 SPI 桥接、会话管理、配置加载、日志 | `src/ctp/`, `src/market_data/` |
| `basis_monitor_data` | 参考 CSV 加载、IF/IC/IM Top4 合约筛选、订阅合约推导 | `src/data/` |
| `basis_monitor_storage` | Tick 记录、基差结果、告警事件、报告文本的 CSV 追加写入 | `src/storage/` |
| `basis_monitor_report` | 11:30/15:00 定时报告格式化、PNG 图片生成、快照缓存 | `src/report/` |
| `basis_monitor_notify` | 企业微信机器人 Markdown/图片消息构造与发送 | `src/notify/` |
| `basis_monitor` | 可执行文件入口，串联全部模块 | `app/main.cpp` |
| `prepare_reference_subset` | 参考数据过滤工具，按监控合约裁剪后推送中泰 | `app/prepare_reference_subset.cpp` |

### 行情数据流

```
XTP 指数 Tick ──→ 缓存 latest_index_prices_（不触发计算）
CTP 期货 Tick ──→ 读取缓存指数价 → 计算基差+年化率 → 告警判定 → CSV 写入 → 终端刷新
```

关键门控：当 XTP 启用时，若指数行情未到达或超过 30 秒未更新，对应合约跳过本次计算，终端显示最近一次有效状态。

## 硬件与环境要求

| 项目 | 最低配置 | 推荐配置 |
|---|---|---|
| CPU | 1 核，x86_64 | 2 核以上 |
| 内存 | 256 MB | 512 MB+ |
| 磁盘 | 500 MB（含 CSV 和日志累积） | 2 GB+（保留历史数据） |
| 操作系统 | CentOS 7 / RHEL 7+ (glibc 2.17+) | CentOS 7.6+ / Ubuntu 20.04+ |
| 网络 | 稳定内网连接（CTP/XTP 行情前置 + SFTP） | 低延迟内网 |
| 运行时依赖 | glibc ≥ 2.17, libstdc++ ≥ 6.0.24, libpthread | — |
| 编译依赖 | GCC 7+/Clang 5+, CMake 3.20+, Python 3.6+ | GCC 9+, CMake 3.22+ |
| 行情 SDK | CTP MdApi (thostmduserapi_se.so), XTP Quote API (libxtpxquoteapi.so) | — |

> **注意**：系统仅支持 Linux 运行。Windows/macOS 可用于代码编辑和测试验证，但生产运行必须在 Linux 环境。

## 快速开始

### 1. 克隆与配置

```sh
git clone <repo-url>
cd basis_monitor

# 编辑 CTP 连接参数和 Webhook
vim config/ctp.ini

# 编辑告警阈值（可选，默认值可用）
vim config/alert.json
```

### 2. 构建

```sh
cmake -S . -B build
cmake --build build
```

### 3. 运行

```sh
# 前台运行（构建+启动）
sh run.sh

# 后台运行
sh start.sh        # 构建后启动
sh start_prebuilt.sh  # 使用已有 bin/basis_monitor 启动

# 停止
sh stop.sh         # 或 sh stop_prebuilt.sh
```

### 4. 运行测试

```sh
ctest --test-dir build --output-on-failure
```

## 配置说明

### config/ctp.ini — 主配置文件

```ini
[config]
# CTP 行情连接
FrontAddr=tcp://broker-ip:51205          # 交易前置
FrontMdAddr=tcp://market-data-ip:51213   # 行情前置
BrokerID=7080
UserID=your_user_id
Password=your_password

# 告警与报告推送
EnableWeComAlert=true                     # 启用负基差实时告警
EnableWeComReport=true                    # 启用定时图文报告
WeComRobotWebhook=https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=YOUR_KEY

# 参考数据目录（指向米筐数据同步目录）
ReferenceFutureMetadataDir=/data/riceQuantData/all_instruments/Future
ReferenceIndexMetadataDir=/data/riceQuantData/all_instruments/INDX
ReferenceFutureEodDir=/data/riceQuantData/eod_price/Future
ReferenceIndexEodDir=/data/riceQuantData/eod_price/INDX
```

### config/alert.json — 告警参数

```json
{
  "negative_threshold": 0.0,        // 年化基差率告警阈值（≤0 触发）
  "repeat_interval_minutes": 20,    // 重复告警间隔（分钟）
  "relay_pull_interval_seconds": 10,// 中继拉取间隔
  "report_relay_times": ["11:29", "14:59"],  // 报告转发时间窗口
  "wecom_max_send_retries": 3       // 企业微信发送重试次数
}
```

### config/sftp.conf — SFTP 连接（双机部署）

```sh
cp config/sftp.conf.example config/sftp.conf
# 编辑填入 SFTP_HOST, SFTP_USER, SFTP_PORT, SFTP_IDENTITY_FILE
```

## Python 脚本说明

本项目包含 5 个 Python 脚本，用于报告生成、告警转发和运维编排。所有脚本均使用 **Python3 标准库**，无需安装第三方包。

### `scripts/relay_zhongtai_notifications.py` — 告警与报告中继

从中泰服务器拉取到的 `alert_events.csv` 和报告文本文件中读取增量数据，转换为企业微信 Markdown 消息格式并推送。核心功能：

- **告警增量转发**：按 `alert_key` 去重，只推送新增的 `EnteredNegative` / `RepeatedNegative` 事件
- **报告 Markdown 格式化**：解析文本报告中的合约行，按品种分组，计算统计摘要（负基差个数、最高/最低年化率），生成结构化 Markdown
- **状态持久化**：通过 JSON state 文件记录已发送的告警和报告，支持跨重启增量续传
- **重试机制**：失败自动重试（可配置次数和间隔），超过上限自动放弃

```sh
python3 scripts/relay_zhongtai_notifications.py \
  --input-root ./relay_spool \
  --state-file ./relay_state.json \
  --webhook "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=..."
```

### `scripts/run_zhongtai_relay_orchestrator.py` — 中泰中继编排器

常驻进程，循环执行"拉取→告警转发→报告转发"流程。核心功能：

- 通过 SFTP 定期拉取中泰服务器的 `alert_events.csv`、`basis_results.csv`、报告文本
- 实时转发告警（每个拉取周期都执行）
- 在配置的报告时间窗口内（如 11:29-11:34、14:59-15:04）转发 MD 格式报告摘要
- 支持 `--once` 单次执行模式（用于 cron 调度）

```sh
python3 scripts/run_zhongtai_relay_orchestrator.py \
  --remote-target zhongtai@10.101.5.62 \
  --remote-project-dir /list/10.101.5.62/basis-monitor-zhongtai/basis_monitor \
  --once
```

### `scripts/generate_report_image_from_basis_results.py` — 报告 PNG 生成

从 `basis_results.csv` 读取指定交易日的数据，筛选截至 11:30 或 15:00 各合约最新 tick，计算涨跌/涨跌幅，生成报告 PNG 图片。核心功能：

- 自动识别 CSV 中最新的交易日
- 按报告时刻（1130/1500）截断数据
- 计算 `CHANGE`（较昨收涨跌）和 `CHANGE%`（涨跌幅）
- WARNING 列：当年化基差率跌破阈值时标红展示
- 输出 JSON 文档供 `render_basis_report_image.py` 渲染

```sh
python3 scripts/generate_report_image_from_basis_results.py \
  --basis-results ./relay_spool/basis_results.csv \
  --output ./reports/2026-05-15_1130_latest_basis.png \
  --moment 1130 \
  --negative-threshold 0.0
```

### `scripts/render_basis_report_image.py` — 纯 Python PNG 渲染器

零外部依赖的 PNG 图像生成引擎，特色是内建了一套 **5×5 像素点阵字体**（含 A-Z, 0-9, 空格, `-`, `.`, `/`, `:`, `%`），直接将 JSON 格式的报告文档渲染为 PNG 图片。核心功能：

- 手工实现 PNG 文件格式（IHDR/IDAT/IEND chunk 编码 + zlib 压缩）
- 内建 38 个字符的位图字体，无需任何字体文件
- 支持文字对齐（左/中/右）、颜色填充、斑马纹行背景
- 使用纯 Python `struct` + `zlib` + `bytearray` 实现像素级渲染

```sh
python3 scripts/render_basis_report_image.py report_document.json output.png
```

### `scripts/build_wecom_image_payload.py` — 企业微信图片载荷构建

将 PNG 图片文件 Base64 编码并计算 MD5，构造成企业微信机器人 `image` 类型的 JSON 消息载荷。

```sh
python3 scripts/build_wecom_image_payload.py report.png wecom_payload.json
```

## Shell 脚本说明

### 构建与运行

| 脚本 | 用途 |
|---|---|
| `run.sh` | CMake 构建 + 前台运行（日志输出到终端） |
| `start.sh` | CMake 构建 + nohup 后台启动，PID 写入 `runtime/basis_monitor.pid` |
| `stop.sh` | 读取 PID 文件，优雅停止后台进程 |
| `start_prebuilt.sh` | 使用 `bin/basis_monitor` 预编译二进制后台启动，不执行 CMake |
| `stop_prebuilt.sh` | 停止预编译模式的后台进程 |

### 双机部署

| 脚本 | 用途 |
|---|---|
| `scripts/push_reference_data_to_zhongtai.sh` | 从本地米筐数据目录选取最新 CSV → 按监控合约过滤 → SFTP 推送到中泰 staging 目录 |
| `scripts/pull_zhongtai_outputs.sh` | 通过 SFTP 从中泰拉取 `alert_events.csv`、`basis_results.csv`、报告文本、运行日志 |
| `scripts/push_prebuilt_runtime_to_zhongtai.sh` | 将本地编译的 `basis_monitor` 二进制 + CTP/XTP .so + 配置/脚本整体推送到中泰 |
| `scripts/prepare_zhongtai_runtime.sh` | 一键：本地构建 → 推参考数据 → 推运行时（组合上述脚本） |
| `scripts/sync_reference_data.sh` | 通过 rsync/scp 从远程同步参考数据到本地，自动校验 CSV 表头 |
| `scripts/validate_reference_data.sh` | 校验 staging 目录中四类 CSV 的存在性和必需列完整性 |
| `scripts/sftp_config.sh` | 被其他脚本 source 引用，统一加载 SFTP 连接配置并构建连接目标 |

### 调度与编排

| 脚本 | 用途 |
|---|---|
| `scripts/run_local_daily_schedule.sh start/stop` | 本地日度调度入口：推参考数据到中泰 + 启停中继编排器 |
| `scripts/run_remote_daily_schedule.sh start/stop` | 中泰日度调度入口：启停中泰上预编译的 `basis_monitor` 进程 |
| `scripts/start_zhongtai_relay_orchestrator.sh` | 后台启动 `run_zhongtai_relay_orchestrator.py` 常驻进程 |
| `scripts/stop_zhongtai_relay_orchestrator.sh` | 优雅停止中继编排器进程（含超时强制 kill） |

## 双服务器部署架构

```
┌─ 本地服务器（编译 + 中继） ─────────────────────────────────────┐
│                                                                   │
│  cmake 编译 → bin/basis_monitor                                   │
│  参考数据过滤 → SFTP 推送 ──────────────────────┐                │
│  中继编排器 ← SFTP 拉取 ←───────────────────────┤                │
│  PNG 报告渲染 → 企业微信 Webhook 推送            │                │
│                                                 │                │
└─────────────────────────────────────────────────┼────────────────┘
                                                  │ SFTP
┌─ 中泰服务器（仅运行二进制） ───────────────────┼─────────────────┐
│                                                 ▼                │
│  bin/basis_monitor（预编译，无源代码）                              │
│  读取本地 staging/ CSV → 连接 CTP/XTP → 基差监控                   │
│  输出 logs/ + data/output/*.csv + txt 报告                        │
│                                                                   │
└───────────────────────────────────────────────────────────────────┘
```

**安全设计要点**：
- 中泰服务器**永不接触源代码**，仅上传编译后的二进制 + .so + 配置文件
- 企业微信 Webhook Key 配置在本地服务器 relay 脚本，中泰不直连外网推送
- 数据摆渡全部通过 SFTP（支持密钥认证），按需拉取而非实时推送

## 年化基差计算逻辑

每笔期货行情到达时：

```
basis = index_close_yesterday - future_last_price
annual_rate = basis / index_close_yesterday × (365 / remaining_days) × 100%
```

- `remaining_days` = 合约到期日 - 当前交易日
- 若 `remaining_days ≤ 0`：年化基差返回 0%，不触发告警
- 若 `index_close_yesterday ≤ 0`：跳过该 tick（无效基准数据）

## 合约筛选规则

系统启动时自动从参考数据中筛选监控合约：

1. 过滤条件：`exchange == CFFEX` + `product == Index` + 真实月份合约
2. 排除连续合约：`88, 888, 889, 99, 88A2, 88A3`
3. 按 `total_turnover` 降序排列，每个品种取前 4 个：
   - **IF** → hs300（沪深300）
   - **IC** → zz500（中证500）
   - **IM** → zz1000（中证1000）
4. 由筛选结果自动推导 CTP 订阅合约 ID 和 XTP 指数订阅 ID

## 告警行为

- 年化基差率**越过** `negative_threshold` 阈值线 → 立即推送企业微信 Markdown 告警
- 持续低于阈值 → 每 `repeat_interval_minutes` 分钟重复提醒一次
- 回到阈值以上 → 静默恢复，不推送恢复通知
- 同一条告警在 relay 侧按 `timestamp|contract|index|transition|annual_rate` 去重

## 运行时输出

| 文件 | 说明 |
|---|---|
| `logs/runtime.log` | 运行时日志 |
| `logs/alert.log` | 告警日志 |
| `data/output/basis_results.csv` | 每 tick 基差计算结果 |
| `data/output/alert_events.csv` | 告警事件记录 |
| `data/output/reports/YYYY-MM-DD_1130_latest_basis.txt` | 11:30 文本报告 |
| `data/output/reports/YYYY-MM-DD_1500_latest_basis.txt` | 15:00 文本报告 |
| `data/output/reports/YYYY-MM-DD_1130_latest_basis.png` | 11:30 PNG 报告（需启用） |
| `data/output/reports/YYYY-MM-DD_1500_latest_basis.png` | 15:00 PNG 报告（需启用） |

## 单元测试

项目包含 **25 个单元测试**，覆盖所有核心模块：

| 测试 | 覆盖模块 |
|---|---|
| `test_basis_calculator` | 基差计算（含边界条件） |
| `test_alert_engine` | 告警引擎（阈值穿越、状态保持、重复提醒） |
| `test_basis_monitor_service` | 监控服务整体流程 |
| `test_market_data_health_tracker` | 行情健康追踪（超时/恢复判定） |
| `test_market_data_health_registry` | 行情健康注册表（多合约管理） |
| `test_config_loader` | INI 配置加载 |
| `test_logger` | 日志系统 |
| `test_reference_data_loader` | 参考 CSV 加载与校验 |
| `test_contract_selector` | 合约筛选逻辑 |
| `test_reference_subset_builder` | 参考数据子集过滤 |
| `test_subscription_instrument_builder` | 订阅合约 ID 推导 |
| `test_md_spi_bridge` | CTP MdApi SPI 回调桥接 |
| `test_md_api_session` | CTP 行情 API 会话管理 |
| `test_market_data_session_factory` | 行情会话工厂 |
| `test_xtp_market_data_session` | XTP 行情会话 |
| `test_tick_store` / `test_basis_result_store` / `test_alert_store` / `test_report_store` | CSV 存储层 |
| `test_latest_basis_snapshot_store` / `test_basis_report_formatter` | 报告快照与格式化 |
| `test_report_image_formatter` / `test_report_image_renderer` | PNG 报告渲染 |
| `test_scheduled_report_service` | 定时报告服务 |
| `test_wecom_robot_notifier` / `test_wecom_message_formatter` | 企业微信通知 |

```sh
# 运行全部测试
ctest --test-dir build --output-on-failure

# 运行单个测试
./build/test_basis_calculator
```

## 参考数据来源

系统依赖四类 CSV 参考数据（通常来自米筐 RiceQuant）：

| 目录 | 内容 | 必需字段 |
|---|---|---|
| `all_instruments/Future/` | 期货合约元数据 | `order_book_id`, `exchange`, `underlying_symbol`, `product`, `maturity_date` |
| `all_instruments/INDX/` | 指数元数据 | `order_book_id`, `symbol` |
| `eod_price/Future/` | 期货日频数据 | `trade_date`, `order_book_id`, `close`, `total_turnover` |
| `eod_price/INDX/` | 指数日频数据 | `trade_date`, `order_book_id`, `close` |

每个目录使用**最新的 CSV 文件**。

## License

Internal use — 私募基金内部工具。
