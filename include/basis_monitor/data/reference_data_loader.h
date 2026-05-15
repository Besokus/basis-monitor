#pragma once

#include <filesystem>
#include <string>

#include "basis_monitor/data/reference_data_types.h"

namespace basis_monitor
{

struct ReferenceDataDirectories
{
    std::filesystem::path future_metadata_dir;
    std::filesystem::path index_metadata_dir;
    std::filesystem::path future_eod_dir;
    std::filesystem::path index_eod_dir;
};

ReferenceDataLoadResult LoadReferenceData(const ReferenceDataDirectories& directories);

} // namespace basis_monitor
