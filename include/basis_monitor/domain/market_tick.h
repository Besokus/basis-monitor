#pragma once

#include <string>

#include "basis_monitor/config/app_config.h"

namespace basis_monitor
{

enum class MarketTickInstrumentType
{
    Unknown,
    Future,
    Index
};

struct MarketTick
{
    std::string instrument_id;
    std::string update_time;
    int update_millisec = 0;
    MarketDataProviderType provider = MarketDataProviderType::Ctp;
    MarketTickInstrumentType instrument_type = MarketTickInstrumentType::Unknown;
    double last_price = 0.0;
    double bid_price_1 = 0.0;
    int bid_volume_1 = 0;
    double ask_price_1 = 0.0;
    int ask_volume_1 = 0;
    int volume = 0;
};

} // namespace basis_monitor
