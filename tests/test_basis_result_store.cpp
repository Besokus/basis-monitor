#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

#include "basis_monitor/domain/basis_result.h"
#include "basis_monitor/domain/monitored_contract.h"
#include "basis_monitor/storage/basis_result_store.h"

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

std::string ReadFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    assert(input.is_open());
    return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

} // namespace

int main()
{
    const auto temp_dir = std::filesystem::temp_directory_path() / "basis_monitor_basis_result_store";
    TempDirCleanup cleanup{temp_dir};
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);

    const auto file_path = temp_dir / "basis_results.csv";
    basis_monitor::BasisResultStore store(file_path);
    basis_monitor::MonitoredContract contract = {};
    contract.instrument_id = "IC2606";
    contract.product_group = "IC";
    contract.report_group = "zz500";
    contract.index_code = "000905.XSHG";
    contract.index_name = "CSI 500";
    contract.index_close_yesterday = 6300.0;
    contract.future_close_yesterday = 6250.0;
    contract.maturity_date = "2026-06-19";

    const basis_monitor::BasisResult result = {
        true,
        110.0,
        27.71,
        23,
    };

    assert(store.Append(contract, 6201.5, 6190.0, result));
    assert(std::filesystem::exists(file_path));

    const auto contents = ReadFile(file_path);
    assert(contents.find("timestamp,contract,product_group,report_group,index_code,index_name,index_price,future_close_yesterday,future_price,basis,annual_rate,remaining_days,negative_flag") != std::string::npos);
    assert(contents.find("IC2606") != std::string::npos);
    assert(contents.find("IC") != std::string::npos);
    assert(contents.find("zz500") != std::string::npos);
    assert(contents.find("000905.XSHG") != std::string::npos);
    assert(contents.find("CSI 500") != std::string::npos);
    assert(contents.find("6201.5") != std::string::npos);
    assert(contents.find("6250") != std::string::npos);
    assert(contents.find("6190") != std::string::npos);
    assert(contents.find("110") != std::string::npos);
    assert(contents.find("27.71") != std::string::npos);
    assert(contents.find("23") != std::string::npos);
    assert(contents.find("false") != std::string::npos);

    assert(store.Append(contract, 6210.5, 6400.0, basis_monitor::BasisResult{true, -100.0, -5.81, 23}));
    const auto second_contents = ReadFile(file_path);
    assert(second_contents.find("timestamp,contract,product_group,report_group,index_code,index_name,index_price,future_close_yesterday,future_price,basis,annual_rate,remaining_days,negative_flag") ==
           second_contents.rfind("timestamp,contract,product_group,report_group,index_code,index_name,index_price,future_close_yesterday,future_price,basis,annual_rate,remaining_days,negative_flag"));
    assert(second_contents.find("true") != std::string::npos);

    return 0;
}
