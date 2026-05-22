#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "basis_monitor/config/app_config.h"
#include "basis_monitor/ctp/md_listener.h"
#include "basis_monitor/domain/market_tick.h"
#include "basis_monitor/market_data/xtp_market_data_session.h"
#include "basis_monitor/market_data/xtp_quote_api.h"
#include "xquote_x_api_struct.h"
#include "xtpx_api_data_type.h"
#include "xtpx_api_struct_common.h"
#include "xtpx_quote_api.h"

namespace
{

class CapturingListener final : public basis_monitor::MdListener
{
public:
    void OnTick(const basis_monitor::MarketTick& tick) override
    {
        ticks.push_back(tick);
    }

    std::vector<basis_monitor::MarketTick> ticks;
};

class FakeXtpQuoteApi final : public basis_monitor::IXtpQuoteApi
{
public:
    struct SubscriptionRequest
    {
        std::vector<std::string> tickers;
        XTPX::API::XTP_EXCHANGE_TYPE exchange = XTPX::API::XTP_EXCHANGE_UNKNOWN;
    };

    bool set_config_file_result = true;
    int login_result = 0;
    int subscribe_result = 0;
    uint32_t heartbeat_interval = 0;
    int login_calls = 0;
    int subscribe_calls = 0;
    int logout_calls = 0;
    std::string config_file_path;
    std::vector<SubscriptionRequest> subscription_requests;
    XTPX::API::QuoteSpi* spi = nullptr;
    XTPX::API::XTPRI last_error = {};

    void RegisterSpi(XTPX::API::QuoteSpi* value) override
    {
        spi = value;
    }

    void SetHeartBeatInterval(uint32_t interval) override
    {
        heartbeat_interval = interval;
    }

    bool SetConfigFile(const char* filename) override
    {
        config_file_path = filename == nullptr ? "" : filename;
        return set_config_file_result;
    }

    int Login(const char*, int, const char*, const char*, XTPX::API::XTP_PROTOCOL_TYPE, const char*) override
    {
        ++login_calls;
        return login_result;
    }

    int SubscribeMarketData(char* ticker[], int count, XTPX::API::XTP_EXCHANGE_TYPE exchange) override
    {
        ++subscribe_calls;
        SubscriptionRequest request = {};
        for (int index = 0; index < count; ++index)
        {
            request.tickers.emplace_back(ticker[index]);
        }
        request.exchange = exchange;
        subscription_requests.push_back(request);
        return subscribe_result;
    }

    int Logout() override
    {
        ++logout_calls;
        return 0;
    }

    XTPX::API::XTPRI* GetApiLastError() override
    {
        return &last_error;
    }

    void EmitDepth(const char* ticker, double last_price, int64_t data_time, int64_t qty, double bid_price, int64_t bid_qty, double ask_price, int64_t ask_qty)
    {
        assert(spi != nullptr);
        XTPX::API::XTPMD market_data = {};
        std::snprintf(market_data.ticker, sizeof(market_data.ticker), "%s", ticker);
        market_data.last_price = last_price;
        market_data.data_time = data_time;
        market_data.qty = qty;
        market_data.bid[0] = bid_price;
        market_data.bid_qty[0] = bid_qty;
        market_data.ask[0] = ask_price;
        market_data.ask_qty[0] = ask_qty;
        spi->OnDepthMarketData(&market_data, nullptr, 0, 0, nullptr, 0, 0);
    }

    void EmitDisconnected(int reason)
    {
        assert(spi != nullptr);
        spi->OnDisconnected(reason);
    }
};

basis_monitor::AppConfig MakeConfig()
{
    basis_monitor::AppConfig config = {};
    config.market_data_provider = basis_monitor::MarketDataProviderType::Xtp;
    config.ctp.instruments = {"IF2606", "IC2606"};
    config.xtp.index_instruments = {"000300.XSHG", "000905.XSHG", "399905.XSHE"};
    config.xtp.server_ip = "10.10.10.10";
    config.xtp.server_port = 6001;
    config.xtp.user = "xtp_user";
    config.xtp.password = "xtp_pass";
    config.xtp.client_id = 7;
    config.xtp.protocol = "udp";
    config.xtp.config_file = "/tmp/quote_config.ini";
    config.xtp.local_ip = "127.0.0.1";
    return config;
}

} // namespace

int main()
{
    {
        auto config = MakeConfig();
        auto api = std::make_unique<FakeXtpQuoteApi>();
        auto* api_ptr = api.get();
        CapturingListener listener;
        basis_monitor::XtpMarketDataSession session(
            config,
            listener,
            [&api = api]() mutable -> std::unique_ptr<basis_monitor::IXtpQuoteApi> {
                return std::move(api);
            });

        assert(session.Start());
        assert(api_ptr->spi != nullptr);
        assert(api_ptr->heartbeat_interval == 30);
        assert(api_ptr->config_file_path == "/tmp/quote_config.ini");
        assert(api_ptr->login_calls == 1);
        assert(api_ptr->subscribe_calls == 2);
        assert(api_ptr->subscription_requests.size() == 2);
        assert(api_ptr->subscription_requests[0].exchange == XTPX::API::XTP_EXCHANGE_SH);
        assert(api_ptr->subscription_requests[0].tickers.size() == 2);
        assert(api_ptr->subscription_requests[0].tickers[0] == "000300");
        assert(api_ptr->subscription_requests[0].tickers[1] == "000905");
        assert(api_ptr->subscription_requests[1].exchange == XTPX::API::XTP_EXCHANGE_SZ);
        assert(api_ptr->subscription_requests[1].tickers.size() == 1);
        assert(api_ptr->subscription_requests[1].tickers[0] == "399905");
        assert(!session.WaitForFirstMarketData(1));

        api_ptr->EmitDepth("000300", 4433.4, 20260403093115999LL, 12345, 4433.2, 10, 4433.6, 12);
        assert(session.WaitForFirstMarketData(1));
        assert(listener.ticks.size() == 1);
        assert(listener.ticks.front().instrument_id == "000300");
        assert(listener.ticks.front().last_price == 4433.4);
        assert(listener.ticks.front().update_time == "09:31:15");
        assert(listener.ticks.front().update_millisec == 999);
        assert(listener.ticks.front().volume == 12345);
        assert(listener.ticks.front().provider == basis_monitor::MarketDataProviderType::Xtp);
        assert(listener.ticks.front().instrument_type == basis_monitor::MarketTickInstrumentType::Index);

        session.Stop();
        assert(api_ptr->logout_calls == 1);
    }

    {
        auto config = MakeConfig();
        auto api = std::make_unique<FakeXtpQuoteApi>();
        auto* api_ptr = api.get();
        CapturingListener listener;
        basis_monitor::XtpMarketDataSession session(
            config,
            listener,
            [&api = api]() mutable -> std::unique_ptr<basis_monitor::IXtpQuoteApi> {
                return std::move(api);
            });

        assert(session.Start());
        api_ptr->EmitDisconnected(1001);
        assert(api_ptr->login_calls == 2);
        assert(api_ptr->subscribe_calls == 4);
    }

    {
        auto config = MakeConfig();
        auto api = std::make_unique<FakeXtpQuoteApi>();
        api->login_result = -1;
        CapturingListener listener;
        basis_monitor::XtpMarketDataSession session(
            config,
            listener,
            [&api = api]() mutable -> std::unique_ptr<basis_monitor::IXtpQuoteApi> {
                return std::move(api);
            });

        assert(!session.Start());
    }

    return 0;
}
