#include "basis_monitor/ctp/ctp_environment.h"

#include <utility>

#include "basis_monitor/logging/logger.h"

#if defined(BASIS_MONITOR_HAS_CTP_DATA_COLLECT)
#include "DataCollect.h"
#endif

namespace basis_monitor
{

namespace
{

class DefaultCtpEnvironment final : public CtpEnvironment
{
public:
    CThostFtdcMdApi* CreateMdApi(const std::string& flow_dir,
                                 bool use_udp,
                                 bool use_multicast,
                                 bool production_mode) override
    {
        return CThostFtdcMdApi::CreateFtdcMdApi(
            flow_dir.c_str(),
            use_udp,
            use_multicast,
            production_mode);
    }

    std::string GetMdApiVersion() const override
    {
        const char* version = CThostFtdcMdApi::GetApiVersion();
        return version == nullptr ? std::string() : std::string(version);
    }

    CtpDataCollectStatus GetDataCollectStatus() const override
    {
#if defined(BASIS_MONITOR_HAS_CTP_DATA_COLLECT)
        const char* version = CTP_GetDataCollectApiVersion();
        CtpDataCollectStatus status;
        status.available = version != nullptr;
        status.version = version == nullptr ? std::string() : std::string(version);
        status.detail = status.available ? std::string() : "CTP_GetDataCollectApiVersion returned null";
        return status;
#else
        return {false, "", "LinuxDataCollect support is not compiled in"};
#endif
    }

    CtpSystemInfoProbeResult ProbeSystemInfo() const override
    {
#if defined(BASIS_MONITOR_HAS_CTP_DATA_COLLECT)
        char system_info[512] = {0};
        int len = static_cast<int>(sizeof(system_info));
        const int ret = CTP_GetSystemInfo(system_info, len);

        CtpSystemInfoProbeResult result;
        result.available = true;
        result.ret_code = ret;
        result.payload_length = len;
        if (ret == 0)
        {
            result.detail = "CTP_GetSystemInfo succeeded";
        }
        else
        {
            result.detail = "CTP_GetSystemInfo returned non-zero";
        }
        return result;
#else
        return {false, -1, 0, "LinuxDataCollect support is not compiled in"};
#endif
    }
};

} // namespace

std::shared_ptr<CtpEnvironment> CreateDefaultCtpEnvironment()
{
    return std::make_shared<DefaultCtpEnvironment>();
}

} // namespace basis_monitor
