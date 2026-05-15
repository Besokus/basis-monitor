#pragma once

#include <memory>
#include <string>

#include "ThostFtdcMdApi.h"

namespace basis_monitor
{

struct CtpDataCollectStatus
{
    bool available = false;
    std::string version;
    std::string detail;
};

struct CtpSystemInfoProbeResult
{
    bool available = false;
    int ret_code = -1;
    int payload_length = 0;
    std::string detail;
};

class CtpEnvironment
{
public:
    virtual ~CtpEnvironment() = default;

    virtual CThostFtdcMdApi* CreateMdApi(const std::string& flow_dir,
                                         bool use_udp,
                                         bool use_multicast,
                                         bool production_mode) = 0;
    virtual std::string GetMdApiVersion() const = 0;
    virtual CtpDataCollectStatus GetDataCollectStatus() const = 0;
    virtual CtpSystemInfoProbeResult ProbeSystemInfo() const = 0;
};

std::shared_ptr<CtpEnvironment> CreateDefaultCtpEnvironment();

} // namespace basis_monitor
