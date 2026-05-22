#include "basis_monitor/notify/wecom_message_formatter.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <vector>

namespace basis_monitor
{

namespace
{

std::string FormatPercent(double value)
{
    std::ostringstream output;
    output << std::fixed << std::setprecision(4) << value << '%';
    return output.str();
}

std::string FormatDouble(double value)
{
    std::ostringstream output;
    output << std::fixed << std::setprecision(4) << value;
    return output.str();
}

std::string MomentTitle(BasisReportMoment moment)
{
    switch (moment)
    {
    case BasisReportMoment::Midday1130:
        return "11:30 最新基差表";
    case BasisReportMoment::Close1500:
        return "15:00 最新基差表";
    }

    return "最新基差表";
}

std::string FormatSummaryLine(const std::string& label, const std::string& value, const char* color)
{
    std::ostringstream output;
    output << '>' << label << "：<font color=\"" << color << "\">" << value << "</font>\n";
    return output.str();
}

std::string FormatSnapshotDetail(const LatestBasisSnapshot& snapshot)
{
    std::ostringstream output;
    output << '>' << snapshot.contract.instrument_id << "：";
    if (!snapshot.has_tick)
    {
        output << "<font color=\"comment\">暂无行情</font>";
        return output.str();
    }

    const bool negative = snapshot.latest_annual_rate < 0.0;
    output << "<font color=\"" << (negative ? "warning" : "comment") << "\">"
           << FormatPercent(snapshot.latest_annual_rate)
           << "</font>"
           << " | 基差 " << FormatDouble(snapshot.latest_basis)
           << " | 剩余 " << snapshot.remaining_days << "天";
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

std::string FormatWeComAlertMarkdown(const MonitorUpdate& update)
{
    std::ostringstream output;
    output << "### 年化基差告警\n"
           << "年化基差率已转为<font color=\"warning\">负值</font>，请及时关注。\n"
           << FormatSummaryLine("品种", update.contract.product_group, "comment")
           << FormatSummaryLine("合约", update.contract.instrument_id, "comment")
           << FormatSummaryLine("指数", update.contract.index_name, "comment")
           << FormatSummaryLine("年化基差率", FormatPercent(update.result.annual_rate), "warning")
           << FormatSummaryLine("基差", FormatDouble(update.result.basis), "comment")
           << FormatSummaryLine("指数价", FormatDouble(update.index_price), "comment")
           << FormatSummaryLine("期货价", FormatDouble(update.future_price), "comment")
           << FormatSummaryLine("剩余天数", std::to_string(update.result.remaining_days), "comment");
    return output.str();
}

std::string FormatWeComBasisReportMarkdown(const std::vector<LatestBasisSnapshot>& snapshots, BasisReportMoment moment)
{
    static const std::vector<std::string> kGroupOrder = {"IF", "IC", "IM", "IH"};

    std::map<std::string, std::vector<LatestBasisSnapshot>> grouped;
    for (const auto& snapshot : snapshots)
    {
        grouped[snapshot.contract.product_group].push_back(snapshot);
    }
    for (auto& entry : grouped)
    {
        std::sort(entry.second.begin(), entry.second.end(), SnapshotLessByContractMonth);
    }

    int with_tick_count = 0;
    int negative_count = 0;
    double min_annual_rate = std::numeric_limits<double>::infinity();
    double max_annual_rate = -std::numeric_limits<double>::infinity();
    std::string min_contract = "无";
    std::string max_contract = "无";

    for (const auto& snapshot : snapshots)
    {
        if (!snapshot.has_tick)
        {
            continue;
        }

        ++with_tick_count;
        if (snapshot.latest_annual_rate < 0.0)
        {
            ++negative_count;
        }
        if (snapshot.latest_annual_rate < min_annual_rate)
        {
            min_annual_rate = snapshot.latest_annual_rate;
            min_contract = snapshot.contract.instrument_id + " " + FormatPercent(snapshot.latest_annual_rate);
        }
        if (snapshot.latest_annual_rate > max_annual_rate)
        {
            max_annual_rate = snapshot.latest_annual_rate;
            max_contract = snapshot.contract.instrument_id + " " + FormatPercent(snapshot.latest_annual_rate);
        }
    }

    std::ostringstream output;
    output << "### " << MomentTitle(moment) << '\n'
           << "盘中监控摘要如下：\n"
           << FormatSummaryLine("监控合约数", std::to_string(snapshots.size()), "comment")
           << FormatSummaryLine("已有行情", std::to_string(with_tick_count), "comment")
           << FormatSummaryLine("负年化基差", std::to_string(negative_count), negative_count > 0 ? "warning" : "comment")
           << FormatSummaryLine("最低年化基差率", min_contract, with_tick_count > 0 && min_annual_rate < 0.0 ? "warning" : "comment")
           << FormatSummaryLine("最高年化基差率", max_contract, "comment");

    for (const auto& group : kGroupOrder)
    {
        output << "\n**" << group << "**\n";
        const auto it = grouped.find(group);
        if (it == grouped.end() || it->second.empty())
        {
            output << ">暂无监控合约\n";
            continue;
        }

        for (const auto& snapshot : it->second)
        {
            output << FormatSnapshotDetail(snapshot) << '\n';
        }
    }

    return output.str();
}

} // namespace basis_monitor
