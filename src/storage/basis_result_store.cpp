#include "basis_monitor/storage/basis_result_store.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

#include "storage_common.h"

namespace basis_monitor
{
BasisResultStore::BasisResultStore(std::filesystem::path file_path)
    : file_path_(std::move(file_path))
{
}

bool BasisResultStore::Append(const MonitoredContract& contract,
                              double index_price,
                              double future_price,
                              const BasisResult& result)
{
    std::ofstream output;
    bool write_header = false;
    if (!storage::OpenCsvAppendFile(file_path_, output, write_header))
    {
        return false;
    }

    if (!storage::WriteCsvHeaderIfNeeded(output, write_header, "timestamp,contract,product_group,report_group,index_code,index_name,index_price,future_close_yesterday,future_price,basis,annual_rate,remaining_days,negative_flag"))
    {
        return false;
    }

    output << storage::CurrentTimestamp() << ','
           << storage::EscapeCsvField(contract.instrument_id) << ','
           << storage::EscapeCsvField(contract.product_group) << ','
           << storage::EscapeCsvField(contract.report_group) << ','
           << storage::EscapeCsvField(contract.index_code) << ','
           << storage::EscapeCsvField(contract.index_name) << ','
           << index_price << ','
           << contract.future_close_yesterday << ','
           << future_price << ','
           << result.basis << ','
           << result.annual_rate << ','
           << result.remaining_days << ','
           << (result.annual_rate < 0.0 ? "true" : "false") << '\n';

    return static_cast<bool>(output);
}

} // namespace basis_monitor
