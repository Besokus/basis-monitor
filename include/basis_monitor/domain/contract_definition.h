#pragma once

#include <string>

namespace basis_monitor
{

struct ContractDefinition
{
    std::string instrument_id;
    std::string expiry_date;
    bool enabled = true;
};

} // namespace basis_monitor
