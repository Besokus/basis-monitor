#include "basis_monitor/monitor/market_data_health_registry.h"

namespace basis_monitor
{

MarketDataHealthRegistry::MarketDataHealthRegistry(int stale_threshold_seconds,
                                                   int recovery_grace_seconds,
                                                   MarketDataHealthTracker::NowFn now_fn)
    : future_tracker_(stale_threshold_seconds, recovery_grace_seconds, now_fn),
      index_tracker_(stale_threshold_seconds, recovery_grace_seconds, now_fn)
{
}

void MarketDataHealthRegistry::RecordTick(MarketTickInstrumentType instrument_type, const std::string& received_at)
{
    switch (instrument_type)
    {
    case MarketTickInstrumentType::Index:
        index_tracker_.RecordTick(received_at);
        return;
    case MarketTickInstrumentType::Future:
    case MarketTickInstrumentType::Unknown:
    default:
        future_tracker_.RecordTick(received_at);
        return;
    }
}

MarketDataHealthEvent MarketDataHealthRegistry::Check(MarketDataChannel channel)
{
    return tracker(channel).Check();
}

bool MarketDataHealthRegistry::HasSeenTick(MarketDataChannel channel) const
{
    return tracker(channel).HasSeenTick();
}

bool MarketDataHealthRegistry::IsStaleActive(MarketDataChannel channel) const
{
    return tracker(channel).IsStaleActive();
}

std::string MarketDataHealthRegistry::LastTickTimestamp(MarketDataChannel channel) const
{
    return tracker(channel).LastTickTimestamp();
}

MarketDataHealthTracker& MarketDataHealthRegistry::tracker(MarketDataChannel channel)
{
    return channel == MarketDataChannel::Index ? index_tracker_ : future_tracker_;
}

const MarketDataHealthTracker& MarketDataHealthRegistry::tracker(MarketDataChannel channel) const
{
    return channel == MarketDataChannel::Index ? index_tracker_ : future_tracker_;
}

} // namespace basis_monitor
