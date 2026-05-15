#include "basis_monitor/data/subscription_instrument_builder.h"

#include <unordered_set>

namespace basis_monitor
{

std::vector<std::string> BuildXtpIndexInstruments(
    const std::vector<MonitoredContract>& monitored_contracts,
    const std::vector<std::string>& fallback_index_instruments)
{
    std::vector<std::string> instruments;
    std::unordered_set<std::string> seen;

    for (const auto& contract : monitored_contracts)
    {
        if (contract.index_code.empty())
        {
            continue;
        }

        if (seen.insert(contract.index_code).second)
        {
            instruments.push_back(contract.index_code);
        }
    }

    if (!instruments.empty())
    {
        return instruments;
    }

    return fallback_index_instruments;
}

} // namespace basis_monitor
