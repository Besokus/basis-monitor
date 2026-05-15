#include <cassert>
#include <string>
#include <vector>

#include "basis_monitor/monitor/basis_monitor_service.h"
#include "basis_monitor/notify/wecom_message_formatter.h"
#include "basis_monitor/report/latest_basis_snapshot.h"

namespace
{

basis_monitor::LatestBasisSnapshot MakeSnapshot(const char* instrument_id,
                                                const char* product_group,
                                                const char* index_name)
{
    basis_monitor::LatestBasisSnapshot snapshot = {};
    snapshot.contract.instrument_id = instrument_id;
    snapshot.contract.product_group = product_group;
    snapshot.contract.index_name = index_name;
    return snapshot;
}

} // namespace

int main()
{
    basis_monitor::MonitorUpdate alert_update = {};
    alert_update.contract.product_group = "IC";
    alert_update.contract.instrument_id = "IC2606";
    alert_update.contract.index_name = "CSI500";
    alert_update.index_price = 7753.72;
    alert_update.future_price = 7788.40;
    alert_update.result.basis = -34.68;
    alert_update.result.remaining_days = 83;
    alert_update.result.annual_rate = -1.97;

    const auto alert_markdown = basis_monitor::FormatWeComAlertMarkdown(alert_update);
    assert(alert_markdown.find("IC2606") != std::string::npos);
    assert(alert_markdown.find("-1.9700%") != std::string::npos);
    assert(alert_markdown.find("-34.6800") != std::string::npos);
    assert(alert_markdown.find("7753.7200") != std::string::npos);
    assert(alert_markdown.find("7788.4000") != std::string::npos);
    assert(alert_markdown.find("83") != std::string::npos);

    std::vector<basis_monitor::LatestBasisSnapshot> snapshots;

    auto ic = MakeSnapshot("IC2606", "IC", "CSI500");
    ic.has_tick = true;
    ic.latest_index_price = 7753.72;
    ic.latest_future_price = 7426.40;
    ic.latest_basis = 327.32;
    ic.remaining_days = 83;
    ic.latest_annual_rate = 18.56;
    snapshots.push_back(ic);

    auto ic_earlier = MakeSnapshot("IC2604", "IC", "CSI500");
    ic_earlier.has_tick = true;
    ic_earlier.latest_index_price = 7740.00;
    ic_earlier.latest_future_price = 7400.00;
    ic_earlier.latest_basis = 340.00;
    ic_earlier.remaining_days = 20;
    ic_earlier.latest_annual_rate = 80.00;
    snapshots.push_back(ic_earlier);

    auto im = MakeSnapshot("IM2606", "IM", "CSI1000");
    im.has_tick = true;
    im.latest_index_price = 8123.11;
    im.latest_future_price = 8150.55;
    im.latest_basis = -27.44;
    im.remaining_days = 83;
    im.latest_annual_rate = -1.48;
    snapshots.push_back(im);

    auto if_snapshot = MakeSnapshot("IF2606", "IF", "HS300");
    snapshots.push_back(if_snapshot);

    const auto report_markdown = basis_monitor::FormatWeComBasisReportMarkdown(
        snapshots, basis_monitor::BasisReportMoment::Midday1130);
    const auto if_group_pos = report_markdown.find("**IF**");
    const auto ic_group_pos = report_markdown.find("**IC**");
    const auto im_group_pos = report_markdown.find("**IM**");
    assert(if_group_pos != std::string::npos);
    assert(ic_group_pos != std::string::npos);
    assert(im_group_pos != std::string::npos);
    assert(if_group_pos < ic_group_pos);
    assert(ic_group_pos < im_group_pos);

    assert(report_markdown.find("IF2606") != std::string::npos);
    assert(report_markdown.find("IC2604") != std::string::npos);
    assert(report_markdown.find("IC2606") != std::string::npos);
    assert(report_markdown.find("IM2606") != std::string::npos);
    assert(report_markdown.find("<font color=\"comment\">") != std::string::npos);

    const auto ic2604_pos = report_markdown.find("IC2604", ic_group_pos);
    const auto ic2606_pos = report_markdown.find("IC2606", ic_group_pos);
    assert(ic2604_pos != std::string::npos);
    assert(ic2606_pos != std::string::npos);
    assert(ic2604_pos < ic2606_pos);

    return 0;
}
