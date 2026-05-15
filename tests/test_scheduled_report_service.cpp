#include <cassert>

#include "basis_monitor/report/scheduled_report_service.h"

int main()
{
    basis_monitor::ScheduledReportService service;

    const auto before_midday = service.Tick("2026-03-31", 11, 29);
    assert(!before_midday.triggered);

    const auto midday = service.Tick("2026-03-31", 11, 30);
    assert(midday.triggered);
    assert(midday.moment == basis_monitor::BasisReportMoment::Midday1130);
    assert(midday.trading_date == "2026-03-31");

    const auto midday_repeat = service.Tick("2026-03-31", 11, 45);
    assert(!midday_repeat.triggered);

    const auto close = service.Tick("2026-03-31", 15, 0);
    assert(close.triggered);
    assert(close.moment == basis_monitor::BasisReportMoment::Close1500);
    assert(close.trading_date == "2026-03-31");

    const auto close_repeat = service.Tick("2026-03-31", 15, 10);
    assert(!close_repeat.triggered);

    basis_monitor::ScheduledReportService late_start_service;
    const auto late_midday = late_start_service.Tick("2026-04-01", 15, 5);
    assert(late_midday.triggered);
    assert(late_midday.moment == basis_monitor::BasisReportMoment::Midday1130);
    const auto late_close = late_start_service.Tick("2026-04-01", 15, 5);
    assert(late_close.triggered);
    assert(late_close.moment == basis_monitor::BasisReportMoment::Close1500);
    const auto late_repeat = late_start_service.Tick("2026-04-01", 15, 5);
    assert(!late_repeat.triggered);

    const auto next_day_midday = service.Tick("2026-04-01", 11, 40);
    assert(next_day_midday.triggered);
    assert(next_day_midday.moment == basis_monitor::BasisReportMoment::Midday1130);
    assert(next_day_midday.trading_date == "2026-04-01");

    return 0;
}
