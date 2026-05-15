#pragma once

#include <string>
#include <vector>

#include "basis_monitor/domain/monitored_contract.h"

namespace basis_monitor
{

std::vector<std::string> BuildXtpIndexInstruments(
    const std::vector<MonitoredContract>& monitored_contracts,
    const std::vector<std::string>& fallback_index_instruments = {});

} // namespace basis_monitor
