#pragma once

#include <string>
#include <vector>

#include "ThostFtdcMdApi.h"
#include "basis_monitor/config/app_config.h"
#include "basis_monitor/ctp/md_listener.h"
#include "basis_monitor/platform/linux_compat.h"

namespace basis_monitor
{

class MdSpiBridge : public CThostFtdcMdSpi
{
public:
    MdSpiBridge(CThostFtdcMdApi* api, const CtpConfig& config, MdListener& listener);
    ~MdSpiBridge();

    void ReqUserLogin();
    void SubscribeMarketData();

    HANDLE ConnectedEvent() const { return connected_event_; }
    HANDLE LoginEvent() const { return login_event_; }
    HANDLE SubscriptionEvent() const { return subscription_event_; }
    HANDLE FirstMarketDataEvent() const { return first_market_data_event_; }

    bool LoginRequestAccepted() const { return login_request_ret_code_ >= 0; }
    bool LoginSucceeded() const { return login_succeeded_; }
    bool SubscriptionRequestAccepted() const { return subscription_request_ret_code_ >= 0; }
    bool SubscriptionSucceeded() const { return subscription_succeeded_; }

    void OnFrontConnected() override;
    void OnFrontDisconnected(int nReason) override;
    void OnHeartBeatWarning(int nTimeLapse) override;
    void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData) override;

private:
    static double NormalizePrice(double price);
    static const char* GetRequestReturnText(int ret_code);
    static const char* GetFrontDisconnectReasonText(int nReason);
    static void LogRequestReturnStatus(const char* request_name, int ret_code);
    static void LogRspError(const char* callback_name, CThostFtdcRspInfoField* pRspInfo);
    void BeginLoginRequest();
    void BeginSubscriptionRequest();

    CThostFtdcMdApi* api_ = nullptr;
    const CtpConfig& config_;
    MdListener& listener_;
    HANDLE connected_event_ = nullptr;
    HANDLE login_event_ = nullptr;
    HANDLE subscription_event_ = nullptr;
    HANDLE first_market_data_event_ = nullptr;
    int login_request_ret_code_ = 0;
    bool login_succeeded_ = false;
    int subscription_request_ret_code_ = 0;
    bool subscription_error_seen_ = false;
    bool subscription_succeeded_ = false;
    bool has_received_first_market_data_ = false;
};

} // namespace basis_monitor
