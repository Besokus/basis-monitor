#include "basis_monitor/data/reference_subset_builder.h"

#include <fstream>
#include <set>
#include <stdexcept>

namespace basis_monitor
{

namespace
{

template <typename Record, typename Predicate>
std::vector<Record> FilterRecords(const std::vector<Record>& records, Predicate predicate)
{
    std::vector<Record> filtered;
    for (const auto& record : records)
    {
        if (predicate(record))
        {
            filtered.push_back(record);
        }
    }
    return filtered;
}

std::set<std::string> SelectedInstrumentIds(const ContractSelectionResult& selection)
{
    std::set<std::string> instrument_ids;
    for (const auto& entry : selection.grouped_contracts)
    {
        for (const auto& contract : entry.second)
        {
            instrument_ids.insert(contract.instrument_id);
        }
    }
    return instrument_ids;
}

std::set<std::string> SelectedIndexCodes(const ContractSelectionResult& selection)
{
    std::set<std::string> index_codes;
    for (const auto& entry : selection.grouped_contracts)
    {
        for (const auto& contract : entry.second)
        {
            index_codes.insert(contract.index_code);
        }
    }
    return index_codes;
}

std::string EscapeCsvField(const std::string& value)
{
    if (value.find_first_of(",\"\n\r") == std::string::npos)
    {
        return value;
    }

    std::string escaped = "\"";
    for (const char ch : value)
    {
        if (ch == '"')
        {
            escaped += "\"\"";
        }
        else
        {
            escaped.push_back(ch);
        }
    }
    escaped.push_back('"');
    return escaped;
}

void EnsureParentDirectory(const std::filesystem::path& file_path)
{
    const auto parent = file_path.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent);
    }
}

template <typename Writer>
void WriteCsvFile(const std::filesystem::path& file_path,
                  const std::string& header_line,
                  Writer writer)
{
    EnsureParentDirectory(file_path);
    std::ofstream output(file_path);
    if (!output.is_open())
    {
        throw std::runtime_error("Cannot write CSV file: " + file_path.string());
    }

    output << header_line << '\n';
    writer(output);
}

} // namespace

ReferenceDataLoadResult BuildReferenceDataSubset(const ReferenceDataLoadResult& reference_data,
                                                 const ContractSelectionResult& selection)
{
    const auto instrument_ids = SelectedInstrumentIds(selection);
    const auto index_codes = SelectedIndexCodes(selection);

    ReferenceDataLoadResult subset = {};
    subset.future_metadata_date = reference_data.future_metadata_date;
    subset.index_metadata_date = reference_data.index_metadata_date;
    subset.future_eod_date = reference_data.future_eod_date;
    subset.index_eod_date = reference_data.index_eod_date;

    subset.future_metadata = FilterRecords<FutureMetadataRecord>(
        reference_data.future_metadata,
        [&](const FutureMetadataRecord& record) {
            return instrument_ids.find(record.order_book_id) != instrument_ids.end();
        });
    subset.index_metadata = FilterRecords<IndexMetadataRecord>(
        reference_data.index_metadata,
        [&](const IndexMetadataRecord& record) {
            return index_codes.find(record.order_book_id) != index_codes.end();
        });
    subset.future_eod = FilterRecords<FutureEodRecord>(
        reference_data.future_eod,
        [&](const FutureEodRecord& record) {
            return instrument_ids.find(record.order_book_id) != instrument_ids.end();
        });
    subset.index_eod = FilterRecords<IndexEodRecord>(
        reference_data.index_eod,
        [&](const IndexEodRecord& record) {
            return index_codes.find(record.order_book_id) != index_codes.end();
        });

    return subset;
}

void WriteReferenceDataSubset(const ReferenceDataLoadResult& reference_data,
                              const std::filesystem::path& output_root)
{
    if (reference_data.future_metadata_date.empty() ||
        reference_data.index_metadata_date.empty() ||
        reference_data.future_eod_date.empty() ||
        reference_data.index_eod_date.empty())
    {
        throw std::runtime_error("Reference data subset is missing source dates");
    }

    const auto future_metadata_path = output_root / "all_instruments" / "Future" / (reference_data.future_metadata_date + ".csv");
    const auto index_metadata_path = output_root / "all_instruments" / "INDX" / (reference_data.index_metadata_date + ".csv");
    const auto future_eod_path = output_root / "eod_price" / "Future" / (reference_data.future_eod_date + ".csv");
    const auto index_eod_path = output_root / "eod_price" / "INDX" / (reference_data.index_eod_date + ".csv");

    WriteCsvFile(
        future_metadata_path,
        "order_book_id,exchange,underlying_symbol,underlying_order_book_id,product,maturity_date",
        [&](std::ofstream& output) {
            for (const auto& record : reference_data.future_metadata)
            {
                output << EscapeCsvField(record.order_book_id) << ','
                       << EscapeCsvField(record.exchange) << ','
                       << EscapeCsvField(record.underlying_symbol) << ','
                       << EscapeCsvField(record.underlying_order_book_id) << ','
                       << EscapeCsvField(record.product) << ','
                       << EscapeCsvField(record.maturity_date) << '\n';
            }
        });

    WriteCsvFile(
        index_metadata_path,
        "order_book_id,symbol",
        [&](std::ofstream& output) {
            for (const auto& record : reference_data.index_metadata)
            {
                output << EscapeCsvField(record.order_book_id) << ','
                       << EscapeCsvField(record.symbol) << '\n';
            }
        });

    WriteCsvFile(
        future_eod_path,
        "trade_date,underlying_symbol,order_book_id,close,total_turnover",
        [&](std::ofstream& output) {
            for (const auto& record : reference_data.future_eod)
            {
                output << EscapeCsvField(record.trade_date) << ','
                       << EscapeCsvField(record.underlying_symbol) << ','
                       << EscapeCsvField(record.order_book_id) << ','
                       << record.close << ','
                       << record.total_turnover << '\n';
            }
        });

    WriteCsvFile(
        index_eod_path,
        "trade_date,order_book_id,close",
        [&](std::ofstream& output) {
            for (const auto& record : reference_data.index_eod)
            {
                output << EscapeCsvField(record.trade_date) << ','
                       << EscapeCsvField(record.order_book_id) << ','
                       << record.close << '\n';
            }
        });
}

} // namespace basis_monitor
