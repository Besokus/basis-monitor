#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "basis_monitor/data/reference_data_loader.h"

namespace
{

struct TempDirCleanup
{
    std::filesystem::path path;

    ~TempDirCleanup()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
};

void WriteTextFile(const std::filesystem::path& path, const std::string& contents)
{
    std::ofstream output(path, std::ios::binary);
    assert(output.is_open());
    output << contents;
}

std::filesystem::path CreateReferenceDataFixtureTree()
{
    const auto root = std::filesystem::temp_directory_path() / "basis_monitor_reference_data_loader";
    std::filesystem::remove_all(root);

    std::filesystem::create_directories(root / "all_instruments" / "Future");
    std::filesystem::create_directories(root / "all_instruments" / "INDX");
    std::filesystem::create_directories(root / "eod_price" / "Future");
    std::filesystem::create_directories(root / "eod_price" / "INDX");

    WriteTextFile(root / "all_instruments" / "Future" / "all_instruments_20260327.csv",
                  "type,order_book_id,exchange,symbol,underlying_symbol,underlying_order_book_id,product,maturity_date\n"
                  "Future,IC2603,CFFEX,Old CSI 500 Index Futures 2603,IC,000905.XSHG,Index,2026-03-20\n");
    WriteTextFile(root / "all_instruments" / "Future" / "all_instruments_20260328.csv",
                  "type,order_book_id,exchange,symbol,underlying_symbol,underlying_order_book_id,product,maturity_date\n"
                  "Future,IC2604,CFFEX,CSI 500 Index Futures 2604,IC,000905.XSHG,Index,2026-04-17\n"
                  "Future,IF2604,CFFEX,CSI 300 Index Futures 2604,IF,000300.XSHG,Index,2026-04-17\n");

    WriteTextFile(root / "all_instruments" / "INDX" / "all_instruments_20260327.csv",
                  "order_book_id,symbol\n"
                  "000300.XSHG,Old CSI 300 Index\n");
    WriteTextFile(root / "all_instruments" / "INDX" / "all_instruments_20260328.csv",
                  "order_book_id,symbol\n"
                  "000016.XSHG,SSE 50 Index\n"
                  "000300.XSHG,CSI 300 Index\n"
                  "000905.XSHG,CSI 500 Index\n"
                  "000852.XSHG,CSI 1000 Index\n");

    WriteTextFile(root / "eod_price" / "Future" / "eod_future_20260326.csv",
                  "trade_date,underlying_symbol,order_book_id,open,close,high,low,limit_up,limit_down,total_turnover\n"
                  "2026-03-26,IC,IC2604,6150,6160,6175,6140,6650,5650,111000000.0\n");
    WriteTextFile(root / "eod_price" / "Future" / "eod_future_20260327.csv",
                  "trade_date,underlying_symbol,order_book_id,open,close,high,low,limit_up,limit_down,total_turnover\n"
                  "2026-03-27,IC,IC2604,6200,6210,6220,6190,6700,5700,125000000.5\n"
                  "2026-03-27,IF,IF2604,4800,4810,4820,4790,5300,4300,225000000.0\n");
    WriteTextFile(root / "eod_price" / "Future" / "eod_future_20260330.csv",
                  "trade_date,underlying_symbol,order_book_id,open,close,high,low,limit_up,limit_down,total_turnover\n"
                  "2026-03-30,IC,IC2604,6300,6310,6325,6280,6800,5800,999999999.0\n");

    WriteTextFile(root / "eod_price" / "INDX" / "eod_indx_20260326.csv",
                  "trade_date,order_book_id,open,close,high,low,total_turnover,volume,prev_close\n"
                  "2026-03-26,000300.XSHG,4780,4788.66,4795,4770,310000000000.0,200000000,4775.0\n");
    WriteTextFile(root / "eod_price" / "INDX" / "eod_indx_20260327.csv",
                  "trade_date,order_book_id,open,close,high,low,total_turnover,volume,prev_close\n"
                  "2026-03-27,000300.XSHG,4790,4808.12,4815,4780,320000000000.0,210000000,4788.66\n"
                  "2026-03-27,000905.XSHG,6100,6123.45,6130,6090,280000000000.0,180000000,6098.76\n");
    WriteTextFile(root / "eod_price" / "INDX" / "eod_indx_20260330.csv",
                  "trade_date,order_book_id,open,close,high,low,total_turnover,volume,prev_close\n"
                  "2026-03-30,000300.XSHG,4820,5000.0,5010,4810,400000000000.0,220000000,4808.12\n");

    return root;
}

} // namespace

int main()
{
    const auto data_root = CreateReferenceDataFixtureTree();
    TempDirCleanup cleanup{data_root};

    const basis_monitor::ReferenceDataDirectories directories = {
        data_root / "all_instruments" / "Future",
        data_root / "all_instruments" / "INDX",
        data_root / "eod_price" / "Future",
        data_root / "eod_price" / "INDX"};
    const auto reference_data = basis_monitor::LoadReferenceData(directories);

    assert(reference_data.future_metadata_date == "2026-03-28");
    assert(reference_data.index_metadata_date == "2026-03-28");
    assert(reference_data.future_eod_date == "2026-03-30");
    assert(reference_data.index_eod_date == "2026-03-30");

    assert(reference_data.future_metadata.size() == 2);
    assert(reference_data.future_metadata[0].order_book_id == "IC2604");
    assert(reference_data.future_metadata[0].exchange == "CFFEX");
    assert(reference_data.future_metadata[0].product == "Index");
    assert(reference_data.future_metadata[0].underlying_order_book_id == "000905.XSHG");
    assert(reference_data.future_metadata[0].maturity_date == "2026-04-17");

    assert(reference_data.index_metadata.size() == 4);
    assert(reference_data.index_metadata[1].order_book_id == "000300.XSHG");
    assert(reference_data.index_metadata[1].symbol == "CSI 300 Index");

    assert(reference_data.future_eod.size() == 1);
    assert(reference_data.future_eod[0].trade_date == "2026-03-30");
    assert(reference_data.future_eod[0].order_book_id == "IC2604");
    assert(std::abs(reference_data.future_eod[0].total_turnover - 999999999.0) < 0.0001);

    assert(reference_data.index_eod.size() == 1);
    assert(reference_data.index_eod[0].trade_date == "2026-03-30");
    assert(reference_data.index_eod[0].order_book_id == "000300.XSHG");
    assert(std::abs(reference_data.index_eod[0].close - 5000.0) < 0.0001);

    for (const auto& record : reference_data.future_eod)
    {
        assert(record.trade_date == "2026-03-30");
        assert(record.total_turnover >= 900000000.0);
    }

    for (const auto& record : reference_data.index_eod)
    {
        assert(record.trade_date == "2026-03-30");
        if (record.order_book_id == "000300.XSHG")
        {
            assert(std::abs(record.close - 5000.0) < 0.0001);
        }
    }

    return 0;
}
