#pragma once

#include <string>

#include "basis_monitor/domain/market_tick.h"
#include "basis_monitor/monitor/market_data_health_tracker.h"

namespace basis_monitor
{

enum class MarketDataChannel
{
    Future,
    Index,
};

class MarketDataHealthRegistry
{
public:
    MarketDataHealthRegistry(int stale_threshold_seconds,
                             int recovery_grace_seconds,
                             MarketDataHealthTracker::NowFn now_fn = {});

    void RecordTick(MarketTickInstrumentType instrument_type, const std::string& received_at);
    MarketDataHealthEvent Check(MarketDataChannel channel);
    bool HasSeenTick(MarketDataChannel channel) const;
    bool IsStaleActive(MarketDataChannel channel) const;
    std::string LastTickTimestamp(MarketDataChannel channel) const;

private:
    MarketDataHealthTracker& tracker(MarketDataChannel channel);
    const MarketDataHealthTracker& tracker(MarketDataChannel channel) const;

    MarketDataHealthTracker future_tracker_;
    MarketDataHealthTracker index_tracker_;
};

} // namespace basis_monitor
