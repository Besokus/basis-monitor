#include "basis_monitor/market_data/market_data_session_factory.h"

#include <memory>
#include <stdexcept>
#include <string>

#include "basis_monitor/ctp/md_api_session.h"
#include "basis_monitor/ctp/md_listener.h"
#include "basis_monitor/market_data/market_data_session.h"
#include "basis_monitor/market_data/xtp_market_data_session.h"

namespace basis_monitor
{

namespace
{

class CtpMarketDataSession final : public IMarketDataSession
{
public:
    CtpMarketDataSession(const AppConfig& config, MdListener& listener)
        : config_(config), listener_(listener)
    {
    }

    bool Start() override
    {
        session_ = std::make_unique<MdApiSession>(config_, listener_);
        return session_->Start();
    }

    bool WaitForFirstMarketData(unsigned long timeout_ms) const override
    {
        return session_ != nullptr && session_->WaitForFirstMarketData(timeout_ms);
    }

    void Stop() override
    {
        session_.reset();
    }

private:
    AppConfig config_;
    MdListener& listener_;
    std::unique_ptr<MdApiSession> session_;
};

class DualMarketDataSession final : public IMarketDataSession
{
public:
    DualMarketDataSession(std::unique_ptr<IMarketDataSession> ctp_session,
                          std::unique_ptr<IMarketDataSession> xtp_session)
        : ctp_session_(std::move(ctp_session)),
          xtp_session_(std::move(xtp_session))
    {
    }

    bool Start() override
    {
        if (ctp_session_ == nullptr || xtp_session_ == nullptr)
        {
            return false;
        }

        if (!ctp_session_->Start())
        {
            return false;
        }

        if (!xtp_session_->Start())
        {
            ctp_session_->Stop();
            return false;
        }

        return true;
    }

    bool WaitForFirstMarketData(unsigned long timeout_ms) const override
    {
        const bool ctp_ready = ctp_session_ != nullptr && ctp_session_->WaitForFirstMarketData(timeout_ms);
        if (!ctp_ready)
        {
            return false;
        }

        if (xtp_session_ != nullptr && !xtp_session_->WaitForFirstMarketData(timeout_ms))
        {
            return true;
        }

        return true;
    }

    void Stop() override
    {
        if (xtp_session_ != nullptr)
        {
            xtp_session_->Stop();
        }
        if (ctp_session_ != nullptr)
        {
            ctp_session_->Stop();
        }
    }

private:
    std::unique_ptr<IMarketDataSession> ctp_session_;
    std::unique_ptr<IMarketDataSession> xtp_session_;
};

void RequireNonEmpty(const std::string& value, const char* field_name)
{
    if (value.empty())
    {
        throw std::runtime_error(std::string("XTP config missing required field: ") + field_name);
    }
}

void ValidateXtpConfig(const XtpConfig& config)
{
    RequireNonEmpty(config.server_ip, "XtpServerIp");
    if (config.server_port <= 0)
    {
        throw std::runtime_error("XTP config missing required field: XtpServerPort");
    }
    RequireNonEmpty(config.user, "XtpUser");
    RequireNonEmpty(config.password, "XtpPassword");
    if (config.client_id <= 0)
    {
        throw std::runtime_error("XTP config missing required field: XtpClientId");
    }
    RequireNonEmpty(config.protocol, "XtpProtocol");
    RequireNonEmpty(config.config_file, "XtpConfigFile");
}

} // namespace

std::unique_ptr<IMarketDataSession> CreateMarketDataSession(const AppConfig& config, MdListener& listener)
{
    // Check enable flags first (new dual mode support)
    bool ctp_enabled = config.ctp.enable_ctp_market_data;
    bool xtp_enabled = config.xtp.enable_xtp_market_data;

    if (ctp_enabled && xtp_enabled)
    {
        ValidateXtpConfig(config.xtp);
        return std::make_unique<DualMarketDataSession>(
            std::make_unique<CtpMarketDataSession>(config, listener),
            std::make_unique<XtpMarketDataSession>(config, listener));
    }

    // If only one is enabled, use that provider
    if (ctp_enabled)
    {
        return std::make_unique<CtpMarketDataSession>(config, listener);
    }

    if (xtp_enabled)
    {
        ValidateXtpConfig(config.xtp);
        return std::make_unique<XtpMarketDataSession>(config, listener);
    }

    // Fallback to legacy MarketDataProvider setting
    switch (config.market_data_provider)
    {
    case MarketDataProviderType::Ctp:
        return std::make_unique<CtpMarketDataSession>(config, listener);
    case MarketDataProviderType::Xtp:
        ValidateXtpConfig(config.xtp);
        return std::make_unique<XtpMarketDataSession>(config, listener);
    default:
        throw std::runtime_error("No market data provider enabled. Set EnableCtpMarketData=true or EnableXtpMarketData=true in config");
    }
}

} // namespace basis_monitor
