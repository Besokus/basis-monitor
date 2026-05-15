#pragma once

#include <string>
#include <vector>

namespace basis_monitor
{

struct FutureMetadataRecord
{
    std::string order_book_id;
    std::string exchange;
    std::string underlying_symbol;
    std::string underlying_order_book_id;
    std::string product;
    std::string maturity_date;
};

struct IndexMetadataRecord
{
    std::string order_book_id;
    std::string symbol;
};

struct FutureEodRecord
{
    std::string trade_date;
    std::string underlying_symbol;
    std::string order_book_id;
    double close = 0.0;
    double total_turnover = 0.0;
};

struct IndexEodRecord
{
    std::string trade_date;
    std::string order_book_id;
    double close = 0.0;
};

struct ReferenceDataLoadResult
{
    std::string future_metadata_date;
    std::string index_metadata_date;
    std::string future_eod_date;
    std::string index_eod_date;
    std::string future_metadata_source_path;
    std::string index_metadata_source_path;
    std::string future_eod_source_path;
    std::string index_eod_source_path;
    std::vector<FutureMetadataRecord> future_metadata;
    std::vector<IndexMetadataRecord> index_metadata;
    std::vector<FutureEodRecord> future_eod;
    std::vector<IndexEodRecord> index_eod;
};

} // namespace basis_monitor
