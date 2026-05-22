#pragma once

#include <string>

#include "basis_monitor/report/basis_report_types.h"

namespace basis_monitor
{

struct ScheduledReportEvent
{
    bool triggered = false;
    BasisReportMoment moment = BasisReportMoment::Midday1130;
    std::string trading_date;
};

class ScheduledReportService
{
public:
    ScheduledReportEvent Tick(const std::string& trading_date, int hour, int minute);

private:
    std::string last_report_date_;
    bool midday_sent_ = false;
    bool close_sent_ = false;
};

} // namespace basis_monitor
