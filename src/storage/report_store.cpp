#include "basis_monitor/storage/report_store.h"

#include <fstream>

namespace basis_monitor
{

namespace
{

std::string SuffixForMoment(BasisReportMoment moment)
{
    switch (moment)
    {
    case BasisReportMoment::Midday1130:
        return "1130";
    case BasisReportMoment::Close1500:
        return "1500";
    }

    return "unknown";
}

} // namespace

ReportStore::ReportStore(std::filesystem::path output_root)
    : output_root_(std::move(output_root))
{
}

std::filesystem::path ReportStore::Write(const std::string& trading_date,
                                         BasisReportMoment moment,
                                         const std::string& report_text) const
{
    const auto reports_dir = output_root_ / "reports";
    std::filesystem::create_directories(reports_dir);

    const auto file_path = reports_dir / (trading_date + "_" + SuffixForMoment(moment) + "_latest_basis.txt");
    std::ofstream output(file_path, std::ios::binary | std::ios::trunc);
    output << report_text;
    return file_path;
}

std::filesystem::path ReportStore::ImagePath(const std::string& trading_date,
                                             BasisReportMoment moment) const
{
    const auto reports_dir = output_root_ / "reports";
    std::filesystem::create_directories(reports_dir);
    return reports_dir / (trading_date + "_" + SuffixForMoment(moment) + "_latest_basis.png");
}

} // namespace basis_monitor
