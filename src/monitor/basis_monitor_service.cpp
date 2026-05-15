#include "basis_monitor/monitor/basis_monitor_service.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "basis_monitor/monitor/basis_calculator.h"

namespace basis_monitor
{

namespace
{

std::tm ParseDate(const std::string& value)
{
    std::tm parsed = {};
    std::istringstream input(value);
    input >> std::get_time(&parsed, "%Y-%m-%d");
    if (input.fail())
    {
        throw std::runtime_error("Invalid date: " + value);
    }
    parsed.tm_hour = 0;
    parsed.tm_min = 0;
    parsed.tm_sec = 0;
    parsed.tm_isdst = -1;
    return parsed;
}

} // namespace

BasisMonitorService::BasisMonitorService(std::vector<MonitoredContract> contracts,
                                         AlertConfig alert_config,
                                         bool require_live_index_prices,
                                         int live_index_stale_threshold_seconds,
                                         NowFn now_fn)
    : contracts_(std::move(contracts)),
      require_live_index_prices_(require_live_index_prices),
      live_index_stale_threshold_seconds_(live_index_stale_threshold_seconds > 0 ? live_index_stale_threshold_seconds : 1),
      now_fn_(std::move(now_fn)),
      alert_engine_(alert_config.negative_threshold,
                    std::chrono::minutes(alert_config.repeat_interval_minutes))
{
}

void BasisMonitorService::OnIndexTick(const MarketTick& tick)
{
    latest_index_prices_[NormalizeIndexInstrumentId(tick.instrument_id)] = IndexPriceSnapshot{tick.last_price, now()};
}

MonitorUpdate BasisMonitorService::OnTick(const MarketTick& tick)
{
    MonitorUpdate update = {};
    update.future_price = tick.last_price;

    for (const auto& contract : contracts_)
    {
        if (contract.instrument_id == tick.instrument_id)
        {
            update.contract_found = true;
            update.contract = contract;
            const auto live_index_it = latest_index_prices_.find(
                NormalizeIndexInstrumentId(contract.index_code));
            const bool live_index_available = live_index_it != latest_index_prices_.end();
            const bool live_index_fresh = live_index_available &&
                std::chrono::duration_cast<std::chrono::seconds>(now() - live_index_it->second.received_at).count() <
                    live_index_stale_threshold_seconds_;

            if (require_live_index_prices_)
            {
                if (!live_index_available)
                {
                    update.waiting_for_live_index = true;
                    return update;
                }

                update.index_price = live_index_it->second.price;
                if (!live_index_fresh)
                {
                    update.stale_live_index = true;
                    return update;
                }
            }
            else
            {
                update.index_price = live_index_available
                    ? live_index_it->second.price
                    : contract.index_close_yesterday;
            }

            if (update.index_price <= 0.0)
            {
                update.invalid_baseline = true;
                return update;
            }

            const int remaining_days = CalculateRemainingDays(contract.maturity_date);
            if (remaining_days <= 0)
            {
                update.result.valid = true;
                update.result.basis = update.index_price - tick.last_price;
                update.result.annual_rate = 0.0;
                update.result.remaining_days = remaining_days;
                update.has_result = true;
                return update;
            }

            update.result = CalculateAnnualizedBasis(
                update.index_price,
                tick.last_price,
                remaining_days);
            update.has_result = update.result.valid;
            if (update.has_result)
            {
                update.alert = alert_engine_.Evaluate(tick.instrument_id, update.result.annual_rate);
                update.alert.product_group = contract.product_group;
                update.alert.index_code = contract.index_code;
                update.alert.index_name = contract.index_name;
                update.alert.index_price = update.index_price;
                update.alert.future_price = update.future_price;
                update.alert.basis = update.result.basis;
                update.alert.remaining_days = update.result.remaining_days;
            }
            return update;
        }
    }

    return update;
}

std::string BasisMonitorService::NormalizeIndexInstrumentId(const std::string& instrument_id)
{
    const auto delimiter = instrument_id.find('.');
    if (delimiter == std::string::npos)
    {
        return instrument_id;
    }
    return instrument_id.substr(0, delimiter);
}

BasisMonitorService::Clock::time_point BasisMonitorService::now() const
{
    if (now_fn_)
    {
        return now_fn_();
    }
    return Clock::now();
}

int BasisMonitorService::CalculateRemainingDays(const std::string& expiry_date)
{
    auto expiry_tm = ParseDate(expiry_date);
    const std::time_t expiry = std::mktime(&expiry_tm);
    if (expiry == static_cast<std::time_t>(-1))
    {
        throw std::runtime_error("Unable to convert expiry date: " + expiry_date);
    }

    const auto now = std::chrono::system_clock::now();
    const std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    std::tm today_tm = {};
#ifdef _WIN32
    localtime_s(&today_tm, &current_time);
#else
    localtime_r(&current_time, &today_tm);
#endif
    today_tm.tm_hour = 0;
    today_tm.tm_min = 0;
    today_tm.tm_sec = 0;
    today_tm.tm_isdst = -1;
    const std::time_t today = std::mktime(&today_tm);
    if (today == static_cast<std::time_t>(-1))
    {
        throw std::runtime_error("Unable to convert current date");
    }

    return static_cast<int>((expiry - today) / (60 * 60 * 24));
}

} // namespace basis_monitor
