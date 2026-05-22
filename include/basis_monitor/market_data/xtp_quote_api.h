#pragma once

#include <memory>

#include "xtpx_api_data_type.h"
#include "xtpx_api_struct_common.h"
#include "xtpx_quote_api.h"

namespace basis_monitor
{

class IXtpQuoteApi
{
public:
    virtual ~IXtpQuoteApi() = default;

    virtual void RegisterSpi(XTPX::API::QuoteSpi* spi) = 0;
    virtual void SetHeartBeatInterval(uint32_t interval) = 0;
    virtual bool SetConfigFile(const char* filename) = 0;
    virtual int Login(
        const char* ip,
        int port,
        const char* user,
        const char* password,
        XTPX::API::XTP_PROTOCOL_TYPE sock_type,
        const char* local_ip) = 0;
    virtual int SubscribeMarketData(char* ticker[], int count, XTPX::API::XTP_EXCHANGE_TYPE exchange_id) = 0;
    virtual int Logout() = 0;
    virtual XTPX::API::XTPRI* GetApiLastError() = 0;
};

std::unique_ptr<IXtpQuoteApi> CreateDefaultXtpQuoteApi(int client_id, const char* save_file_path);

} // namespace basis_monitor
