#include <cassert>
#include <vector>

#include "basis_monitor/domain/basis_result.h"
#include "basis_monitor/domain/monitored_contract.h"
#include "basis_monitor/report/latest_basis_snapshot_store.h"

namespace
{

basis_monitor::MonitoredContract MakeContract(const char* instrument_id,
                                              const char* product_group,
                                              const char* index_code,
                                              const char* index_name,
                                              double index_price,
                                              const char* maturity_date)
{
    basis_monitor::MonitoredContract contract = {};
    contract.instrument_id = instrument_id;
    contract.product_group = product_group;
    contract.index_code = index_code;
    contract.index_name = index_name;
    contract.index_close_yesterday = index_price;
    contract.maturity_date = maturity_date;
    return contract;
}

} // namespace

int main()
{
    std::vector<basis_monitor::MonitoredContract> contracts = {
        MakeContract("IC2606", "IC", "000905.XSHG", "CSI 500", 6123.45, "2026-06-19"),
        MakeContract("IF2606", "IF", "000300.XSHG", "CSI 300", 4808.12, "2026-06-19"),
    };

    basis_monitor::LatestBasisSnapshotStore store(contracts);
    const auto initial_snapshots = store.GetAll();
    assert(initial_snapshots.size() == 2);
    assert(!initial_snapshots[0].has_tick);
    assert(initial_snapshots[0].contract.instrument_id == "IC2606");
    assert(initial_snapshots[0].contract.product_group == "IC");
    assert(initial_snapshots[0].latest_annual_rate == 0.0);
    assert(initial_snapshots[0].latest_future_price == 0.0);
    assert(initial_snapshots[1].contract.instrument_id == "IF2606");
    assert(!initial_snapshots[1].has_tick);

    basis_monitor::BasisResult result = {};
    result.valid = true;
    result.basis = 123.45;
    result.annual_rate = 7.89;
    result.remaining_days = 80;

    store.Update(contracts[0], 6100.0, 6000.0, result, "2026-03-31 10:15:00.123");

    const auto updated_snapshots = store.GetAll();
    assert(updated_snapshots.size() == 2);
    assert(updated_snapshots[0].has_tick);
    assert(updated_snapshots[0].contract.instrument_id == "IC2606");
    assert(updated_snapshots[0].latest_index_price == 6100.0);
    assert(updated_snapshots[0].latest_future_price == 6000.0);
    assert(updated_snapshots[0].latest_basis == 123.45);
    assert(updated_snapshots[0].latest_annual_rate == 7.89);
    assert(updated_snapshots[0].remaining_days == 80);
    assert(updated_snapshots[0].snapshot_time == "2026-03-31 10:15:00.123");

    assert(!updated_snapshots[1].has_tick);
    assert(updated_snapshots[1].contract.instrument_id == "IF2606");
    assert(updated_snapshots[1].latest_future_price == 0.0);
    assert(updated_snapshots[1].latest_annual_rate == 0.0);

    const auto snapshot = store.Find("IC2606");
    assert(snapshot.has_value());
    assert(snapshot->has_tick);
    assert(snapshot->latest_basis == 123.45);

    const auto missing = store.Find("IH2606");
    assert(!missing.has_value());

    return 0;
}
