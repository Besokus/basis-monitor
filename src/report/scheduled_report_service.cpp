#include "basis_monitor/report/scheduled_report_service.h"

namespace basis_monitor
{

ScheduledReportEvent ScheduledReportService::Tick(const std::string& trading_date, int hour, int minute)
{
    if (trading_date != last_report_date_)
    {
        last_report_date_ = trading_date;
        midday_sent_ = false;
        close_sent_ = false;
    }

    ScheduledReportEvent event = {};
    event.trading_date = trading_date;

    const bool after_midday = (hour > 11) || (hour == 11 && minute >= 30);
    const bool after_close = hour >= 15;

    if (!midday_sent_ && after_midday)
    {
        midday_sent_ = true;
        event.triggered = true;
        event.moment = BasisReportMoment::Midday1130;
        return event;
    }

    if (!close_sent_ && after_close)
    {
        close_sent_ = true;
        event.triggered = true;
        event.moment = BasisReportMoment::Close1500;
        return event;
    }

    return event;
}

} // namespace basis_monitor
