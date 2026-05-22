# 中泰服务器部署说明

本文档面向中泰服务器部署 `basis_monitor` 的场景，默认项目根目录为：

```text
/list/10.101.5.62/basis-monitor-zhongtai
```

其中程序目录为：

```text
/list/10.101.5.62/basis-monitor-zhongtai/basis_monitor
```

## 1. 推荐目录约定

推荐使用以下目录结构：

```text
/list/10.101.5.62/basis-monitor-zhongtai/
  basis_monitor/
  data/
    staging/
      all_instruments/Future/
      all_instruments/INDX/
      eod_price/Future/
      eod_price/INDX/
```

说明：

- `basis_monitor/`：项目代码目录
- `data/staging/`：中泰服务器本地参考数据 staging 目录
- 程序运行时只读取本地 staging 目录，不直接读取远程服务器路径

## 2. 参考数据同步

推荐在盘前先将参考数据同步到中泰服务器本地 staging 目录，再启动监控程序。

项目内置了同步脚本：

```sh
cd /list/10.101.5.62/basis-monitor-zhongtai/basis_monitor
bash scripts/sync_reference_data.sh rsync <源服务器用户>@<源服务器地址>:<源数据根目录> /list/10.101.5.62/basis-monitor-zhongtai/data/staging
```

如果当前环境不能使用 `rsync`，也可以改用：

```sh
bash scripts/sync_reference_data.sh scp <源服务器用户>@<源服务器地址>:<源数据根目录> /list/10.101.5.62/basis-monitor-zhongtai/data/staging
```

脚本会自动同步并校验以下 4 个目录：

- `all_instruments/Future`
- `all_instruments/INDX`
- `eod_price/Future`
- `eod_price/INDX`

校验内容包括：

- 最新 CSV 文件存在
- 最新 CSV 文件非空
- 必要表头存在

如果校验失败，不要启动 `basis_monitor`。

## 3. 配置文件修改

编辑：

```text
/list/10.101.5.62/basis-monitor-zhongtai/basis_monitor/config/ctp.ini
```

将参考数据目录指向中泰服务器本地 staging：

```ini
MarketDataProvider=ctp

ReferenceFutureMetadataDir=/list/10.101.5.62/basis-monitor-zhongtai/data/staging/all_instruments/Future
ReferenceIndexMetadataDir=/list/10.101.5.62/basis-monitor-zhongtai/data/staging/all_instruments/INDX
ReferenceFutureEodDir=/list/10.101.5.62/basis-monitor-zhongtai/data/staging/eod_price/Future
ReferenceIndexEodDir=/list/10.101.5.62/basis-monitor-zhongtai/data/staging/eod_price/INDX
```

说明：

- 一期仍使用 `MarketDataProvider=ctp`
- `xtp` 配置入口已经预留，但当前阶段不作为实盘股指期货主行情源启用

## 4. 手工启动方式

### 4.1 前台调试启动

```sh
cd /list/10.101.5.62/basis-monitor-zhongtai/basis_monitor
bash run.sh
```

### 4.2 后台启动

```sh
cd /list/10.101.5.62/basis-monitor-zhongtai/basis_monitor
bash start.sh
```

### 4.3 停止

```sh
cd /list/10.101.5.62/basis-monitor-zhongtai/basis_monitor
bash stop.sh
```

后台运行时的重要文件：

- `logs/nohup.out` (launcher-level startup failures only)
- `logs/runtime.log`
- `logs/alert.log`
- `runtime/basis_monitor.pid`

## 5. systemd 服务方式

项目提供了服务模板：

```text
/list/10.101.5.62/basis-monitor-zhongtai/basis_monitor/deploy/systemd/basis-monitor.service
```

推荐安装步骤：

```sh
sudo cp /list/10.101.5.62/basis-monitor-zhongtai/basis_monitor/deploy/systemd/basis-monitor.service /etc/systemd/system/basis-monitor.service
sudo systemctl daemon-reload
sudo systemctl enable basis-monitor
sudo systemctl start basis-monitor
```

查看状态：

```sh
sudo systemctl status basis-monitor
```

查看 systemd 日志：

```sh
sudo journalctl -u basis-monitor -f
```

停止服务：

```sh
sudo systemctl stop basis-monitor
```

重启服务：

```sh
sudo systemctl restart basis-monitor
```

## 6. 推荐日常运行顺序

每天建议按以下顺序执行：

1. 同步参考数据到本地 staging
2. 校验 staging 中最新 CSV 是否正常
3. 确认 `ctp.ini` 指向 staging 路径
4. 启动 `basis_monitor`
5. 检查启动日志和选池结果

## 7. 验收检查项

启动成功后，至少检查以下内容：

- 启动日志中出现 `MarketDataProvider = ctp`
- 启动日志中选出了：
  - `hs300`
  - `zz500`
  - `zz1000`
- `logs/runtime.log` 正常写入
- `logs/alert.log` 正常写入
- `data/output/basis_results.csv` 正常写入
- `data/output/alert_events.csv` 正常写入
- `11:30 / 15:00` 定时报表正常生成
- 企业微信图片报表正常发送

## 8. 当前一期方案边界

当前这套部署方案的边界如下：

- 参考数据通过 staging 本地目录提供
- 实时行情仍以当前稳定的 `CTP` provider 为主
- `XTP` provider 只完成了配置入口和 fail-fast 保护
- 后续如果中泰确认可提供支持 `IF / IC / IM` 的真实行情接口，再在现有 provider 抽象上继续扩展
