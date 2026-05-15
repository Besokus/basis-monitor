#include "basis_monitor/market_data/xtp_quote_api.h"

#include <memory>
#include <stdexcept>

namespace basis_monitor
{

namespace
{

class XtpQuoteApiAdapter final : public IXtpQuoteApi
{
public:
    explicit XtpQuoteApiAdapter(XTPX::API::QuoteApi* api)
        : api_(api)
    {
        if (api_ == nullptr)
        {
            throw std::runtime_error("Failed to create XTP quote api instance");
        }
    }

    ~XtpQuoteApiAdapter() override
    {
        if (api_ != nullptr)
        {
            api_->Release();
            api_ = nullptr;
        }
    }

    void RegisterSpi(XTPX::API::QuoteSpi* spi) override
    {
        api_->RegisterSpi(spi);
    }

    void SetHeartBeatInterval(uint32_t interval) override
    {
        api_->SetHeartBeatInterval(interval);
    }

    bool SetConfigFile(const char* filename) override
    {
        return api_->SetConfigFile(filename);
    }

    int Login(
        const char* ip,
        int port,
        const char* user,
        const char* password,
        XTPX::API::XTP_PROTOCOL_TYPE sock_type,
        const char* local_ip) override
    {
        return api_->Login(ip, port, user, password, sock_type, local_ip);
    }

    int SubscribeMarketData(char* ticker[], int count, XTPX::API::XTP_EXCHANGE_TYPE exchange_id) override
    {
        return api_->SubscribeMarketData(ticker, count, exchange_id);
    }

    int Logout() override
    {
        return api_->Logout();
    }

    XTPX::API::XTPRI* GetApiLastError() override
    {
        return api_->GetApiLastError();
    }

private:
    XTPX::API::QuoteApi* api_ = nullptr;
};

} // namespace

std::unique_ptr<IXtpQuoteApi> CreateDefaultXtpQuoteApi(int client_id, const char* save_file_path)
{
    return std::make_unique<XtpQuoteApiAdapter>(
        XTPX::API::QuoteApi::CreateQuoteApi(
            static_cast<uint8_t>(client_id),
            save_file_path,
            XTPX::API::XTP_LOG_LEVEL_INFO,
            true));
}

} // namespace basis_monitor
