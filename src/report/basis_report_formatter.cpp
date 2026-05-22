#include "basis_monitor/report/basis_report_formatter.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <utility>

namespace basis_monitor
{

namespace
{

std::string ReportTitle(BasisReportMoment moment)
{
    switch (moment)
    {
    case BasisReportMoment::Midday1130:
        return "[Basis Monitor] 11:30 最新基差表";
    case BasisReportMoment::Close1500:
        return "[Basis Monitor] 15:00 最新基差表";
    }

    return "[Basis Monitor] 最新基差表";
}

std::string FormatSnapshotLine(const LatestBasisSnapshot& snapshot)
{
    std::ostringstream output;
    if (!snapshot.has_tick)
    {
        output << snapshot.contract.instrument_id
               << " | index=" << snapshot.contract.index_name
               << " | 暂无行情";
        return output.str();
    }

    output << std::fixed << std::setprecision(4)
           << snapshot.contract.instrument_id
           << " | index=" << snapshot.contract.index_name
           << " | index_price=" << snapshot.latest_index_price
           << " | future=" << snapshot.latest_future_price
           << " | basis=" << snapshot.latest_basis
           << " | remaining_days=" << snapshot.remaining_days
           << " | annual_rate=" << snapshot.latest_annual_rate << '%';
    return output.str();
}

int ExtractContractMonthKey(const std::string& instrument_id)
{
    std::size_t end = instrument_id.size();
    std::size_t begin = end;
    while (begin > 0 && std::isdigit(static_cast<unsigned char>(instrument_id[begin - 1])))
    {
        --begin;
    }
    if (begin == end)
    {
        return std::numeric_limits<int>::max();
    }

    try
    {
        return std::stoi(instrument_id.substr(begin, end - begin));
    }
    catch (...)
    {
        return std::numeric_limits<int>::max();
    }
}

bool SnapshotLessByContractMonth(const LatestBasisSnapshot& left, const LatestBasisSnapshot& right)
{
    const int left_key = ExtractContractMonthKey(left.contract.instrument_id);
    const int right_key = ExtractContractMonthKey(right.contract.instrument_id);
    if (left_key != right_key)
    {
        return left_key < right_key;
    }
    return left.contract.instrument_id < right.contract.instrument_id;
}

} // namespace

std::string FormatBasisReport(const std::vector<LatestBasisSnapshot>& snapshots, BasisReportMoment moment)
{
    return FormatBasisReport(snapshots, moment, BasisReportMetadata{});
}

std::string FormatBasisReport(const std::vector<LatestBasisSnapshot>& snapshots,
                              BasisReportMoment moment,
                              const BasisReportMetadata& metadata)
{
    static const std::vector<std::string> kGroupOrder = {"hs300", "zz500", "zz1000"};

    std::map<std::string, std::vector<LatestBasisSnapshot>> grouped_snapshots;
    for (const auto& snapshot : snapshots)
    {
        grouped_snapshots[snapshot.contract.report_group].push_back(snapshot);
    }
    for (auto& entry : grouped_snapshots)
    {
        std::sort(entry.second.begin(), entry.second.end(), SnapshotLessByContractMonth);
    }

    std::ostringstream output;
    output << ReportTitle(moment) << '\n';
    if (!metadata.report_generated_at.empty())
    {
        output << "report_generated_at=" << metadata.report_generated_at << '\n';
    }
    output << "data_as_of=" << (metadata.data_as_of.empty() ? "N/A" : metadata.data_as_of) << '\n';
    output << "market_data_status=" << (metadata.stale ? "STALE" : "OK") << '\n';
    output << "===============================\n";
    for (const auto& group : kGroupOrder)
    {
        output << "[GROUP] " << group << '\n';
        const auto group_it = grouped_snapshots.find(group);
        if (group_it == grouped_snapshots.end() || group_it->second.empty())
        {
            output << "(no contracts)\n";
            continue;
        }

        for (const auto& snapshot : group_it->second)
        {
            output << FormatSnapshotLine(snapshot) << '\n';
        }
    }
    output << "===============================\n";
    return output.str();
}

} // namespace basis_monitor
