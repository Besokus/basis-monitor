#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "basis_monitor/domain/basis_result.h"
#include "basis_monitor/domain/monitored_contract.h"
#include "basis_monitor/report/latest_basis_snapshot.h"

namespace basis_monitor
{

class LatestBasisSnapshotStore
{
public:
    explicit LatestBasisSnapshotStore(const std::vector<MonitoredContract>& contracts);

    void Update(const MonitoredContract& contract,
                double index_price,
                double future_price,
                const BasisResult& result,
                const std::string& snapshot_time);

    std::optional<LatestBasisSnapshot> Find(const std::string& instrument_id) const;
    std::vector<LatestBasisSnapshot> GetAll() const;

private:
    mutable std::mutex mutex_;
    std::vector<LatestBasisSnapshot> snapshots_;
    std::unordered_map<std::string, std::size_t> snapshot_index_by_instrument_;
};

} // namespace basis_monitor
