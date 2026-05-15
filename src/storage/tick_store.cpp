#include "basis_monitor/storage/tick_store.h"

#include <filesystem>
#include <fstream>
#include <utility>

#include "storage_common.h"

namespace basis_monitor
{
TickStore::TickStore(std::filesystem::path file_path)
    : file_path_(std::move(file_path))
{
}

bool TickStore::Append(const MarketTick& tick)
{
    std::ofstream output;
    bool write_header = false;
    if (!storage::OpenCsvAppendFile(file_path_, output, write_header))
    {
        return false;
    }

    if (!storage::WriteCsvHeaderIfNeeded(output, write_header, "timestamp,instrument_id,update_time,update_millisec,last_price,bid_price_1,bid_volume_1,ask_price_1,ask_volume_1,volume"))
    {
        return false;
    }

    output << storage::CurrentTimestamp() << ','
           << storage::EscapeCsvField(tick.instrument_id) << ','
           << storage::EscapeCsvField(tick.update_time) << ','
           << tick.update_millisec << ','
           << tick.last_price << ','
           << tick.bid_price_1 << ','
           << tick.bid_volume_1 << ','
           << tick.ask_price_1 << ','
           << tick.ask_volume_1 << ','
           << tick.volume << '\n';

    return static_cast<bool>(output);
}

} // namespace basis_monitor
