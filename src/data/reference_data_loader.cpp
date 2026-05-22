#include "basis_monitor/data/reference_data_loader.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace basis_monitor
{

namespace
{

struct DatedFile
{
    std::filesystem::path path;
    std::string date;
};

std::string Trim(const std::string& value)
{
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
    {
        ++start;
    }

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
    {
        --end;
    }

    return value.substr(start, end - start);
}

std::string StripUtf8Bom(const std::string& value)
{
    if (value.size() >= 3 &&
        static_cast<unsigned char>(value[0]) == 0xEF &&
        static_cast<unsigned char>(value[1]) == 0xBB &&
        static_cast<unsigned char>(value[2]) == 0xBF)
    {
        return value.substr(3);
    }

    return value;
}

std::vector<std::string> ParseCsvRow(const std::string& line)
{
    std::vector<std::string> fields;
    std::string current;
    bool in_quotes = false;

    for (std::size_t index = 0; index < line.size(); ++index)
    {
        const char ch = line[index];
        if (ch == '"')
        {
            if (in_quotes && index + 1 < line.size() && line[index + 1] == '"')
            {
                current.push_back('"');
                ++index;
            }
            else
            {
                in_quotes = !in_quotes;
            }
            continue;
        }

        if (ch == ',' && !in_quotes)
        {
            fields.push_back(current);
            current.clear();
            continue;
        }

        current.push_back(ch);
    }

    if (in_quotes)
    {
        throw std::runtime_error("Malformed CSV row with unclosed quote");
    }

    fields.push_back(current);
    return fields;
}

std::map<std::string, std::size_t> ParseHeader(std::ifstream& input, const std::filesystem::path& file_path)
{
    std::string header_line;
    if (!std::getline(input, header_line))
    {
        throw std::runtime_error("Missing CSV header in " + file_path.string());
    }

    const auto header_fields = ParseCsvRow(Trim(StripUtf8Bom(header_line)));
    std::map<std::string, std::size_t> header_index;
    for (std::size_t index = 0; index < header_fields.size(); ++index)
    {
        header_index[Trim(header_fields[index])] = index;
    }

    return header_index;
}

std::size_t RequireColumn(const std::map<std::string, std::size_t>& header_index,
                          const std::string& column_name,
                          const std::filesystem::path& file_path)
{
    const auto it = header_index.find(column_name);
    if (it == header_index.end())
    {
        throw std::runtime_error("Missing CSV column " + column_name + " in " + file_path.string());
    }

    return it->second;
}

std::string ReadField(const std::vector<std::string>& fields, std::size_t index)
{
    if (index >= fields.size())
    {
        return std::string();
    }

    return Trim(fields[index]);
}

double ParseDoubleField(const std::vector<std::string>& fields,
                        std::size_t index,
                        const std::string& field_name,
                        const std::filesystem::path& file_path)
{
    const auto value = ReadField(fields, index);
    if (value.empty())
    {
        throw std::runtime_error("Missing numeric field " + field_name + " in " + file_path.string());
    }

    return std::stod(value);
}

std::string NormalizeDateToken(const std::string& token)
{
    static const std::regex compact_date_pattern("(\\d{4})(\\d{2})(\\d{2})");
    static const std::regex dashed_date_pattern("\\d{4}-\\d{2}-\\d{2}");

    std::smatch match;
    if (std::regex_match(token, match, compact_date_pattern))
    {
        return match[1].str() + "-" + match[2].str() + "-" + match[3].str();
    }

    if (std::regex_match(token, dashed_date_pattern))
    {
        return token;
    }

    return std::string();
}

std::string ExtractDateFromPath(const std::filesystem::path& file_path)
{
    static const std::regex date_pattern("(\\d{8}|\\d{4}-\\d{2}-\\d{2})");
    const std::string file_name = file_path.filename().string();

    std::smatch match;
    if (!std::regex_search(file_name, match, date_pattern))
    {
        throw std::runtime_error("Could not extract date from file name: " + file_path.string());
    }

    const auto normalized = NormalizeDateToken(match[1].str());
    if (normalized.empty())
    {
        throw std::runtime_error("Unsupported date format in file name: " + file_path.string());
    }

    return normalized;
}

DatedFile FindLatestFile(const std::filesystem::path& directory)
{
    DatedFile latest = {};
    const bool debug_reference_selection = std::getenv("BASIS_MONITOR_DEBUG_REFERENCE_SELECTION") != nullptr;

    if (debug_reference_selection)
    {
        std::cerr << "[REFERENCE_SCAN] directory=" << directory.string() << '\n';
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".csv")
        {
            continue;
        }

        const auto date = ExtractDateFromPath(entry.path());
        if (debug_reference_selection)
        {
            std::cerr << "[REFERENCE_CANDIDATE] directory=" << directory.string()
                      << " path=" << entry.path().string()
                      << " date=" << date << '\n';
        }
        if (latest.path.empty() || date > latest.date)
        {
            latest.path = entry.path();
            latest.date = date;
            if (debug_reference_selection)
            {
                std::cerr << "[REFERENCE_LATEST] directory=" << directory.string()
                          << " path=" << latest.path.string()
                          << " date=" << latest.date << '\n';
            }
        }
    }

    if (latest.path.empty())
    {
        throw std::runtime_error("No CSV files found in " + directory.string());
    }

    return latest;
}

std::vector<FutureMetadataRecord> LoadFutureMetadata(const std::filesystem::path& file_path)
{
    std::ifstream input(file_path);
    if (!input.is_open())
    {
        throw std::runtime_error("Cannot open CSV file: " + file_path.string());
    }

    const auto header_index = ParseHeader(input, file_path);
    const auto order_book_id_index = RequireColumn(header_index, "order_book_id", file_path);
    const auto exchange_index = RequireColumn(header_index, "exchange", file_path);
    const auto underlying_symbol_index = RequireColumn(header_index, "underlying_symbol", file_path);
    const auto underlying_order_book_id_index = RequireColumn(header_index, "underlying_order_book_id", file_path);
    const auto product_index = RequireColumn(header_index, "product", file_path);
    const auto maturity_date_index = RequireColumn(header_index, "maturity_date", file_path);

    std::vector<FutureMetadataRecord> records;
    std::string line;
    while (std::getline(input, line))
    {
        line = Trim(line);
        if (line.empty())
        {
            continue;
        }

        const auto fields = ParseCsvRow(line);

        FutureMetadataRecord record = {};
        record.order_book_id = ReadField(fields, order_book_id_index);
        record.exchange = ReadField(fields, exchange_index);
        record.underlying_symbol = ReadField(fields, underlying_symbol_index);
        record.underlying_order_book_id = ReadField(fields, underlying_order_book_id_index);
        record.product = ReadField(fields, product_index);
        record.maturity_date = ReadField(fields, maturity_date_index);
        records.push_back(record);
    }

    return records;
}

std::vector<IndexMetadataRecord> LoadIndexMetadata(const std::filesystem::path& file_path)
{
    std::ifstream input(file_path);
    if (!input.is_open())
    {
        throw std::runtime_error("Cannot open CSV file: " + file_path.string());
    }

    const auto header_index = ParseHeader(input, file_path);
    const auto order_book_id_index = RequireColumn(header_index, "order_book_id", file_path);
    const auto symbol_index = RequireColumn(header_index, "symbol", file_path);

    std::vector<IndexMetadataRecord> records;
    std::string line;
    while (std::getline(input, line))
    {
        line = Trim(line);
        if (line.empty())
        {
            continue;
        }

        const auto fields = ParseCsvRow(line);

        IndexMetadataRecord record = {};
        record.order_book_id = ReadField(fields, order_book_id_index);
        record.symbol = ReadField(fields, symbol_index);
        records.push_back(record);
    }

    return records;
}

std::vector<FutureEodRecord> LoadFutureEod(const std::filesystem::path& file_path)
{
    std::ifstream input(file_path);
    if (!input.is_open())
    {
        throw std::runtime_error("Cannot open CSV file: " + file_path.string());
    }

    const auto header_index = ParseHeader(input, file_path);
    const auto trade_date_index = RequireColumn(header_index, "trade_date", file_path);
    const auto underlying_symbol_index = RequireColumn(header_index, "underlying_symbol", file_path);
    const auto order_book_id_index = RequireColumn(header_index, "order_book_id", file_path);
    const auto close_index = RequireColumn(header_index, "close", file_path);
    const auto total_turnover_index = RequireColumn(header_index, "total_turnover", file_path);

    std::vector<FutureEodRecord> records;
    std::string line;
    while (std::getline(input, line))
    {
        line = Trim(line);
        if (line.empty())
        {
            continue;
        }

        const auto fields = ParseCsvRow(line);

        FutureEodRecord record = {};
        record.trade_date = ReadField(fields, trade_date_index);
        record.underlying_symbol = ReadField(fields, underlying_symbol_index);
        record.order_book_id = ReadField(fields, order_book_id_index);
        record.close = ParseDoubleField(fields, close_index, "close", file_path);
        record.total_turnover = ParseDoubleField(fields, total_turnover_index, "total_turnover", file_path);
        records.push_back(record);
    }

    return records;
}

std::vector<IndexEodRecord> LoadIndexEod(const std::filesystem::path& file_path)
{
    std::ifstream input(file_path);
    if (!input.is_open())
    {
        throw std::runtime_error("Cannot open CSV file: " + file_path.string());
    }

    const auto header_index = ParseHeader(input, file_path);
    const auto trade_date_index = RequireColumn(header_index, "trade_date", file_path);
    const auto order_book_id_index = RequireColumn(header_index, "order_book_id", file_path);
    const auto close_index = RequireColumn(header_index, "close", file_path);

    std::vector<IndexEodRecord> records;
    std::string line;
    while (std::getline(input, line))
    {
        line = Trim(line);
        if (line.empty())
        {
            continue;
        }

        const auto fields = ParseCsvRow(line);

        IndexEodRecord record = {};
        record.trade_date = ReadField(fields, trade_date_index);
        record.order_book_id = ReadField(fields, order_book_id_index);
        record.close = ParseDoubleField(fields, close_index, "close", file_path);
        records.push_back(record);
    }

    return records;
}

} // namespace

ReferenceDataLoadResult LoadReferenceData(const ReferenceDataDirectories& directories)
{
    ReferenceDataLoadResult result = {};

    const auto future_metadata_file = FindLatestFile(directories.future_metadata_dir);
    const auto index_metadata_file = FindLatestFile(directories.index_metadata_dir);
    const auto future_eod_file = FindLatestFile(directories.future_eod_dir);
    const auto index_eod_file = FindLatestFile(directories.index_eod_dir);

    result.future_metadata_date = future_metadata_file.date;
    result.index_metadata_date = index_metadata_file.date;
    result.future_eod_date = future_eod_file.date;
    result.index_eod_date = index_eod_file.date;
    result.future_metadata_source_path = future_metadata_file.path.string();
    result.index_metadata_source_path = index_metadata_file.path.string();
    result.future_eod_source_path = future_eod_file.path.string();
    result.index_eod_source_path = index_eod_file.path.string();

    result.future_metadata = LoadFutureMetadata(future_metadata_file.path);
    result.index_metadata = LoadIndexMetadata(index_metadata_file.path);
    result.future_eod = LoadFutureEod(future_eod_file.path);
    result.index_eod = LoadIndexEod(index_eod_file.path);

    return result;
}

} // namespace basis_monitor
