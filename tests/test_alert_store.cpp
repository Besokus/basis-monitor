#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

#include "basis_monitor/domain/alert_event.h"
#include "basis_monitor/storage/alert_store.h"

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
    const auto temp_dir = std::filesystem::temp_directory_path() / "basis_monitor_alert_store";
    TempDirCleanup cleanup{temp_dir};
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);

    const auto file_path = temp_dir / "alerts.csv";
    basis_monitor::AlertStore store(file_path);

    const basis_monitor::AlertEvent event = {
        "IC2606",
        "IC",
        "000905.XSHG",
        "CSI 500",
        8024.6930,
        7890.0,
        134.6930,
        -1.25,
        73,
        basis_monitor::AlertTransition::EnteredNegative,
    };

    assert(store.Append(event, "annual rate below zero"));
    assert(std::filesystem::exists(file_path));

    const auto contents = ReadFile(file_path);
    assert(contents.find("timestamp,contract,product_group,index_code,index_name,index_price,future_price,basis,annual_rate,remaining_days,transition,reason") != std::string::npos);
    assert(contents.find("IC2606") != std::string::npos);
    assert(contents.find("IC") != std::string::npos);
    assert(contents.find("000905.XSHG") != std::string::npos);
    assert(contents.find("CSI 500") != std::string::npos);
    assert(contents.find("8024.69") != std::string::npos);
    assert(contents.find("7890") != std::string::npos);
    assert(contents.find("134.693") != std::string::npos);
    assert(contents.find("-1.25") != std::string::npos);
    assert(contents.find("73") != std::string::npos);
    assert(contents.find("EnteredNegative") != std::string::npos);
    assert(contents.find("annual rate below zero") != std::string::npos);

    assert(store.Append(event, "annual rate below zero"));
    const auto second_contents = ReadFile(file_path);
    assert(second_contents.find("timestamp,contract,product_group,index_code,index_name,index_price,future_price,basis,annual_rate,remaining_days,transition,reason") ==
           second_contents.rfind("timestamp,contract,product_group,index_code,index_name,index_price,future_price,basis,annual_rate,remaining_days,transition,reason"));

    const auto escaped_file_path = temp_dir / "alerts_escaped.csv";
    basis_monitor::AlertStore escaped_store(escaped_file_path);
    assert(escaped_store.Append(event, "reason, with \"quotes\"\nand newline"));

    const auto escaped_contents = ReadFile(escaped_file_path);
    assert(escaped_contents.find("\"reason, with \"\"quotes\"\"\nand newline\"") != std::string::npos);
    assert(escaped_contents.find("reason, with \"quotes\"\nand newline") == std::string::npos);

    return 0;
}
