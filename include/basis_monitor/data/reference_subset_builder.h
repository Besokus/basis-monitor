#pragma once

#include <filesystem>

#include "basis_monitor/data/contract_selector.h"
#include "basis_monitor/data/reference_data_types.h"

namespace basis_monitor
{

ReferenceDataLoadResult BuildReferenceDataSubset(const ReferenceDataLoadResult& reference_data,
                                                 const ContractSelectionResult& selection);

void WriteReferenceDataSubset(const ReferenceDataLoadResult& reference_data,
                              const std::filesystem::path& output_root);

} // namespace basis_monitor
