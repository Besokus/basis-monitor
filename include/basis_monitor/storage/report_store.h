#pragma once

#include <filesystem>
#include <string>

#include "basis_monitor/report/basis_report_types.h"

namespace basis_monitor
{

class ReportStore
{
public:
    explicit ReportStore(std::filesystem::path output_root);

    std::filesystem::path Write(const std::string& trading_date,
                                BasisReportMoment moment,
                                const std::string& report_text) const;
    std::filesystem::path ImagePath(const std::string& trading_date,
                                    BasisReportMoment moment) const;

private:
    std::filesystem::path output_root_;
};

} // namespace basis_monitor
