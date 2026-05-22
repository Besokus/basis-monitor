#pragma once

#include <string>

#include "basis_monitor/domain/monitored_contract.h"

namespace basis_monitor
{

struct LatestBasisSnapshot
{
    MonitoredContract contract = {};
    bool has_tick = false;
    double latest_index_price = 0.0;
    double latest_future_price = 0.0;
    double latest_basis = 0.0;
    double latest_annual_rate = 0.0;
    int remaining_days = 0;
    std::string snapshot_time;
};

} // namespace basis_monitor
