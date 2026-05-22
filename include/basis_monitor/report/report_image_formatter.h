#pragma once

#include <string>
#include <vector>

#include "basis_monitor/report/basis_report_formatter.h"
#include "basis_monitor/report/basis_report_types.h"
#include "basis_monitor/report/latest_basis_snapshot.h"

namespace basis_monitor
{

struct BasisReportImageRow
{
    std::string instrument_id;
    int remaining_days = 0;
    std::string price_text;
    std::string change_text;
    std::string change_percent_text;
    std::string basis_text;
    std::string annual_rate_text;
    std::string remaining_days_text;
    std::string warning_text;
    bool warning_negative = false;
};

struct BasisReportImageGroup
{
    std::string name;
    std::vector<BasisReportImageRow> rows;
};

struct BasisReportImageDocument
{
    std::string title;
    std::string subtitle;
    std::vector<std::string> columns;
    std::vector<BasisReportImageGroup> groups;
};

BasisReportImageDocument BuildBasisReportImageDocument(const std::vector<LatestBasisSnapshot>& snapshots,
                                                       BasisReportMoment moment,
                                                       double negative_threshold,
                                                       const std::string& trading_date,
                                                       const BasisReportMetadata& metadata = {});

std::string SerializeBasisReportImageDocument(const BasisReportImageDocument& document);

} // namespace basis_monitor
