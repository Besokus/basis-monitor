#include "basis_monitor/ctp/md_api_session.h"

#include <stdexcept>
#include <utility>

#include "basis_monitor/ctp/md_spi_bridge.h"
#include "basis_monitor/logging/logger.h"
#include "basis_monitor/platform/linux_compat.h"

namespace basis_monitor
{

MdApiSession::MdApiSession(const AppConfig& config, MdListener& listener)
    : config_(config), listener_(listener)
{
    environment_ = CreateDefaultCtpEnvironment();
}

MdApiSession::MdApiSession(const AppConfig& config, MdListener& listener, std::shared_ptr<CtpEnvironment> environment)
    : config_(config), listener_(listener), environment_(std::move(environment))
{
    if (environment_ == nullptr)
    {
        environment_ = CreateDefaultCtpEnvironment();
    }
}

MdApiSession::~MdApiSession()
{
    if (api_ != nullptr)
    {
        api_->Release();
        api_ = nullptr;
    }
}

bool MdApiSession::Start()
{
    Log("[CTP_RUNTIME] MdApiVersion=[%s]\n", environment_->GetMdApiVersion().c_str());
    const auto data_collect = environment_->GetDataCollectStatus();
    if (data_collect.available)
    {
        Log("[CTP_RUNTIME] DataCollectVersion=[%s]\n", data_collect.version.c_str());
    }
    else
    {
        Log("[CTP_RUNTIME] DataCollectUnavailable detail=[%s]\n", data_collect.detail.c_str());
    }

    const auto system_info = environment_->ProbeSystemInfo();
    if (system_info.available)
    {
        Log("[CTP_RUNTIME] SystemInfoProbe ret=[%d] len=[%d] detail=[%s]\n",
            system_info.ret_code,
            system_info.payload_length,
            system_info.detail.c_str());
    }
    else
    {
        Log("[CTP_RUNTIME] SystemInfoProbeUnavailable detail=[%s]\n", system_info.detail.c_str());
    }

    api_ = environment_->CreateMdApi(config_.ctp.flow_dir, false, false, true);
    if (api_ == nullptr)
    {
        throw std::runtime_error("CreateFtdcMdApi returned null");
    }

    spi_ = std::make_unique<MdSpiBridge>(api_, config_.ctp, listener_);
    api_->RegisterSpi(spi_.get());
    api_->RegisterFront(const_cast<char*>(config_.ctp.front_md_addr.c_str()));
    api_->Init();

    if (WaitForSingleObject(spi_->ConnectedEvent(), INFINITE) != WAIT_OBJECT_0)
    {
        Log("[MARKET_DATA_START_FAILED] Wait for OnFrontConnected timed out.\n");
        return false;
    }

    if (WaitForSingleObject(spi_->LoginEvent(), INFINITE) != WAIT_OBJECT_0)
    {
        Log("[MARKET_DATA_START_FAILED] Wait for OnRspUserLogin timed out.\n");
        return false;
    }
    if (!spi_->LoginRequestAccepted())
    {
        Log("[MARKET_DATA_START_FAILED] ReqUserLogin returned a negative status before callback.\n");
        return false;
    }
    if (!spi_->LoginSucceeded())
    {
        Log("[MARKET_DATA_START_FAILED] Login failed.\n");
        return false;
    }

    if (WaitForSingleObject(spi_->SubscriptionEvent(), INFINITE) != WAIT_OBJECT_0)
    {
        Log("[MARKET_DATA_START_FAILED] Wait for OnRspSubMarketData timed out.\n");
        return false;
    }
    if (!spi_->SubscriptionRequestAccepted())
    {
        Log("[MARKET_DATA_START_FAILED] SubscribeMarketData returned a negative status before callback.\n");
        return false;
    }
    if (!spi_->SubscriptionSucceeded())
    {
        Log("[MARKET_DATA_START_FAILED] SubscribeMarketData failed.\n");
        return false;
    }

    return true;
}

bool MdApiSession::WaitForFirstMarketData(unsigned long timeout_ms) const
{
    return spi_ != nullptr && WaitForSingleObject(spi_->FirstMarketDataEvent(), timeout_ms) == WAIT_OBJECT_0;
}

} // namespace basis_monitor
