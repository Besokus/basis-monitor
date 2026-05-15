#include "basis_monitor/monitor/market_data_health_tracker.h"

#include <algorithm>

namespace basis_monitor
{

MarketDataHealthTracker::MarketDataHealthTracker(int stale_threshold_seconds,
                                                 int recovery_grace_seconds,
                                                 NowFn now_fn)
    : stale_threshold_seconds_(std::max(1, stale_threshold_seconds)),
      recovery_grace_seconds_(std::max(0, recovery_grace_seconds)),
      now_fn_(std::move(now_fn))
{
}

void MarketDataHealthTracker::RecordTick(const std::string& received_at)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    has_seen_tick_ = true;
    last_tick_time_ = now();
    last_tick_timestamp_ = received_at;

    if (stale_active_ && !recovery_pending_)
    {
        recovery_pending_ = true;
        recovery_candidate_since_ = last_tick_time_;
    }
}

MarketDataHealthEvent MarketDataHealthTracker::Check()
{
    const std::lock_guard<std::mutex> lock(mutex_);
    MarketDataHealthEvent event = {};
    event.stale_active = stale_active_;
    event.data_as_of = last_tick_timestamp_;

    if (!has_seen_tick_)
    {
        return event;
    }

    const auto current_time = now();
    const auto idle = std::chrono::duration_cast<std::chrono::seconds>(current_time - last_tick_time_).count();
    event.idle_seconds = idle > 0 ? static_cast<int>(idle) : 0;

    if (!stale_active_)
    {
        if (event.idle_seconds >= stale_threshold_seconds_)
        {
            stale_active_ = true;
            recovery_pending_ = false;
            event.stale_active = true;
            event.transition = MarketDataHealthTransition::EnteredStale;
        }
        return event;
    }

    if (event.idle_seconds >= stale_threshold_seconds_)
    {
        recovery_pending_ = false;
        event.stale_active = true;
        return event;
    }

    if (!recovery_pending_)
    {
        recovery_pending_ = true;
        recovery_candidate_since_ = current_time;
        event.stale_active = true;
        return event;
    }

    const auto healthy_seconds = std::chrono::duration_cast<std::chrono::seconds>(current_time - recovery_candidate_since_).count();
    if (healthy_seconds >= recovery_grace_seconds_)
    {
        stale_active_ = false;
        recovery_pending_ = false;
        event.stale_active = false;
        event.transition = MarketDataHealthTransition::Recovered;
    }
    else
    {
        event.stale_active = true;
    }

    return event;
}

bool MarketDataHealthTracker::HasSeenTick() const
{
    const std::lock_guard<std::mutex> lock(mutex_);
    return has_seen_tick_;
}

bool MarketDataHealthTracker::IsStaleActive() const
{
    const std::lock_guard<std::mutex> lock(mutex_);
    return stale_active_;
}

std::string MarketDataHealthTracker::LastTickTimestamp() const
{
    const std::lock_guard<std::mutex> lock(mutex_);
    return last_tick_timestamp_;
}

MarketDataHealthTracker::Clock::time_point MarketDataHealthTracker::now() const
{
    if (now_fn_)
    {
        return now_fn_();
    }
    return Clock::now();
}

} // namespace basis_monitor
