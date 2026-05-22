#include <cassert>
#include <chrono>

#include "basis_monitor/domain/market_tick.h"
#include "basis_monitor/monitor/market_data_health_registry.h"

int main()
{
    using Clock = basis_monitor::MarketDataHealthTracker::Clock;

    Clock::time_point now = Clock::time_point{};
    basis_monitor::MarketDataHealthRegistry registry(
        30,
        5,
        [&now]() {
            return now;
        });

    registry.RecordTick(
        basis_monitor::MarketTickInstrumentType::Future,
        "2026-04-08 09:30:00.000");
    registry.RecordTick(
        basis_monitor::MarketTickInstrumentType::Index,
        "2026-04-08 09:30:01.000");

    assert(registry.HasSeenTick(basis_monitor::MarketDataChannel::Future));
    assert(registry.HasSeenTick(basis_monitor::MarketDataChannel::Index));
    assert(registry.LastTickTimestamp(basis_monitor::MarketDataChannel::Future) == "2026-04-08 09:30:00.000");
    assert(registry.LastTickTimestamp(basis_monitor::MarketDataChannel::Index) == "2026-04-08 09:30:01.000");

    now += std::chrono::seconds(30);
    const auto future_stale = registry.Check(basis_monitor::MarketDataChannel::Future);
    const auto index_stale = registry.Check(basis_monitor::MarketDataChannel::Index);
    assert(future_stale.transition == basis_monitor::MarketDataHealthTransition::EnteredStale);
    assert(index_stale.transition == basis_monitor::MarketDataHealthTransition::EnteredStale);

    registry.RecordTick(
        basis_monitor::MarketTickInstrumentType::Index,
        "2026-04-08 09:30:31.000");
    const auto future_still_stale = registry.Check(basis_monitor::MarketDataChannel::Future);
    const auto index_recovery_pending = registry.Check(basis_monitor::MarketDataChannel::Index);
    assert(future_still_stale.stale_active);
    assert(index_recovery_pending.stale_active);

    now += std::chrono::seconds(5);
    const auto future_stale_again = registry.Check(basis_monitor::MarketDataChannel::Future);
    const auto index_recovered = registry.Check(basis_monitor::MarketDataChannel::Index);
    assert(future_stale_again.stale_active);
    assert(index_recovered.transition == basis_monitor::MarketDataHealthTransition::Recovered);
    assert(!registry.IsStaleActive(basis_monitor::MarketDataChannel::Index));
    assert(registry.IsStaleActive(basis_monitor::MarketDataChannel::Future));

    return 0;
}
