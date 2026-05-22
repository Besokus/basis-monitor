#pragma once

#include <filesystem>

#include "basis_monitor/domain/market_tick.h"

namespace basis_monitor
{

class TickStore
{
public:
    explicit TickStore(std::filesystem::path file_path);

    bool Append(const MarketTick& tick);

private:
    std::filesystem::path file_path_;
};

} // namespace basis_monitor
