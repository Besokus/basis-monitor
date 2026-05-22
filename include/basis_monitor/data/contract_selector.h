#pragma once

#include <map>
#include <string>
#include <vector>

#include "basis_monitor/data/reference_data_types.h"
#include "basis_monitor/domain/monitored_contract.h"

namespace basis_monitor
{

struct ContractSelectionWarning
{
    std::string instrument_id;
    std::string reason;
};

struct ContractSelectionResult
{
    std::map<std::string, std::vector<MonitoredContract>> grouped_contracts;
    std::vector<ContractSelectionWarning> warnings;
};

bool IsCurrentMonthContract(const std::string& instrument_id);

ContractSelectionResult SelectPerProductTop4(const ReferenceDataLoadResult& reference_data,
                                             const std::string& trading_date);

} // namespace basis_monitor
