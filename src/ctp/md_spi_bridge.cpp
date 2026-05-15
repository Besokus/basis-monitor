#include "basis_monitor/ctp/md_spi_bridge.h"

#include <vector>

#include "basis_monitor/logging/logger.h"

namespace basis_monitor
{

MdSpiBridge::MdSpiBridge(CThostFtdcMdApi* api, const CtpConfig& config, MdListener& listener)
    : api_(api), config_(config), listener_(listener)
{
    connected_event_ = CreateEvent(nullptr, false, false, nullptr);
    login_event_ = CreateEvent(nullptr, false, false, nullptr);
    subscription_event_ = CreateEvent(nullptr, false, false, nullptr);
    first_market_data_event_ = CreateEvent(nullptr, false, false, nullptr);
}

MdSpiBridge::~MdSpiBridge()
{
    CloseHandle(connected_event_);
    CloseHandle(login_event_);
    CloseHandle(subscription_event_);
    CloseHandle(first_market_data_event_);
}

double MdSpiBridge::NormalizePrice(double price)
{
    return price > 10000000.0 ? 0.0 : price;
}

const char* MdSpiBridge::GetRequestReturnText(int ret_code)
{
    switch (ret_code)
    {
    case 0: return "REQUEST_ACCEPTED";
    case -1: return "NETWORK_SEND_FAILED";
    case -2: return "REQUESTS_OVERFLOW";
    case -3: return "REQUEST_TOO_FREQUENT";
    default: return "UNKNOWN_REQUEST_RETURN";
    }
}

const char* MdSpiBridge::GetFrontDisconnectReasonText(int nReason)
{
    switch (nReason)
    {
    case 0x1001: return "NETWORK_READ_FAIL";
    case 0x1002: return "NETWORK_WRITE_FAIL";
    case 0x2001: return "HEARTBEAT_TIMEOUT";
    case 0x2002: return "HEARTBEAT_SEND_FAIL";
    case 0x2003: return "RECV_ERROR_MESSAGE";
    default: return "UNKNOWN_FRONT_DISCONNECT_REASON";
    }
}

void MdSpiBridge::LogRequestReturnStatus(const char* request_name, int ret_code)
{
    Log("[REQ_STATUS] request=[%s], ret=[%d], retText=[%s]\n",
        request_name,
        ret_code,
        GetRequestReturnText(ret_code));
}

void MdSpiBridge::LogRspError(const char* callback_name, CThostFtdcRspInfoField* pRspInfo)
{
    if (pRspInfo == nullptr)
    {
        Log("[%s] pRspInfo is null\n", callback_name);
        return;
    }

    if (pRspInfo->ErrorID == 0)
    {
        return;
    }

    Log("[API_ERROR] callback=[%s], code=[%d], raw=[%s]\n",
        callback_name,
        pRspInfo->ErrorID,
        pRspInfo->ErrorMsg);
}

void MdSpiBridge::ReqUserLogin()
{
    BeginLoginRequest();
}

void MdSpiBridge::BeginLoginRequest()
{
    ResetEvent(login_event_);
    login_request_ret_code_ = 0;
    login_succeeded_ = false;

    CThostFtdcReqUserLoginField req = {0};
    strcpy_s(req.BrokerID, config_.broker_id.c_str());
    strcpy_s(req.UserID, config_.user_id.c_str());
    strcpy_s(req.Password, config_.password.c_str());
    login_request_ret_code_ = api_->ReqUserLogin(&req, 111);
    LogRequestReturnStatus("MdApi::ReqUserLogin", login_request_ret_code_);
    if (login_request_ret_code_ < 0)
    {
        login_succeeded_ = false;
        SetEvent(login_event_);
    }
}

void MdSpiBridge::SubscribeMarketData()
{
    BeginSubscriptionRequest();
}

void MdSpiBridge::BeginSubscriptionRequest()
{
    ResetEvent(subscription_event_);
    subscription_request_ret_code_ = 0;
    subscription_error_seen_ = false;
    subscription_succeeded_ = false;
    std::vector<char*> instrument_ptrs;
    instrument_ptrs.reserve(config_.instruments.size());
    for (const auto& instrument : config_.instruments)
    {
        instrument_ptrs.push_back(const_cast<char*>(instrument.c_str()));
    }

    subscription_request_ret_code_ = api_->SubscribeMarketData(instrument_ptrs.data(), static_cast<int>(instrument_ptrs.size()));
    LogRequestReturnStatus("MdApi::SubscribeMarketData", subscription_request_ret_code_);
    if (subscription_request_ret_code_ < 0)
    {
        subscription_succeeded_ = false;
        SetEvent(subscription_event_);
    }
}

void MdSpiBridge::OnFrontConnected()
{
    Log("<OnFrontConnected>\n");
    Log("</OnFrontConnected>\n");
    BeginLoginRequest();
    SetEvent(connected_event_);
}

void MdSpiBridge::OnFrontDisconnected(int nReason)
{
    Log("<OnFrontDisconnected>\n");
    Log("\tnReason [%d] [%s]\n", nReason, GetFrontDisconnectReasonText(nReason));
    Log("</OnFrontDisconnected>\n");
    login_succeeded_ = false;
    subscription_succeeded_ = false;
    subscription_error_seen_ = false;
}

void MdSpiBridge::OnHeartBeatWarning(int nTimeLapse)
{
    Log("<OnHeartBeatWarning>\n");
    Log("\tnTimeLapse [%d]\n", nTimeLapse);
    Log("</OnHeartBeatWarning>\n");
}

void MdSpiBridge::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    Log("<OnRspUserLogin>\n");
    if (pRspUserLogin != nullptr)
    {
        Log("\tTradingDay [%s]\n", pRspUserLogin->TradingDay);
        Log("\tLoginTime [%s]\n", pRspUserLogin->LoginTime);
        Log("\tBrokerID [%s]\n", pRspUserLogin->BrokerID);
        Log("\tUserID [%s]\n", pRspUserLogin->UserID);
        Log("\tSystemName [%s]\n", pRspUserLogin->SystemName);
        Log("\tMaxOrderRef [%s]\n", pRspUserLogin->MaxOrderRef);
        Log("\tSHFETime [%s]\n", pRspUserLogin->SHFETime);
        Log("\tDCETime [%s]\n", pRspUserLogin->DCETime);
        Log("\tCZCETime [%s]\n", pRspUserLogin->CZCETime);
        Log("\tFFEXTime [%s]\n", pRspUserLogin->FFEXTime);
        Log("\tINETime [%s]\n", pRspUserLogin->INETime);
        Log("\tFrontID [%d]\n", pRspUserLogin->FrontID);
        Log("\tSessionID [%d]\n", pRspUserLogin->SessionID);
    }
    if (pRspInfo != nullptr)
    {
        Log("\tErrorMsg [%s]\n", pRspInfo->ErrorMsg);
        Log("\tErrorID [%d]\n", pRspInfo->ErrorID);
    }
    Log("\tnRequestID [%d]\n", nRequestID);
    Log("\tbIsLast [%d]\n", bIsLast ? 1 : 0);
    Log("</OnRspUserLogin>\n");

    LogRspError("Md::OnRspUserLogin", pRspInfo);
    login_succeeded_ = (pRspInfo == nullptr || pRspInfo->ErrorID == 0);
    if (login_succeeded_)
    {
        BeginSubscriptionRequest();
    }
    SetEvent(login_event_);
}

void MdSpiBridge::OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
    Log("<OnRspSubMarketData>\n");
    if (pSpecificInstrument != nullptr)
    {
        Log("\tInstrumentID [%s]\n", pSpecificInstrument->InstrumentID);
    }
    if (pRspInfo != nullptr)
    {
        Log("\tErrorMsg [%s]\n", pRspInfo->ErrorMsg);
        Log("\tErrorID [%d]\n", pRspInfo->ErrorID);
    }
    Log("\tnRequestID [%d]\n", nRequestID);
    Log("\tbIsLast [%d]\n", bIsLast ? 1 : 0);
    Log("</OnRspSubMarketData>\n");

    LogRspError("Md::OnRspSubMarketData", pRspInfo);
    if (pRspInfo != nullptr && pRspInfo->ErrorID != 0)
    {
        subscription_error_seen_ = true;
    }
    if (bIsLast)
    {
        subscription_succeeded_ = !subscription_error_seen_;
        SetEvent(subscription_event_);
    }
}

void MdSpiBridge::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData)
{
    if (pDepthMarketData == nullptr)
    {
        return;
    }

    MarketTick tick = {};
    tick.instrument_id = pDepthMarketData->InstrumentID;
    tick.update_time = pDepthMarketData->UpdateTime;
    tick.update_millisec = pDepthMarketData->UpdateMillisec;
    tick.provider = MarketDataProviderType::Ctp;
    tick.instrument_type = MarketTickInstrumentType::Future;
    tick.last_price = NormalizePrice(pDepthMarketData->LastPrice);
    tick.bid_price_1 = NormalizePrice(pDepthMarketData->BidPrice1);
    tick.bid_volume_1 = pDepthMarketData->BidVolume1;
    tick.ask_price_1 = NormalizePrice(pDepthMarketData->AskPrice1);
    tick.ask_volume_1 = pDepthMarketData->AskVolume1;
    tick.volume = pDepthMarketData->Volume;

    if (!has_received_first_market_data_)
    {
        has_received_first_market_data_ = true;
        Log("[MARKET_DATA_OK] InstrumentID=[%s] UpdateTime=[%s.%03d] LastPrice=[%.8lf] Bid1=[%.8lf@%d] Ask1=[%.8lf@%d] Volume=[%d]\n",
            tick.instrument_id.c_str(),
            tick.update_time.c_str(),
            tick.update_millisec,
            tick.last_price,
            tick.bid_price_1,
            tick.bid_volume_1,
            tick.ask_price_1,
            tick.ask_volume_1,
            tick.volume);
        SetEvent(first_market_data_event_);
    }

    Log("[MD_TICK] InstrumentID=[%s] UpdateTime=[%s.%03d] LastPrice=[%.8lf] Bid1=[%.8lf@%d] Ask1=[%.8lf@%d] Volume=[%d]\n",
        tick.instrument_id.c_str(),
        tick.update_time.c_str(),
        tick.update_millisec,
        tick.last_price,
        tick.bid_price_1,
        tick.bid_volume_1,
        tick.ask_price_1,
        tick.ask_volume_1,
        tick.volume);

    listener_.OnTick(tick);
}

} // namespace basis_monitor
