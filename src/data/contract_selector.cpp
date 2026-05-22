#include "basis_monitor/data/contract_selector.h"

#include <algorithm>
#include <array>
#include <ctime>
#include <iomanip>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <unordered_map>

namespace basis_monitor
{

namespace
{

bool IsSupportedProductGroup(const std::string& product_group)
{
    static const std::set<std::string> kSupportedProductGroups = {"IC", "IF", "IM"};
    return kSupportedProductGroups.find(product_group) != kSupportedProductGroups.end();
}

std::string ReportGroupForProductGroup(const std::string& product_group)
{
    if (product_group == "IF")
    {
        return "hs300";
    }
    if (product_group == "IC")
    {
        return "zz500";
    }
    if (product_group == "IM")
    {
        return "zz1000";
    }
    return {};
}

bool IsRealMonthContract(const std::string& instrument_id, std::string* product_group)
{
    static const std::regex kPattern("^(IC|IF|IH|IM)(\\d{4})$");

    std::smatch match;
    if (!std::regex_match(instrument_id, match, kPattern))
    {
        return false;
    }

    if (product_group != nullptr)
    {
        *product_group = match[1].str();
    }
    return true;
}

bool IsExpiredContract(const std::string& maturity_date, const std::string& trading_date)
{
    return !maturity_date.empty() && maturity_date < trading_date;
}

std::string CurrentContractMonthCode()
{
    const std::time_t current_time = std::time(nullptr);
    std::tm local_time = {};
#if defined(_WIN32)
    localtime_s(&local_time, &current_time);
#else
    localtime_r(&current_time, &local_time);
#endif

    std::ostringstream stream;
    stream << std::setfill('0')
           << std::setw(2) << ((local_time.tm_year + 1900) % 100)
           << std::setw(2) << (local_time.tm_mon + 1);
    return stream.str();
}

void AddWarning(ContractSelectionResult* result,
                const std::string& instrument_id,
                const std::string& reason)
{
    ContractSelectionWarning warning = {};
    warning.instrument_id = instrument_id;
    warning.reason = reason;
    result->warnings.push_back(warning);
}

} // namespace

ContractSelectionResult SelectPerProductTop4(const ReferenceDataLoadResult& reference_data,
                                             const std::string& trading_date)
{
    ContractSelectionResult result = {};
    const std::array<std::string, 3> report_groups = {"hs300", "zz500", "zz1000"};
    for (const auto& report_group : report_groups)
    {
        result.grouped_contracts[report_group] = {};
    }

    std::unordered_map<std::string, FutureMetadataRecord> future_metadata_by_id;
    for (const auto& record : reference_data.future_metadata)
    {
        future_metadata_by_id[record.order_book_id] = record;
    }

    std::unordered_map<std::string, std::string> index_name_by_code;
    for (const auto& record : reference_data.index_metadata)
    {
        index_name_by_code[record.order_book_id] = record.symbol;
    }

    std::unordered_map<std::string, double> index_close_by_code;
    for (const auto& record : reference_data.index_eod)
    {
        index_close_by_code[record.order_book_id] = record.close;
    }

    for (const auto& record : reference_data.future_eod)
    {
        std::string product_group;
        if (!IsRealMonthContract(record.order_book_id, &product_group) || !IsSupportedProductGroup(product_group))
        {
            continue;
        }

        const auto metadata_it = future_metadata_by_id.find(record.order_book_id);
        if (metadata_it == future_metadata_by_id.end())
        {
            AddWarning(&result, record.order_book_id, "missing future metadata");
            continue;
        }

        const auto& metadata = metadata_it->second;
        if (metadata.exchange != "CFFEX" || metadata.product != "Index")
        {
            continue;
        }

        if (metadata.maturity_date.empty() || metadata.maturity_date == "0000-00-00")
        {
            AddWarning(&result, record.order_book_id, "missing maturity date");
            continue;
        }

        if (IsExpiredContract(metadata.maturity_date, trading_date))
        {
            continue;
        }

        if (metadata.underlying_order_book_id.empty())
        {
            AddWarning(&result, record.order_book_id, "missing index mapping");
            continue;
        }

        const auto index_name_it = index_name_by_code.find(metadata.underlying_order_book_id);
        const auto index_close_it = index_close_by_code.find(metadata.underlying_order_book_id);
        if (index_name_it == index_name_by_code.end() ||
            index_close_it == index_close_by_code.end() ||
            index_close_it->second <= 0.0)
        {
            AddWarning(&result, record.order_book_id, "missing index mapping");
            continue;
        }

        MonitoredContract monitored_contract = {};
        monitored_contract.instrument_id = record.order_book_id;
        monitored_contract.product_group = product_group;
        monitored_contract.report_group = ReportGroupForProductGroup(product_group);
        monitored_contract.index_code = metadata.underlying_order_book_id;
        monitored_contract.index_name = index_name_it->second;
        monitored_contract.index_close_yesterday = index_close_it->second;
        monitored_contract.future_close_yesterday = record.close;
        monitored_contract.maturity_date = metadata.maturity_date;
        monitored_contract.yesterday_turnover = record.total_turnover;
        result.grouped_contracts[monitored_contract.report_group].push_back(monitored_contract);
    }

    for (auto& entry : result.grouped_contracts)
    {
        auto& contracts = entry.second;
        std::sort(contracts.begin(),
                  contracts.end(),
                  [](const MonitoredContract& left, const MonitoredContract& right) {
                      if (left.yesterday_turnover != right.yesterday_turnover)
                      {
                          return left.yesterday_turnover > right.yesterday_turnover;
                      }
                      return left.instrument_id < right.instrument_id;
                  });
        if (contracts.size() > 4)
        {
            contracts.resize(4);
        }
    }

    return result;
}

bool IsCurrentMonthContract(const std::string& instrument_id)
{
    std::string product_group;
    if (!IsRealMonthContract(instrument_id, &product_group))
    {
        return false;
    }

    return instrument_id.size() >= 4 &&
           instrument_id.substr(instrument_id.size() - 4) == CurrentContractMonthCode();
}

} // namespace basis_monitor
