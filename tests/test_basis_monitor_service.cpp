#include <cassert>
#include <chrono>

#include "basis_monitor/domain/market_tick.h"
#include "basis_monitor/monitor/basis_monitor_service.h"
#include "basis_monitor/config/app_config.h"

int main()
{
    using Clock = basis_monitor::BasisMonitorService::Clock;

    Clock::time_point now = Clock::time_point{};

    basis_monitor::MonitoredContract active = {};
    active.instrument_id = "IC2606";
    active.product_group = "IC";
    active.index_code = "000905.XSHG";
    active.index_name = "CSI 500";
    active.index_close_yesterday = 6300.0;
    active.maturity_date = "2099-06-19";

    basis_monitor::MonitoredContract expired = {};
    expired.instrument_id = "IC2503";
    expired.product_group = "IC";
    expired.index_code = "000905.XSHG";
    expired.index_name = "CSI 500";
    expired.index_close_yesterday = 6300.0;
    expired.maturity_date = "2000-03-17";

    basis_monitor::MonitoredContract invalid_baseline = {};
    invalid_baseline.instrument_id = "IF2606";
    invalid_baseline.product_group = "IF";
    invalid_baseline.index_code = "000300.XSHG";
    invalid_baseline.index_name = "CSI 300";
    invalid_baseline.index_close_yesterday = 0.0;
    invalid_baseline.maturity_date = "2099-06-19";

    basis_monitor::AlertConfig alert_config = {};
    alert_config.negative_threshold = 0.0;
    alert_config.repeat_interval_minutes = 20;

    basis_monitor::BasisMonitorService service(
        {active, expired, invalid_baseline},
        alert_config,
        true,
        30,
        [&now]() {
            return now;
        });

    basis_monitor::MarketTick negative_tick = {};
    negative_tick.instrument_id = "IC2606";
    negative_tick.last_price = 6400.0;

    const auto waiting_live_index = service.OnTick(negative_tick);
    assert(waiting_live_index.contract_found);
    assert(!waiting_live_index.has_result);
    assert(waiting_live_index.waiting_for_live_index);
    assert(!waiting_live_index.stale_live_index);

    basis_monitor::MarketTick index_tick = {};
    index_tick.instrument_id = "000905";
    index_tick.instrument_type = basis_monitor::MarketTickInstrumentType::Index;
    index_tick.last_price = 6200.0;
    service.OnIndexTick(index_tick);

    const auto negative = service.OnTick(negative_tick);
    assert(negative.contract_found);
    assert(negative.has_result);
    assert(!negative.invalid_baseline);
    assert(negative.contract.instrument_id == "IC2606");
    assert(negative.index_price == 6200.0);
    assert(negative.future_price == 6400.0);
    assert(negative.result.annual_rate < 0.0);
    assert(negative.alert.transition == basis_monitor::AlertTransition::EnteredNegative);
    assert(negative.alert.product_group == "IC");
    assert(negative.alert.index_code == "000905.XSHG");
    assert(negative.alert.index_name == "CSI 500");

    now += std::chrono::seconds(31);
    const auto stale_live_index = service.OnTick(negative_tick);
    assert(stale_live_index.contract_found);
    assert(!stale_live_index.has_result);
    assert(!stale_live_index.waiting_for_live_index);
    assert(stale_live_index.stale_live_index);

    now = Clock::time_point{};
    service.OnIndexTick(index_tick);

    const auto repeat = service.OnTick(negative_tick);
    assert(repeat.alert.transition == basis_monitor::AlertTransition::None);

    basis_monitor::MarketTick recovery_tick = {};
    recovery_tick.instrument_id = "IC2606";
    recovery_tick.last_price = 6200.0;

    const auto recovery = service.OnTick(recovery_tick);
    assert(recovery.contract_found);
    assert(recovery.has_result);
    assert(!recovery.invalid_baseline);
    assert(recovery.index_price == 6200.0);
    assert(recovery.result.annual_rate >= 0.0);
    assert(recovery.alert.transition == basis_monitor::AlertTransition::None);

    basis_monitor::MarketTick expired_tick = {};
    expired_tick.instrument_id = "IC2503";
    expired_tick.last_price = 6200.0;

    const auto expired_update = service.OnTick(expired_tick);
    assert(expired_update.contract_found);
    assert(expired_update.has_result);
    assert(!expired_update.invalid_baseline);
    assert(expired_update.result.remaining_days <= 0);
    assert(expired_update.index_price == 6200.0);
    assert(expired_update.result.basis == 0.0);
    assert(expired_update.result.annual_rate == 0.0);
    assert(expired_update.alert.transition == basis_monitor::AlertTransition::None);

    basis_monitor::MarketTick invalid_baseline_tick = {};
    invalid_baseline_tick.instrument_id = "IF2606";
    invalid_baseline_tick.last_price = 4400.0;

    const auto invalid_update = service.OnTick(invalid_baseline_tick);
    assert(invalid_update.contract_found);
    assert(!invalid_update.has_result);
    assert(!invalid_update.invalid_baseline);
    assert(invalid_update.waiting_for_live_index);
    assert(invalid_update.index_price == 0.0);
    assert(invalid_update.alert.transition == basis_monitor::AlertTransition::None);

    basis_monitor::MarketTick unknown_tick = {};
    unknown_tick.instrument_id = "IC9999";
    unknown_tick.last_price = 6100.0;

    const auto unknown = service.OnTick(unknown_tick);
    assert(!unknown.contract_found);
    assert(!unknown.has_result);
    assert(!unknown.invalid_baseline);

    basis_monitor::BasisMonitorService fallback_service({active}, alert_config);
    const auto fallback = fallback_service.OnTick(negative_tick);
    assert(fallback.contract_found);
    assert(fallback.has_result);
    assert(fallback.index_price == 6300.0);

    return 0;
}
