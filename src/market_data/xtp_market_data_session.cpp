#include "basis_monitor/market_data/xtp_market_data_session.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <map>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "basis_monitor/ctp/md_listener.h"
#include "basis_monitor/domain/market_tick.h"
#include "basis_monitor/logging/logger.h"
#include "basis_monitor/market_data/xtp_quote_api.h"
#include "xquote_x_api_struct.h"
#include "xtpx_api_struct_common.h"
#include "xtpx_quote_api.h"

namespace basis_monitor
{

namespace
{

constexpr uint32_t kHeartbeatIntervalSeconds = 30;

const char* ProtocolName(XTPX::API::XTP_PROTOCOL_TYPE protocol)
{
    switch (protocol)
    {
    case XTPX::API::XTP_PROTOCOL_UDP:
        return "UDP";
    case XTPX::API::XTP_PROTOCOL_TCP:
    default:
        return "TCP";
    }
}

XTPX::API::XTP_PROTOCOL_TYPE ParseProtocol(const std::string& value)
{
    std::string normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return normalized == "udp" ? XTPX::API::XTP_PROTOCOL_UDP : XTPX::API::XTP_PROTOCOL_TCP;
}

XTPX::API::XTP_EXCHANGE_TYPE ParseExchangeId(const std::string& value)
{
    std::string normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if (normalized == "sh" || normalized == "sse")
    {
        return XTPX::API::XTP_EXCHANGE_SH;
    }
    if (normalized == "sz" || normalized == "szse")
    {
        return XTPX::API::XTP_EXCHANGE_SZ;
    }
    if (normalized == "nq")
    {
        return XTPX::API::XTP_EXCHANGE_NQ;
    }
    if (normalized == "hk")
    {
        return XTPX::API::XTP_EXCHANGE_HK;
    }
    return XTPX::API::XTP_EXCHANGE_UNKNOWN;
}

std::string SafeErrorMessage(const XTPX::API::XTPRI* error)
{
    if (error == nullptr)
    {
        return "unknown";
    }
    return error->error_msg;
}

struct XtpSubscriptionInstrument
{
    std::string ticker;
    XTPX::API::XTP_EXCHANGE_TYPE exchange = XTPX::API::XTP_EXCHANGE_UNKNOWN;
};

XtpSubscriptionInstrument NormalizeSubscriptionInstrument(const std::string& instrument,
                                                         const std::string& fallback_exchange_id)
{
    XtpSubscriptionInstrument normalized = {};
    const auto delimiter = instrument.find('.');
    if (delimiter == std::string::npos)
    {
        normalized.ticker = instrument;
        normalized.exchange = ParseExchangeId(fallback_exchange_id);
        return normalized;
    }

    normalized.ticker = instrument.substr(0, delimiter);
    const auto suffix = instrument.substr(delimiter + 1);
    if (suffix == "XSHG")
    {
        normalized.exchange = XTPX::API::XTP_EXCHANGE_SH;
    }
    else if (suffix == "XSHE")
    {
        normalized.exchange = XTPX::API::XTP_EXCHANGE_SZ;
    }
    else
    {
        normalized.exchange = ParseExchangeId(fallback_exchange_id);
    }
    return normalized;
}

void ParseDataTime(int64_t data_time, std::string& update_time, int& update_millisec)
{
    char buffer[32] = {0};
    std::snprintf(buffer, sizeof(buffer), "%017lld", static_cast<long long>(data_time));
    update_time.assign(buffer + 8, 2);
    update_time.push_back(':');
    update_time.append(buffer + 10, 2);
    update_time.push_back(':');
    update_time.append(buffer + 12, 2);
    update_millisec = std::stoi(std::string(buffer + 14, 3));
}

} // namespace

class XtpMarketDataSession::XtpQuoteSpiBridge final : public XTPX::API::QuoteSpi
{
public:
    explicit XtpQuoteSpiBridge(XtpMarketDataSession& session)
        : session_(session)
    {
    }

    void OnDisconnected(int reason) override
    {
        Log("[XTP_DISCONNECTED] reason=%d\n", reason);
        if (!session_.started_)
        {
            return;
        }

        if (!session_.LoginAndSubscribe())
        {
            Log("[XTP_RECOVER_FAILED]\n");
        }
    }

    void OnDepthMarketData(
        XTPX::API::XTPMD* market_data,
        int64_t[],
        int32_t,
        int32_t,
        int64_t[],
        int32_t,
        int32_t) override
    {
        if (market_data == nullptr)
        {
            return;
        }

        MarketTick tick = {};
        tick.instrument_id = market_data->ticker;
        ParseDataTime(market_data->data_time, tick.update_time, tick.update_millisec);
        tick.provider = MarketDataProviderType::Xtp;
        tick.instrument_type = MarketTickInstrumentType::Index;
        tick.last_price = market_data->last_price;
        tick.bid_price_1 = market_data->bid[0];
        tick.bid_volume_1 = static_cast<int>(market_data->bid_qty[0]);
        tick.ask_price_1 = market_data->ask[0];
        tick.ask_volume_1 = static_cast<int>(market_data->ask_qty[0]);
        tick.volume = static_cast<int>(market_data->qty);

        {
            std::lock_guard<std::mutex> lock(session_.first_tick_mutex_);
            session_.first_market_data_received_ = true;
        }
        session_.first_tick_cv_.notify_all();
        session_.listener_.OnTick(tick);
    }

private:
    XtpMarketDataSession& session_;
};

XtpMarketDataSession::XtpMarketDataSession(const AppConfig& config, MdListener& listener, QuoteApiFactory factory)
    : config_(config), listener_(listener), factory_(std::move(factory))
{
    if (!factory_)
    {
        factory_ = [client_id = config_.xtp.client_id]() {
            return CreateDefaultXtpQuoteApi(client_id, "runtime/flow/xtp");
        };
    }
}

XtpMarketDataSession::~XtpMarketDataSession()
{
    Stop();
}

bool XtpMarketDataSession::Start()
{
    Stop();
    {
        std::lock_guard<std::mutex> lock(first_tick_mutex_);
        first_market_data_received_ = false;
    }

    api_ = factory_();
    spi_bridge_ = std::make_unique<XtpQuoteSpiBridge>(*this);
    api_->RegisterSpi(spi_bridge_.get());
    started_ = true;
    if (!ConfigureAndLogin())
    {
        started_ = false;
        return false;
    }
    return true;
}

bool XtpMarketDataSession::WaitForFirstMarketData(unsigned long timeout_ms) const
{
    std::unique_lock<std::mutex> lock(first_tick_mutex_);
    return first_tick_cv_.wait_for(
        lock,
        std::chrono::milliseconds(timeout_ms),
        [this]() { return first_market_data_received_; });
}

void XtpMarketDataSession::Stop()
{
    started_ = false;
    if (api_ != nullptr)
    {
        api_->Logout();
    }
    spi_bridge_.reset();
    api_.reset();
}

bool XtpMarketDataSession::ConfigureAndLogin()
{
    api_->SetHeartBeatInterval(kHeartbeatIntervalSeconds);
    if (!config_.xtp.config_file.empty() && !api_->SetConfigFile(config_.xtp.config_file.c_str()))
    {
        Log("[XTP_CONFIG_FAILED] path=%s\n", config_.xtp.config_file.c_str());
        return false;
    }
    return LoginAndSubscribe();
}

bool XtpMarketDataSession::LoginAndSubscribe()
{
    const auto protocol = ParseProtocol(config_.xtp.protocol);
    const char* local_ip = config_.xtp.local_ip.empty() ? nullptr : config_.xtp.local_ip.c_str();
    const int login_ret = api_->Login(
        config_.xtp.server_ip.c_str(),
        config_.xtp.server_port,
        config_.xtp.user.c_str(),
        config_.xtp.password.c_str(),
        protocol,
        local_ip);
    if (login_ret != 0)
    {
        Log("[XTP_LOGIN_FAILED] ret=%d protocol=%s error=%s\n",
            login_ret,
            ProtocolName(protocol),
            SafeErrorMessage(api_->GetApiLastError()).c_str());
        return false;
    }

    return SubscribeCurrentInstruments();
}

bool XtpMarketDataSession::SubscribeCurrentInstruments()
{
    std::map<XTPX::API::XTP_EXCHANGE_TYPE, std::vector<std::string>> grouped_tickers;
    for (const auto& instrument : config_.xtp.index_instruments)
    {
        const auto normalized = NormalizeSubscriptionInstrument(instrument, config_.xtp.exchange_id);
        if (normalized.ticker.empty() || normalized.exchange == XTPX::API::XTP_EXCHANGE_UNKNOWN)
        {
            continue;
        }
        grouped_tickers[normalized.exchange].push_back(normalized.ticker);
    }

    if (grouped_tickers.empty())
    {
        Log("[XTP_SUBSCRIBE_SKIPPED] reason=empty_index_instruments\n");
        return false;
    }

    for (const auto& entry : grouped_tickers)
    {
        std::vector<char*> tickers;
        tickers.reserve(entry.second.size());
        for (const auto& ticker : entry.second)
        {
            tickers.push_back(const_cast<char*>(ticker.c_str()));
        }

        const int subscribe_ret = api_->SubscribeMarketData(
            tickers.data(),
            static_cast<int>(tickers.size()),
            entry.first);
        if (subscribe_ret != 0)
        {
            Log("[XTP_SUBSCRIBE_FAILED] ret=%d error=%s\n",
                subscribe_ret,
                SafeErrorMessage(api_->GetApiLastError()).c_str());
            return false;
        }
    }

    return true;
}

} // namespace basis_monitor
