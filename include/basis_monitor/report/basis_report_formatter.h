#pragma once

#include <string>
#include <vector>

#include "basis_monitor/report/basis_report_types.h"
#include "basis_monitor/report/latest_basis_snapshot.h"

namespace basis_monitor
{

struct BasisReportMetadata
{
    std::string report_generated_at;
    std::string data_as_of;
    bool stale = false;
};

std::string FormatBasisReport(const std::vector<LatestBasisSnapshot>& snapshots, BasisReportMoment moment);
std::string FormatBasisReport(const std::vector<LatestBasisSnapshot>& snapshots,
                              BasisReportMoment moment,
                              const BasisReportMetadata& metadata);

} // namespace basis_monitor
