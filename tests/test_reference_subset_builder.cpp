#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

#include "basis_monitor/data/contract_selector.h"
#include "basis_monitor/data/reference_data_types.h"
#include "basis_monitor/data/reference_subset_builder.h"

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
                                             double close,
                                             double total_turnover)
{
    basis_monitor::FutureEodRecord record = {};
    record.trade_date = "2026-04-07";
    record.order_book_id = order_book_id;
    record.underlying_symbol = underlying_symbol;
    record.close = close;
    record.total_turnover = total_turnover;
    return record;
}

basis_monitor::IndexEodRecord MakeIndexEod(const std::string& order_book_id,
                                           double close)
{
    basis_monitor::IndexEodRecord record = {};
    record.trade_date = "2026-04-07";
    record.order_book_id = order_book_id;
    record.close = close;
    return record;
}

bool ContainsFutureContract(const std::vector<basis_monitor::FutureMetadataRecord>& records,
                            const std::string& instrument_id)
{
    return std::any_of(records.begin(), records.end(), [&](const auto& record) {
        return record.order_book_id == instrument_id;
    });
}

bool ContainsFutureEod(const std::vector<basis_monitor::FutureEodRecord>& records,
                       const std::string& instrument_id)
{
    return std::any_of(records.begin(), records.end(), [&](const auto& record) {
        return record.order_book_id == instrument_id;
    });
}

bool ContainsIndexRecord(const std::vector<basis_monitor::IndexMetadataRecord>& records,
                         const std::string& index_code)
{
    return std::any_of(records.begin(), records.end(), [&](const auto& record) {
        return record.order_book_id == index_code;
    });
}

bool ContainsIndexEod(const std::vector<basis_monitor::IndexEodRecord>& records,
                      const std::string& index_code)
{
    return std::any_of(records.begin(), records.end(), [&](const auto& record) {
        return record.order_book_id == index_code;
    });
}

std::string ReadWholeFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    assert(input.is_open());
    return std::string((std::istreambuf_iterator<char>(input)),
                       std::istreambuf_iterator<char>());
}

} // namespace

int main()
{
    basis_monitor::ReferenceDataLoadResult reference_data = {};
    reference_data.future_metadata_date = "2026-04-07";
    reference_data.index_metadata_date = "2026-04-07";
    reference_data.future_eod_date = "2026-04-07";
    reference_data.index_eod_date = "2026-04-07";
    reference_data.future_metadata = {
        MakeFutureMetadata("IC2603", "CFFEX", "IC", "000905.XSHG", "Index", "2026-04-06"),
        MakeFutureMetadata("IC2604", "CFFEX", "IC", "000905.XSHG", "Index", "2026-04-17"),
        MakeFutureMetadata("IC2605", "CFFEX", "IC", "000905.XSHG", "Index", "2026-05-15"),
        MakeFutureMetadata("IC2606", "CFFEX", "IC", "000905.XSHG", "Index", "2026-06-19"),
        MakeFutureMetadata("IC2609", "CFFEX", "IC", "000905.XSHG", "Index", "2026-09-18"),
        MakeFutureMetadata("IC2612", "CFFEX", "IC", "000905.XSHG", "Index", "2026-12-18"),
        MakeFutureMetadata("IF2604", "CFFEX", "IF", "000300.XSHG", "Index", "2026-04-17"),
        MakeFutureMetadata("IF2605", "CFFEX", "IF", "000300.XSHG", "Index", "2026-05-15"),
        MakeFutureMetadata("IF2606", "CFFEX", "IF", "000300.XSHG", "Index", "2026-06-19"),
        MakeFutureMetadata("IF2609", "CFFEX", "IF", "000300.XSHG", "Index", "2026-09-18"),
        MakeFutureMetadata("IM2604", "CFFEX", "IM", "000852.XSHG", "Index", "2026-04-17"),
        MakeFutureMetadata("IM2605", "CFFEX", "IM", "000852.XSHG", "Index", "2026-05-15"),
        MakeFutureMetadata("IM2606", "CFFEX", "IM", "000852.XSHG", "Index", "2026-06-19"),
        MakeFutureMetadata("IM2609", "CFFEX", "IM", "000852.XSHG", "Index", "2026-09-18"),
        MakeFutureMetadata("IM2612", "CFFEX", "IM", "000852.XSHG", "Index", "2026-12-18"),
        MakeFutureMetadata("IH2605", "CFFEX", "IH", "000016.XSHG", "Index", "2026-05-15")
    };
    reference_data.index_metadata = {
        MakeIndexMetadata("000300.XSHG", "CSI 300"),
        MakeIndexMetadata("000905.XSHG", "CSI 500"),
        MakeIndexMetadata("000852.XSHG", "CSI 1000"),
        MakeIndexMetadata("000016.XSHG", "SSE 50")
    };
    reference_data.future_eod = {
        MakeFutureEod("IC2603", "IC", 100.0, 12000.0),
        MakeFutureEod("IC2604", "IC", 100.0, 5000.0),
        MakeFutureEod("IC2605", "IC", 100.0, 7000.0),
        MakeFutureEod("IC2606", "IC", 100.0, 9000.0),
        MakeFutureEod("IC2609", "IC", 100.0, 8000.0),
        MakeFutureEod("IC2612", "IC", 100.0, 4000.0),
        MakeFutureEod("IF2604", "IF", 100.0, 6000.0),
        MakeFutureEod("IF2605", "IF", 100.0, 3000.0),
        MakeFutureEod("IF2606", "IF", 100.0, 2000.0),
        MakeFutureEod("IF2609", "IF", 100.0, 1000.0),
        MakeFutureEod("IM2604", "IM", 100.0, 9000.0),
        MakeFutureEod("IM2605", "IM", 100.0, 8000.0),
        MakeFutureEod("IM2606", "IM", 100.0, 7000.0),
        MakeFutureEod("IM2609", "IM", 100.0, 6000.0),
        MakeFutureEod("IM2612", "IM", 100.0, 5000.0),
        MakeFutureEod("IH2605", "IH", 100.0, 10000.0)
    };
    reference_data.index_eod = {
        MakeIndexEod("000300.XSHG", 4500.0),
        MakeIndexEod("000905.XSHG", 6200.0),
        MakeIndexEod("000852.XSHG", 7700.0),
        MakeIndexEod("000016.XSHG", 3100.0)
    };

    const auto selection = basis_monitor::SelectPerProductTop4(reference_data, "2026-04-07");
    const auto subset = basis_monitor::BuildReferenceDataSubset(reference_data, selection);

    assert(subset.future_metadata_date == "2026-04-07");
    assert(subset.future_eod_date == "2026-04-07");
    assert(subset.future_metadata.size() == 12);
    assert(subset.future_eod.size() == 12);
    assert(subset.index_metadata.size() == 3);
    assert(subset.index_eod.size() == 3);

    assert(ContainsFutureContract(subset.future_metadata, "IC2606"));
    assert(ContainsFutureContract(subset.future_metadata, "IC2609"));
    assert(ContainsFutureContract(subset.future_metadata, "IC2605"));
    assert(ContainsFutureContract(subset.future_metadata, "IC2604"));
    assert(!ContainsFutureContract(subset.future_metadata, "IC2603"));
    assert(ContainsFutureContract(subset.future_metadata, "IF2604"));
    assert(ContainsFutureContract(subset.future_metadata, "IF2605"));
    assert(ContainsFutureContract(subset.future_metadata, "IF2606"));
    assert(ContainsFutureContract(subset.future_metadata, "IF2609"));
    assert(ContainsFutureContract(subset.future_metadata, "IM2604"));
    assert(ContainsFutureContract(subset.future_metadata, "IM2605"));
    assert(ContainsFutureContract(subset.future_metadata, "IM2606"));
    assert(ContainsFutureContract(subset.future_metadata, "IM2609"));
    assert(!ContainsFutureContract(subset.future_metadata, "IC2612"));
    assert(!ContainsFutureContract(subset.future_metadata, "IM2612"));
    assert(!ContainsFutureContract(subset.future_metadata, "IH2605"));

    assert(ContainsFutureEod(subset.future_eod, "IC2606"));
    assert(!ContainsFutureEod(subset.future_eod, "IC2612"));
    assert(!ContainsFutureEod(subset.future_eod, "IH2605"));
    assert(!ContainsFutureEod(subset.future_eod, "IC2603"));

    assert(ContainsIndexRecord(subset.index_metadata, "000300.XSHG"));
    assert(ContainsIndexRecord(subset.index_metadata, "000905.XSHG"));
    assert(ContainsIndexRecord(subset.index_metadata, "000852.XSHG"));
    assert(!ContainsIndexRecord(subset.index_metadata, "000016.XSHG"));

    assert(ContainsIndexEod(subset.index_eod, "000300.XSHG"));
    assert(!ContainsIndexEod(subset.index_eod, "000016.XSHG"));

    const auto temp_root = std::filesystem::temp_directory_path() / "basis_monitor_reference_subset_builder";
    std::filesystem::remove_all(temp_root);
    basis_monitor::WriteReferenceDataSubset(subset, temp_root);

    const auto future_metadata_path = temp_root / "all_instruments" / "Future" / "2026-04-07.csv";
    const auto index_metadata_path = temp_root / "all_instruments" / "INDX" / "2026-04-07.csv";
    const auto future_eod_path = temp_root / "eod_price" / "Future" / "2026-04-07.csv";
    const auto index_eod_path = temp_root / "eod_price" / "INDX" / "2026-04-07.csv";

    assert(std::filesystem::exists(future_metadata_path));
    assert(std::filesystem::exists(index_metadata_path));
    assert(std::filesystem::exists(future_eod_path));
    assert(std::filesystem::exists(index_eod_path));

    const auto future_metadata_contents = ReadWholeFile(future_metadata_path);
    assert(future_metadata_contents.find("order_book_id,exchange,underlying_symbol,underlying_order_book_id,product,maturity_date") != std::string::npos);
    assert(future_metadata_contents.find("IC2606,CFFEX,IC,000905.XSHG,Index,2026-06-19") != std::string::npos);
    assert(future_metadata_contents.find("IH2605") == std::string::npos);

    const auto index_metadata_contents = ReadWholeFile(index_metadata_path);
    assert(index_metadata_contents.find("000905.XSHG,CSI 500") != std::string::npos);
    assert(index_metadata_contents.find("000016.XSHG") == std::string::npos);

    const auto future_eod_contents = ReadWholeFile(future_eod_path);
    assert(future_eod_contents.find("trade_date,underlying_symbol,order_book_id,close,total_turnover") != std::string::npos);
    assert(future_eod_contents.find("2026-04-07,IC,IC2606,100,9000") != std::string::npos);
    assert(future_eod_contents.find("IH2605") == std::string::npos);

    const auto index_eod_contents = ReadWholeFile(index_eod_path);
    assert(index_eod_contents.find("trade_date,order_book_id,close") != std::string::npos);
    assert(index_eod_contents.find("2026-04-07,000852.XSHG,7700") != std::string::npos);
    assert(index_eod_contents.find("000016.XSHG") == std::string::npos);

    std::filesystem::remove_all(temp_root);
    return 0;
}
