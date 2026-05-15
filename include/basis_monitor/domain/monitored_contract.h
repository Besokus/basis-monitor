#pragma once

#include <string>

namespace basis_monitor
{

struct MonitoredContract
{
    std::string instrument_id;
    std::string product_group;
    std::string report_group;
    std::string index_code;
    std::string index_name;
    double index_close_yesterday = 0.0;
    double future_close_yesterday = 0.0;
    std::string maturity_date;
    double yesterday_turnover = 0.0;
};

} // namespace basis_monitor
