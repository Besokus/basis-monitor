#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>

#include "basis_monitor/config/config_loader.h"
#include "basis_monitor/data/subscription_instrument_builder.h"

namespace
{

struct TempDirCleanup
{
    std::filesystem::path path;

    ~TempDirCleanup()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
};

void CopyFile(const std::filesystem::path& from, const std::filesystem::path& to)
{
    std::ifstream input(from, std::ios::binary);
    assert(input.is_open());

    std::ofstream output(to, std::ios::binary);
    assert(output.is_open());

    output << input.rdbuf();
}

void WriteTextFile(const std::filesystem::path& path, const std::string& contents)
{
    std::ofstream output(path, std::ios::binary);
    assert(output.is_open());
    output << contents;
}

std::filesystem::path FixtureConfigPath(const std::string& file_name)
{
    return std::filesystem::path(__FILE__).parent_path() / "fixtures" / "config" / file_name;
}

} // namespace

int main()
{
    const auto config = basis_monitor::LoadAppConfig((std::filesystem::path(__FILE__).parent_path() / "fixtures" / "config").string());

    assert(config.market_data_provider == basis_monitor::MarketDataProviderType::Ctp);
    assert(config.ctp.front_md_addr == "tcp://119.188.3.11:16113");
    assert(config.ctp.broker_id == "7080");
    assert(config.ctp.user_id == "21300292");
    assert(config.ctp.password == "aa123456");
    assert(config.ctp.auth_code == "auth-code-demo");
    assert(config.ctp.app_id == "simnow_client_test");
    assert(config.ctp.user_product_info == "basis_monitor");
    assert(config.ctp.instruments.size() == 2);
    assert(config.ctp.instruments[0] == "IC2606");
    assert(config.ctp.flow_dir == "runtime/flow/");
    assert(config.ctp.first_tick_timeout_ms == 15000);
    assert(config.ctp.reference_future_metadata_dir == "/srv/marketdata/all_instruments/Future");
    assert(config.ctp.reference_index_metadata_dir == "/srv/marketdata/all_instruments/INDX");
    assert(config.ctp.reference_future_eod_dir == "/srv/marketdata/eod_price/Future");
    assert(config.ctp.reference_index_eod_dir == "/srv/marketdata/eod_price/INDX");
    assert(config.ctp.enable_wecom_alert);
    assert(!config.ctp.enable_wecom_report);
    assert(!config.ctp.generate_local_report_image);
    assert(config.ctp.wecom_robot_webhook == "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=test-key");
    assert(config.ctp.market_data_stale_threshold_sec == 45);
    assert(config.ctp.market_data_recovery_grace_sec == 8);
    assert(config.xtp.enable_xtp_market_data);
    assert(config.xtp.server_ip == "10.10.10.10");
    assert(config.xtp.server_port == 6001);
    assert(config.xtp.user == "xtp_user");
    assert(config.xtp.password == "xtp_pass");
    assert(config.xtp.client_id == 7);
    assert(config.xtp.protocol == "udp");
    assert(config.xtp.exchange_id == "unknown");
    assert(config.xtp.local_ip == "10.10.10.20");
    assert(config.xtp.config_file == "/etc/xtp/quote_config.ini");
    assert(config.xtp.first_tick_timeout_ms == 12000);
    assert(config.xtp.index_instruments.size() == 3);
    assert(config.xtp.index_instruments[0] == "000300");
    assert(config.xtp.index_instruments[1] == "000905");
    assert(config.xtp.index_instruments[2] == "000852");
    assert(config.xtp.reference_future_metadata_dir == "/srv/xtp/all_instruments/Future");
    assert(config.xtp.reference_index_metadata_dir == "/srv/xtp/all_instruments/INDX");
    assert(config.xtp.reference_future_eod_dir == "/srv/xtp/eod_price/Future");
    assert(config.xtp.reference_index_eod_dir == "/srv/xtp/eod_price/INDX");

    assert(config.alert.terminal_alert);
    assert(!config.alert.file_alert);
    assert(std::abs(config.alert.negative_threshold - (-0.25)) < 0.0001);
    assert(config.alert.repeat_interval_minutes == 20);

    assert(config.contracts.size() == 2);
    assert(config.contracts[0].instrument_id == "IC2606");
    assert(config.contracts[0].expiry_date == "2026-06-19");
    assert(config.contracts[0].enabled);
    assert(!config.contracts[1].enabled);

    assert(config.spot.index_symbol == "CSI500");
    assert(std::abs(config.spot.current_spot_price - 6300.0) < 0.0001);
    assert(config.spot.update_timestamp == "2026-03-27T14:30:00+08:00");

    const auto missing_contracts_dir = std::filesystem::temp_directory_path() / "basis_monitor_config_loader_missing_contracts";
    TempDirCleanup missing_contracts_cleanup{missing_contracts_dir};
    std::filesystem::remove_all(missing_contracts_dir);
    std::filesystem::create_directories(missing_contracts_dir);

    CopyFile(FixtureConfigPath("ctp.ini"), missing_contracts_dir / "ctp.ini");
    CopyFile(FixtureConfigPath("spot_price.json"), missing_contracts_dir / "spot_price.json");
    WriteTextFile(missing_contracts_dir / "alert.json", "{\n  \"terminal_alert\": true,\n  \"file_alert\": false\n}\n");

    const auto missing_contracts_config = basis_monitor::LoadAppConfig(missing_contracts_dir.string());
    assert(missing_contracts_config.ctp.front_md_addr == "tcp://119.188.3.11:16113");
    assert(missing_contracts_config.alert.terminal_alert);
    assert(!missing_contracts_config.alert.file_alert);
    assert(std::abs(missing_contracts_config.alert.negative_threshold - 0.0) < 0.0001);
    assert(missing_contracts_config.alert.repeat_interval_minutes == 20);
    assert(missing_contracts_config.contracts.empty());
    assert(missing_contracts_config.spot.index_symbol == "CSI500");
    assert(std::abs(missing_contracts_config.spot.current_spot_price - 6300.0) < 0.0001);
    assert(missing_contracts_config.spot.update_timestamp == "2026-03-27T14:30:00+08:00");
    assert(missing_contracts_config.ctp.reference_future_metadata_dir == "/srv/marketdata/all_instruments/Future");
    assert(missing_contracts_config.ctp.reference_index_metadata_dir == "/srv/marketdata/all_instruments/INDX");
    assert(missing_contracts_config.ctp.reference_future_eod_dir == "/srv/marketdata/eod_price/Future");
    assert(missing_contracts_config.ctp.reference_index_eod_dir == "/srv/marketdata/eod_price/INDX");
    assert(missing_contracts_config.ctp.enable_wecom_alert);
    assert(!missing_contracts_config.ctp.enable_wecom_report);
    assert(!missing_contracts_config.ctp.generate_local_report_image);
    assert(missing_contracts_config.ctp.wecom_robot_webhook == "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=test-key");
    assert(missing_contracts_config.ctp.market_data_stale_threshold_sec == 45);
    assert(missing_contracts_config.ctp.market_data_recovery_grace_sec == 8);

    const auto missing_spot_dir = std::filesystem::temp_directory_path() / "basis_monitor_config_loader_missing_spot";
    TempDirCleanup missing_spot_cleanup{missing_spot_dir};
    std::filesystem::remove_all(missing_spot_dir);
    std::filesystem::create_directories(missing_spot_dir);

    CopyFile(FixtureConfigPath("ctp.ini"), missing_spot_dir / "ctp.ini");
    CopyFile(FixtureConfigPath("contracts.json"), missing_spot_dir / "contracts.json");
    WriteTextFile(missing_spot_dir / "alert.json", "{\n  \"terminal_alert\": true,\n  \"file_alert\": false\n}\n");

    const auto missing_spot_config = basis_monitor::LoadAppConfig(missing_spot_dir.string());
    assert(missing_spot_config.ctp.front_md_addr == "tcp://119.188.3.11:16113");
    assert(missing_spot_config.alert.terminal_alert);
    assert(!missing_spot_config.alert.file_alert);
    assert(std::abs(missing_spot_config.alert.negative_threshold - 0.0) < 0.0001);
    assert(missing_spot_config.alert.repeat_interval_minutes == 20);
    assert(missing_spot_config.contracts.size() == 2);
    assert(missing_spot_config.contracts[0].instrument_id == "IC2606");
    assert(missing_spot_config.spot.index_symbol.empty());
    assert(missing_spot_config.spot.current_spot_price == 0.0);
    assert(missing_spot_config.spot.update_timestamp.empty());
    assert(missing_spot_config.ctp.reference_future_metadata_dir == "/srv/marketdata/all_instruments/Future");
    assert(missing_spot_config.ctp.reference_index_metadata_dir == "/srv/marketdata/all_instruments/INDX");
    assert(missing_spot_config.ctp.reference_future_eod_dir == "/srv/marketdata/eod_price/Future");
    assert(missing_spot_config.ctp.reference_index_eod_dir == "/srv/marketdata/eod_price/INDX");
    assert(missing_spot_config.ctp.enable_wecom_alert);
    assert(!missing_spot_config.ctp.enable_wecom_report);
    assert(!missing_spot_config.ctp.generate_local_report_image);
    assert(missing_spot_config.ctp.wecom_robot_webhook == "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=test-key");
    assert(missing_spot_config.ctp.market_data_stale_threshold_sec == 45);
    assert(missing_spot_config.ctp.market_data_recovery_grace_sec == 8);

    const auto broken_contracts_dir = std::filesystem::temp_directory_path() / "basis_monitor_config_loader_broken_contracts";
    TempDirCleanup broken_contracts_cleanup{broken_contracts_dir};
    std::filesystem::remove_all(broken_contracts_dir);
    std::filesystem::create_directories(broken_contracts_dir);

    CopyFile(FixtureConfigPath("ctp.ini"), broken_contracts_dir / "ctp.ini");
    CopyFile(FixtureConfigPath("spot_price.json"), broken_contracts_dir / "spot_price.json");
    WriteTextFile(broken_contracts_dir / "alert.json", "{\n  \"terminal_alert\": true,\n  \"file_alert\": false\n}\n");
    std::filesystem::create_directory(broken_contracts_dir / "contracts.json");

    bool threw_for_broken_contracts = false;
    try
    {
        (void)basis_monitor::LoadAppConfig(broken_contracts_dir.string());
    }
    catch (const std::exception&)
    {
        threw_for_broken_contracts = true;
    }
    assert(threw_for_broken_contracts);

    const auto broken_spot_dir = std::filesystem::temp_directory_path() / "basis_monitor_config_loader_broken_spot";
    TempDirCleanup broken_spot_cleanup{broken_spot_dir};
    std::filesystem::remove_all(broken_spot_dir);
    std::filesystem::create_directories(broken_spot_dir);

    CopyFile(FixtureConfigPath("ctp.ini"), broken_spot_dir / "ctp.ini");
    CopyFile(FixtureConfigPath("contracts.json"), broken_spot_dir / "contracts.json");
    WriteTextFile(broken_spot_dir / "alert.json", "{\n  \"terminal_alert\": true,\n  \"file_alert\": false\n}\n");
    std::filesystem::create_directory(broken_spot_dir / "spot_price.json");

    bool threw_for_broken_spot = false;
    try
    {
        (void)basis_monitor::LoadAppConfig(broken_spot_dir.string());
    }
    catch (const std::exception&)
    {
        threw_for_broken_spot = true;
    }
    assert(threw_for_broken_spot);

    const auto missing_legacy_dir = std::filesystem::temp_directory_path() / "basis_monitor_config_loader_missing_legacy";
    TempDirCleanup missing_legacy_cleanup{missing_legacy_dir};
    std::filesystem::remove_all(missing_legacy_dir);
    std::filesystem::create_directories(missing_legacy_dir);

    CopyFile(FixtureConfigPath("ctp.ini"), missing_legacy_dir / "ctp.ini");
    WriteTextFile(missing_legacy_dir / "alert.json", "{\n  \"terminal_alert\": true\n}\n");

    const auto missing_legacy_config = basis_monitor::LoadAppConfig(missing_legacy_dir.string());
    assert(missing_legacy_config.ctp.front_md_addr == "tcp://119.188.3.11:16113");
    assert(missing_legacy_config.alert.terminal_alert);
    assert(!missing_legacy_config.alert.file_alert);
    assert(std::abs(missing_legacy_config.alert.negative_threshold - 0.0) < 0.0001);
    assert(missing_legacy_config.alert.repeat_interval_minutes == 20);
    assert(missing_legacy_config.contracts.empty());
    assert(missing_legacy_config.spot.index_symbol.empty());
    assert(missing_legacy_config.spot.current_spot_price == 0.0);
    assert(missing_legacy_config.spot.update_timestamp.empty());
    assert(missing_legacy_config.ctp.reference_future_metadata_dir == "/srv/marketdata/all_instruments/Future");
    assert(missing_legacy_config.ctp.reference_index_metadata_dir == "/srv/marketdata/all_instruments/INDX");
    assert(missing_legacy_config.ctp.reference_future_eod_dir == "/srv/marketdata/eod_price/Future");
    assert(missing_legacy_config.ctp.reference_index_eod_dir == "/srv/marketdata/eod_price/INDX");
    assert(missing_legacy_config.ctp.enable_wecom_alert);
    assert(!missing_legacy_config.ctp.enable_wecom_report);
    assert(!missing_legacy_config.ctp.generate_local_report_image);
    assert(missing_legacy_config.ctp.wecom_robot_webhook == "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=test-key");
    assert(missing_legacy_config.ctp.market_data_stale_threshold_sec == 45);
    assert(missing_legacy_config.ctp.market_data_recovery_grace_sec == 8);

    const auto missing_alert_dir = std::filesystem::temp_directory_path() / "basis_monitor_config_loader_missing_alert";
    TempDirCleanup missing_alert_cleanup{missing_alert_dir};
    std::filesystem::remove_all(missing_alert_dir);
    std::filesystem::create_directories(missing_alert_dir);

    CopyFile(FixtureConfigPath("ctp.ini"), missing_alert_dir / "ctp.ini");
    CopyFile(FixtureConfigPath("contracts.json"), missing_alert_dir / "contracts.json");
    CopyFile(FixtureConfigPath("spot_price.json"), missing_alert_dir / "spot_price.json");

    const auto missing_alert_config = basis_monitor::LoadAppConfig(missing_alert_dir.string());
    assert(!missing_alert_config.alert.terminal_alert);
    assert(!missing_alert_config.alert.file_alert);
    assert(std::abs(missing_alert_config.alert.negative_threshold - 0.0) < 0.0001);
    assert(missing_alert_config.alert.repeat_interval_minutes == 20);
    assert(missing_alert_config.ctp.market_data_stale_threshold_sec == 45);
    assert(missing_alert_config.ctp.market_data_recovery_grace_sec == 8);

    const auto partial_dir = std::filesystem::temp_directory_path() / "basis_monitor_config_loader_partial_alert";
    TempDirCleanup partial_cleanup{partial_dir};
    std::filesystem::remove_all(partial_dir);
    std::filesystem::create_directories(partial_dir);

    CopyFile(FixtureConfigPath("ctp.ini"), partial_dir / "ctp.ini");
    CopyFile(FixtureConfigPath("contracts.json"), partial_dir / "contracts.json");
    CopyFile(FixtureConfigPath("spot_price.json"), partial_dir / "spot_price.json");
    WriteTextFile(partial_dir / "alert.json", "{\n  \"terminal_alert\": true\n}\n");

    const auto partial_alert_config = basis_monitor::LoadAppConfig(partial_dir.string());
    assert(partial_alert_config.alert.terminal_alert);
    assert(!partial_alert_config.alert.file_alert);
    assert(std::abs(partial_alert_config.alert.negative_threshold - 0.0) < 0.0001);
    assert(partial_alert_config.alert.repeat_interval_minutes == 20);
    assert(partial_alert_config.ctp.market_data_stale_threshold_sec == 45);
    assert(partial_alert_config.ctp.market_data_recovery_grace_sec == 8);

    const auto xtp_dir = std::filesystem::temp_directory_path() / "basis_monitor_config_loader_xtp";
    TempDirCleanup xtp_cleanup{xtp_dir};
    std::filesystem::remove_all(xtp_dir);
    std::filesystem::create_directories(xtp_dir);

    WriteTextFile(
        xtp_dir / "ctp.ini",
        "[config]\n"
        "MarketDataProvider=xtp\n"
        "FrontMdAddr=tcp://119.188.3.11:16113\n"
        "BrokerID=7080\n"
        "UserID=21300292\n"
        "Password=aa123456\n"
        "InstrumentID=IC2606,IC2609\n"
        "ReferenceFutureMetadataDir=/srv/staging/all_instruments/Future\n"
        "ReferenceIndexMetadataDir=/srv/staging/all_instruments/INDX\n"
        "ReferenceFutureEodDir=/srv/staging/eod_price/Future\n"
        "ReferenceIndexEodDir=/srv/staging/eod_price/INDX\n"
        "XtpServerIp=10.10.10.10\n"
        "XtpServerPort=6001\n"
        "XtpUser=xtp_user\n"
        "XtpPassword=xtp_pass\n"
        "XtpClientId=7\n"
        "XtpProtocol=udp\n"
        "XtpExchangeId=unknown\n"
        "XtpLocalIp=10.10.10.20\n"
        "XtpConfigFile=/etc/xtp/quote_config.ini\n"
        "XtpFirstTickTimeoutMs=12000\n");
    WriteTextFile(xtp_dir / "alert.json", "{\n  \"terminal_alert\": true\n}\n");

    const auto xtp_config = basis_monitor::LoadAppConfig(xtp_dir.string());
    assert(xtp_config.market_data_provider == basis_monitor::MarketDataProviderType::Xtp);
    assert(xtp_config.xtp.server_ip == "10.10.10.10");
    assert(xtp_config.xtp.server_port == 6001);
    assert(xtp_config.xtp.user == "xtp_user");
    assert(xtp_config.xtp.password == "xtp_pass");
    assert(xtp_config.xtp.client_id == 7);
    assert(xtp_config.xtp.protocol == "udp");
    assert(xtp_config.xtp.exchange_id == "unknown");
    assert(xtp_config.xtp.local_ip == "10.10.10.20");
    assert(xtp_config.xtp.config_file == "/etc/xtp/quote_config.ini");
    assert(xtp_config.xtp.first_tick_timeout_ms == 12000);
    assert(xtp_config.ctp.reference_future_metadata_dir == "/srv/staging/all_instruments/Future");
    assert(xtp_config.ctp.reference_index_metadata_dir == "/srv/staging/all_instruments/INDX");
    assert(xtp_config.ctp.reference_future_eod_dir == "/srv/staging/eod_price/Future");
    assert(xtp_config.ctp.reference_index_eod_dir == "/srv/staging/eod_price/INDX");
    assert(xtp_config.xtp.index_instruments.empty());

    const auto dual_dir = std::filesystem::temp_directory_path() / "basis_monitor_config_loader_dual";
    TempDirCleanup dual_cleanup{dual_dir};
    std::filesystem::remove_all(dual_dir);
    std::filesystem::create_directories(dual_dir);

    WriteTextFile(
        dual_dir / "ctp.ini",
        "[config]\n"
        "MarketDataProvider=ctp\n"
        "EnableCtpMarketData=true\n"
        "FrontMdAddr=tcp://119.188.3.11:16113\n"
        "BrokerID=7080\n"
        "UserID=21300292\n"
        "Password=aa123456\n"
        "InstrumentID=IC2606,IC2609\n"
        "ReferenceFutureMetadataDir=/srv/staging/all_instruments/Future\n"
        "ReferenceIndexMetadataDir=/srv/staging/all_instruments/INDX\n"
        "ReferenceFutureEodDir=/srv/staging/eod_price/Future\n"
        "ReferenceIndexEodDir=/srv/staging/eod_price/INDX\n");
    CopyFile(FixtureConfigPath("xtp.ini"), dual_dir / "xtp.ini");
    WriteTextFile(dual_dir / "alert.json", "{\n  \"terminal_alert\": true\n}\n");

    const auto dual_config = basis_monitor::LoadAppConfig(dual_dir.string());
    assert(dual_config.market_data_provider == basis_monitor::MarketDataProviderType::Ctp);
    assert(dual_config.ctp.enable_ctp_market_data);
    assert(dual_config.xtp.enable_xtp_market_data);
    assert(dual_config.ctp.instruments.size() == 2);
    assert(dual_config.ctp.instruments[0] == "IC2606");
    assert(dual_config.xtp.server_ip == "10.10.10.10");
    assert(dual_config.xtp.server_port == 6001);
    assert(dual_config.xtp.index_instruments.size() == 3);
    assert(dual_config.xtp.index_instruments[0] == "000300");
    assert(dual_config.xtp.index_instruments[1] == "000905");
    assert(dual_config.xtp.index_instruments[2] == "000852");
    assert(dual_config.xtp.reference_future_metadata_dir == "/srv/xtp/all_instruments/Future");
    assert(dual_config.xtp.reference_index_metadata_dir == "/srv/xtp/all_instruments/INDX");
    assert(dual_config.xtp.reference_future_eod_dir == "/srv/xtp/eod_price/Future");
    assert(dual_config.xtp.reference_index_eod_dir == "/srv/xtp/eod_price/INDX");

    {
        std::vector<basis_monitor::MonitoredContract> monitored_contracts = {
            {"IF2606", "IF", "hs300", "000300.XSHG", "沪深300", 0.0, 0.0, "2026-06-19", 1000.0},
            {"IF2609", "IF", "hs300", "000300.XSHG", "沪深300", 0.0, 0.0, "2026-09-18", 900.0},
            {"IC2606", "IC", "zz500", "000905.XSHG", "中证500", 0.0, 0.0, "2026-06-19", 800.0},
            {"IM2606", "IM", "zz1000", "000852.XSHG", "中证1000", 0.0, 0.0, "2026-06-19", 700.0}
        };

        auto derived_xtp_instruments = basis_monitor::BuildXtpIndexInstruments(monitored_contracts);
        assert(derived_xtp_instruments.size() == 3);
        assert(derived_xtp_instruments[0] == "000300.XSHG");
        assert(derived_xtp_instruments[1] == "000905.XSHG");
        assert(derived_xtp_instruments[2] == "000852.XSHG");
    }

    return 0;
}
