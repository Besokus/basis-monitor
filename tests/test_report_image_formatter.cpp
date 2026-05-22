#include <cassert>
#include <string>
#include <vector>

#include "basis_monitor/report/latest_basis_snapshot.h"
#include "basis_monitor/report/report_image_formatter.h"

namespace
{

basis_monitor::LatestBasisSnapshot MakeSnapshot(const char* instrument_id,
                                                const char* product_group,
                                                const char* report_group,
                                                const char* maturity_date,
                                                double previous_close)
{
    basis_monitor::LatestBasisSnapshot snapshot = {};
    snapshot.contract.instrument_id = instrument_id;
    snapshot.contract.product_group = product_group;
    snapshot.contract.report_group = report_group;
    snapshot.contract.maturity_date = maturity_date;
    snapshot.contract.future_close_yesterday = previous_close;
    return snapshot;
}

} // namespace

int main()
{
    std::vector<basis_monitor::LatestBasisSnapshot> snapshots;

    auto if_near = MakeSnapshot("IF2604", "IF", "hs300", "2026-04-17", 4440.0);
    if_near.has_tick = true;
    if_near.latest_future_price = 4433.8;
    if_near.latest_basis = 16.25;
    if_near.latest_annual_rate = 7.84;
    if_near.remaining_days = 16;
    snapshots.push_back(if_near);

    auto if_no_tick = MakeSnapshot("IF2605", "IF", "hs300", "2026-05-15", 4400.0);
    snapshots.push_back(if_no_tick);

    auto ic_mid = MakeSnapshot("IC2605", "IC", "zz500", "2026-05-15", 7520.0);
    ic_mid.has_tick = true;
    ic_mid.latest_future_price = 7500.0;
    ic_mid.latest_basis = 90.93;
    ic_mid.latest_annual_rate = 9.68;
    ic_mid.remaining_days = 44;
    snapshots.push_back(ic_mid);

    auto im_warn = MakeSnapshot("IM2606", "IM", "zz1000", "2026-06-19", 7390.0);
    im_warn.has_tick = true;
    im_warn.latest_future_price = 7379.4;
    im_warn.latest_basis = 240.45;
    im_warn.latest_annual_rate = -0.80;
    im_warn.remaining_days = 82;
    snapshots.push_back(im_warn);

    basis_monitor::BasisReportMetadata metadata = {};
    metadata.report_generated_at = "2026-04-01 11:30:00";
    metadata.data_as_of = "2026-04-01 11:29:42";
    metadata.stale = true;

    const auto document = basis_monitor::BuildBasisReportImageDocument(
        snapshots,
        basis_monitor::BasisReportMoment::Midday1130,
        0.0,
        "2026-04-01",
        metadata);

    assert(document.title == "BASIS MONITOR 11:30 REPORT");
    assert(document.subtitle == "DATE 2026-04-01 | STAT 2026-04-01 11:30:00 | DATA 2026-04-01 11:29:42 | STATUS STALE");
    assert(document.columns.size() == 8);
    assert(document.columns[0] == "CONTRACT");
    assert(document.columns[7] == "WARNING");

    assert(document.groups.size() == 3);
    assert(document.groups[0].name == "hs300");
    assert(document.groups[1].name == "zz500");
    assert(document.groups[2].name == "zz1000");

    assert(document.groups[0].rows.size() == 2);
    assert(document.groups[0].rows[0].instrument_id == "IF2604");
    assert(document.groups[0].rows[0].remaining_days == 16);
    assert(document.groups[0].rows[1].instrument_id == "IF2605");
    assert(document.groups[0].rows[1].remaining_days == 44);

    assert(document.groups[0].rows[0].price_text == "4433.8");
    assert(document.groups[0].rows[0].change_text == "-6.2");
    assert(document.groups[0].rows[0].change_percent_text == "-0.14%");
    assert(document.groups[0].rows[0].basis_text == "16.25");
    assert(document.groups[0].rows[0].annual_rate_text == "7.84%");
    assert(document.groups[0].rows[0].warning_text == "-");
    assert(!document.groups[0].rows[0].warning_negative);

    assert(document.groups[0].rows[1].warning_text == "-");
    assert(!document.groups[0].rows[1].warning_negative);

    assert(document.groups[1].rows.size() == 1);
    assert(document.groups[1].rows[0].instrument_id == "IC2605");
    assert(document.groups[1].rows[0].remaining_days == 44);
    assert(document.groups[1].rows[0].price_text == "7500.0");
    assert(document.groups[1].rows[0].change_text == "-20.0");
    assert(document.groups[1].rows[0].warning_text == "-");

    assert(document.groups[2].rows.size() == 1);
    assert(document.groups[2].rows[0].instrument_id == "IM2606");
    assert(document.groups[2].rows[0].warning_text == "-0.80%");
    assert(document.groups[2].rows[0].warning_negative);

    const auto json = basis_monitor::SerializeBasisReportImageDocument(document);
    assert(json.find("\"title\":\"BASIS MONITOR 11:30 REPORT\"") != std::string::npos);
    assert(json.find("\"subtitle\":\"DATE 2026-04-01 | STAT 2026-04-01 11:30:00 | DATA 2026-04-01 11:29:42 | STATUS STALE\"") != std::string::npos);
    assert(json.find("\"name\":\"hs300\"") != std::string::npos);
    assert(json.find("\"name\":\"zz500\"") != std::string::npos);
    assert(json.find("\"name\":\"zz1000\"") != std::string::npos);
    assert(json.find("\"instrument_id\":\"IF2604\"") != std::string::npos);
    assert(json.find("\"warning_negative\":true") != std::string::npos);

    return 0;
}
