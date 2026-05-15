#include <chrono>
#include <csignal>
#include <filesystem>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "basis_monitor/config/config_loader.h"
#include "basis_monitor/ctp/md_listener.h"
#include "basis_monitor/data/contract_selector.h"
#include "basis_monitor/data/reference_data_loader.h"
#include "basis_monitor/data/subscription_instrument_builder.h"
#include "basis_monitor/domain/market_tick.h"
#include "basis_monitor/logging/logger.h"
#include "basis_monitor/market_data/market_data_session.h"
#include "basis_monitor/market_data/market_data_session_factory.h"
#include "basis_monitor/monitor/basis_monitor_service.h"
#include "basis_monitor/monitor/market_data_health_registry.h"
#include "basis_monitor/monitor/market_data_health_tracker.h"
#include "basis_monitor/notify/notifier.h"
#include "basis_monitor/notify/wecom_message_formatter.h"
#include "basis_monitor/notify/wecom_robot_notifier.h"
#include "basis_monitor/report/basis_report_formatter.h"
#include "basis_monitor/report/latest_basis_snapshot_store.h"
#include "basis_monitor/report/scheduled_report_service.h"
#include "basis_monitor/storage/alert_store.h"
#include "basis_monitor/storage/basis_result_store.h"
#include "basis_monitor/storage/report_store.h"

namespace
{

volatile std::sig_atomic_t g_keepRunning = 1;

void HandleSignal(int)
{
    g_keepRunning = 0;
}

std::string CurrentTradingDate()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_time = {};
#ifdef _WIN32
    localtime_s(&local_time, &current_time);
#else
    localtime_r(&current_time, &local_time);
#endif

    std::ostringstream output;
    output << std::put_time(&local_time, "%Y-%m-%d");
    return output.str();
}

std::string CurrentTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_time = {};
#ifdef _WIN32
    localtime_s(&local_time, &current_time);
#else
    localtime_r(&current_time, &local_time);
#endif

    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream output;
    output << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    output << '.' << std::setw(3) << std::setfill('0') << millis.count();
    return output.str();
}

const char* MarketDataProviderName(basis_monitor::MarketDataProviderType provider)
{
    switch (provider)
    {
    case basis_monitor::MarketDataProviderType::Ctp:
        return "ctp";
    case basis_monitor::MarketDataProviderType::Xtp:
        return "xtp";
    default:
        return "unknown";
    }
}

const char* MarketTickInstrumentTypeName(basis_monitor::MarketTickInstrumentType instrument_type)
{
    switch (instrument_type)
    {
    case basis_monitor::MarketTickInstrumentType::Future:
        return "future";
    case basis_monitor::MarketTickInstrumentType::Index:
        return "index";
    default:
        return "unknown";
    }
}

std::tm CurrentLocalTime()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_time = {};
#ifdef _WIN32
    localtime_s(&local_time, &current_time);
#else
    localtime_r(&current_time, &local_time);
#endif
    return local_time;
}

const char* MarketDataChannelName(basis_monitor::MarketDataChannel channel)
{
    switch (channel)
    {
    case basis_monitor::MarketDataChannel::Future:
        return "future";
    case basis_monitor::MarketDataChannel::Index:
        return "index";
    default:
        return "unknown";
    }
}

std::string JoinInstrumentIds(const std::vector<basis_monitor::MonitoredContract>& contracts)
{
    std::ostringstream output;
    for (std::size_t index = 0; index < contracts.size(); ++index)
    {
        if (index > 0)
        {
            output << ",";
        }
        output << contracts[index].instrument_id;
    }
    return output.str();
}

std::vector<basis_monitor::MonitoredContract> FlattenContracts(
    const std::map<std::string, std::vector<basis_monitor::MonitoredContract>>& grouped_contracts)
{
    std::vector<basis_monitor::MonitoredContract> flattened;
    for (const auto& entry : grouped_contracts)
    {
        flattened.insert(flattened.end(), entry.second.begin(), entry.second.end());
    }
    return flattened;
}

basis_monitor::ReferenceDataDirectories BuildReferenceDataDirectories(const basis_monitor::CtpConfig& config)
{
    namespace fs = std::filesystem;

    const fs::path default_root = "data";
    basis_monitor::ReferenceDataDirectories directories = {};
    directories.future_metadata_dir = config.reference_future_metadata_dir.empty()
        ? default_root / "all_instruments" / "Future"
        : fs::path(config.reference_future_metadata_dir);
    directories.index_metadata_dir = config.reference_index_metadata_dir.empty()
        ? default_root / "all_instruments" / "INDX"
        : fs::path(config.reference_index_metadata_dir);
    directories.future_eod_dir = config.reference_future_eod_dir.empty()
        ? default_root / "eod_price" / "Future"
        : fs::path(config.reference_future_eod_dir);
    directories.index_eod_dir = config.reference_index_eod_dir.empty()
        ? default_root / "eod_price" / "INDX"
        : fs::path(config.reference_index_eod_dir);
    return directories;
}

class TerminalMdListener final : public basis_monitor::MdListener
{
public:
    TerminalMdListener(basis_monitor::BasisMonitorService& service,
                       basis_monitor::BasisResultStore& basis_result_store,
                       basis_monitor::AlertStore& alert_store,
                       basis_monitor::LatestBasisSnapshotStore& snapshot_store,
                       basis_monitor::MarketDataHealthRegistry& health_registry,
                       basis_monitor::Notifier& notifier,
                       bool enable_wecom_alert)
        : service_(service),
          basis_result_store_(basis_result_store),
          alert_store_(alert_store),
          snapshot_store_(snapshot_store),
          health_registry_(health_registry),
          notifier_(notifier),
          enable_wecom_alert_(enable_wecom_alert)
    {
    }

    void OnTick(const basis_monitor::MarketTick& tick) override
    {
        if (tick.instrument_type == basis_monitor::MarketTickInstrumentType::Index)
        {
            service_.OnIndexTick(tick);
            health_registry_.RecordTick(tick.instrument_type, CurrentTimestamp());
            basis_monitor::Log("[INDEX_TICK] provider=[%s] instrument=[%s] update_time=[%s.%03d] last_price=[%.8lf]\n",
                MarketDataProviderName(tick.provider),
                tick.instrument_id.c_str(),
                tick.update_time.c_str(),
                tick.update_millisec,
                tick.last_price);
            return;
        }

        health_registry_.RecordTick(tick.instrument_type, CurrentTimestamp());
        const auto update = service_.OnTick(tick);
        // 合约不在监控池中
        if (!update.contract_found)
        {
            basis_monitor::Log("[BASIS_MONITOR] contract=[%s] skipped=[CONTRACT_NOT_CONFIGURED]\n", tick.instrument_id.c_str());
            return;
        }
        if (update.invalid_baseline)
        {
            basis_monitor::Log("[BASIS_MONITOR] product=[%s] contract=[%s] skipped=[INVALID_INDEX_BASELINE] index=[%s] index_price=[%.4lf]\n",
                update.contract.product_group.c_str(),
                update.contract.instrument_id.c_str(),
                update.contract.index_code.c_str(),
                update.index_price);
            return;
        }
        if (update.waiting_for_live_index)
        {
            basis_monitor::Log("[BASIS_MONITOR] product=[%s] contract=[%s] skipped=[WAITING_FOR_LIVE_INDEX] index=[%s]\n",
                update.contract.product_group.c_str(),
                update.contract.instrument_id.c_str(),
                update.contract.index_code.c_str());
            return;
        }
        if (update.stale_live_index)
        {
            basis_monitor::LogAlert("[BASIS_MONITOR] product=[%s] contract=[%s] skipped=[STALE_LIVE_INDEX] index=[%s] index_price=[%.4lf]\n",
                update.contract.product_group.c_str(),
                update.contract.instrument_id.c_str(),
                update.contract.index_code.c_str(),
                update.index_price);
            return;
        }
        // 指数基准价无效
        if (!update.has_result)
        {
            basis_monitor::Log("[BASIS_MONITOR] product=[%s] contract=[%s] skipped=[INVALID_RESULT] remaining_days=[%d]\n",
                update.contract.product_group.c_str(),
                tick.instrument_id.c_str(),
                update.result.remaining_days);
            return;
        }
        // 结果落盘
        if (!basis_result_store_.Append(update.contract, update.index_price, update.future_price, update.result))
        {
            basis_monitor::Log("[BASIS_MONITOR] contract=[%s] skipped=[BASIS_RESULT_STORE_FAILED]\n",
                update.contract.instrument_id.c_str());
        }

        snapshot_store_.Update(update.contract, update.index_price, update.future_price, update.result, CurrentTimestamp());

        latest_lines_[update.contract.report_group][update.contract.instrument_id] = FormatContractLine(update);
        RenderGroupedSnapshot();

        if ((update.alert.transition == basis_monitor::AlertTransition::EnteredNegative ||
             update.alert.transition == basis_monitor::AlertTransition::RepeatedNegative) &&
            !basis_monitor::IsCurrentMonthContract(update.contract.instrument_id))
        {
            const char* reason_text = update.alert.transition == basis_monitor::AlertTransition::EnteredNegative
                ? "annual rate below threshold"
                : "annual rate remains below threshold reminder";

            if (!alert_store_.Append(update.alert, reason_text))
            {
                basis_monitor::Log("[ALERT] contract=[%s] skipped=[ALERT_STORE_FAILED]\n",
                    update.contract.instrument_id.c_str());
            }
            basis_monitor::LogAlert("[ALERT] product=[%s] contract=[%s] index=[%s] annual_basis_negative annual_rate=[%.4lf%%] basis=[%.4lf] type=[%s]\n",
                update.contract.product_group.c_str(),
                update.contract.instrument_id.c_str(),
                update.contract.index_name.c_str(),
                update.result.annual_rate,
                update.result.basis,
                update.alert.transition == basis_monitor::AlertTransition::EnteredNegative ? "entered" : "reminder");
            if (enable_wecom_alert_)
            {
                const auto notify_result = notifier_.SendMarkdown(
                    basis_monitor::FormatWeComAlertMarkdown(update));
                basis_monitor::Log("[WECOM_ALERT] contract=[%s] sent=[%d] reason=[%s]\n",
                    update.contract.instrument_id.c_str(),
                    notify_result.sent ? 1 : 0,
                    notify_result.reason.c_str());
            }
        }
    }

private:
    static std::string BuildAlertMessage(const basis_monitor::MonitorUpdate& update)
    {
        std::ostringstream output;
        output << "[Basis Monitor] 负年化基差告警\n"
               << "product=" << update.contract.product_group << '\n'
               << "contract=" << update.contract.instrument_id << '\n'
               << "index=" << update.contract.index_name << '\n'
               << std::fixed << std::setprecision(4)
               << "index_price=" << update.index_price << '\n'
               << "future_price=" << update.future_price << '\n'
               << "basis=" << update.result.basis << '\n'
               << "remaining_days=" << update.result.remaining_days << '\n'
               << "annual_rate=" << update.result.annual_rate << '%';
        return output.str();
    }

    static std::string FormatContractLine(const basis_monitor::MonitorUpdate& update)
    {
        std::ostringstream output;
        output << std::fixed << std::setprecision(4)
               << update.contract.instrument_id
               << " | index=" << update.contract.index_name
               << " | index_price=" << update.index_price
               << " | future=" << update.future_price
               << " | basis=" << update.result.basis
               << " | remaining_days=" << update.result.remaining_days
               << " | annual_rate=" << update.result.annual_rate << '%';
        return output.str();
    }

    void RenderGroupedSnapshot() const
    {
        static const std::vector<std::string> kGroupOrder = {"hs300", "zz500", "zz1000"};

        std::ostringstream output;
        output << "===============================\n";
        for (const auto& group : kGroupOrder)
        {
            output << "[GROUP] " << group << '\n';
            const auto group_it = latest_lines_.find(group);
            if (group_it == latest_lines_.end() || group_it->second.empty())
            {
                output << "(no updates)\n";
                continue;
            }

            for (const auto& contract_entry : group_it->second)
            {
                output << contract_entry.second << '\n';
            }
        }
        output << "===============================\n";
        basis_monitor::Log("%s", output.str().c_str());
    }

    basis_monitor::BasisMonitorService& service_;
    basis_monitor::BasisResultStore& basis_result_store_;
    basis_monitor::AlertStore& alert_store_;
    basis_monitor::LatestBasisSnapshotStore& snapshot_store_;
    basis_monitor::MarketDataHealthRegistry& health_registry_;
    basis_monitor::Notifier& notifier_;
    bool enable_wecom_alert_ = false;
    std::map<std::string, std::map<std::string, std::string>> latest_lines_;
};

} // namespace

int main()
{
    namespace fs = std::filesystem;

    try
    {
        fs::create_directories("logs");
        fs::create_directories("data/output");
        fs::create_directories("runtime/flow");

        basis_monitor::InitializeLogger("logs/runtime.log", "logs/alert.log");
        const auto config = basis_monitor::LoadAppConfig("config");
        const auto trading_date = CurrentTradingDate();
        const auto reference_directories = BuildReferenceDataDirectories(config.ctp);
        const auto reference_data = basis_monitor::LoadReferenceData(reference_directories);
        const auto selection = basis_monitor::SelectPerProductTop4(reference_data, trading_date);
        const auto monitored_contracts = FlattenContracts(selection.grouped_contracts);

        if (monitored_contracts.empty())
        {
            throw std::runtime_error("No monitored contracts were selected from previous-trading-day futures EOD data");
        }

        for (const auto& warning : selection.warnings)
        {
            basis_monitor::Log("[SELECTION_WARNING] contract=[%s] reason=[%s]\n",
                warning.instrument_id.c_str(),
                warning.reason.c_str());
        }

        for (const auto& entry : selection.grouped_contracts)
        {
            if (entry.second.size() < 4)
            {
                basis_monitor::Log("[SELECTION_WARNING] product=[%s] selected_count=[%zu] expected_top4=[4]\n",
                    entry.first.c_str(),
                    entry.second.size());
            }

            basis_monitor::Log("[SELECTION] product=[%s] selected_count=[%zu] contracts=[%s]\n",
                entry.first.c_str(),
                entry.second.size(),
                JoinInstrumentIds(entry.second).c_str());
        }

        basis_monitor::AppConfig runtime_config = config;
        runtime_config.ctp.instruments.clear();
        for (const auto& contract : monitored_contracts)
        {
            runtime_config.ctp.instruments.push_back(contract.instrument_id);
        }
        runtime_config.xtp.index_instruments = basis_monitor::BuildXtpIndexInstruments(
            monitored_contracts,
            config.xtp.index_instruments);

        basis_monitor::Log("FrontMdAddr = %s\n", config.ctp.front_md_addr.c_str());
        basis_monitor::Log("MarketDataProvider = %s\n", MarketDataProviderName(runtime_config.market_data_provider));
        basis_monitor::Log("EnableCtpMarketData = %d\n", runtime_config.ctp.enable_ctp_market_data ? 1 : 0);
        basis_monitor::Log("EnableXtpMarketData = %d\n", runtime_config.xtp.enable_xtp_market_data ? 1 : 0);
        if (!config.ctp.instruments.empty())
        {
            basis_monitor::Log("ConfiguredInstrumentID = %s\n", config.ctp.instruments.front().c_str());
        }
        if (!runtime_config.ctp.instruments.empty())
        {
            std::ostringstream ctp_subscriptions;
            for (std::size_t index = 0; index < runtime_config.ctp.instruments.size(); ++index)
            {
                if (index > 0)
                {
                    ctp_subscriptions << ",";
                }
                ctp_subscriptions << runtime_config.ctp.instruments[index];
            }
            basis_monitor::Log("CtpInstrumentIDs = %s\n", ctp_subscriptions.str().c_str());
        }
        if (!runtime_config.xtp.index_instruments.empty())
        {
            std::ostringstream xtp_subscriptions;
            for (std::size_t index = 0; index < runtime_config.xtp.index_instruments.size(); ++index)
            {
                if (index > 0)
                {
                    xtp_subscriptions << ",";
                }
                xtp_subscriptions << runtime_config.xtp.index_instruments[index];
            }
            basis_monitor::Log("XtpIndexInstrumentIDs = %s\n", xtp_subscriptions.str().c_str());
        }
        basis_monitor::Log("TradingDate = %s\n", trading_date.c_str());
        basis_monitor::Log("ReferenceFutureMetadataDir = %s\n", reference_directories.future_metadata_dir.string().c_str());
        basis_monitor::Log("ReferenceIndexMetadataDir = %s\n", reference_directories.index_metadata_dir.string().c_str());
        basis_monitor::Log("ReferenceFutureEodDir = %s\n", reference_directories.future_eod_dir.string().c_str());
        basis_monitor::Log("ReferenceIndexEodDir = %s\n", reference_directories.index_eod_dir.string().c_str());
        basis_monitor::Log("SelectedInstrumentCount = %zu\n", runtime_config.ctp.instruments.size());

        basis_monitor::BasisMonitorService service(
            monitored_contracts,
            config.alert,
            runtime_config.xtp.enable_xtp_market_data,
            config.ctp.market_data_stale_threshold_sec);
        basis_monitor::BasisResultStore basis_result_store(fs::path("data") / "output" / "basis_results.csv");
        basis_monitor::AlertStore alert_store(fs::path("data") / "output" / "alert_events.csv");
        basis_monitor::LatestBasisSnapshotStore snapshot_store(monitored_contracts);
        basis_monitor::MarketDataHealthRegistry health_registry(
            config.ctp.market_data_stale_threshold_sec,
            config.ctp.market_data_recovery_grace_sec);
        basis_monitor::ReportStore report_store(fs::path("data") / "output");
        basis_monitor::ScheduledReportService scheduled_report_service;
        auto notifier = std::make_unique<basis_monitor::WeComRobotNotifier>(config.ctp.wecom_robot_webhook);
        TerminalMdListener listener(service, basis_result_store, alert_store, snapshot_store, health_registry, *notifier, config.ctp.enable_wecom_alert);
        auto session = basis_monitor::CreateMarketDataSession(runtime_config, listener);
        if (!session->Start())
        {
            basis_monitor::Log("[MARKET_DATA_START_FAILED] Unable to finish connect/login/subscribe sequence.\n");
            return 1;
        }

        const auto first_tick_timeout_ms = runtime_config.ctp.enable_ctp_market_data
            ? runtime_config.ctp.first_tick_timeout_ms
            : (runtime_config.market_data_provider == basis_monitor::MarketDataProviderType::Xtp
                ? runtime_config.xtp.first_tick_timeout_ms
                : runtime_config.ctp.first_tick_timeout_ms);

        basis_monitor::Log("Waiting up to %d milliseconds for first market data tick...\n", first_tick_timeout_ms);
        if (!session->WaitForFirstMarketData(static_cast<unsigned long>(first_tick_timeout_ms)))
        {
            basis_monitor::Log("[MARKET_DATA_WAIT_TIMEOUT] No market data received within %d milliseconds after subscription.\n",
                first_tick_timeout_ms);
            return 1;
        }

        basis_monitor::Log("Market data stream is active. Press Ctrl+C to exit.\n");
        std::signal(SIGINT, HandleSignal);
        std::signal(SIGTERM, HandleSignal);
        while (g_keepRunning)
        {
            for (const auto channel : {basis_monitor::MarketDataChannel::Future, basis_monitor::MarketDataChannel::Index})
            {
                const auto health_event = health_registry.Check(channel);
                if (health_event.transition == basis_monitor::MarketDataHealthTransition::EnteredStale)
                {
                    basis_monitor::LogAlert("[MARKET_DATA_STALE] channel=[%s] idle_seconds=[%d] data_as_of=[%s]\n",
                        MarketDataChannelName(channel),
                        health_event.idle_seconds,
                        health_event.data_as_of.empty() ? "N/A" : health_event.data_as_of.c_str());
                }
                else if (health_event.transition == basis_monitor::MarketDataHealthTransition::Recovered)
                {
                    basis_monitor::LogAlert("[MARKET_DATA_RECOVERED] channel=[%s] data_as_of=[%s]\n",
                        MarketDataChannelName(channel),
                        health_event.data_as_of.empty() ? "N/A" : health_event.data_as_of.c_str());
                }
            }

            const auto local_time = CurrentLocalTime();
            const auto report_event = scheduled_report_service.Tick(
                CurrentTradingDate(),
                local_time.tm_hour,
                local_time.tm_min);
            if (report_event.triggered)
            {
                const auto snapshots = snapshot_store.GetAll();
                basis_monitor::BasisReportMetadata report_metadata = {};
                report_metadata.report_generated_at = CurrentTimestamp();
                report_metadata.data_as_of = health_registry.LastTickTimestamp(basis_monitor::MarketDataChannel::Future);
                report_metadata.stale = health_registry.IsStaleActive(basis_monitor::MarketDataChannel::Future);
                const auto report_text = basis_monitor::FormatBasisReport(snapshots, report_event.moment, report_metadata);
                const auto report_path = report_store.Write(report_event.trading_date, report_event.moment, report_text);
                basis_monitor::Log("[SCHEDULED_REPORT] trading_date=[%s] saved=[%s]\n",
                    report_event.trading_date.c_str(),
                    report_path.string().c_str());
                if (config.ctp.enable_wecom_report)
                {
                    const auto report_markdown = basis_monitor::FormatWeComBasisReportMarkdown(snapshots, report_event.moment);
                    const auto notify_result = notifier->SendText(report_markdown);
                    basis_monitor::Log("[WECOM_REPORT] trading_date=[%s] sent=[%d] mode=[text] reason=[%s]\n",
                        report_event.trading_date.c_str(),
                        notify_result.sent ? 1 : 0,
                        notify_result.reason.c_str());
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        session->Stop();

        basis_monitor::Log("Stopping basis_monitor market-data session.\n");
        return 0;
    }
    catch (const std::exception& ex)
    {
        basis_monitor::InitializeLogger("logs/runtime.log", "logs/alert.log");
        basis_monitor::Log("[FATAL] %s\n", ex.what());
        return 1;
    }
}
