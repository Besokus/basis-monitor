#include <cassert>
#include <memory>
#include <stdexcept>

#include "basis_monitor/config/app_config.h"
#include "basis_monitor/ctp/md_listener.h"
#include "basis_monitor/market_data/market_data_session.h"
#include "basis_monitor/market_data/market_data_session_factory.h"

namespace
{

class NullListener final : public basis_monitor::MdListener
{
public:
    void OnTick(const basis_monitor::MarketTick&) override
    {
    }
};

basis_monitor::AppConfig MakeBaseConfig()
{
    basis_monitor::AppConfig config = {};
    config.ctp.front_md_addr = "tcp://127.0.0.1:12345";
    config.ctp.broker_id = "7080";
    config.ctp.user_id = "demo";
    config.ctp.password = "secret";
    config.ctp.instruments = {"IF2606"};
    return config;
}

} // namespace

int main()
{
    NullListener listener;

    {
        auto config = MakeBaseConfig();
        config.market_data_provider = basis_monitor::MarketDataProviderType::Ctp;

        auto session = basis_monitor::CreateMarketDataSession(config, listener);
        assert(session != nullptr);
    }

    {
        auto config = MakeBaseConfig();
        config.market_data_provider = basis_monitor::MarketDataProviderType::Xtp;
        bool threw = false;
        try
        {
            (void)basis_monitor::CreateMarketDataSession(config, listener);
        }
        catch (const std::runtime_error&)
        {
            threw = true;
        }
        assert(threw);
    }

    {
        auto config = MakeBaseConfig();
        config.market_data_provider = basis_monitor::MarketDataProviderType::Xtp;
        config.xtp.server_ip = "10.10.10.10";
        config.xtp.server_port = 6001;
        config.xtp.user = "xtp_user";
        config.xtp.password = "xtp_pass";
        config.xtp.client_id = 7;
        config.xtp.protocol = "udp";
        config.xtp.config_file = "/etc/xtp/quote_config.ini";

        auto session = basis_monitor::CreateMarketDataSession(config, listener);
        assert(session != nullptr);
    }

    {
        auto config = MakeBaseConfig();
        config.ctp.enable_ctp_market_data = true;
        config.xtp.enable_xtp_market_data = true;
        config.xtp.server_ip = "10.10.10.10";
        config.xtp.server_port = 6001;
        config.xtp.user = "xtp_user";
        config.xtp.password = "xtp_pass";
        config.xtp.client_id = 7;
        config.xtp.protocol = "udp";
        config.xtp.config_file = "/etc/xtp/quote_config.ini";
        config.xtp.index_instruments = {"000300", "000905"};

        auto session = basis_monitor::CreateMarketDataSession(config, listener);
        assert(session != nullptr);
    }

    return 0;
}
