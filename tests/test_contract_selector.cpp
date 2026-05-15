#include <algorithm>
#include <cassert>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "basis_monitor/data/contract_selector.h"
#include "basis_monitor/data/reference_data_types.h"

namespace
{

basis_monitor::FutureMetadataRecord MakeFutureMetadata(const std::string& order_book_id,
                                                       const std::string& exchange,
                                                       const std::string& underlying_symbol,
                                                       const std::string& index_code,
                                                       const std::string& product,
                                                       const std::string& maturity_date)
{
    basis_monitor::FutureMetadataRecord record = {};
    record.order_book_id = order_book_id;
    record.exchange = exchange;
    record.underlying_symbol = underlying_symbol;
    record.underlying_order_book_id = index_code;
    record.product = product;
    record.maturity_date = maturity_date;
    return record;
}

basis_monitor::IndexMetadataRecord MakeIndexMetadata(const std::string& order_book_id,
                                                     const std::string& symbol)
{
    basis_monitor::IndexMetadataRecord record = {};
    record.order_book_id = order_book_id;
    record.symbol = symbol;
    return record;
}

basis_monitor::FutureEodRecord MakeFutureEod(const std::string& order_book_id,
                                             const std::string& underlying_symbol,
                                             double total_turnover)
{
    basis_monitor::FutureEodRecord record = {};
    record.trade_date = "2026-03-27";
    record.order_book_id = order_book_id;
    record.underlying_symbol = underlying_symbol;
    record.close = 100.0;
    record.total_turnover = total_turnover;
    return record;
}

basis_monitor::IndexEodRecord MakeIndexEod(const std::string& order_book_id,
                                           double close)
{
    basis_monitor::IndexEodRecord record = {};
    record.trade_date = "2026-03-27";
    record.order_book_id = order_book_id;
    record.close = close;
    return record;
}

bool HasWarningFor(const basis_monitor::ContractSelectionResult& result,
                   const std::string& instrument_id,
                   const std::string& reason_fragment)
{
    return std::any_of(result.warnings.begin(),
                       result.warnings.end(),
                       [&](const basis_monitor::ContractSelectionWarning& warning) {
                           return warning.instrument_id == instrument_id &&
                                  warning.reason.find(reason_fragment) != std::string::npos;
                       });
}

std::tm CurrentLocalTm()
{
    const std::time_t current_time = std::time(nullptr);
    std::tm local_time = {};
#if defined(_WIN32)
    localtime_s(&local_time, &current_time);
#else
    localtime_r(&current_time, &local_time);
#endif
    return local_time;
}

std::string YearMonthCodeFromTm(std::tm local_time)
{
    std::ostringstream stream;
    stream << std::setfill('0')
           << std::setw(2) << ((local_time.tm_year + 1900) % 100)
           << std::setw(2) << (local_time.tm_mon + 1);
    return stream.str();
}

std::string ContractMonthCodeOffset(int month_offset)
{
    std::tm local_time = CurrentLocalTm();
    local_time.tm_mday = 1;
    local_time.tm_mon += month_offset;
    local_time.tm_isdst = -1;
    std::mktime(&local_time);
    return YearMonthCodeFromTm(local_time);
}

std::string CurrentMonthContractId(const std::string& product_group)
{
    return product_group + YearMonthCodeFromTm(CurrentLocalTm());
}

std::string ContractIdForMonthOffset(const std::string& product_group, int month_offset)
{
    return product_group + ContractMonthCodeOffset(month_offset);
}

} // namespace

int main()
{
    const std::string current_month_ic = CurrentMonthContractId("IC");
    const std::string current_month_if = CurrentMonthContractId("IF");
    const std::string current_month_im = CurrentMonthContractId("IM");

    const std::string ic_1 = ContractIdForMonthOffset("IC", 1);
    const std::string ic_2 = ContractIdForMonthOffset("IC", 2);
    const std::string ic_3 = ContractIdForMonthOffset("IC", 3);
    const std::string ic_4 = ContractIdForMonthOffset("IC", 4);
    const std::string ic_expired = "IC2603";

    const std::string if_1 = ContractIdForMonthOffset("IF", 1);
    const std::string if_2 = ContractIdForMonthOffset("IF", 2);
    const std::string if_3 = ContractIdForMonthOffset("IF", 3);
    const std::string if_4 = ContractIdForMonthOffset("IF", 4);

    const std::string im_1 = ContractIdForMonthOffset("IM", 1);
    const std::string im_2 = ContractIdForMonthOffset("IM", 2);
    const std::string im_3 = ContractIdForMonthOffset("IM", 3);
    const std::string im_4 = ContractIdForMonthOffset("IM", 4);

    basis_monitor::ReferenceDataLoadResult reference_data = {};
    reference_data.future_metadata = {
        MakeFutureMetadata(current_month_ic, "CFFEX", "IC", "000905.XSHG", "Index", "2026-04-17"),
        MakeFutureMetadata(ic_expired, "CFFEX", "IC", "000905.XSHG", "Index", "2026-04-16"),
        MakeFutureMetadata(ic_1, "CFFEX", "IC", "000905.XSHG", "Index", "2026-05-15"),
        MakeFutureMetadata(ic_2, "CFFEX", "IC", "000905.XSHG", "Index", "2026-06-19"),
        MakeFutureMetadata(ic_3, "CFFEX", "IC", "000905.XSHG", "Index", "2026-09-18"),
        MakeFutureMetadata(ic_4, "CFFEX", "IC", "000905.XSHG", "Index", "2026-12-18"),
        MakeFutureMetadata(current_month_if, "CFFEX", "IF", "000300.XSHG", "Index", "2026-04-17"),
        MakeFutureMetadata(if_1, "CFFEX", "IF", "000300.XSHG", "Index", "2026-05-15"),
        MakeFutureMetadata(if_2, "CFFEX", "IF", "000300.XSHG", "Index", ""),
        MakeFutureMetadata(if_3, "CFFEX", "IF", "", "Index", "2026-09-18"),
        MakeFutureMetadata(if_4, "CFFEX", "IF", "000300.XSHG", "Index", "2026-12-18"),
        MakeFutureMetadata(current_month_im, "CFFEX", "IM", "000852.XSHG", "Index", "2026-04-17"),
        MakeFutureMetadata(im_1, "CFFEX", "IM", "000852.XSHG", "Index", "2026-05-15"),
        MakeFutureMetadata(im_2, "CFFEX", "IM", "000852.XSHG", "Index", "2026-06-19"),
        MakeFutureMetadata(im_3, "CFFEX", "IM", "000852.XSHG", "Index", "2026-09-18"),
        MakeFutureMetadata(im_4, "CFFEX", "IM", "000852.XSHG", "Index", "2026-12-18"),
        MakeFutureMetadata("IC2604_X", "SHFE", "IC", "000905.XSHG", "Index", "2026-04-17"),
        MakeFutureMetadata("IH2605_ALT", "CFFEX", "IH", "000016.XSHG", "Commodity", "2026-05-15")
    };
    reference_data.index_metadata = {
        MakeIndexMetadata("000300.XSHG", "CSI 300"),
        MakeIndexMetadata("000852.XSHG", "CSI 1000"),
        MakeIndexMetadata("000905.XSHG", "CSI 500")
    };
    reference_data.future_eod = {
        MakeFutureEod(current_month_ic, "IC", 9000.0),
        MakeFutureEod(ic_expired, "IC", 20000.0),
        MakeFutureEod(ic_1, "IC", 4000.0),
        MakeFutureEod(ic_2, "IC", 7000.0),
        MakeFutureEod(ic_3, "IC", 6000.0),
        MakeFutureEod(ic_4, "IC", 3000.0),
        MakeFutureEod("IC88", "IC", 1000.0),
        MakeFutureEod(current_month_if, "IF", 8000.0),
        MakeFutureEod(if_1, "IF", 6000.0),
        MakeFutureEod(if_2, "IF", 7000.0),
        MakeFutureEod(if_3, "IF", 5000.0),
        MakeFutureEod(if_4, "IF", 4500.0),
        MakeFutureEod("IF99", "IF", 10000.0),
        MakeFutureEod(current_month_im, "IM", 1000.0),
        MakeFutureEod(im_1, "IM", 5000.0),
        MakeFutureEod(im_2, "IM", 4000.0),
        MakeFutureEod(im_3, "IM", 3000.0),
        MakeFutureEod(im_4, "IM", 2000.0),
        MakeFutureEod("A2605", "A", 999999.0)
    };
    reference_data.index_eod = {
        MakeIndexEod("000300.XSHG", 4500.0),
        MakeIndexEod("000852.XSHG", 7700.0),
        MakeIndexEod("000905.XSHG", 6200.0)
    };

    assert(basis_monitor::IsCurrentMonthContract(current_month_ic));
    assert(basis_monitor::IsCurrentMonthContract(current_month_if));
    assert(basis_monitor::IsCurrentMonthContract(current_month_im));
    assert(!basis_monitor::IsCurrentMonthContract(ic_1));
    assert(!basis_monitor::IsCurrentMonthContract("IC88"));
    assert(!basis_monitor::IsCurrentMonthContract("A2605"));

    const auto trading_date = std::string("2026-04-17");
    const auto selection = basis_monitor::SelectPerProductTop4(reference_data, trading_date);

    assert(selection.grouped_contracts.size() == 3);

    const auto ic_it = selection.grouped_contracts.find("zz500");
    assert(ic_it != selection.grouped_contracts.end());
    assert(ic_it->second.size() == 4);
    assert(ic_it->second[0].instrument_id == current_month_ic);
    assert(ic_it->second[1].instrument_id == ic_2);
    assert(ic_it->second[2].instrument_id == ic_3);
    assert(ic_it->second[3].instrument_id == ic_1);
    assert(std::none_of(ic_it->second.begin(), ic_it->second.end(), [&](const auto& contract) {
        return contract.instrument_id == ic_expired;
    }));
    assert(ic_it->second[0].index_code == "000905.XSHG");
    assert(ic_it->second[0].index_name == "CSI 500");
    assert(ic_it->second[0].report_group == "zz500");
    assert(std::abs(ic_it->second[0].index_close_yesterday - 6200.0) < 0.0001);
    assert(std::abs(ic_it->second[0].future_close_yesterday - 100.0) < 0.0001);
    assert(ic_it->second[0].maturity_date == "2026-04-17");

    const auto if_it = selection.grouped_contracts.find("hs300");
    assert(if_it != selection.grouped_contracts.end());
    assert(if_it->second.size() == 3);
    assert(if_it->second[0].instrument_id == current_month_if);
    assert(if_it->second[1].instrument_id == if_1);
    assert(if_it->second[2].instrument_id == if_4);
    assert(if_it->second[0].report_group == "hs300");

    const auto ih_it = selection.grouped_contracts.find("IH");
    assert(ih_it == selection.grouped_contracts.end());

    const auto im_it = selection.grouped_contracts.find("zz1000");
    assert(im_it != selection.grouped_contracts.end());
    assert(im_it->second.size() == 4);
    assert(im_it->second[0].instrument_id == im_1);
    assert(im_it->second[1].instrument_id == im_2);
    assert(im_it->second[2].instrument_id == im_3);
    assert(im_it->second[3].instrument_id == im_4);
    assert(im_it->second[0].report_group == "zz1000");

    assert(!HasWarningFor(selection, "IC88", ""));
    assert(!HasWarningFor(selection, "IF99", ""));
    assert(!HasWarningFor(selection, "A2605", ""));
    assert(HasWarningFor(selection, if_2, "maturity"));
    assert(HasWarningFor(selection, if_3, "index mapping"));

    return 0;
}
