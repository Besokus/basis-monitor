#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>

#include "basis_monitor/config/app_config.h"
#include "basis_monitor/ctp/ctp_environment.h"
#include "basis_monitor/ctp/md_api_session.h"
#include "basis_monitor/ctp/md_listener.h"
#include "basis_monitor/logging/logger.h"
#include "basis_monitor/platform/linux_compat.h"

namespace
{

std::string ReadFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

class NullListener final : public basis_monitor::MdListener
{
public:
    void OnTick(const basis_monitor::MarketTick&) override
    {
    }
};

class FakeMdApi final : public CThostFtdcMdApi
{
public:
    void SetSpi(CThostFtdcMdSpi* spi)
    {
        spi_ = spi;
    }

    void Release() override
    {
    }

    void Init() override
    {
        assert(spi_ != nullptr);
        spi_->OnFrontConnected();
    }

    int Join() override
    {
        return 0;
    }

    const char* GetTradingDay() override
    {
        return "20260408";
    }

    void RegisterFront(char* pszFrontAddress) override
    {
        front_address_ = pszFrontAddress == nullptr ? "" : pszFrontAddress;
    }

    void RegisterNameServer(char*) override
    {
    }

    void RegisterFensUserInfo(CThostFtdcFensUserInfoField*) override
    {
    }

    void RegisterSpi(CThostFtdcMdSpi* pSpi) override
    {
        spi_ = pSpi;
    }

    int SubscribeMarketData(char*[], int) override
    {
        CThostFtdcSpecificInstrumentField instrument = {};
        strcpy_s(instrument.InstrumentID, "IF2606");

        CThostFtdcRspInfoField rsp = {};
        rsp.ErrorID = 0;

        spi_->OnRspSubMarketData(&instrument, &rsp, 222, true);

        CThostFtdcDepthMarketDataField depth = {};
        strcpy_s(depth.InstrumentID, "IF2606");
        strcpy_s(depth.UpdateTime, "10:15:30");
        depth.UpdateMillisec = 123;
        depth.LastPrice = 3500.5;
        depth.BidPrice1 = 3500.4;
        depth.BidVolume1 = 10;
        depth.AskPrice1 = 3500.6;
        depth.AskVolume1 = 12;
        depth.Volume = 99;
        spi_->OnRtnDepthMarketData(&depth);

        return 0;
    }

    int UnSubscribeMarketData(char*[], int) override
    {
        return 0;
    }

    int SubscribeForQuoteRsp(char*[], int) override
    {
        return 0;
    }

    int UnSubscribeForQuoteRsp(char*[], int) override
    {
        return 0;
    }

    int ReqUserLogin(CThostFtdcReqUserLoginField*, int nRequestID) override
    {
        CThostFtdcRspUserLoginField login = {};
        strcpy_s(login.TradingDay, "20260408");
        strcpy_s(login.LoginTime, "10:15:30");
        strcpy_s(login.BrokerID, "7080");
        strcpy_s(login.UserID, "demo");
        strcpy_s(login.SystemName, "test_system");

        CThostFtdcRspInfoField rsp = {};
        rsp.ErrorID = 0;

        spi_->OnRspUserLogin(&login, &rsp, nRequestID, true);
        return 0;
    }

    int ReqUserLogout(CThostFtdcUserLogoutField*, int) override
    {
        return 0;
    }

    int ReqQryMulticastInstrument(CThostFtdcQryMulticastInstrumentField*, int) override
    {
        return 0;
    }

    std::string front_address_;

private:
    CThostFtdcMdSpi* spi_ = nullptr;
};

class FakeCtpEnvironment final : public basis_monitor::CtpEnvironment
{
public:
    CThostFtdcMdApi* CreateMdApi(const std::string& flow_dir,
                                 bool use_udp,
                                 bool use_multicast,
                                 bool production_mode) override
    {
        last_flow_dir = flow_dir;
        last_use_udp = use_udp;
        last_use_multicast = use_multicast;
        last_production_mode = production_mode;
        return &api;
    }

    std::string GetMdApiVersion() const override
    {
        return "md-formal-6.7.11";
    }

    basis_monitor::CtpDataCollectStatus GetDataCollectStatus() const override
    {
        return {true, "collect-6.7.0", ""};
    }

    basis_monitor::CtpSystemInfoProbeResult ProbeSystemInfo() const override
    {
        return {true, 0, 344, "CTP_GetSystemInfo succeeded"};
    }

    mutable FakeMdApi api;
    std::string last_flow_dir;
    bool last_use_udp = true;
    bool last_use_multicast = true;
    bool last_production_mode = false;
};

basis_monitor::AppConfig MakeConfig()
{
    basis_monitor::AppConfig config = {};
    config.ctp.front_md_addr = "tcp://10.101.1.102:51213";
    config.ctp.broker_id = "7080";
    config.ctp.user_id = "demo";
    config.ctp.password = "secret";
    config.ctp.flow_dir = "flow/md";
    config.ctp.instruments = {"IF2606"};
    return config;
}

} // namespace

int main()
{
    namespace fs = std::filesystem;

    const auto temp_dir = fs::temp_directory_path() / "basis_monitor_md_api_session_test";
    fs::create_directories(temp_dir);

    const auto runtime_log = temp_dir / "runtime.log";
    const auto terminal_log = temp_dir / "terminal.log";

    const auto redirected_stdout = std::freopen(terminal_log.string().c_str(), "w", stdout);
    assert(redirected_stdout != nullptr);

    basis_monitor::InitializeLogger(runtime_log.string());

    NullListener listener;
    auto environment = std::make_shared<FakeCtpEnvironment>();
    basis_monitor::MdApiSession session(MakeConfig(), listener, environment);

    const bool started = session.Start();
    assert(started);
    assert(session.WaitForFirstMarketData(1));

    basis_monitor::ShutdownLogger();
    std::fflush(stdout);

    assert(environment->last_flow_dir == "flow/md");
    assert(!environment->last_use_udp);
    assert(!environment->last_use_multicast);
    assert(environment->last_production_mode);
    assert(environment->api.front_address_ == "tcp://10.101.1.102:51213");

    const auto log_contents = ReadFile(runtime_log);
    assert(log_contents.find("[CTP_RUNTIME] MdApiVersion=[md-formal-6.7.11]") != std::string::npos);
    assert(log_contents.find("[CTP_RUNTIME] DataCollectVersion=[collect-6.7.0]") != std::string::npos);
    assert(log_contents.find("[CTP_RUNTIME] SystemInfoProbe ret=[0] len=[344] detail=[CTP_GetSystemInfo succeeded]") != std::string::npos);

    return 0;
}
