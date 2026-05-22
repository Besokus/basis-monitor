# Basis Monitor Dual-Server Deployment

This document describes the recommended production split for `basis_monitor`.

## Role split

### Zhongtai server

The Zhongtai server is the runtime host only.

It is responsible for:

- keeping the local staging CSV directories
- running the prebuilt `basis_monitor/bin/basis_monitor`
- loading local staging reference data
- connecting to `ctp` or `xtp`
- writing local outputs:
  - `logs/runtime.log`
  - `logs/alert.log`
  - `data/output/alert_events.csv`
  - `data/output/basis_results.csv`

It does **not** send WeCom messages directly.
It also does not render report PNGs in the recommended split.

### Your own server

Your own server acts as the control plane.

It is responsible for:

- building the latest `basis_monitor/bin/basis_monitor`
- filtering reference CSV down to today's monitored contracts and required index rows
- pushing the filtered staging CSV to Zhongtai through SFTP
- optionally pushing the refreshed runtime bundle
- pulling Zhongtai outputs back
- generating report PNGs locally from pulled `basis_results.csv`
- relaying WeCom alerts and report PNGs

## Required Zhongtai paths

Project root:

```text
/list/10.101.5.62/basis-monitor-zhongtai
```

Runtime directory:

```text
/list/10.101.5.62/basis-monitor-zhongtai/basis_monitor
```

Local staging root:

```text
/list/10.101.5.62/basis-monitor-zhongtai/data/staging
```

Required staging subdirectories:

```text
/list/10.101.5.62/basis-monitor-zhongtai/data/staging/all_instruments/Future
/list/10.101.5.62/basis-monitor-zhongtai/data/staging/all_instruments/INDX
/list/10.101.5.62/basis-monitor-zhongtai/data/staging/eod_price/Future
/list/10.101.5.62/basis-monitor-zhongtai/data/staging/eod_price/INDX
```

## Zhongtai config requirements

`basis_monitor/config/ctp.ini` on Zhongtai must point only to local staging:

```ini
ReferenceFutureMetadataDir=/list/10.101.5.62/basis-monitor-zhongtai/data/staging/all_instruments/Future
ReferenceIndexMetadataDir=/list/10.101.5.62/basis-monitor-zhongtai/data/staging/all_instruments/INDX
ReferenceFutureEodDir=/list/10.101.5.62/basis-monitor-zhongtai/data/staging/eod_price/Future
ReferenceIndexEodDir=/list/10.101.5.62/basis-monitor-zhongtai/data/staging/eod_price/INDX
EnableWeComAlert=false
EnableWeComReport=false
GenerateLocalReportImage=false
WeComRobotWebhook=
```

Do not leave the original source-server paths such as `/data/disk1/share_data/...` in the Zhongtai runtime config.

## Daily workflow

### 1. Build on your own server

```sh
sh basis_monitor/run.sh
```

This refreshes:

```text
basis_monitor/bin/basis_monitor
```

### 2. Push reference CSV to Zhongtai

```sh
sh basis_monitor/scripts/push_reference_data_to_zhongtai.sh \
  /list/10.101.5.62/basis-monitor-zhongtai/data/staging
```

This helper performs the local contract selection first and only uploads the filtered four CSV snapshots required by the current trading day.

The SFTP connection fields can be stored in `basis_monitor/config/sftp.conf` on your own server:

```sh
cp basis_monitor/config/sftp.conf.example basis_monitor/config/sftp.conf
```

Set:

- `SFTP_HOST`
- `SFTP_USER`
- `SFTP_PORT`
- `SFTP_IDENTITY_FILE`

### 3. Validate Zhongtai staging

Run on Zhongtai:

```sh
sh basis_monitor/scripts/validate_reference_data.sh \
  /list/10.101.5.62/basis-monitor-zhongtai/data/staging
```

### 4. Push prebuilt runtime bundle if needed

```sh
sh basis_monitor/scripts/push_prebuilt_runtime_to_zhongtai.sh \
  /list/10.101.5.62/basis-monitor-zhongtai
```

### 5. Start Zhongtai runtime

Run from a local writable directory on the Zhongtai server. Do not run directly from an `sftp` mounted directory.

```sh
cd /list/10.101.5.62/basis-monitor-zhongtai/basis_monitor
sh start_prebuilt.sh
```

### 6. Pull outputs back to your own server

```sh
sh basis_monitor/scripts/pull_zhongtai_outputs.sh \
  /list/10.101.5.62/basis-monitor-zhongtai/basis_monitor \
  ./relay_spool
```

### 7. Generate report PNGs on your own server

```sh
python3 basis_monitor/scripts/generate_report_image_from_basis_results.py \
  --basis-results ./relay_spool/basis_results.csv \
  --output ./relay_spool/reports/$(date +%F)_1130_latest_basis.png \
  --moment 1130 \
  --negative-threshold 0.0
```

### 8. Relay WeCom notifications from your own server

```sh
python3 basis_monitor/scripts/relay_zhongtai_notifications.py \
  --input-root ./relay_spool \
  --state-file ./relay_state.json \
  --webhook "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=..."
```

## Notes

- Use `alert_events.csv` as the structured source for realtime alert relay.
- Use `basis_results.csv` to generate local report PNGs.
- Relay those locally generated `reports/*.png` from your own server.
- `alert.log` remains an operator-facing log, not the primary notification input.
- The Zhongtai server can stay isolated from outbound internet as long as your own server performs the WeCom relay.
