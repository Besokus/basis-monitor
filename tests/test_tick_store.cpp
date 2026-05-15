#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

#include "basis_monitor/domain/market_tick.h"
#include "basis_monitor/storage/tick_store.h"

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
    const auto temp_dir = std::filesystem::temp_directory_path() / "basis_monitor_tick_store";
    TempDirCleanup cleanup{temp_dir};
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);

    const auto file_path = temp_dir / "ticks.csv";
    basis_monitor::TickStore store(file_path);

    basis_monitor::MarketTick tick = {};
    tick.instrument_id = "IC2606";
    tick.update_time = "14:30:01";
    tick.update_millisec = 500;
    tick.last_price = 6080.5;
    tick.bid_price_1 = 6080.0;
    tick.bid_volume_1 = 12;
    tick.ask_price_1 = 6081.0;
    tick.ask_volume_1 = 15;
    tick.volume = 88;

    assert(store.Append(tick));
    assert(std::filesystem::exists(file_path));

    const auto contents = ReadFile(file_path);
    assert(contents.find("timestamp,instrument_id,update_time,update_millisec,last_price,bid_price_1,bid_volume_1,ask_price_1,ask_volume_1,volume") != std::string::npos);
    assert(contents.find("IC2606") != std::string::npos);
    assert(contents.find("6080.5") != std::string::npos);

    assert(store.Append(tick));
    const auto second_contents = ReadFile(file_path);
    assert(second_contents.find("timestamp,instrument_id,update_time,update_millisec,last_price,bid_price_1,bid_volume_1,ask_price_1,ask_volume_1,volume") == second_contents.rfind("timestamp,instrument_id,update_time,update_millisec,last_price,bid_price_1,bid_volume_1,ask_price_1,ask_volume_1,volume"));

    return 0;
}
