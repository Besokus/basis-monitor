#include "basis_monitor/config/config_loader.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <vector>

namespace basis_monitor
{

namespace
{

std::string Trim(const std::string& value)
{
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
    {
        ++start;
    }

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
    {
        --end;
    }
    return value.substr(start, end - start);
}

std::string StripUtf8Bom(const std::string& value)
{
    if (value.size() >= 3 &&
        static_cast<unsigned char>(value[0]) == 0xEF &&
        static_cast<unsigned char>(value[1]) == 0xBB &&
        static_cast<unsigned char>(value[2]) == 0xBF)
    {
        return value.substr(3);
    }
    return value;
}

MarketDataProviderType ParseMarketDataProviderType(const std::string& value)
{
    if (value.empty())
    {
        return MarketDataProviderType::Ctp;
    }

    std::string normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if (normalized == "ctp")
    {
        return MarketDataProviderType::Ctp;
    }
    if (normalized == "xtp")
    {
        return MarketDataProviderType::Xtp;
    }
    throw std::runtime_error("Unsupported MarketDataProvider value: " + value);
}

std::string ReadIniValue(const std::string& file_path, const std::string& section, const std::string& key)
{
    std::ifstream input(file_path.c_str());
    if (!input.is_open())
    {
        throw std::runtime_error("Cannot open config file: " + file_path);
    }

    bool in_target_section = false;
    std::string line;
    while (std::getline(input, line))
    {
        line = Trim(StripUtf8Bom(line));
        if (line.empty() || line[0] == '#' || line[0] == ';')
        {
            continue;
        }

        if (line.front() == '[' && line.find(']') != std::string::npos)
        {
            const auto section_name = Trim(line.substr(1, line.find(']') - 1));
            in_target_section = section_name == section;
            continue;
        }

        if (!in_target_section)
        {
            continue;
        }

        const auto equal_pos = line.find('=');
        if (equal_pos == std::string::npos)
        {
            continue;
        }

        const auto current_key = Trim(line.substr(0, equal_pos));
        if (current_key == key)
        {
            return Trim(line.substr(equal_pos + 1));
        }
    }

    throw std::runtime_error("Missing config key [" + section + "] " + key + " in " + file_path);
}

std::vector<std::string> SplitInstruments(const std::string& value)
{
    std::string normalized = value;
    std::replace(normalized.begin(), normalized.end(), ':', ',');
    std::replace(normalized.begin(), normalized.end(), ';', ',');

    std::vector<std::string> instruments;
    std::stringstream ss(normalized);
    std::string token;
    while (std::getline(ss, token, ','))
    {
        token = Trim(token);
        if (!token.empty())
        {
            instruments.push_back(token);
        }
    }
    return instruments;
}

std::string ReadWholeFile(const std::string& file_path)
{
    std::ifstream input(file_path.c_str());
    if (!input.is_open())
    {
        throw std::runtime_error("Cannot open config file: " + file_path);
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool ShouldLoadOptionalFile(const std::string& file_path)
{
    std::error_code exists_error;
    const bool exists = std::filesystem::exists(file_path, exists_error);
    if (exists_error)
    {
        throw std::runtime_error("Cannot inspect config file: " + file_path);
    }

    if (!exists)
    {
        return false;
    }

    std::error_code regular_error;
    const bool is_regular_file = std::filesystem::is_regular_file(file_path, regular_error);
    if (regular_error)
    {
        throw std::runtime_error("Cannot inspect config file: " + file_path);
    }

    if (!is_regular_file)
    {
        throw std::runtime_error("Cannot open config file: " + file_path);
    }

    return true;
}

std::string MatchFirstString(const std::string& input, const std::regex& pattern, const std::string& field_name)
{
    std::smatch match;
    if (!std::regex_search(input, match, pattern))
    {
        throw std::runtime_error("Missing JSON field: " + field_name);
    }
    return match[1].str();
}

bool MatchOptionalBool(const std::string& input, const std::regex& pattern, bool default_value)
{
    std::smatch match;
    if (!std::regex_search(input, match, pattern))
    {
        return default_value;
    }
    return match[1].str() == "true";
}

double MatchFirstDouble(const std::string& input, const std::regex& pattern, const std::string& field_name)
{
    std::smatch match;
    if (!std::regex_search(input, match, pattern))
    {
        throw std::runtime_error("Missing JSON number field: " + field_name);
    }
    return std::stod(match[1].str());
}

double MatchOptionalDouble(const std::string& input, const std::regex& pattern, double default_value)
{
    std::smatch match;
    if (!std::regex_search(input, match, pattern))
    {
        return default_value;
    }
    return std::stod(match[1].str());
}

int MatchOptionalInt(const std::string& input, const std::regex& pattern, int default_value)
{
    std::smatch match;
    if (!std::regex_search(input, match, pattern))
    {
        return default_value;
    }
    return std::stoi(match[1].str());
}

std::vector<ContractDefinition> ParseContractsJson(const std::string& json)
{
    const std::regex contract_pattern(
        "\\{\\s*\"instrument_id\"\\s*:\\s*\"([^\"]+)\"\\s*,\\s*\"expiry_date\"\\s*:\\s*\"([^\"]+)\"\\s*,\\s*\"enabled\"\\s*:\\s*(true|false)\\s*\\}");

    std::vector<ContractDefinition> contracts;
    for (std::sregex_iterator it(json.begin(), json.end(), contract_pattern), end; it != end; ++it)
    {
        ContractDefinition contract = {};
        contract.instrument_id = (*it)[1].str();
        contract.expiry_date = (*it)[2].str();
        contract.enabled = (*it)[3].str() == "true";
        contracts.push_back(contract);
    }

    if (contracts.empty())
    {
        throw std::runtime_error("contracts.json does not contain any contract definitions");
    }
    return contracts;
}

AlertConfig ParseAlertJson(const std::string& json)
{
    AlertConfig alert = {};
    alert.terminal_alert = MatchOptionalBool(json, std::regex("\"terminal_alert\"\\s*:\\s*(true|false)"), alert.terminal_alert);
    alert.file_alert = MatchOptionalBool(json, std::regex("\"file_alert\"\\s*:\\s*(true|false)"), alert.file_alert);
    alert.negative_threshold = MatchOptionalDouble(
        json,
        std::regex("\"negative_threshold\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)"),
        alert.negative_threshold);
    alert.repeat_interval_minutes = MatchOptionalInt(
        json,
        std::regex("\"repeat_interval_minutes\"\\s*:\\s*([0-9]+)"),
        alert.repeat_interval_minutes);
    if (alert.repeat_interval_minutes <= 0)
    {
        alert.repeat_interval_minutes = 20;
    }
    return alert;
}

SpotPriceConfig ParseSpotPriceJson(const std::string& json)
{
    SpotPriceConfig spot = {};
    spot.index_symbol = MatchFirstString(json, std::regex("\"index_symbol\"\\s*:\\s*\"([^\"]+)\""), "index_symbol");
    spot.current_spot_price = MatchFirstDouble(json, std::regex("\"current_spot_price\"\\s*:\\s*([0-9]+(?:\\.[0-9]+)?)"), "current_spot_price");
    spot.update_timestamp = MatchFirstString(json, std::regex("\"update_timestamp\"\\s*:\\s*\"([^\"]+)\""), "update_timestamp");
    return spot;
}

int ParsePositiveIntOrDefault(const std::string& value, int default_value)
{
    if (value.empty())
    {
        return default_value;
    }

    try
    {
        const int parsed = std::stoi(value);
        return parsed > 0 ? parsed : default_value;
    }
    catch (...)
    {
        return default_value;
    }
}

bool ParseBoolOrDefault(const std::string& value, bool default_value)
{
    if (value.empty())
    {
        return default_value;
    }

    std::string normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on")
    {
        return true;
    }

    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off")
    {
        return false;
    }

    return default_value;
}

} // namespace

AppConfig LoadAppConfig(const std::string& config_path)
{
    AppConfig config = {};
    const std::string ctp_ini_path = config_path + "/ctp.ini";
    const std::string alert_path = config_path + "/alert.json";
    const std::string contracts_path = config_path + "/contracts.json";
    const std::string spot_price_path = config_path + "/spot_price.json";

    config.ctp.front_md_addr = ReadIniValue(ctp_ini_path, "config", "FrontMdAddr");
    config.ctp.broker_id = ReadIniValue(ctp_ini_path, "config", "BrokerID");
    config.ctp.user_id = ReadIniValue(ctp_ini_path, "config", "UserID");
    config.ctp.password = ReadIniValue(ctp_ini_path, "config", "Password");
    config.ctp.instruments = SplitInstruments(ReadIniValue(ctp_ini_path, "config", "InstrumentID"));

    try
    {
        config.ctp.auth_code = ReadIniValue(ctp_ini_path, "config", "AuthCode");
    }
    catch (...)
    {
    }

    try
    {
        config.ctp.app_id = ReadIniValue(ctp_ini_path, "config", "AppID");
    }
    catch (...)
    {
    }

    try
    {
        config.ctp.user_product_info = ReadIniValue(ctp_ini_path, "config", "UserProductInfo");
    }
    catch (...)
    {
    }

    std::ifstream alert_input(alert_path.c_str());
    if (alert_input.good())
    {
        config.alert = ParseAlertJson(ReadWholeFile(alert_path));
    }

    if (ShouldLoadOptionalFile(contracts_path))
    {
        config.contracts = ParseContractsJson(ReadWholeFile(contracts_path));
    }

    if (ShouldLoadOptionalFile(spot_price_path))
    {
        config.spot = ParseSpotPriceJson(ReadWholeFile(spot_price_path));
    }

    if (config.ctp.instruments.empty())
    {
        throw std::runtime_error("InstrumentID is empty in " + ctp_ini_path);
    }

    std::ifstream input(ctp_ini_path.c_str());
    if (input.good())
    {
        try
        {
            const auto provider_value = ReadIniValue(ctp_ini_path, "config", "MarketDataProvider");
            config.market_data_provider = ParseMarketDataProviderType(provider_value);
        }
        catch (...)
        {
        }

        try
        {
            const auto flow_dir = ReadIniValue(ctp_ini_path, "config", "FlowDir");
            if (!flow_dir.empty())
            {
                config.ctp.flow_dir = flow_dir;
            }
        }
        catch (...)
        {
        }

        try
        {
            const auto timeout_value = ReadIniValue(ctp_ini_path, "config", "FirstTickTimeoutMs");
            config.ctp.first_tick_timeout_ms = ParsePositiveIntOrDefault(timeout_value, config.ctp.first_tick_timeout_ms);
        }
        catch (...)
        {
        }

        try
        {
            config.ctp.reference_future_metadata_dir = ReadIniValue(ctp_ini_path, "config", "ReferenceFutureMetadataDir");
        }
        catch (...)
        {
        }

        try
        {
            config.ctp.reference_index_metadata_dir = ReadIniValue(ctp_ini_path, "config", "ReferenceIndexMetadataDir");
        }
        catch (...)
        {
        }

        try
        {
            config.ctp.reference_future_eod_dir = ReadIniValue(ctp_ini_path, "config", "ReferenceFutureEodDir");
        }
        catch (...)
        {
        }

        try
        {
            config.ctp.reference_index_eod_dir = ReadIniValue(ctp_ini_path, "config", "ReferenceIndexEodDir");
        }
        catch (...)
        {
        }

        try
        {
            const auto enable_wecom_alert = ReadIniValue(ctp_ini_path, "config", "EnableWeComAlert");
            config.ctp.enable_wecom_alert = ParseBoolOrDefault(enable_wecom_alert, config.ctp.enable_wecom_alert);
        }
        catch (...)
        {
        }

        try
        {
            const auto enable_wecom_report = ReadIniValue(ctp_ini_path, "config", "EnableWeComReport");
            config.ctp.enable_wecom_report = ParseBoolOrDefault(enable_wecom_report, config.ctp.enable_wecom_report);
        }
        catch (...)
        {
        }

        try
        {
            const auto generate_local_report_image = ReadIniValue(ctp_ini_path, "config", "GenerateLocalReportImage");
            config.ctp.generate_local_report_image =
                ParseBoolOrDefault(generate_local_report_image, config.ctp.generate_local_report_image);
        }
        catch (...)
        {
        }

        try
        {
            config.ctp.wecom_robot_webhook = ReadIniValue(ctp_ini_path, "config", "WeComRobotWebhook");
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.server_ip = ReadIniValue(ctp_ini_path, "config", "XtpServerIp");
        }
        catch (...)
        {
        }

        try
        {
            const auto value = ReadIniValue(ctp_ini_path, "config", "XtpServerPort");
            config.xtp.server_port = ParsePositiveIntOrDefault(value, config.xtp.server_port);
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.user = ReadIniValue(ctp_ini_path, "config", "XtpUser");
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.password = ReadIniValue(ctp_ini_path, "config", "XtpPassword");
        }
        catch (...)
        {
        }

        try
        {
            const auto value = ReadIniValue(ctp_ini_path, "config", "XtpClientId");
            config.xtp.client_id = ParsePositiveIntOrDefault(value, config.xtp.client_id);
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.protocol = ReadIniValue(ctp_ini_path, "config", "XtpProtocol");
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.exchange_id = ReadIniValue(ctp_ini_path, "config", "XtpExchangeId");
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.local_ip = ReadIniValue(ctp_ini_path, "config", "XtpLocalIp");
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.config_file = ReadIniValue(ctp_ini_path, "config", "XtpConfigFile");
        }
        catch (...)
        {
        }

        try
        {
            const auto value = ReadIniValue(ctp_ini_path, "config", "XtpFirstTickTimeoutMs");
            config.xtp.first_tick_timeout_ms = ParsePositiveIntOrDefault(value, config.xtp.first_tick_timeout_ms);
        }
        catch (...)
        {
        }

        try
        {
            const auto threshold_value = ReadIniValue(ctp_ini_path, "config", "MarketDataStaleThresholdSec");
            config.ctp.market_data_stale_threshold_sec = ParsePositiveIntOrDefault(
                threshold_value, config.ctp.market_data_stale_threshold_sec);
        }
        catch (...)
        {
        }

        try
        {
            const auto grace_value = ReadIniValue(ctp_ini_path, "config", "MarketDataRecoveryGraceSec");
            config.ctp.market_data_recovery_grace_sec = ParsePositiveIntOrDefault(
                grace_value, config.ctp.market_data_recovery_grace_sec);
        }
        catch (...)
        {
        }

        // Load EnableCtpMarketData
        try
        {
            const auto enable_ctp = ReadIniValue(ctp_ini_path, "config", "EnableCtpMarketData");
            config.ctp.enable_ctp_market_data = ParseBoolOrDefault(enable_ctp, config.ctp.enable_ctp_market_data);
        }
        catch (...)
        {
        }
    }

    // Load xtp.ini configuration
    const std::string xtp_ini_path = config_path + "/xtp.ini";
    std::ifstream xtp_input(xtp_ini_path.c_str());
    if (xtp_input.good())
    {
        try
        {
            const auto enable_xtp = ReadIniValue(xtp_ini_path, "config", "EnableXtpMarketData");
            config.xtp.enable_xtp_market_data = ParseBoolOrDefault(enable_xtp, config.xtp.enable_xtp_market_data);
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.server_ip = ReadIniValue(xtp_ini_path, "config", "XtpServerIp");
        }
        catch (...)
        {
        }

        try
        {
            const auto value = ReadIniValue(xtp_ini_path, "config", "XtpServerPort");
            config.xtp.server_port = ParsePositiveIntOrDefault(value, config.xtp.server_port);
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.user = ReadIniValue(xtp_ini_path, "config", "XtpUser");
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.password = ReadIniValue(xtp_ini_path, "config", "XtpPassword");
        }
        catch (...)
        {
        }

        try
        {
            const auto value = ReadIniValue(xtp_ini_path, "config", "XtpClientId");
            config.xtp.client_id = ParsePositiveIntOrDefault(value, config.xtp.client_id);
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.protocol = ReadIniValue(xtp_ini_path, "config", "XtpProtocol");
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.exchange_id = ReadIniValue(xtp_ini_path, "config", "XtpExchangeId");
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.local_ip = ReadIniValue(xtp_ini_path, "config", "XtpLocalIp");
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.config_file = ReadIniValue(xtp_ini_path, "config", "XtpConfigFile");
        }
        catch (...)
        {
        }

        try
        {
            const auto value = ReadIniValue(xtp_ini_path, "config", "XtpFirstTickTimeoutMs");
            config.xtp.first_tick_timeout_ms = ParsePositiveIntOrDefault(value, config.xtp.first_tick_timeout_ms);
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.index_instruments = SplitInstruments(ReadIniValue(xtp_ini_path, "config", "IndexInstrumentID"));
        }
        catch (...)
        {
        }

        // Load reference data directories from xtp.ini
        try
        {
            config.xtp.reference_future_metadata_dir = ReadIniValue(xtp_ini_path, "config", "ReferenceFutureMetadataDir");
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.reference_index_metadata_dir = ReadIniValue(xtp_ini_path, "config", "ReferenceIndexMetadataDir");
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.reference_future_eod_dir = ReadIniValue(xtp_ini_path, "config", "ReferenceFutureEodDir");
        }
        catch (...)
        {
        }

        try
        {
            config.xtp.reference_index_eod_dir = ReadIniValue(xtp_ini_path, "config", "ReferenceIndexEodDir");
        }
        catch (...)
        {
        }
    }

    return config;
}

} // namespace basis_monitor
