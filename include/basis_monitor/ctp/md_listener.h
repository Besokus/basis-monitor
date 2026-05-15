#pragma once

#include "basis_monitor/domain/market_tick.h"

namespace basis_monitor
{

class MdListener
{
public:
    virtual ~MdListener() = default;
    virtual void OnTick(const MarketTick& tick) = 0;
};

} // namespace basis_monitor
