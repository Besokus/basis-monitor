#pragma once

#include <chrono>
#include <functional>
#include <unordered_map>
#include <vector>

#include "basis_monitor/domain/alert_event.h"
#include "basis_monitor/domain/basis_result.h"
#include "basis_monitor/domain/market_tick.h"
#include "basis_monitor/domain/monitored_contract.h"
#include "basis_monitor/config/app_config.h"
#include "basis_monitor/monitor/alert_engine.h"

namespace basis_monitor
{

struct MonitorUpdate
{
    bool has_result = false;
    bool contract_found = false;
    bool invalid_baseline = false;
    bool waiting_for_live_index = false;
    bool stale_live_index = false;
    MonitoredContract contract = {};
    BasisResult result = {};
    AlertEvent alert = {};
    double index_price = 0.0;
    double future_price = 0.0;
};

class BasisMonitorService
{
public:
    using Clock = std::chrono::steady_clock;
    using NowFn = std::function<Clock::time_point()>;

    explicit BasisMonitorService(std::vector<MonitoredContract> contracts,
                                 AlertConfig alert_config = {},
                                 bool require_live_index_prices = false,
                                 int live_index_stale_threshold_seconds = 30,
                                 NowFn now_fn = {});

    void OnIndexTick(const MarketTick& tick);
    MonitorUpdate OnTick(const MarketTick& tick);

private:
    struct IndexPriceSnapshot
    {
        double price = 0.0;
        Clock::time_point received_at = {};
    };

    static int CalculateRemainingDays(const std::string& expiry_date);
    static std::string NormalizeIndexInstrumentId(const std::string& instrument_id);
    Clock::time_point now() const;

    std::vector<MonitoredContract> contracts_;
    std::unordered_map<std::string, IndexPriceSnapshot> latest_index_prices_;
    bool require_live_index_prices_ = false;
    int live_index_stale_threshold_seconds_ = 30;
    NowFn now_fn_;
    AlertEngine alert_engine_;
};

} // namespace basis_monitor
