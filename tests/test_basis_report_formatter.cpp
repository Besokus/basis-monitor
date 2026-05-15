#include <cassert>
#include <string>
#include <vector>

#include "basis_monitor/report/basis_report_formatter.h"
#include "basis_monitor/report/latest_basis_snapshot.h"

namespace
{

basis_monitor::LatestBasisSnapshot MakeSnapshot(const char* instrument_id,
                                                const char* product_group,
                                                const char* report_group,
                                                const char* index_name)
{
    basis_monitor::LatestBasisSnapshot snapshot = {};
    snapshot.contract.instrument_id = instrument_id;
    snapshot.contract.product_group = product_group;
    snapshot.contract.report_group = report_group;
    snapshot.contract.index_name = index_name;
    return snapshot;
}

} // namespace

int main()
{
    std::vector<basis_monitor::LatestBasisSnapshot> snapshots;

    auto ic = MakeSnapshot("IC2606", "IC", "zz500", "CSI 500");
    ic.has_tick = true;
    ic.latest_index_price = 6123.45;
    ic.latest_future_price = 6000.0;
    ic.latest_basis = 123.45;
    ic.remaining_days = 80;
    ic.latest_annual_rate = 7.89;
    snapshots.push_back(ic);

    auto ic_earlier = MakeSnapshot("IC2604", "IC", "zz500", "CSI 500");
    ic_earlier.has_tick = true;
    ic_earlier.latest_index_price = 6100.00;
    ic_earlier.latest_future_price = 5980.0;
    ic_earlier.latest_basis = 120.00;
    ic_earlier.remaining_days = 20;
    ic_earlier.latest_annual_rate = 9.00;
    snapshots.push_back(ic_earlier);

    auto if_snapshot = MakeSnapshot("IF2606", "IF", "hs300", "CSI 300");
    snapshots.push_back(if_snapshot);

    auto im_snapshot = MakeSnapshot("IM2606", "IM", "zz1000", "CSI 1000");
    snapshots.push_back(im_snapshot);

    basis_monitor::BasisReportMetadata metadata = {};
    metadata.report_generated_at = "2026-04-01 11:30:00";
    metadata.data_as_of = "2026-04-01 11:29:42";
    metadata.stale = true;

    const auto report_1130 = basis_monitor::FormatBasisReport(
        snapshots, basis_monitor::BasisReportMoment::Midday1130, metadata);
    assert(report_1130.find("[Basis Monitor]") != std::string::npos);
    assert(report_1130.find("11:30") != std::string::npos);
    assert(report_1130.find("report_generated_at=2026-04-01 11:30:00") != std::string::npos);
    assert(report_1130.find("data_as_of=2026-04-01 11:29:42") != std::string::npos);
    assert(report_1130.find("market_data_status=STALE") != std::string::npos);
    assert(report_1130.find("[GROUP] hs300") != std::string::npos);
    assert(report_1130.find("[GROUP] zz500") != std::string::npos);
    assert(report_1130.find("[GROUP] zz1000") != std::string::npos);
    assert(report_1130.find("[GROUP] IH") == std::string::npos);
    assert(report_1130.find(
               "IC2606 | index=CSI 500 | index_price=6123.4500 | future=6000.0000 | basis=123.4500 | remaining_days=80 | annual_rate=7.8900%")
           != std::string::npos);
    assert(report_1130.find("IF2606 | index=CSI 300") != std::string::npos);
    assert(report_1130.find("IM2606 | index=CSI 1000") != std::string::npos);
    const auto ic2604_pos = report_1130.find("IC2604 | index=CSI 500");
    const auto ic2606_pos = report_1130.find("IC2606 | index=CSI 500");
    assert(ic2604_pos != std::string::npos);
    assert(ic2606_pos != std::string::npos);
    assert(ic2604_pos < ic2606_pos);

    const auto report_1500 = basis_monitor::FormatBasisReport(
        snapshots, basis_monitor::BasisReportMoment::Close1500);
    assert(report_1500.find("[Basis Monitor]") != std::string::npos);
    assert(report_1500.find("15:00") != std::string::npos);
    assert(report_1500.find("data_as_of=N/A") != std::string::npos);
    assert(report_1500.find("market_data_status=OK") != std::string::npos);

    return 0;
}
