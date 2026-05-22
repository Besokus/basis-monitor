#include "basis_monitor/report/latest_basis_snapshot_store.h"

namespace basis_monitor
{

LatestBasisSnapshotStore::LatestBasisSnapshotStore(const std::vector<MonitoredContract>& contracts)
{
    snapshots_.reserve(contracts.size());
    for (const auto& contract : contracts)
    {
        LatestBasisSnapshot snapshot = {};
        snapshot.contract = contract;
        snapshot.latest_index_price = contract.index_close_yesterday;
        snapshot_index_by_instrument_[contract.instrument_id] = snapshots_.size();
        snapshots_.push_back(snapshot);
    }
}

void LatestBasisSnapshotStore::Update(const MonitoredContract& contract,
                                      double index_price,
                                      double future_price,
                                      const BasisResult& result,
                                      const std::string& snapshot_time)
{
    const std::lock_guard<std::mutex> lock(mutex_);
    const auto it = snapshot_index_by_instrument_.find(contract.instrument_id);
    if (it == snapshot_index_by_instrument_.end())
    {
        return;
    }

    auto& snapshot = snapshots_[it->second];
    snapshot.contract = contract;
    snapshot.has_tick = true;
    snapshot.latest_index_price = index_price;
    snapshot.latest_future_price = future_price;
    snapshot.latest_basis = result.basis;
    snapshot.latest_annual_rate = result.annual_rate;
    snapshot.remaining_days = result.remaining_days;
    snapshot.snapshot_time = snapshot_time;
}

std::optional<LatestBasisSnapshot> LatestBasisSnapshotStore::Find(const std::string& instrument_id) const
{
    const std::lock_guard<std::mutex> lock(mutex_);
    const auto it = snapshot_index_by_instrument_.find(instrument_id);
    if (it == snapshot_index_by_instrument_.end())
    {
        return std::nullopt;
    }

    return snapshots_[it->second];
}

std::vector<LatestBasisSnapshot> LatestBasisSnapshotStore::GetAll() const
{
    const std::lock_guard<std::mutex> lock(mutex_);
    return snapshots_;
}

} // namespace basis_monitor
