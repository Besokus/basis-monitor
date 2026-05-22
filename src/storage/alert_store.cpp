#include "basis_monitor/storage/alert_store.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

#include "storage_common.h"

namespace basis_monitor
{
namespace
{

std::string TransitionToText(AlertTransition transition)
{
    switch (transition)
    {
    case AlertTransition::EnteredNegative:
        return "EnteredNegative";
    case AlertTransition::RepeatedNegative:
        return "RepeatedNegative";
    case AlertTransition::Recovered:
        return "Recovered";
    case AlertTransition::None:
    default:
        return "None";
    }
}

} // namespace

AlertStore::AlertStore(std::filesystem::path file_path)
    : file_path_(std::move(file_path))
{
}

bool AlertStore::Append(const AlertEvent& event, const std::string& reason_text)
{
    std::ofstream output;
    bool write_header = false;
    if (!storage::OpenCsvAppendFile(file_path_, output, write_header))
    {
        return false;
    }

    if (!storage::WriteCsvHeaderIfNeeded(output, write_header, "timestamp,contract,product_group,index_code,index_name,index_price,future_price,basis,annual_rate,remaining_days,transition,reason"))
    {
        return false;
    }

    output << storage::CurrentTimestamp() << ','
           << storage::EscapeCsvField(event.instrument_id) << ','
           << storage::EscapeCsvField(event.product_group) << ','
           << storage::EscapeCsvField(event.index_code) << ','
           << storage::EscapeCsvField(event.index_name) << ','
           << event.index_price << ','
           << event.future_price << ','
           << event.basis << ','
           << event.annual_rate << ','
           << event.remaining_days << ','
           << storage::EscapeCsvField(TransitionToText(event.transition)) << ','
           << storage::EscapeCsvField(reason_text) << '\n';

    return static_cast<bool>(output);
}

} // namespace basis_monitor
