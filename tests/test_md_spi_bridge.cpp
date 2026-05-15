#include <cassert>
#include <string>
#include <vector>

#include "ThostFtdcMdApi.h"
#include "basis_monitor/config/app_config.h"
#include "basis_monitor/ctp/md_listener.h"
#include "basis_monitor/ctp/md_spi_bridge.h"

namespace
{

class NullListener final : public basis_monitor::MdListener
{
public:
    void OnTick(const basis_monitor::MarketTick& tick) override
    {
        ticks.push_back(tick);
    }

    std::vector<basis_monitor::MarketTick> ticks;
};

class FakeMdApi final : public CThostFtdcMdApi
{
public:
    int login_calls = 0;
    int subscribe_calls = 0;
    std::vector<std::string> subscribed_instruments;

    void Release() override {}
    void Init() override {}
    int Join() override { return 0; }
    const char* GetTradingDay() override { return "2026-04-01"; }
    void RegisterFront(char*) override {}
    void RegisterNameServer(char*) override {}
    void RegisterFensUserInfo(CThostFtdcFensUserInfoField*) override {}
    void RegisterSpi(CThostFtdcMdSpi*) override {}

    int SubscribeMarketData(char* ppInstrumentID[], int nCount) override
    {
        ++subscribe_calls;
        subscribed_instruments.clear();
        for (int index = 0; index < nCount; ++index)
        {
            subscribed_instruments.emplace_back(ppInstrumentID[index]);
        }
        return 0;
    }

    int UnSubscribeMarketData(char*[], int) override { return 0; }
    int SubscribeForQuoteRsp(char*[], int) override { return 0; }
    int UnSubscribeForQuoteRsp(char*[], int) override { return 0; }

    int ReqUserLogin(CThostFtdcReqUserLoginField*, int) override
    {
        ++login_calls;
        return 0;
    }

    int ReqUserLogout(CThostFtdcUserLogoutField*, int) override { return 0; }
    int ReqQryMulticastInstrument(CThostFtdcQryMulticastInstrumentField*, int) override { return 0; }
};

} // namespace

int main()
{
    basis_monitor::CtpConfig config = {};
    config.broker_id = "9999";
    config.user_id = "demo";
    config.password = "secret";
    config.instruments = {"IC2604", "IF2604"};

    FakeMdApi api;
    NullListener listener;
    basis_monitor::MdSpiBridge bridge(&api, config, listener);

    bridge.OnFrontConnected();
    assert(api.login_calls == 1);
    assert(bridge.LoginRequestAccepted());

    CThostFtdcRspInfoField success_rsp = {};
    success_rsp.ErrorID = 0;
    bridge.OnRspUserLogin(nullptr, &success_rsp, 1, true);
    assert(bridge.LoginSucceeded());
    assert(api.subscribe_calls == 1);
    assert(api.subscribed_instruments.size() == 2);
    assert(api.subscribed_instruments[0] == "IC2604");

    CThostFtdcSpecificInstrumentField sub_ic = {};
    strcpy_s(sub_ic.InstrumentID, sizeof(sub_ic.InstrumentID), "IC2604");
    bridge.OnRspSubMarketData(&sub_ic, &success_rsp, 1, false);
    CThostFtdcSpecificInstrumentField sub_if = {};
    strcpy_s(sub_if.InstrumentID, sizeof(sub_if.InstrumentID), "IF2604");
    bridge.OnRspSubMarketData(&sub_if, &success_rsp, 1, true);
    assert(bridge.SubscriptionSucceeded());

    bridge.OnFrontDisconnected(0x2001);
    bridge.OnFrontConnected();
    assert(api.login_calls == 2);

    bridge.OnRspUserLogin(nullptr, &success_rsp, 2, true);
    assert(api.subscribe_calls == 2);

    bridge.OnRspSubMarketData(&sub_ic, &success_rsp, 2, false);
    bridge.OnRspSubMarketData(&sub_if, &success_rsp, 2, true);
    assert(bridge.SubscriptionSucceeded());

    CThostFtdcDepthMarketDataField market_data = {};
    strcpy_s(market_data.InstrumentID, sizeof(market_data.InstrumentID), "IC2604");
    strcpy_s(market_data.UpdateTime, sizeof(market_data.UpdateTime), "09:31:15");
    market_data.UpdateMillisec = 500;
    market_data.LastPrice = 6123.4;
    market_data.BidPrice1 = 6123.2;
    market_data.BidVolume1 = 11;
    market_data.AskPrice1 = 6123.6;
    market_data.AskVolume1 = 15;
    market_data.Volume = 88;
    bridge.OnRtnDepthMarketData(&market_data);
    assert(listener.ticks.size() == 1);
    assert(listener.ticks.front().instrument_id == "IC2604");
    assert(listener.ticks.front().provider == basis_monitor::MarketDataProviderType::Ctp);
    assert(listener.ticks.front().instrument_type == basis_monitor::MarketTickInstrumentType::Future);

    return 0;
}
