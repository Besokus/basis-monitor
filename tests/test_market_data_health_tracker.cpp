#include <cassert>
#include <chrono>

#include "basis_monitor/monitor/market_data_health_tracker.h"

int main()
{
    using Clock = basis_monitor::MarketDataHealthTracker::Clock;

    Clock::time_point now = Clock::time_point{};
    basis_monitor::MarketDataHealthTracker tracker(
        30,
        5,
        [&now]() {
            return now;
        });

    auto initial = tracker.Check();
    assert(initial.transition == basis_monitor::MarketDataHealthTransition::None);
    assert(!initial.stale_active);
    assert(initial.data_as_of.empty());
    assert(!tracker.HasSeenTick());
    assert(!tracker.IsStaleActive());
    assert(tracker.LastTickTimestamp().empty());

    tracker.RecordTick("2026-04-01 10:00:00.000");
    assert(tracker.HasSeenTick());
    assert(tracker.LastTickTimestamp() == "2026-04-01 10:00:00.000");
    auto healthy = tracker.Check();
    assert(healthy.transition == basis_monitor::MarketDataHealthTransition::None);
    assert(!healthy.stale_active);
    assert(healthy.data_as_of == "2026-04-01 10:00:00.000");

    now += std::chrono::seconds(29);
    auto still_healthy = tracker.Check();
    assert(still_healthy.transition == basis_monitor::MarketDataHealthTransition::None);
    assert(!still_healthy.stale_active);

    now += std::chrono::seconds(1);
    auto stale = tracker.Check();
    assert(stale.transition == basis_monitor::MarketDataHealthTransition::EnteredStale);
    assert(stale.stale_active);
    assert(stale.idle_seconds == 30);

    tracker.RecordTick("2026-04-01 10:00:31.000");
    auto pending_recovery = tracker.Check();
    assert(pending_recovery.transition == basis_monitor::MarketDataHealthTransition::None);
    assert(pending_recovery.stale_active);

    now += std::chrono::seconds(4);
    auto still_pending = tracker.Check();
    assert(still_pending.transition == basis_monitor::MarketDataHealthTransition::None);
    assert(still_pending.stale_active);

    now += std::chrono::seconds(1);
    auto recovered = tracker.Check();
    assert(recovered.transition == basis_monitor::MarketDataHealthTransition::Recovered);
    assert(!recovered.stale_active);
    assert(!tracker.IsStaleActive());

    now += std::chrono::seconds(31);
    auto stale_again = tracker.Check();
    assert(stale_again.transition == basis_monitor::MarketDataHealthTransition::EnteredStale);
    assert(stale_again.stale_active);

    return 0;
}
