#pragma once

#include <string>
#include <vector>

#include "basis_monitor/monitor/basis_monitor_service.h"
#include "basis_monitor/report/basis_report_types.h"
#include "basis_monitor/report/latest_basis_snapshot.h"

namespace basis_monitor
{

std::string FormatWeComAlertMarkdown(const MonitorUpdate& update);
std::string FormatWeComBasisReportMarkdown(const std::vector<LatestBasisSnapshot>& snapshots,
                                           BasisReportMoment moment);

} // namespace basis_monitor
