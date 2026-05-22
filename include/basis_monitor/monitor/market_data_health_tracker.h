#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <string>

namespace basis_monitor
{

enum class MarketDataHealthTransition
{
    None,
    EnteredStale,
    Recovered,
};

struct MarketDataHealthEvent
{
    MarketDataHealthTransition transition = MarketDataHealthTransition::None;
    bool stale_active = false;
    int idle_seconds = 0;
    std::string data_as_of;
};

class MarketDataHealthTracker
{
public:
    using Clock = std::chrono::steady_clock;
    using NowFn = std::function<Clock::time_point()>;

    MarketDataHealthTracker(int stale_threshold_seconds,
                            int recovery_grace_seconds,
                            NowFn now_fn = {});

    void RecordTick(const std::string& received_at);
    MarketDataHealthEvent Check();

    bool HasSeenTick() const;
    bool IsStaleActive() const;
    std::string LastTickTimestamp() const;

private:
    Clock::time_point now() const;

    mutable std::mutex mutex_;
    int stale_threshold_seconds_ = 30;
    int recovery_grace_seconds_ = 5;
    NowFn now_fn_;

    bool has_seen_tick_ = false;
    bool stale_active_ = false;
    bool recovery_pending_ = false;
    Clock::time_point last_tick_time_ = {};
    Clock::time_point recovery_candidate_since_ = {};
    std::string last_tick_timestamp_;
};

} // namespace basis_monitor
