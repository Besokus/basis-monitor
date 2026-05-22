#pragma once

#include <string>

namespace basis_monitor
{

enum class AlertTransition
{
    None,
    EnteredNegative,
    RepeatedNegative,
    Recovered,
};

struct AlertEvent
{
    std::string instrument_id;
    std::string product_group;
    std::string index_code;
    std::string index_name;
    double index_price = 0.0;
    double future_price = 0.0;
    double basis = 0.0;
    double annual_rate = 0.0;
    int remaining_days = 0;
    AlertTransition transition = AlertTransition::None;
};

} // namespace basis_monitor
