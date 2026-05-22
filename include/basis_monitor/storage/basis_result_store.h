#pragma once

#include <filesystem>

#include "basis_monitor/domain/basis_result.h"
#include "basis_monitor/domain/monitored_contract.h"

namespace basis_monitor
{

class BasisResultStore
{
public:
    explicit BasisResultStore(std::filesystem::path file_path);

    bool Append(const MonitoredContract& contract, double index_price, double future_price, const BasisResult& result);

private:
    std::filesystem::path file_path_;
};

} // namespace basis_monitor
