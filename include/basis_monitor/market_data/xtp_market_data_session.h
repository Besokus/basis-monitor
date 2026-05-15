#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>

#include "basis_monitor/config/app_config.h"
#include "basis_monitor/market_data/market_data_session.h"

namespace basis_monitor
{

class IXtpQuoteApi;
class MdListener;

class XtpMarketDataSession final : public IMarketDataSession
{
public:
    using QuoteApiFactory = std::function<std::unique_ptr<IXtpQuoteApi>()>;

    XtpMarketDataSession(const AppConfig& config, MdListener& listener, QuoteApiFactory factory = {});
    ~XtpMarketDataSession() override;

    bool Start() override;
    bool WaitForFirstMarketData(unsigned long timeout_ms) const override;
    void Stop() override;

private:
    class XtpQuoteSpiBridge;

    bool ConfigureAndLogin();
    bool LoginAndSubscribe();
    bool SubscribeCurrentInstruments();

    AppConfig config_;
    MdListener& listener_;
    QuoteApiFactory factory_;
    std::unique_ptr<IXtpQuoteApi> api_;
    std::unique_ptr<XtpQuoteSpiBridge> spi_bridge_;

    mutable std::mutex first_tick_mutex_;
    mutable std::condition_variable first_tick_cv_;
    bool first_market_data_received_ = false;
    bool started_ = false;
};

} // namespace basis_monitor
