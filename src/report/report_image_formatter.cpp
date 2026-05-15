#include "basis_monitor/report/report_image_formatter.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

namespace basis_monitor
{

namespace
{

std::string TitleForMoment(BasisReportMoment moment)
{
    switch (moment)
    {
    case BasisReportMoment::Midday1130:
        return "BASIS MONITOR 11:30 REPORT";
    case BasisReportMoment::Close1500:
        return "BASIS MONITOR 15:00 REPORT";
    }

    return "BASIS MONITOR REPORT";
}

std::string EscapeJson(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (const char ch : value)
    {
        switch (ch)
        {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }
    return escaped;
}

bool ParseIsoDate(const std::string& text, std::tm* result)
{
    if (text.size() != 10 || result == nullptr)
    {
        return false;
    }

    std::tm parsed = {};
    parsed.tm_year = std::stoi(text.substr(0, 4)) - 1900;
    parsed.tm_mon = std::stoi(text.substr(5, 2)) - 1;
    parsed.tm_mday = std::stoi(text.substr(8, 2));
    parsed.tm_hour = 0;
    parsed.tm_min = 0;
    parsed.tm_sec = 0;
    parsed.tm_isdst = -1;
    *result = parsed;
    return true;
}

int DaysUntilMaturity(const std::string& trading_date, const std::string& maturity_date)
{
    std::tm trading_tm = {};
    std::tm maturity_tm = {};
    if (!ParseIsoDate(trading_date, &trading_tm) || !ParseIsoDate(maturity_date, &maturity_tm))
    {
        return std::numeric_limits<int>::max();
    }

    const std::time_t trading_time = std::mktime(&trading_tm);
    const std::time_t maturity_time = std::mktime(&maturity_tm);
    if (trading_time == static_cast<std::time_t>(-1) || maturity_time == static_cast<std::time_t>(-1))
    {
        return std::numeric_limits<int>::max();
    }

    const auto seconds = std::difftime(maturity_time, trading_time);
    return static_cast<int>(seconds / (60.0 * 60.0 * 24.0));
}

std::string FormatFixed(double value, int decimals)
{
    std::ostringstream output;
    output << std::fixed << std::setprecision(decimals) << value;
    return output.str();
}

std::string FormatPercent(double value, int decimals)
{
    return FormatFixed(value, decimals) + "%";
}

BasisReportImageRow BuildRow(const LatestBasisSnapshot& snapshot,
                             double negative_threshold,
                             const std::string& trading_date)
{
    BasisReportImageRow row = {};
    row.instrument_id = snapshot.contract.instrument_id;
    row.remaining_days = snapshot.has_tick
        ? snapshot.remaining_days
        : DaysUntilMaturity(trading_date, snapshot.contract.maturity_date);
    row.remaining_days_text = row.remaining_days == std::numeric_limits<int>::max()
        ? "N/A"
        : std::to_string(row.remaining_days);

    if (!snapshot.has_tick)
    {
        row.price_text = "N/A";
        row.change_text = "N/A";
        row.change_percent_text = "N/A";
        row.basis_text = "N/A";
        row.annual_rate_text = "N/A";
        row.warning_text = "-";
        return row;
    }

    row.price_text = FormatFixed(snapshot.latest_future_price, 1);
    row.basis_text = FormatFixed(snapshot.latest_basis, 2);
    row.annual_rate_text = FormatPercent(snapshot.latest_annual_rate, 2);

    if (snapshot.contract.future_close_yesterday > 0.0)
    {
        const double change = snapshot.latest_future_price - snapshot.contract.future_close_yesterday;
        const double change_percent = (change / snapshot.contract.future_close_yesterday) * 100.0;
        row.change_text = FormatFixed(change, 1);
        row.change_percent_text = FormatPercent(change_percent, 2);
    }
    else
    {
        row.change_text = "N/A";
        row.change_percent_text = "N/A";
    }

    if (snapshot.latest_annual_rate < negative_threshold)
    {
        row.warning_text = FormatPercent(snapshot.latest_annual_rate, 2);
        row.warning_negative = true;
    }
    else
    {
        row.warning_text = "-";
    }

    return row;
}

std::string SerializeRow(const BasisReportImageRow& row)
{
    std::ostringstream output;
    output << "{"
           << "\"instrument_id\":\"" << EscapeJson(row.instrument_id) << "\","
           << "\"remaining_days\":" << row.remaining_days << ","
           << "\"price_text\":\"" << EscapeJson(row.price_text) << "\","
           << "\"change_text\":\"" << EscapeJson(row.change_text) << "\","
           << "\"change_percent_text\":\"" << EscapeJson(row.change_percent_text) << "\","
           << "\"basis_text\":\"" << EscapeJson(row.basis_text) << "\","
           << "\"annual_rate_text\":\"" << EscapeJson(row.annual_rate_text) << "\","
           << "\"remaining_days_text\":\"" << EscapeJson(row.remaining_days_text) << "\","
           << "\"warning_text\":\"" << EscapeJson(row.warning_text) << "\","
           << "\"warning_negative\":" << (row.warning_negative ? "true" : "false")
           << "}";
    return output.str();
}

std::string SerializeGroup(const BasisReportImageGroup& group)
{
    std::ostringstream output;
    output << "{"
           << "\"name\":\"" << EscapeJson(group.name) << "\","
           << "\"rows\":[";
    for (std::size_t index = 0; index < group.rows.size(); ++index)
    {
        if (index > 0)
        {
            output << ",";
        }
        output << SerializeRow(group.rows[index]);
    }
    output << "]}";
    return output.str();
}

} // namespace

BasisReportImageDocument BuildBasisReportImageDocument(const std::vector<LatestBasisSnapshot>& snapshots,
                                                       BasisReportMoment moment,
                                                       double negative_threshold,
                                                       const std::string& trading_date,
                                                       const BasisReportMetadata& metadata)
{
    static const std::vector<std::string> kGroupOrder = {"hs300", "zz500", "zz1000"};

    BasisReportImageDocument document = {};
    document.title = TitleForMoment(moment);
    std::ostringstream subtitle;
    subtitle << "DATE " << trading_date
             << " | STAT " << (metadata.report_generated_at.empty() ? "N/A" : metadata.report_generated_at)
             << " | DATA " << (metadata.data_as_of.empty() ? "N/A" : metadata.data_as_of)
             << " | STATUS " << (metadata.stale ? "STALE" : "OK");
    document.subtitle = subtitle.str();
    document.columns = {"CONTRACT", "PRICE", "CHG", "CHG%", "BASIS", "ANNUAL", "DTE", "WARNING"};

    for (const auto& group_name : kGroupOrder)
    {
        BasisReportImageGroup group = {};
        group.name = group_name;
        for (const auto& snapshot : snapshots)
        {
            if (snapshot.contract.report_group != group_name)
            {
                continue;
            }
            group.rows.push_back(BuildRow(snapshot, negative_threshold, trading_date));
        }

        std::sort(group.rows.begin(), group.rows.end(),
                  [](const BasisReportImageRow& left, const BasisReportImageRow& right) {
                      if (left.remaining_days != right.remaining_days)
                      {
                          return left.remaining_days < right.remaining_days;
                      }
                      return left.instrument_id < right.instrument_id;
                  });
        document.groups.push_back(std::move(group));
    }

    return document;
}

std::string SerializeBasisReportImageDocument(const BasisReportImageDocument& document)
{
    std::ostringstream output;
    output << "{"
           << "\"title\":\"" << EscapeJson(document.title) << "\","
           << "\"subtitle\":\"" << EscapeJson(document.subtitle) << "\","
           << "\"columns\":[";
    for (std::size_t index = 0; index < document.columns.size(); ++index)
    {
        if (index > 0)
        {
            output << ",";
        }
        output << "\"" << EscapeJson(document.columns[index]) << "\"";
    }
    output << "],"
           << "\"groups\":[";
    for (std::size_t index = 0; index < document.groups.size(); ++index)
    {
        if (index > 0)
        {
            output << ",";
        }
        output << SerializeGroup(document.groups[index]);
    }
    output << "]}";
    return output.str();
}

} // namespace basis_monitor
