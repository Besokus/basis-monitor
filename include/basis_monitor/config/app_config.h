#pragma once

#include <string>
#include <vector>

#include "basis_monitor/domain/contract_definition.h"

namespace basis_monitor
{

enum class MarketDataProviderType
{
    Ctp,
    Xtp
};

struct SpotPriceConfig
{
    std::string index_symbol;
    double current_spot_price = 0.0;
    std::string update_timestamp;
};

struct CtpConfig
{
    bool enable_ctp_market_data = false;
    std::string front_md_addr;
    std::string broker_id;
    std::string user_id;
    std::string password;
    std::string auth_code;
    std::string app_id;
    std::string user_product_info;
    std::vector<std::string> instruments;
    std::string flow_dir = "runtime/flow/";
    int first_tick_timeout_ms = 10000;
    std::string reference_future_metadata_dir;
    std::string reference_index_metadata_dir;
    std::string reference_future_eod_dir;
    std::string reference_index_eod_dir;
    bool enable_wecom_alert = false;
    bool enable_wecom_report = false;
    bool generate_local_report_image = true;
    std::string wecom_robot_webhook;
    int market_data_stale_threshold_sec = 30;
    int market_data_recovery_grace_sec = 5;
};

struct XtpConfig
{
    bool enable_xtp_market_data = false;
    std::string server_ip;
    int server_port = 0;
    std::string user;
    std::string password;
    int client_id = 0;
    std::string protocol;
    std::string exchange_id = "unknown";
    std::string local_ip;
    std::string config_file;
    int first_tick_timeout_ms = 10000;
    std::vector<std::string> index_instruments;
    // Reference data directories
    std::string reference_future_metadata_dir;
    std::string reference_index_metadata_dir;
    std::string reference_future_eod_dir;
    std::string reference_index_eod_dir;
};

struct AlertConfig
{
    bool terminal_alert = false;
    bool file_alert = false;
    double negative_threshold = 0.0;
    int repeat_interval_minutes = 20;
};

struct AppConfig
{
    MarketDataProviderType market_data_provider = MarketDataProviderType::Ctp;
    CtpConfig ctp;
    XtpConfig xtp;
    AlertConfig alert;
    SpotPriceConfig spot;
    std::vector<ContractDefinition> contracts;
};

} // namespace basis_monitor
