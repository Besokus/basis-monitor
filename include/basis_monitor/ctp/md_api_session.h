#pragma once

#include <memory>

#include "ThostFtdcMdApi.h"
#include "basis_monitor/config/app_config.h"
#include "basis_monitor/ctp/ctp_environment.h"
#include "basis_monitor/ctp/md_listener.h"

namespace basis_monitor
{

class MdSpiBridge;

class MdApiSession
{
public:
    MdApiSession(const AppConfig& config, MdListener& listener);
    MdApiSession(const AppConfig& config, MdListener& listener, std::shared_ptr<CtpEnvironment> environment);
    ~MdApiSession();

    bool Start();
    bool WaitForFirstMarketData(unsigned long timeout_ms) const;

private:
    AppConfig config_;
    MdListener& listener_;
    std::shared_ptr<CtpEnvironment> environment_;
    CThostFtdcMdApi* api_ = nullptr;
    std::unique_ptr<MdSpiBridge> spi_;
};

} // namespace basis_monitor
