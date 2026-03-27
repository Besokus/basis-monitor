#pragma once
#include "stdafx.h"
#include <list>
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include "linux_compat.h"
#endif
#include <time.h>
#include "ThostFtdcTraderApi.h"
#include "ThostFtdcMdApi.h"
#include "DataCollect.h"
#include "getconfig.h"
#include "traderApi.h"
#include "traderSpi.h"
#include "define.h"
#include <vector>
#include <map>
#include <fstream>
#include <deque>
#include <chrono>
#include <thread>
#include <cmath>
#include <cstdlib>

using namespace std;
FILE *logfile;

// ???????????????????
// Create a manual reset event with no signal
HANDLE g_hEvent = CreateEvent(NULL, false, false, NULL);
/// ???????
TThostFtdcBrokerIDType g_chBrokerID;
/// ???????????
TThostFtdcUserIDType g_chUserID;
/// ???????????
TThostFtdcPasswordType g_chPassword;
/// ??????????
TThostFtdcExchangeIDType g_chExchangeID;
///???????
TThostFtdcInstrumentIDType	g_chInstrumentID;
///????????
TThostFtdcInvestorIDType g_chInvestorID;
///???????
TThostFtdcParkedOrderActionIDType	g_chParkedOrderActionID1;
///???????
TThostFtdcParkedOrderIDType	g_chParkedOrderID1;
///????????
TThostFtdcOrderRefType	g_chOrderRef;
///?????
TThostFtdcFrontIDType	g_chFrontID;
///?????
TThostFtdcSessionIDType	g_chSessionID;
///???????
TThostFtdcOrderSysIDType	g_chOrderSysID;
///????
TThostFtdcPriceType	g_chStopPrice;
///????????
TThostFtdcOrderRefType	g_chQuoteRef;
int FrontID = 0;
int SessionID = 0;
int Limitprice = 0;
int nRequestID = 0;
int chioce_action = 0;//?0???????
int OrderRef_num = 0;

vector<string> vector_OrderSysID;
vector<string> vector_ExchangeID;
vector<string> vector_InstrumentID;
vector<string> vector_OrderRef;
vector<int> vector_FrontID;
vector<int> vector_SessionID;
vector<char> vector_OrderStatus;
vector<string> md_InstrumentID;
int action_number;

///???????????
TThostFtdcOrderRefType	g_NewExecOrderRef;
///?????????
TThostFtdcExecOrderSysIDType	g_NewExecOrderSysID;
///?????
TThostFtdcFrontIDType	g_NewFrontID;
///?????
TThostFtdcSessionIDType	g_NewSessionID;

//????????????
///?????????
TThostFtdcOrderSysIDType	g_chOptionSelfCloseSysID;
///???????????
TThostFtdcOrderRefType	g_chOptionSelfCloseRef;
///??????????
TThostFtdcProductInfoType	g_chUserProductInfo;
///?????
TThostFtdcAuthCodeType	g_chAuthCode;
///App????
TThostFtdcAppIDType	g_chAppID;
///???????
TThostFtdcInstrumentIDType	g_chProductID;

HANDLE xinhao = CreateEvent(NULL, false, false, NULL);

CTraderApi *pUserApi = new CTraderApi;

int g_lastApiErrorCode = 0;
string g_lastApiErrorId = "NONE";
string g_lastApiErrorPrompt = "CTP:OK";
string g_errorXmlLoadedPath = "";
bool g_errorXmlLoaded = false;
map<int, pair<string, string> > g_errorMap;

struct SimpleInstrumentInfo
{
	bool valid;
	string exchangeId;
	string instrumentId;
	double priceTick;
	int maxLimitOrderVolume;
	int minLimitOrderVolume;
	SimpleInstrumentInfo() : valid(false), priceTick(0.0), maxLimitOrderVolume(0), minLimitOrderVolume(0) {}
};

struct SimpleDepthLimits
{
	bool valid;
	string exchangeId;
	string instrumentId;
	double upperLimitPrice;
	double lowerLimitPrice;
	double bandingUpperPrice;
	double bandingLowerPrice;
	SimpleDepthLimits() : valid(false), upperLimitPrice(0.0), lowerLimitPrice(0.0), bandingUpperPrice(0.0), bandingLowerPrice(0.0) {}
};

map<string, SimpleInstrumentInfo> g_instrumentInfoCache;
map<string, SimpleDepthLimits> g_depthLimitCache;

struct AlertConfig
{
	int orderCountThreshold;
	int cancelCountThreshold;
	int duplicateOrderCountThreshold;
	int tradeCountThreshold;
	int queryPerSecThreshold;
	int orderPerSecThreshold;
	int programOrderPeakPerSecThreshold;
	int duplicateWindowSec;
	int simulateTradeAlert;
	AlertConfig()
		: orderCountThreshold(0),
		cancelCountThreshold(0),
		duplicateOrderCountThreshold(0),
		tradeCountThreshold(0),
		queryPerSecThreshold(2),
		orderPerSecThreshold(20),
		programOrderPeakPerSecThreshold(200),
		duplicateWindowSec(3),
		simulateTradeAlert(0)
	{
	}
};

AlertConfig g_alertConfig;

int g_orderSubmitCount = 0;
int g_cancelSubmitCount = 0;
int g_duplicateOrderCount = 0;
int g_tradeCount = 0;

deque<long long> g_queryReqMs;
deque<long long> g_orderReqMs;
map<string, deque<long long> > g_orderFingerprintMs;

static long long NowMs()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
}

static void PruneWindow(deque<long long>& q, long long nowMs, long long windowMs)
{
	while (!q.empty() && nowMs - q.front() >= windowMs)
	{
		q.pop_front();
	}
}

static int ParseThreshold(const string& key, int defaultValue)
{
	string v = getConfigOptional("config", key, "");
	if (v.empty())
	{
		return defaultValue;
	}
	int parsed = atoi(v.c_str());
	if (parsed < 0)
	{
		return defaultValue;
	}
	return parsed;
}

static void EmitAlert(const string& metric, int value, int threshold, const string& detail)
{
	LOG("[ALERT] metric=[%s] value=[%d] threshold=[%d] detail=[%s]\a\n",
		metric.c_str(), value, threshold, detail.c_str());
}

static void LoadAlertConfig()
{
	g_alertConfig.orderCountThreshold = ParseThreshold("OrderCountThreshold", 0);
	g_alertConfig.cancelCountThreshold = ParseThreshold("CancelCountThreshold", 0);
	g_alertConfig.duplicateOrderCountThreshold = ParseThreshold("DuplicateOrderThreshold", 0);
	g_alertConfig.tradeCountThreshold = ParseThreshold("TradeCountThreshold", 0);
	g_alertConfig.queryPerSecThreshold = ParseThreshold("QueryPerSecThreshold", 2);
	g_alertConfig.orderPerSecThreshold = ParseThreshold("OrderPerSecThreshold", 20);
	g_alertConfig.programOrderPeakPerSecThreshold = ParseThreshold("ProgramOrderPeakPerSecThreshold", 200);
	g_alertConfig.duplicateWindowSec = ParseThreshold("DuplicateWindowSec", 3);
	g_alertConfig.simulateTradeAlert = ParseThreshold("SimulateTradeAlert", 0);
}

static void LogAlertConfig()
{
	LOG("[THRESHOLD_CONFIG] OrderCountThreshold=[%d] CancelCountThreshold=[%d] DuplicateOrderThreshold=[%d] TradeCountThreshold=[%d]\n",
		g_alertConfig.orderCountThreshold, g_alertConfig.cancelCountThreshold, g_alertConfig.duplicateOrderCountThreshold, g_alertConfig.tradeCountThreshold);
	LOG("[THRESHOLD_CONFIG] QueryPerSecThreshold=[%d] OrderPerSecThreshold=[%d] ProgramOrderPeakPerSecThreshold=[%d] DuplicateWindowSec=[%d]\n",
		g_alertConfig.queryPerSecThreshold, g_alertConfig.orderPerSecThreshold, g_alertConfig.programOrderPeakPerSecThreshold, g_alertConfig.duplicateWindowSec);
	LOG("[THRESHOLD_CONFIG] SimulateTradeAlert=[%d]\n", g_alertConfig.simulateTradeAlert);
}

static void TrackQueryRate(const string& sourceTag)
{
	long long now = NowMs();
	g_queryReqMs.push_back(now);
	PruneWindow(g_queryReqMs, now, 1000);
	int qps = static_cast<int>(g_queryReqMs.size());
	if (g_alertConfig.queryPerSecThreshold > 0 && qps >= g_alertConfig.queryPerSecThreshold)
	{
		EmitAlert("QueryPerSec", qps, g_alertConfig.queryPerSecThreshold, sourceTag);
	}
}

static void TrackOrderRateAndDuplicate(const string& fingerprint)
{
	long long now = NowMs();
	g_orderReqMs.push_back(now);
	PruneWindow(g_orderReqMs, now, 1000);
	int ops = static_cast<int>(g_orderReqMs.size());
	if (g_alertConfig.orderPerSecThreshold > 0 && ops >= g_alertConfig.orderPerSecThreshold)
	{
		EmitAlert("OrderPerSec", ops, g_alertConfig.orderPerSecThreshold, "CTP report threshold");
	}
	if (g_alertConfig.programOrderPeakPerSecThreshold > 0 && ops >= g_alertConfig.programOrderPeakPerSecThreshold)
	{
		EmitAlert("ProgramOrderPeakPerSec", ops, g_alertConfig.programOrderPeakPerSecThreshold, "CTP program peak threshold");
	}

	if (!fingerprint.empty() && fingerprint.find("CANCEL|") != 0)
	{
		deque<long long>& fp = g_orderFingerprintMs[fingerprint];
		fp.push_back(now);
		PruneWindow(fp, now, static_cast<long long>(g_alertConfig.duplicateWindowSec) * 1000);
		if (fp.size() >= 2)
		{
			++g_duplicateOrderCount;
			if (g_alertConfig.duplicateOrderCountThreshold > 0 && g_duplicateOrderCount >= g_alertConfig.duplicateOrderCountThreshold)
			{
				EmitAlert("DuplicateOrderCount", g_duplicateOrderCount, g_alertConfig.duplicateOrderCountThreshold, fingerprint);
			}
		}
	}
}

static void TrackOrderCount()
{
	++g_orderSubmitCount;
	if (g_alertConfig.orderCountThreshold > 0 && g_orderSubmitCount >= g_alertConfig.orderCountThreshold)
	{
		EmitAlert("OrderCount", g_orderSubmitCount, g_alertConfig.orderCountThreshold, "order request submitted");
	}
}

static void TrackCancelCount()
{
	++g_cancelSubmitCount;
	if (g_alertConfig.cancelCountThreshold > 0 && g_cancelSubmitCount >= g_alertConfig.cancelCountThreshold)
	{
		EmitAlert("CancelCount", g_cancelSubmitCount, g_alertConfig.cancelCountThreshold, "cancel request submitted");
	}
}

static void TrackTradeCount()
{
	++g_tradeCount;
	if (g_alertConfig.tradeCountThreshold > 0 && g_tradeCount >= g_alertConfig.tradeCountThreshold)
	{
		EmitAlert("TradeCount", g_tradeCount, g_alertConfig.tradeCountThreshold, "trade callback received");
	}
}

static void MaybeSimulateTradeAlert()
{
	if (g_alertConfig.simulateTradeAlert <= 0)
	{
		return;
	}
	if (g_alertConfig.tradeCountThreshold <= 0)
	{
		EmitAlert("TradeCount", 1, 0, "SIMULATED (set TradeCountThreshold>0 to use threshold)");
		return;
	}
	g_tradeCount = g_alertConfig.tradeCountThreshold;
	EmitAlert("TradeCount", g_tradeCount, g_alertConfig.tradeCountThreshold, "SIMULATED");
}

static bool IsNearlyInteger(double value)
{
	return std::fabs(value - std::round(value)) < 1e-9;
}

static bool IsPriceOnTick(double price, double tick)
{
	if (tick <= 0.0)
	{
		return true;
	}
	double steps = price / tick;
	return IsNearlyInteger(steps);
}

string ExtractXmlAttr(const string& line, const string& attrName)
{
	string key = attrName + "=\"";
	size_t begin = line.find(key);
	if (begin == string::npos)
	{
		return "";
	}
	begin += key.size();
	size_t end = line.find("\"", begin);
	if (end == string::npos)
	{
		return "";
	}
	return line.substr(begin, end - begin);
}

bool LookupErrorInXml(int errorCode, string& errorId, string& errorPrompt)
{
	if (!g_errorXmlLoaded)
	{
		vector<string> xmlCandidates;
		xmlCandidates.push_back("error.xml");
		xmlCandidates.push_back("./error.xml");
		xmlCandidates.push_back("../error.xml");

		ifstream xmlFile;
		for (size_t i = 0; i < xmlCandidates.size(); ++i)
		{
			xmlFile.open(xmlCandidates[i].c_str());
			if (xmlFile.is_open())
			{
				g_errorXmlLoadedPath = xmlCandidates[i];
				break;
			}
			xmlFile.clear();
		}
		if (!xmlFile.is_open())
		{
			g_errorXmlLoadedPath = "NOT_FOUND";
			g_errorXmlLoaded = true;
			return false;
		}

		string line;
		while (getline(xmlFile, line))
		{
			if (line.find("<error ") == string::npos)
			{
				continue;
			}

			string valueAttr = ExtractXmlAttr(line, "value");
			if (valueAttr.empty())
			{
				continue;
			}

			int value = atoi(valueAttr.c_str());
			string idAttr = ExtractXmlAttr(line, "id");
			string promptAttr = ExtractXmlAttr(line, "prompt");
			g_errorMap[value] = make_pair(idAttr, promptAttr);
		}

		g_errorXmlLoaded = true;
		LOG("[ERROR_XML] loaded=[%s], entries=[%d]\n", g_errorXmlLoadedPath.c_str(), (int)g_errorMap.size());
	}

	map<int, pair<string, string> >::const_iterator it = g_errorMap.find(errorCode);
	if (it == g_errorMap.end())
	{
		return false;
	}
	errorId = it->second.first;
	errorPrompt = it->second.second;
	return !errorId.empty() || !errorPrompt.empty();
}

const char* GetFrontDisconnectReasonText(int nReason)
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

static int GetChinaLocalMinutes()
{
	std::time_t tt = std::time(nullptr);
	std::tm gmt = {};
#ifdef _WIN32
	gmtime_s(&gmt, &tt);
#else
	gmtime_r(&tt, &gmt);
#endif
	int hour = gmt.tm_hour + 8;
	if (hour >= 24) hour -= 24;
	if (hour < 0) hour += 24;
	return hour * 60 + gmt.tm_min;
}

static bool IsWithinChinaTradingWindow()
{
	int minutes = GetChinaLocalMinutes();
	return minutes >= (8 * 60 + 30) && minutes <= (15 * 60 + 30);
}

void LogTraderErrorDetail(const char* callbackName, CThostFtdcRspInfoField* pRspInfo);

static void LogLocalApiError(const char* callbackName, int errorId, const char* errorMsg)
{
	CThostFtdcRspInfoField rsp = {};
	rsp.ErrorID = errorId;
	strcpy_s(rsp.ErrorMsg, errorMsg);
	LogTraderErrorDetail(callbackName, &rsp);
}

void LogTraderErrorDetail(const char* callbackName, CThostFtdcRspInfoField* pRspInfo)
{
	if (!pRspInfo)
	{
		LOG("[%s] pRspInfo is null\n", callbackName);
		return;
	}

	g_lastApiErrorCode = pRspInfo->ErrorID;
	if (pRspInfo->ErrorID == 0)
	{
		g_lastApiErrorId = "NONE";
		g_lastApiErrorPrompt = "CTP:OK";
		return;
	}

	string errorId;
	string errorPrompt;
	if (LookupErrorInXml(pRspInfo->ErrorID, errorId, errorPrompt))
	{
		g_lastApiErrorId = errorId;
		g_lastApiErrorPrompt = errorPrompt;
	}
	else
	{
		g_lastApiErrorId = "UNMAPPED_ERROR";
		g_lastApiErrorPrompt = pRspInfo->ErrorMsg;
	}

	LOG("[API_ERROR] callback=[%s], code=[%d], id=[%s], prompt=[%s], raw=[%s], xml=[%s]\n",
		callbackName,
		pRspInfo->ErrorID,
		g_lastApiErrorId.c_str(),
		g_lastApiErrorPrompt.c_str(),
		pRspInfo->ErrorMsg,
		g_errorXmlLoadedPath.empty() ? "UNKNOWN" : g_errorXmlLoadedPath.c_str());

	LOG("[%s] ErrorID=[%d], ErrorCode=[%s], Prompt=[%s], RawMsg=[%s]\n",
		callbackName,
		pRspInfo->ErrorID,
		g_lastApiErrorId.c_str(),
		g_lastApiErrorPrompt.c_str(),
		pRspInfo->ErrorMsg);
}

string MaskSensitive(const string& value)
{
	if (value.empty())
	{
		return "";
	}
	if (value.size() <= 4)
	{
		return "****";
	}
	string head = value.substr(0, 2);
	string tail = value.substr(value.size() - 2);
	return head + "****" + tail;
}

void LogLoginConfigSnapshot()
{
	string frontAddr = getConfig("config", "FrontAddr");
	string frontMdAddr = getConfig("config", "FrontMdAddr");
	string authCode = getConfig("config", "AuthCode");
	string password = getConfig("config", "Password");
	string userProductInfo = getConfigOptional("config", "UserProductInfo", "");

	LOG("[LOGIN_CONFIG]\n");
	LOG("  BrokerID        = [%s]\n", g_chBrokerID);
	LOG("  UserID          = [%s]\n", g_chUserID);
	LOG("  InvestorID      = [%s]\n", g_chInvestorID);
	LOG("  FrontAddr       = [%s]\n", frontAddr.c_str());
	LOG("  FrontMdAddr     = [%s]\n", frontMdAddr.c_str());
	LOG("  AppID           = [%s]\n", g_chAppID);
	LOG("  UserProductInfo = [%s]\n", userProductInfo.c_str());
	LOG("  AuthCodeMasked  = [%s]\n", MaskSensitive(authCode).c_str());
	LOG("  PasswordMasked  = [%s]\n", MaskSensitive(password).c_str());
}

const char* GetRequestReturnText(int retCode)
{
	switch (retCode)
	{
	case 0: return "REQUEST_ACCEPTED";
	case -1: return "NETWORK_SEND_FAILED";
	case -2: return "REQUESTS_OVERFLOW";
	case -3: return "REQUEST_TOO_FREQUENT";
	default: return "UNKNOWN_REQUEST_RETURN";
	}
}

void LogRequestReturnStatus(const char* requestName, int retCode)
{
	LOG("[REQ_STATUS] request=[%s], ret=[%d], retText=[%s]\n",
		requestName,
		retCode,
		GetRequestReturnText(retCode));
}

//??????
class CSimpleMdHandler : public CThostFtdcMdSpi
{
public:
	// ????????????????效?????CThostFtdcMduserApi????????
	CSimpleMdHandler(CThostFtdcMdApi *pUserApi) : m_pUserMdApi(pUserApi) {}
	~CSimpleMdHandler() {}
	// ????????????泄????????????????????????????械??
	void deletemyself()
	{
		delete this;
	}

	virtual void OnFrontConnected()
	{
		strcpy_s(g_chBrokerID, getConfig("config", "BrokerID").c_str());
		strcpy_s(g_chUserID, getConfig("config", "UserID").c_str());
		strcpy_s(g_chPassword, getConfig("config", "Password").c_str());
		//SetEvent(xinhao);
		LOG("<OnFrontConnected>\n");
		LOG("</OnFrontConnected>\n");
		SetEvent(xinhao);
		//ReqUserLogin();
	}

	void RegisterFensUserInfo()
	{
		strcpy_s(g_chBrokerID, getConfig("config", "BrokerID").c_str());
		strcpy_s(g_chUserID, getConfig("config", "UserID").c_str());
		CThostFtdcFensUserInfoField pFens = { 0 };
		strcpy_s(pFens.BrokerID, g_chBrokerID);
		strcpy_s(pFens.UserID, g_chUserID);
		pFens.LoginMode = THOST_FTDC_LM_Trade;
		m_pUserMdApi->RegisterFensUserInfo(&pFens);
	}

	void ReqUserLogin()
	{
		CThostFtdcReqUserLoginField reqUserLogin = { 0 };
		strcpy_s(reqUserLogin.BrokerID, g_chBrokerID);
		//strcpy_s(reqUserLogin.UserID, g_chUserID);
		//strcpy_s(reqUserLogin.UserID, g_chPassword);
		int num = m_pUserMdApi->ReqUserLogin(&reqUserLogin, 111);
		LogRequestReturnStatus("MdApi::ReqUserLogin", num);
	}

	void ReqUserLogout()
	{
		CThostFtdcUserLogoutField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.UserID, g_chUserID);
		int num = m_pUserMdApi->ReqUserLogout(&a, 1);
		LogRequestReturnStatus("MdApi::ReqUserLogout", num);
	}

	virtual void OnHeartBeatWarning(int nTimeLapse)
	{
		LOG("<OnHeartBeatWarning>\n");
		LOG("\tnTimeLapse [%d]\n", nTimeLapse);
		LOG("</OnHeartBeatWarning>\n");
	}

	// ????????????泄????????????????梅?????????
	virtual void OnFrontDisconnected(int nReason)
	{
		// ??????????????API???????????????????????????
		LOG("<OnFrontDisconnected>\n");
		LOG("\tnReason= = [%d]", nReason);
		LOG("</OnFrontDisconnected>\n");
	}

	// ????????????????????梅????????????????????????
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		LOG("<OnRspUserLogin>\n");
		if (pRspUserLogin)
		{
			LOG("\tTradingDay [%s]\n", pRspUserLogin->TradingDay);
			LOG("\tLoginTime [%s]\n", pRspUserLogin->LoginTime);
			LOG("\tBrokerID [%s]\n", pRspUserLogin->BrokerID);
			LOG("\tUserID [%s]\n", pRspUserLogin->UserID);
			LOG("\tSystemName [%s]\n", pRspUserLogin->SystemName);
			LOG("\tMaxOrderRef [%s]\n", pRspUserLogin->MaxOrderRef);
			LOG("\tSHFETime [%s]\n", pRspUserLogin->SHFETime);
			LOG("\tDCETime [%s]\n", pRspUserLogin->DCETime);
			LOG("\tCZCETime [%s]\n", pRspUserLogin->CZCETime);
			LOG("\tFFEXTime [%s]\n", pRspUserLogin->FFEXTime);
			LOG("\tINETime [%s]\n", pRspUserLogin->INETime);
			LOG("\tFrontID [%d]\n", pRspUserLogin->FrontID);
			LOG("\tSessionID [%d]\n", pRspUserLogin->SessionID);
		}
		if (pRspInfo)
		{
			LOG("\tErrorMsg [%s]\n", pRspInfo->ErrorMsg);
			LOG("\tErrorID [%d]\n", pRspInfo->ErrorID);
		}
		LOG("\tnRequestID [%d]\n", nRequestID);
		LOG("\tbIsLast [%d]\n", bIsLast);
		LOG("</OnRspUserLogin>\n");
		LogTraderErrorDetail("Md::OnRspUserLogin", pRspInfo);
		if (pRspInfo && pRspInfo->ErrorID != 0) {
			// ????????????????写?????
			LOG("\tFailed to login, errorcode=%d errormsg=%s requestid=%d chain = %d",
				pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
			cin.get();
			cin.ignore();
			exit(-1);
		}
		SetEvent(xinhao);
	}

	///??????????
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		LOG("<OnRspUserLogout>\n");
		if (pUserLogout)
		{
			LOG("\tBrokerID [%s]\n", pUserLogout->BrokerID);
			LOG("\tUserID [%s]\n", pUserLogout->UserID);
		}
		if (pRspInfo)
		{
			LOG("\tErrorMsg [%s]\n", pRspInfo->ErrorMsg);
			LOG("\tErrorID [%d]\n", pRspInfo->ErrorID);
		}
		LOG("\tnRequestID [%d]\n", nRequestID);
		LOG("\tbIsLast [%d]\n", bIsLast);
		LOG("</OnRspUserLogout>\n");
		LogTraderErrorDetail("Md::OnRspUserLogout", pRspInfo);
		SetEvent(xinhao);
		//pUserApi->Release();
	}

	void UnSubscribeMarketData()
	{
		int md_num = 0;
		char **ppInstrumentID = new char*[5000];
		for (int count1 = 0; count1 <= md_InstrumentID.size() / 500; count1++)
		{
			if (count1 < md_InstrumentID.size() / 500)
			{
				int a = 0;
				for (a; a < 500; a++)
				{
					ppInstrumentID[a] = const_cast<char *>(md_InstrumentID.at(md_num).c_str());
					md_num++;
				}
				int result = m_pUserMdApi->UnSubscribeMarketData(ppInstrumentID, a);
				LOG((result == 0) ? "???????????1......??????\n" : "???????????1......???????????????=[%d]\n", result);
			}
			else if (count1 == md_InstrumentID.size() / 500)
			{
				int count2 = 0;
				for (count2; count2 < md_InstrumentID.size() % 500; count2++)
				{
					ppInstrumentID[count2] = const_cast<char *>(md_InstrumentID.at(md_num).c_str());
					md_num++;
				}
				int result = m_pUserMdApi->UnSubscribeMarketData(ppInstrumentID, count2);
				LOG((result == 0) ? "???????????2......??????\n" : "???????????2......???????????????=[%d]\n", result);
			}
		}
	}

	virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		LOG("<OnRspUnSubMarketData>\n");
		if (pSpecificInstrument)
		{
			LOG("\tInstrumentID [%s]\n", pSpecificInstrument->InstrumentID);
		}
		if (pRspInfo)
		{
			LOG("\tErrorMsg [%s]\n", pRspInfo->ErrorMsg);
			LOG("\tErrorID [%d]\n", pRspInfo->ErrorID);
		}
		LOG("\tnRequestID [%d]\n", nRequestID);
		LOG("\tbIsLast [%d]\n", bIsLast);
		LOG("</OnRspUnSubMarketData>\n");
		LogTraderErrorDetail("Md::OnRspUnSubMarketData", pRspInfo);
		if (bIsLast)
		{
			SetEvent(xinhao);
		}
	};

	void SubscribeMarketData()//??????
	{
		int md_num = 0;
		char **ppInstrumentID = new char*[5000];
		for (int count1 = 0; count1 <= md_InstrumentID.size() / 500; count1++)
		{
			if (count1 < md_InstrumentID.size() / 500)
			{
				int a = 0;
				for (a; a < 500; a++)
				{
					ppInstrumentID[a] = const_cast<char *>(md_InstrumentID.at(md_num).c_str());
					md_num++;
				}
				int result = m_pUserMdApi->SubscribeMarketData(ppInstrumentID, a);
				LOG((result == 0) ? "????????????1......??????\n" : "????????????1......???????????????=[%d]\n", result);
			}
			else if (count1 == md_InstrumentID.size() / 500)
			{
				int count2 = 0;
				for (count2; count2 < md_InstrumentID.size() % 500; count2++)
				{
					ppInstrumentID[count2] = const_cast<char *>(md_InstrumentID.at(md_num).c_str());
					md_num++;
				}
				int result = m_pUserMdApi->SubscribeMarketData(ppInstrumentID, count2);
				LOG((result == 0) ? "????????????2......??????\n" : "????????????2......???????????????=[%d]\n", result);
			}
		}
	}

	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		LOG("<OnRspSubMarketData>\n");
		if (pSpecificInstrument)
		{
			printf("\tInstrumentID [%s]\n", pSpecificInstrument->InstrumentID);
		}
		if (pRspInfo)
		{
			LOG("\tErrorMsg [%s]\n", pRspInfo->ErrorMsg);
			LOG("\tErrorID [%d]\n", pRspInfo->ErrorID);
		}
		LOG("\tnRequestID [%d]\n", nRequestID);
		LOG("\tbIsLast [%d]\n", bIsLast);
		LOG("</OnRspSubMarketData>\n");
		LogTraderErrorDetail("Md::OnRspSubMarketData", pRspInfo);
		if (bIsLast)
		{
			SetEvent(xinhao);
		}
	};

	///?????????
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
	{
		//????????
		SYSTEMTIME sys;
		GetLocalTime(&sys);
		LOG("%02d:%02d:%02d.%03d\t",
			sys.wHour,
			sys.wMinute,
			sys.wSecond,
			sys.wMilliseconds);
		LOG("<OnRtnDepthMarketData>");
		if (pDepthMarketData)
		{
			LOG("\tTradingDay [%s]\n", pDepthMarketData->TradingDay);
			LOG("\tInstrumentID [%s]", pDepthMarketData->InstrumentID);
			LOG("\tExchangeID [%s]", pDepthMarketData->ExchangeID);
			LOG("\tExchangeInstID [%s]\n", pDepthMarketData->ExchangeInstID);
			LOG("\tUpdateTime [%s]", pDepthMarketData->UpdateTime);
			LOG("\tActionDay [%s]\n", pDepthMarketData->ActionDay);
			LOG("\tVolume [%d]\n", pDepthMarketData->Volume);
			LOG("\tUpdateMillisec [%d]", pDepthMarketData->UpdateMillisec);
			LOG("\tBidVolume1 [%d]\n", pDepthMarketData->BidVolume1);
			LOG("\tAskVolume1 [%d]\n", pDepthMarketData->AskVolume1);
			LOG("\tBidVolume2 [%d]\n", pDepthMarketData->BidVolume2);
			LOG("\tAskVolume2 [%d]\n", pDepthMarketData->AskVolume2);
			LOG("\tBidVolume3 [%d]\n", pDepthMarketData->BidVolume3);
			LOG("\tAskVolume3 [%d]\n", pDepthMarketData->AskVolume3);
			LOG("\tBidVolume4 [%d]\n", pDepthMarketData->BidVolume4);
			LOG("\tAskVolume4 [%d]\n", pDepthMarketData->AskVolume4);
			LOG("\tBidVolume5 [%d]\n", pDepthMarketData->BidVolume5);
			LOG("\tAskVolume5 [%d]\n", pDepthMarketData->AskVolume5);
			LOG("\tLastPrice [%.8lf]\n", (pDepthMarketData->LastPrice > 10000000) ? 0 : pDepthMarketData->LastPrice);
			LOG("\tPreSettlementPrice [%.8lf]\n", (pDepthMarketData->PreSettlementPrice > 10000000) ? 0 : pDepthMarketData->PreSettlementPrice);
			LOG("\tPreClosePrice [%.8lf]\n", (pDepthMarketData->PreClosePrice > 10000000) ? 0 : pDepthMarketData->PreClosePrice);
			LOG("\tPreOpenInterest [%.8lf]\n", (pDepthMarketData->PreOpenInterest > 10000000) ? 0 : pDepthMarketData->PreOpenInterest);
			LOG("\tOpenPrice [%.8lf]\n", (pDepthMarketData->OpenPrice > 10000000) ? 0 : pDepthMarketData->OpenPrice);
			LOG("\tHighestPrice [%.8lf]\n", (pDepthMarketData->HighestPrice > 10000000) ? 0 : pDepthMarketData->HighestPrice);
			LOG("\tLowestPrice [%.8lf]\n", (pDepthMarketData->LowestPrice > 10000000) ? 0 : pDepthMarketData->LowestPrice);
			LOG("\tTurnover [%.8lf]\n", (pDepthMarketData->Turnover > 10000000) ? 0 : pDepthMarketData->Turnover);
			LOG("\tOpenInterest [%.8lf]\n", (pDepthMarketData->OpenInterest > 10000000) ? 0 : pDepthMarketData->OpenInterest);
			LOG("\tClosePrice [%.8lf]\n", (pDepthMarketData->ClosePrice > 10000000) ? 0 : pDepthMarketData->ClosePrice);
			LOG("\tSettlementPrice [%.8lf]\n", (pDepthMarketData->SettlementPrice > 10000000) ? 0 : pDepthMarketData->SettlementPrice);
			LOG("\tUpperLimitPrice [%.8lf]\n", (pDepthMarketData->UpperLimitPrice > 10000000) ? 0 : pDepthMarketData->UpperLimitPrice);
			LOG("\tLowerLimitPrice [%.8lf]\n", (pDepthMarketData->LowerLimitPrice > 10000000) ? 0 : pDepthMarketData->LowerLimitPrice);
			LOG("\tPreDelta [%.8lf]\n", (pDepthMarketData->PreDelta > 10000000) ? 0 : pDepthMarketData->PreDelta);
			LOG("\tCurrDelta [%.8lf]\n", (pDepthMarketData->CurrDelta > 10000000) ? 0 : pDepthMarketData->CurrDelta);
			LOG("\tBidPrice1 [%.8lf]\n", (pDepthMarketData->BidPrice1 > 10000000) ? 0 : pDepthMarketData->BidPrice1);
			LOG("\tAskPrice1 [%.8lf]\n", (pDepthMarketData->AskPrice1 > 10000000) ? 0 : pDepthMarketData->AskPrice1);
			LOG("\tBidPrice2 [%.8lf]\n", (pDepthMarketData->BidPrice2 > 10000000) ? 0 : pDepthMarketData->BidPrice2);
			LOG("\tAskPrice2 [%.8lf]\n", (pDepthMarketData->AskPrice2 > 10000000) ? 0 : pDepthMarketData->AskPrice2);
			LOG("\tBidPrice3 [%.8lf]\n", (pDepthMarketData->BidPrice3 > 10000000) ? 0 : pDepthMarketData->BidPrice3);
			LOG("\tAskPrice3 [%.8lf]\n", (pDepthMarketData->AskPrice3 > 10000000) ? 0 : pDepthMarketData->AskPrice3);
			LOG("\tBidPrice4 [%.8lf]\n", (pDepthMarketData->BidPrice4 > 10000000) ? 0 : pDepthMarketData->BidPrice4);
			LOG("\tAskPrice4 [%.8lf]\n", (pDepthMarketData->AskPrice4 > 10000000) ? 0 : pDepthMarketData->AskPrice4);
			LOG("\tBidPrice5 [%.8lf]\n", (pDepthMarketData->BidPrice5 > 10000000) ? 0 : pDepthMarketData->BidPrice5);
			LOG("\tAskPrice5 [%.8lf]\n", (pDepthMarketData->AskPrice5 > 10000000) ? 0 : pDepthMarketData->AskPrice5);
			LOG("\tAveragePrice [%.8lf]\n", (pDepthMarketData->AveragePrice > 10000000) ? 0 : pDepthMarketData->AveragePrice);
		}
		LOG("</OnRtnDepthMarketData>\n");
	};

	///???????????
	void SubscribeForQuoteRsp()
	{
		int md_num = 0;
		char **ppInstrumentID = new char*[5000];
		for (int count1 = 0; count1 <= md_InstrumentID.size() / 500; count1++)
		{
			if (count1 < md_InstrumentID.size() / 500)
			{
				int a = 0;
				for (a; a < 500; a++)
				{
					ppInstrumentID[a] = const_cast<char *>(md_InstrumentID.at(md_num).c_str());
					md_num++;
				}
				int result = m_pUserMdApi->SubscribeForQuoteRsp(ppInstrumentID, a);
				LOG((result == 0) ? "???????????1......??????\n" : "???????????1......???????????????=[%d]\n", result);
			}
			else if (count1 == md_InstrumentID.size() / 500)
			{
				int count2 = 0;
				for (count2; count2 < md_InstrumentID.size() % 500; count2++)
				{
					ppInstrumentID[count2] = const_cast<char *>(md_InstrumentID.at(md_num).c_str());
					md_num++;
				}
				int result = m_pUserMdApi->SubscribeForQuoteRsp(ppInstrumentID, count2);
				LOG((result == 0) ? "???????????2......??????\n" : "???????????2......???????????????=[%d]\n", result);
			}
		}
	}

	virtual void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		LOG("<OnRspSubForQuoteRsp>\n");
		if (pSpecificInstrument)
		{
			LOG("\tInstrumentID [%s]\n", pSpecificInstrument->InstrumentID);
		}
		if (pRspInfo)
		{
			LOG("\tErrorMsg [%s]\n", pRspInfo->ErrorMsg);
			LOG("\tErrorID [%d]\n", pRspInfo->ErrorID);
		}
		LOG("\tnRequestID [%d]\n", nRequestID);
		LOG("\tbIsLast [%d]\n", bIsLast);
		LOG("</OnRspSubForQuoteRsp>\n");
		LogTraderErrorDetail("Md::OnRspSubForQuoteRsp", pRspInfo);
		if (bIsLast)
		{
			SetEvent(xinhao);
		}
	};

	void UnSubscribeForQuoteRsp()
	{
		int md_num = 0;
		char **ppInstrumentID = new char*[5000];
		for (int count1 = 0; count1 <= md_InstrumentID.size() / 500; count1++)
		{
			if (count1 < md_InstrumentID.size() / 500)
			{
				int a = 0;
				for (a; a < 500; a++)
				{
					ppInstrumentID[a] = const_cast<char *>(md_InstrumentID.at(md_num).c_str());
					md_num++;
				}
				int result = m_pUserMdApi->UnSubscribeForQuoteRsp(ppInstrumentID, a);
				LOG((result == 0) ? "??????????1......??????\n" : "??????????1......???????????????=[%d]\n", result);
			}
			else if (count1 == md_InstrumentID.size() / 500)
			{
				int count2 = 0;
				for (count2; count2 < md_InstrumentID.size() % 500; count2++)
				{
					ppInstrumentID[count2] = const_cast<char *>(md_InstrumentID.at(md_num).c_str());
					md_num++;
				}
				int result = m_pUserMdApi->UnSubscribeForQuoteRsp(ppInstrumentID, count2);
				LOG((result == 0) ? "??????????2......??????\n" : "??????????2......???????????????=[%d]\n", result);
			}
		}
	}

	virtual void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		LOG("<OnRspUnSubForQuoteRsp>\n");
		if (pSpecificInstrument)
		{
			LOG("\tInstrumentID [%s]\n", pSpecificInstrument->InstrumentID);
		}
		if (pRspInfo)
		{
			LOG("\tErrorMsg [%s]\n", pRspInfo->ErrorMsg);
			LOG("\tErrorID [%d]\n", pRspInfo->ErrorID);
		}
		LOG("\tnRequestID [%d]\n", nRequestID);
		LOG("\tbIsLast [%d]\n", bIsLast);
		LOG("</OnRspUnSubForQuoteRsp>\n");
		LogTraderErrorDetail("Md::OnRspUnSubForQuoteRsp", pRspInfo);
		if (bIsLast)
		{
			SetEvent(xinhao);
		}
	};

	///?????
	virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp)
	{
		LOG("<OnRtnForQuoteRsp>\n");
		if (pForQuoteRsp)
		{
			LOG("\tTradingDay [%s]\n", pForQuoteRsp->TradingDay);
			LOG("\tInstrumentID [%s]\n", pForQuoteRsp->InstrumentID);
			LOG("\tForQuoteSysID [%s]\n", pForQuoteRsp->ForQuoteSysID);
			LOG("\tForQuoteTime [%s]\n", pForQuoteRsp->ForQuoteTime);
			LOG("\tActionDay [%s]\n", pForQuoteRsp->ActionDay);
			LOG("\tExchangeID [%s]\n", pForQuoteRsp->ExchangeID);
		}
		LOG("</OnRtnForQuoteRsp>\n");
	}

	void ReqQryMulticastInstrument()
	{
		string new_TopicID;
		LOG("??????TopicID???:\n");
		cin >> new_TopicID;

		CThostFtdcQryMulticastInstrumentField a = { 0 };
		a.TopicID = atoi(new_TopicID.c_str());
		//strcpy_s(a.InstrumentID, "");
		int b = m_pUserMdApi->ReqQryMulticastInstrument(&a, nRequestID++);
		LOG((b == 0) ? "???????椴???......??????\n" : "???????椴???......???????????=[%d]\n", b);
	}

	virtual void OnRspQryMulticastInstrument(CThostFtdcMulticastInstrumentField *pMulticastInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		LOG("<OnRspQryMulticastInstrument>\n");
		if (pMulticastInstrument)
		{
			LOG("\tInstrumentID [%s]\n", pMulticastInstrument->InstrumentID);
			LOG("\tTopicID [%d]\n", pMulticastInstrument->TopicID);
			LOG("\tInstrumentNo [%d]\n", pMulticastInstrument->InstrumentNo);
			LOG("\tVolumeMultiple [%d]\n", pMulticastInstrument->VolumeMultiple);
			LOG("\tCodePrice [%.8lf]\n", (pMulticastInstrument->CodePrice > 10000000) ? 0 : pMulticastInstrument->CodePrice);
			LOG("\tPriceTick [%.8lf]\n", (pMulticastInstrument->PriceTick > 10000000) ? 0 : pMulticastInstrument->PriceTick);
		}
		if (pRspInfo)
		{
			LOG("\tErrorMsg [%s]\n", pRspInfo->ErrorMsg);
			LOG("\tErrorID [%d]\n", pRspInfo->ErrorID);
		}
		LOG("\tnRequestID [%d]\n", nRequestID);
		LOG("\tbIsLast [%d]\n", bIsLast);
		LOG("</OnRspQryMulticastInstrument>\n");
		LogTraderErrorDetail("Md::OnRspQryMulticastInstrument", pRspInfo);
	};

private:
	// ???CThostFtdcMduserApi????????
	CThostFtdcMdApi *m_pUserMdApi;
};

//??????
class CSimpleHandler : public CTraderSpi
{
public:
	CSimpleHandler(CThostFtdcTraderApi *pUserApi) :
		m_pUserApi(pUserApi) {}
	~CSimpleHandler() {}
	virtual void OnFrontConnected()
	{
		printf("ThreadID: %lu\n", static_cast<unsigned long>(GetCurrentThreadId())); //???????????????????
		LOG("<OnFrontConnected>\n");
		LOG("</OnFrontConnected>\n");
		strcpy_s(g_chBrokerID, getConfig("config", "BrokerID").c_str());
		strcpy_s(g_chUserID, getConfig("config", "UserID").c_str());
		strcpy_s(g_chPassword, getConfig("config", "Password").c_str());
		strcpy_s(g_chInvestorID, getConfig("config", "InvestorID").c_str());
		strcpy_s(g_chAuthCode, getConfig("config", "AuthCode").c_str());
		strcpy_s(g_chAppID, getConfig("config", "AppID").c_str());
		strcpy_s(g_chInstrumentID, getConfig("config", "InstrumentID").c_str());
		strcpy_s(g_chExchangeID, getConfig("config", "ExchangeID").c_str());
		strcpy_s(g_chProductID, getConfig("config", "ProductID").c_str());
		// strcpy_s(g_chProductID, getConfigOptional("config", "ProductID", "").c_str());
		LoadAlertConfig();
		LogAlertConfig();
		MaybeSimulateTradeAlert();

		strcpy_s(g_NewExecOrderRef, "");
		strcpy_s(g_NewExecOrderSysID, "");
		g_NewFrontID = 0;
		g_NewSessionID = 0;
		SetEvent(g_hEvent);
	}

	//???????????????
	void ReqQryAccountregister()
	{
		CThostFtdcQryAccountregisterField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.AccountID, g_chInvestorID);
		//strcpy_s(a.BankID, "3");
		//strcpy_s(a.BankBranchID, "0740");
		//strcpy_s(a.CurrencyID, "CNY");
		int b = m_pUserApi->ReqQryAccountregister(&a, nRequestID++);
		LOG((b == 0) ? "???????????????......??????\n" : "???????????????......???????????=[%d]\n", b);
	}

	//????????????
	void ReqQryContractBank()
	{
		CThostFtdcQryContractBankField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		int b = m_pUserApi->ReqQryContractBank(&a, nRequestID++);
		LOG((b == 0) ? "???????????????......??????\n" : "???????????????......???????????=[%d]\n", b);
	}

	//??????????????????
	void ReqQryBrokerTradingAlgos()
	{
		CThostFtdcQryBrokerTradingAlgosField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		//strcpy_s(a.ExchangeID, g_chExchangeID);
		//strcpy_s(a.InstrumentID, g_chInstrumentID);
		int b = m_pUserApi->ReqQryBrokerTradingAlgos(&a, nRequestID++);
		LOG((b == 0) ? "??????????????????......??????\n" : "??????????????????......???????????=[%d]\n", b);
	}

	//??????????????????????????????
	void ReqQryCFMMCTradingAccountKey()
	{
		CThostFtdcQryCFMMCTradingAccountKeyField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chUserID);
		int b = m_pUserApi->ReqQryCFMMCTradingAccountKey(&a, nRequestID++);
		LOG((b == 0) ? "??????????????????????????????......??????\n" : "??????????????????????????????......???????????=[%d]\n", b);
	}

	//????????
	void ReqAuthenticate()
	{
		strcpy_s(g_chAuthCode, getConfig("config", "AuthCode").c_str());
		strcpy_s(g_chAppID, getConfig("config", "AppID").c_str());
		strcpy_s(g_chUserProductInfo, getConfigOptional("config", "UserProductInfo", "").c_str());

		LogLoginConfigSnapshot();

		CThostFtdcReqAuthenticateField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.UserID, g_chUserID);
		strcpy_s(a.AuthCode, g_chAuthCode);
		strcpy_s(a.AppID, g_chAppID);
		strcpy_s(a.UserProductInfo, g_chUserProductInfo);
		int b = m_pUserApi->ReqAuthenticate(&a, nRequestID++);
		LogRequestReturnStatus("TraderApi::ReqAuthenticate", b);
	}

	///???????????
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo,
		int nRequestID, bool bIsLast)
	{
		CTraderSpi::OnRspAuthenticate(pRspAuthenticateField, pRspInfo, nRequestID, bIsLast);
		LogTraderErrorDetail("OnRspAuthenticate", pRspInfo);
		SetEvent(g_hEvent);
	}

	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		CTraderSpi::OnRspError(pRspInfo, nRequestID, bIsLast);
		LogTraderErrorDetail("OnRspError", pRspInfo);
		SetEvent(g_hEvent);
	}

	void RegisterFensUserInfo()
	{
		CThostFtdcFensUserInfoField pFensUserInfo = { 0 };
		strcpy_s(pFensUserInfo.BrokerID, getConfig("config", "BrokerID").c_str());
		strcpy_s(pFensUserInfo.UserID, getConfig("config", "UserID").c_str());
		pFensUserInfo.LoginMode = THOST_FTDC_LM_Trade;
		m_pUserApi->RegisterFensUserInfo(&pFensUserInfo);
	}

	virtual void OnFrontDisconnected(int nReason)
	{
		LOG("<OnFrontDisconnected>\n");
		LOG("\tnReason = %d(0x%X)\n", nReason, nReason);
		LOG("\tnReasonText = %s\n", GetFrontDisconnectReasonText(nReason));
		LOG("</OnFrontDisconnected>\n");
		g_lastApiErrorCode = nReason;
		g_lastApiErrorId = "FRONT_DISCONNECTED";
		g_lastApiErrorPrompt = GetFrontDisconnectReasonText(nReason);
		SetEvent(g_hEvent);
	}

	void GetFrontInfo()
	{
		// 褰撳墠浣跨敤鐨勫ご鏂囦欢鐗堟湰鏈彁渚?CThostFtdcFrontInfoField 鍙?GetFrontInfo 鎺ュ彛锛岃繖閲屼繚鐣欑┖瀹炵幇浠ュ吋瀹圭紪璇?
	}

	void ReqUserLogin()
	{
		CThostFtdcReqUserLoginField reqUserLogin = { 0 };
		strcpy_s(reqUserLogin.BrokerID, g_chBrokerID);
		strcpy_s(reqUserLogin.UserID, g_chUserID);
		strcpy_s(reqUserLogin.Password, g_chPassword);
		//strcpy_s(reqUserLogin.ClientIPAddress, "::c0a8:0101");
		strcpy_s(reqUserLogin.LoginRemark, "4444444444444444444444444");
		// ???????????
		int ret = m_pUserApi->ReqUserLogin(&reqUserLogin, nRequestID++);
		LogRequestReturnStatus("TraderApi::ReqUserLogin", ret);
	}

	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		//pUserApi->Release();
		if (pRspUserLogin)
		{
			FrontID = pRspUserLogin->FrontID;
			SessionID = pRspUserLogin->SessionID;
		}
		CTraderSpi::OnRspUserLogin(pRspUserLogin, pRspInfo, nRequestID, bIsLast);
		LogTraderErrorDetail("OnRspUserLogin", pRspInfo);
		
		if (pRspInfo && pRspInfo->ErrorID != 0)
		{
			LOG("\tFailed to login, errorcode=[%d]\n \terrormsg=[%s]\n \trequestid = [%d]\n \tchain = [%d]\n",
				pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
			//exit(-1);
		}
		SetEvent(g_hEvent);
	}

	void ReqUserLogout()
	{
		CThostFtdcUserLogoutField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.UserID, g_chUserID);
		int ret = m_pUserApi->ReqUserLogout(&a, nRequestID++);
		LogRequestReturnStatus("TraderApi::ReqUserLogout", ret);
	}

	///??????????
	virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		LOG("<OnRspUserLogout>\n");
		if (pUserLogout)
		{
			LOG("\tBrokerID [%s]\n", pUserLogout->BrokerID);
			LOG("\tUserID [%s]\n", pUserLogout->UserID);
		}
		if (pRspInfo)
		{
			LOG("\tErrorMsg [%s]\n", pRspInfo->ErrorMsg);
			LOG("\tErrorID [%d]\n", pRspInfo->ErrorID);
		}
		LOG("\tnRequestID [%d]\n", nRequestID);
		LOG("\tbIsLast [%d]\n", bIsLast);
		LOG("</OnRspUserLogout>\n");
		LogTraderErrorDetail("OnRspUserLogout", pRspInfo);
		SetEvent(g_hEvent);
		//pUserApi->Release();
	}

	///??????????
	void ReqSettlementInfoConfirm()
	{
		CThostFtdcSettlementInfoConfirmField Confirm = { 0 };
		///??????????
		strcpy_s(Confirm.BrokerID, g_chBrokerID);
		///????????
		strcpy_s(Confirm.InvestorID, g_chInvestorID);
		int ret = m_pUserApi->ReqSettlementInfoConfirm(&Confirm, nRequestID++);
		LogRequestReturnStatus("TraderApi::ReqSettlementInfoConfirm", ret);
	}

	///????????????????
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		CTraderSpi::OnRspSettlementInfoConfirm(pSettlementInfoConfirm, pRspInfo, nRequestID, bIsLast);
		LogTraderErrorDetail("OnRspSettlementInfoConfirm", pRspInfo);
		SetEvent(g_hEvent);
	}

	///??????????????
	void ReqUserPasswordUpdate()
	{
		string newpassword;
		LOG("??????????????\n");
		cin >> newpassword;
		CThostFtdcUserPasswordUpdateField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.UserID, g_chUserID);
		strcpy_s(a.OldPassword, g_chPassword);
		strcpy_s(a.NewPassword, newpassword.c_str());
		int b = m_pUserApi->ReqUserPasswordUpdate(&a, nRequestID++);
		LOG((b == 0) ? "??????????????......??????\n" : "??????????????......???????????=[%d]\n", b);
	}

	///?????????????????
	virtual void OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField *pUserPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		CTraderSpi::OnRspUserPasswordUpdate(pUserPasswordUpdate, pRspInfo, nRequestID, bIsLast);
		LogTraderErrorDetail("OnRspUserPasswordUpdate", pRspInfo);
		SetEvent(g_hEvent);
	}

	///?????????????????
	void ReqTradingAccountPasswordUpdate()
	{
		string newpassword;
		LOG("???????????????\n");
		cin >> newpassword;
		CThostFtdcTradingAccountPasswordUpdateField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.AccountID, g_chInvestorID);
		strcpy_s(a.OldPassword, g_chPassword);
		strcpy_s(a.NewPassword, newpassword.c_str());
		strcpy_s(a.CurrencyID, "CNY");
		int b = m_pUserApi->ReqTradingAccountPasswordUpdate(&a, nRequestID++);
		LOG((b == 0) ? "?????????????????......??????\n" : "?????????????????......???????????=[%d]\n", b);
	}

	///????????????????????
	virtual void OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		CTraderSpi::OnRspTradingAccountPasswordUpdate(pTradingAccountPasswordUpdate, pRspInfo, nRequestID, bIsLast);
		LogTraderErrorDetail("OnRspTradingAccountPasswordUpdate", pRspInfo);
		SetEvent(g_hEvent);
	}

	///??????//????
	void ReqParkedOrderInsert()
	{
		int limitprice = 0;
		LOG("?????????????(???0)\n");
		cin >> limitprice;
		CThostFtdcParkedOrderField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		strcpy_s(a.UserID, g_chUserID);
		OrderRef_num++;
		itoa(OrderRef_num, a.OrderRef, 10);
		a.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		a.Direction = THOST_FTDC_D_Buy;
		a.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		a.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		a.LimitPrice = limitprice;
		a.VolumeTotalOriginal = 1;
		a.TimeCondition = THOST_FTDC_TC_GFD;
		a.VolumeCondition = THOST_FTDC_VC_AV;
		a.MinVolume = 1;
		a.ContingentCondition = THOST_FTDC_CC_Immediately;
		a.StopPrice = 0;
		a.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		a.IsAutoSuspend = 0;
		strcpy_s(a.ExchangeID, g_chExchangeID);
		int b = m_pUserApi->ReqParkedOrderInsert(&a, nRequestID++);
		LOG((b == 0) ? "??????????......??????\n" : "??????????......???????????=[%d]\n", b);
	}

	///???????????
	void ReqParkedOrderAction()
	{
		CThostFtdcParkedOrderActionField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		//strcpy_s(a.OrderRef, "          15");
		strcpy_s(a.ExchangeID, g_chExchangeID);
		/*a.FrontID = 1;
		a.SessionID = -287506422;*/
		strcpy_s(a.OrderSysID, g_chOrderSysID);
		strcpy_s(a.UserID, g_chUserID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		a.ActionFlag = THOST_FTDC_AF_Delete;
		int b = m_pUserApi->ReqParkedOrderAction(&a, nRequestID++);
		LOG((b == 0) ? "???????????......??????\n" : "???????????......???????????=[%d]\n", b);
	}

	///??????????
	void ReqRemoveParkedOrder()
	{
		CThostFtdcRemoveParkedOrderField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.ParkedOrderID, g_chParkedOrderID1);
		int b = m_pUserApi->ReqRemoveParkedOrder(&a, nRequestID++);
		LOG((b == 0) ? "??????????......??????\n" : "??????????......???????????=[%d]\n", b);
	}

	///???????????
	void ReqRemoveParkedOrderAction()
	{
		CThostFtdcRemoveParkedOrderActionField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.ParkedOrderActionID, g_chParkedOrderActionID1);
		int b = m_pUserApi->ReqRemoveParkedOrderAction(&a, nRequestID++);
		LOG((b == 0) ? "???????????......??????\n" : "???????????......???????????=[%d]\n", b);
	}

	///???????????
	void ReqOrderInsert_Ordinary()
	{
		system("cls");
		ReqOrderInsert_Ordinary_Checked();
		return;
		string new_limitprice;
		LOG("请输入指定价格：\n");
		cin >> new_limitprice;

		CThostFtdcInputOrderField ord = { 0 };
		strcpy_s(ord.InvestUnitID, "InvestUnitID");
		strcpy_s(ord.AccountID, "1");
		strcpy_s(ord.BrokerID, g_chBrokerID);
		strcpy_s(ord.InvestorID, g_chInvestorID);
		strcpy_s(ord.InstrumentID, g_chInstrumentID);
		strcpy_s(ord.UserID, g_chUserID);
		OrderRef_num++;
		itoa(OrderRef_num, ord.OrderRef,10);
		//strcpy_s(ord.OrderRef,"");
		//strcpy_s(ord.OrderRef, itoa(OrderRef_num));
		ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		
		int num1;
	Direction:LOG("请选择买卖方向\t1.买\t2.卖\n");
		cin >> num1;
		if(num1==1){
			ord.Direction = THOST_FTDC_D_Buy;//??
		}
		else if (num1 == 2) {
			ord.Direction = THOST_FTDC_D_Sell;//??
		}
		else {
			LOG("输入错误请重新输入\n");
			_getch();
			goto Direction;
		}
		int num2;
	CombOffsetFlag:LOG("请输入开平方向\t1.开仓\t2.平仓\t3.强平\t4.平今\t5.平昨\t6.强减\t7.本地强平\n");
		cin >> num2;
		if (num2 == 1) {
			ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		}
		else if (num2 == 2) {
			ord.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
		}
		else if (num2 == 3) {
			ord.CombOffsetFlag[0] = THOST_FTDC_OF_ForceClose;
		}
		else if (num2 == 4) {
			ord.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
		}
		else if (num2 == 5) {
			ord.CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;
		}
		else if (num2 == 6) {
			ord.CombOffsetFlag[0] = THOST_FTDC_OF_ForceOff;
		}
		else if (num2 == 7) {
			ord.CombOffsetFlag[0] = THOST_FTDC_OF_LocalForceClose;
		}
		else {
			LOG("?????????????????\n");
			_getch();
			goto CombOffsetFlag;
		}
		ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		ord.LimitPrice = atoi(new_limitprice.c_str());
		//ord.LimitPrice = 598;
		ord.VolumeTotalOriginal = 3;
		ord.TimeCondition = THOST_FTDC_TC_IOC;///??????????????
		ord.VolumeCondition = THOST_FTDC_VC_MV;///??小????
		ord.MinVolume = 3;
		ord.ContingentCondition = THOST_FTDC_CC_Immediately;
		ord.StopPrice = 0;
		ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		ord.IsAutoSuspend = 0;
		strcpy_s(ord.ExchangeID, g_chExchangeID);
		int a = m_pUserApi->ReqOrderInsert(&ord, 1);
		LOG((a == 0) ? "报单录入请求限价单......发送成功\n" : "报单录入请求限价单......发送失败，序号=[%d]\n", a);
	}

	///????????????
	void ReqOrderInsert_Touch1()
	{
		int new_limitprice;
		LOG("???????????limitprice??\n");
		cin >> new_limitprice;

		int new_StopPrice;
		LOG("???????????stopprice??\n");
		cin >> new_StopPrice;

		CThostFtdcInputOrderField ord = { 0 };
		strcpy_s(ord.BrokerID, g_chBrokerID);
		strcpy_s(ord.InvestorID, g_chInvestorID);
		strcpy_s(ord.InstrumentID, g_chInstrumentID);
		strcpy_s(ord.UserID, g_chUserID);
		OrderRef_num++;
		itoa(OrderRef_num, ord.OrderRef, 10);
		ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		ord.Direction = THOST_FTDC_D_Buy;//??
		ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		ord.LimitPrice = new_limitprice;
		ord.VolumeTotalOriginal = 1;
		ord.TimeCondition = THOST_FTDC_TC_GFD;
		ord.VolumeCondition = THOST_FTDC_VC_CV;
		ord.MinVolume = 1;
		ord.ContingentCondition = THOST_FTDC_CC_Touch;
		ord.StopPrice = new_StopPrice;
		ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		ord.IsAutoSuspend = 0;
		strcpy_s(ord.ExchangeID, g_chExchangeID);
		int a = m_pUserApi->ReqOrderInsert(&ord, 1);
		LOG((a == 0) ? "?????????????????......??????\n" : "?????????????????......???????????=[%d]\n", a);
	}
	///???????屑????
	void ReqOrderInsert_Touch()
	{
		int new_StopPrice;
		LOG("???????????stopprice??\n");
		cin >> new_StopPrice;

		CThostFtdcInputOrderField ord = { 0 };
		strcpy_s(ord.BrokerID, g_chBrokerID);
		strcpy_s(ord.InvestorID, g_chInvestorID);
		strcpy_s(ord.InstrumentID, g_chInstrumentID);
		strcpy_s(ord.UserID, g_chUserID);
		OrderRef_num++;
		itoa(OrderRef_num, ord.OrderRef, 10);
		ord.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
		ord.Direction = THOST_FTDC_D_Buy;//??
		ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		ord.LimitPrice = 0;//?屑???0????
		ord.VolumeTotalOriginal = 1;
		ord.TimeCondition = THOST_FTDC_TC_GFD;
		ord.VolumeCondition = THOST_FTDC_VC_AV;
		ord.MinVolume = 0;
		ord.ContingentCondition = THOST_FTDC_CC_Touch;
		ord.StopPrice = new_StopPrice;
		ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		ord.IsAutoSuspend = 0;
		strcpy_s(ord.ExchangeID, g_chExchangeID);
		int a = m_pUserApi->ReqOrderInsert(&ord, 1);
		LOG((a == 0) ? "????????????屑????......??????\n" : "????????????屑????......???????????=[%d]\n", a);
	}

	///??????????
	void ReqOrderInsert_TouchProfit()
	{
		int new_limitprice;
		LOG("????????????limitprice??\n");
		cin >> new_limitprice;

		int new_StopPrice;
		LOG("????????????stopprice??\n");
		cin >> new_StopPrice;

		CThostFtdcInputOrderField ord = { 0 };
		strcpy_s(ord.BrokerID, g_chBrokerID);
		strcpy_s(ord.InvestorID, g_chInvestorID);
		strcpy_s(ord.InstrumentID, g_chInstrumentID);
		strcpy_s(ord.UserID, g_chUserID);
		OrderRef_num++;
		itoa(OrderRef_num, ord.OrderRef, 10);
		//strcpy_s(ord.OrderRef, "");
		ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		ord.Direction = THOST_FTDC_D_Buy;//??
		ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		//ord.LimitPrice = atoi(getConfig("config", "LimitPrice").c_str());
		ord.LimitPrice = new_limitprice;
		ord.VolumeTotalOriginal = 1;
		ord.TimeCondition = THOST_FTDC_TC_GFD;///??????效
		ord.VolumeCondition = THOST_FTDC_VC_AV;///???????
		ord.MinVolume = 1;
		ord.ContingentCondition = THOST_FTDC_CC_TouchProfit;
		ord.StopPrice = new_StopPrice;
		ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		ord.IsAutoSuspend = 0;
		strcpy_s(ord.ExchangeID, g_chExchangeID);
		int a = m_pUserApi->ReqOrderInsert(&ord, 1);
		LOG((a == 0) ? "???????????????......??????\n" : "???????????????......???????????=[%d]\n", a);
	}

	//?????? FOK
	void ReqOrderInsert_VC_CV()
	{
		int new_limitprice;
		LOG("????????????\n");
		cin >> new_limitprice;

		int insert_num;
		LOG("???????????????\n");
		cin >> insert_num;

		CThostFtdcInputOrderField ord = { 0 };
		strcpy_s(ord.BrokerID, g_chBrokerID);
		strcpy_s(ord.InvestorID, g_chInvestorID);
		strcpy_s(ord.InstrumentID, g_chInstrumentID);
		strcpy_s(ord.UserID, g_chUserID);
		OrderRef_num++;
		itoa(OrderRef_num, ord.OrderRef, 10);
		//strcpy_s(ord.OrderRef, "");
		ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		ord.Direction = THOST_FTDC_D_Buy;//??
		ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		//ord.LimitPrice = atoi(getConfig("config", "LimitPrice").c_str());
		ord.LimitPrice = new_limitprice;
		ord.VolumeTotalOriginal = insert_num;
		ord.TimeCondition = THOST_FTDC_TC_GFD;///??????效
		ord.VolumeCondition = THOST_FTDC_VC_CV;///???????
		ord.MinVolume = 1;
		ord.ContingentCondition = THOST_FTDC_CC_Immediately;
		ord.StopPrice = 0;
		ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		ord.IsAutoSuspend = 0;
		strcpy_s(ord.ExchangeID, g_chExchangeID);
		int a = m_pUserApi->ReqOrderInsert(&ord, 1);
		LOG((a == 0) ? "???????????????......??????\n" : "???????????????......???????????=[%d]\n", a);
	}

	//??????? FAK
	void ReqOrderInsert_VC_AV()
	{
		int new_limitprice;
		LOG("????????????\n");
		cin >> new_limitprice;

		int insert_num;
		LOG("???????????????\n");
		cin >> insert_num;

		CThostFtdcInputOrderField ord = { 0 };
		strcpy_s(ord.BrokerID, g_chBrokerID);
		strcpy_s(ord.InvestorID, g_chInvestorID);
		strcpy_s(ord.InstrumentID, g_chInstrumentID);
		strcpy_s(ord.UserID, g_chUserID);
		OrderRef_num++;
		itoa(OrderRef_num, ord.OrderRef, 10);
		//strcpy_s(ord.OrderRef, "");
		ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		ord.Direction = THOST_FTDC_D_Buy;//??
		ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		//ord.LimitPrice = atoi(getConfig("config", "LimitPrice").c_str());
		ord.LimitPrice = new_limitprice;
		ord.VolumeTotalOriginal = insert_num;
		ord.TimeCondition = THOST_FTDC_TC_IOC;///??????????????
		ord.VolumeCondition = THOST_FTDC_VC_AV;///?魏?????
		ord.MinVolume = 2;
		ord.ContingentCondition = THOST_FTDC_CC_Immediately;
		ord.StopPrice = 0;
		ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		ord.IsAutoSuspend = 0;
		strcpy_s(ord.ExchangeID, g_chExchangeID);
		int a = m_pUserApi->ReqOrderInsert(&ord, 1);
		LOG((a == 0) ? "???????????????......??????\n" : "???????????????......???????????=[%d]\n", a);
	}

	//?屑??
	void ReqOrderInsert_AnyPrice()
	{
		CThostFtdcInputOrderField ord = { 0 };
		strcpy_s(ord.BrokerID, g_chBrokerID);
		strcpy_s(ord.InvestorID, g_chInvestorID);
		strcpy_s(ord.InstrumentID, g_chInstrumentID);
		strcpy_s(ord.UserID, g_chUserID);
		OrderRef_num++;
		itoa(OrderRef_num, ord.OrderRef, 10);
		//strcpy_s(ord.OrderRef, "");
		ord.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
		ord.Direction = THOST_FTDC_D_Buy;//??
		ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		//ord.LimitPrice = atoi(getConfig("config", "LimitPrice").c_str());
		//ord.LimitPrice = new_limitprice;
		ord.LimitPrice = 0;//???
		ord.VolumeTotalOriginal = 40;
		ord.TimeCondition = THOST_FTDC_TC_IOC;///??????????????
		ord.VolumeCondition = THOST_FTDC_VC_AV;///?魏?????
		ord.MinVolume = 1;
		ord.ContingentCondition = THOST_FTDC_CC_Immediately;//????
		//ord.StopPrice = 0;
		ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;//????
		ord.IsAutoSuspend = 0;
		strcpy_s(ord.ExchangeID, g_chExchangeID);
		int a = m_pUserApi->ReqOrderInsert(&ord, 1);
		LOG((a == 0) ? "???????????????......??????\n" : "???????????????......???????????=[%d]\n", a);
	}

	//?屑??????(?薪???)
	void ReqOrderInsert_BestPrice()
	{
		CThostFtdcInputOrderField ord = { 0 };
		strcpy_s(ord.BrokerID, g_chBrokerID);
		strcpy_s(ord.InvestorID, g_chInvestorID);
		strcpy_s(ord.InstrumentID, g_chInstrumentID);
		strcpy_s(ord.UserID, g_chUserID);
		OrderRef_num++;
		itoa(OrderRef_num, ord.OrderRef, 10);
		//strcpy_s(ord.OrderRef, "");
		ord.OrderPriceType = THOST_FTDC_OPT_BestPrice;
		ord.Direction = THOST_FTDC_D_Buy;//??
		ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		//ord.LimitPrice = atoi(getConfig("config", "LimitPrice").c_str());
		//ord.LimitPrice = new_limitprice;
		ord.VolumeTotalOriginal = 1;
		ord.TimeCondition = THOST_FTDC_TC_GFD;///??????效
		ord.VolumeCondition = THOST_FTDC_VC_AV;///?魏?????
		ord.MinVolume = 1;
		ord.ContingentCondition = THOST_FTDC_CC_Immediately;
		ord.StopPrice = 0;
		ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		ord.IsAutoSuspend = 0;
		strcpy_s(ord.ExchangeID, g_chExchangeID);
		int a = m_pUserApi->ReqOrderInsert(&ord, 1);
		LOG((a == 0) ? "???????????????......??????\n" : "???????????????......???????????=[%d]\n", a);
	}

	//???????
	void ReqOrderInsert_Arbitrage()
	{
		int new_limitprice;
		LOG("????????????\n");
		cin >> new_limitprice;

		CThostFtdcInputOrderField ord = { 0 };
		strcpy_s(ord.BrokerID, g_chBrokerID);
		strcpy_s(ord.InvestorID, g_chInvestorID);
		strcpy_s(ord.InstrumentID, g_chInstrumentID);
		strcpy_s(ord.UserID, g_chUserID);
		OrderRef_num++;
		itoa(OrderRef_num, ord.OrderRef, 10);
		//strcpy_s(ord.OrderRef, "");
		ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		ord.Direction = THOST_FTDC_D_Buy;//??
		ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		//ord.LimitPrice = atoi(getConfig("config", "LimitPrice").c_str());
		ord.LimitPrice = new_limitprice;
		ord.VolumeTotalOriginal = 1;
		ord.TimeCondition = THOST_FTDC_TC_GFD;///??????效
		ord.VolumeCondition = THOST_FTDC_VC_AV;///?魏?????
		ord.MinVolume = 1;
		ord.ContingentCondition = THOST_FTDC_CC_Immediately;
		ord.StopPrice = 0;
		ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		ord.IsAutoSuspend = 0;
		strcpy_s(ord.ExchangeID, g_chExchangeID);
		int a = m_pUserApi->ReqOrderInsert(&ord, 1);
		LOG((a == 0) ? "???????????????......??????\n" : "???????????????......???????????=[%d]\n", a);
	}

	//??????
	void ReqOrderInsert_IsSwapOrder()
	{
		int new_limitprice;
		LOG("????????????\n");
		cin >> new_limitprice;

		CThostFtdcInputOrderField ord = { 0 };
		strcpy_s(ord.BrokerID, g_chBrokerID);
		strcpy_s(ord.InvestorID, g_chInvestorID);
		strcpy_s(ord.InstrumentID, g_chInstrumentID);
		strcpy_s(ord.UserID, g_chUserID);
		OrderRef_num++;
		itoa(OrderRef_num, ord.OrderRef, 10);
		//strcpy_s(ord.OrderRef, "");
		ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		ord.Direction = THOST_FTDC_D_Buy;//??
		ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		//ord.LimitPrice = atoi(getConfig("config", "LimitPrice").c_str());
		ord.LimitPrice = new_limitprice;
		ord.VolumeTotalOriginal = 1;
		ord.TimeCondition = THOST_FTDC_TC_GFD;///??????效
		ord.VolumeCondition = THOST_FTDC_VC_AV;///?魏?????
		ord.MinVolume = 1;
		ord.ContingentCondition = THOST_FTDC_CC_Immediately;
		ord.StopPrice = 0;
		ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		ord.IsAutoSuspend = 0;
		ord.IsSwapOrder = 1;//?????????
		strcpy_s(ord.ExchangeID, g_chExchangeID);
		int a = m_pUserApi->ReqOrderInsert(&ord, 1);
		LOG((a == 0) ? "???????????????......??????\n" : "???????????????......???????????=[%d]\n", a);
	}

	///??????????????
	void ReqCombActionInsert()
	{
		CThostFtdcInputCombActionField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		//strcpy_s(a.CombActionRef,"");//???????
		a.Direction = THOST_FTDC_D_Sell;//??
		a.Volume = 1;
		a.CombDirection = THOST_FTDC_CMDR_Comb;
		a.HedgeFlag = THOST_FTDC_HF_Speculation;//???
		strcpy_s(a.ExchangeID, g_chExchangeID);
		int ab = m_pUserApi->ReqCombActionInsert(&a, nRequestID++);
		LOG((ab == 0) ? "??????????????......??????\n" : "??????????????......???????????=[%d]\n", ab);
	}


	///???????????
	void ReqOffsetSetting()
	{
		LOG("ReqOffsetSetting is not supported in current API header version.\n");
	}

	///??????贸???????
	void ReqCancelOffsetSetting()
	{
		LOG("ReqCancelOffsetSetting is not supported in current API header version.\n");
	}

	///??????????貌??
	void ReqQryOffsetSetting()
	{
		LOG("ReqQryOffsetSetting is not supported in current API header version.\n");
	}

	///鎶ュ崟鎿嶄綔璇锋眰
	void ReqOrderAction_Ordinary()
	{
		CThostFtdcInputOrderActionField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		a.OrderActionRef = 1;
		strcpy_s(a.OrderRef, g_chOrderRef);
		//a.FrontID = g_chFrontID;
		//a.SessionID = g_chSessionID;
		strcpy_s(a.ExchangeID, g_chExchangeID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		strcpy_s(a.OrderSysID, g_chOrderSysID);
		a.ActionFlag = THOST_FTDC_AF_Delete;
		strcpy_s(a.UserID, g_chUserID);
		int ab = m_pUserApi->ReqOrderAction(&a, nRequestID++);
        if (ab == 0)
        {
            TrackCancelCount();
            TrackOrderRateAndDuplicate(string("CANCEL|") + a.ExchangeID + "|" + a.InstrumentID + "|" + a.OrderRef + "|" + a.OrderSysID);
        }
		LOG((ab == 0) ? "鎶ュ崟鎿嶄綔璇锋眰......鍙戦€佹垚鍔焅n" : "鎶ュ崟鎿嶄綔璇锋眰......鍙戦€佸け璐ワ紝搴忓彿=[%d]\n", ab);
	}

	///??????????????
	void ReqExecOrderInsert(int a)
	{
		CThostFtdcInputExecOrderField OrderInsert = { 0 };
		strcpy_s(OrderInsert.BrokerID, g_chBrokerID);
		strcpy_s(OrderInsert.InvestorID, g_chInvestorID);
		strcpy_s(OrderInsert.InstrumentID, g_chInstrumentID);
		strcpy_s(OrderInsert.ExchangeID, g_chExchangeID);
		//strcpy_s(OrderInsert.ExecOrderRef, "00001");
		strcpy_s(OrderInsert.UserID, g_chUserID);
		OrderInsert.Volume = 1;
		OrderInsert.RequestID = 1;
		OrderInsert.OffsetFlag = THOST_FTDC_OF_Close;//??????
		OrderInsert.HedgeFlag = THOST_FTDC_HF_Speculation;//?????????
		if (a == 0) {
			OrderInsert.ActionType = THOST_FTDC_ACTP_Exec;//???????????
		}
		if (a == 1) {
			OrderInsert.ActionType = THOST_FTDC_ACTP_Abandon;//???????????
		}
		OrderInsert.PosiDirection = THOST_FTDC_PD_Long;//????????????
		OrderInsert.ReservePositionFlag = THOST_FTDC_EOPF_Reserve;//??????????????????????????
		//OrderInsert.ReservePositionFlag = THOST_FTDC_EOPF_UnReserve;//?????????
		OrderInsert.CloseFlag = THOST_FTDC_EOCF_NotToClose;//?????????????????????????????
		//OrderInsert.CloseFlag = THOST_FTDC_EOCF_AutoClose;//??????
		//strcpy_s(OrderInsert.InvestUnitID, "");AccountID
		//strcpy_s(OrderInsert.AccountID, "");
		//strcpy_s(OrderInsert.CurrencyID, "CNY");
		//strcpy_s(OrderInsert.ClientID, "");
		int b = m_pUserApi->ReqExecOrderInsert(&OrderInsert, 1);
		LOG((b == 0) ? "??????????????......??????\n" : "??????????????......???????????????=[%d]\n", b);
	}

	///??????????????
	void ReqExecOrderAction()
	{
		CThostFtdcInputExecOrderActionField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		a.ExecOrderActionRef = 1;
		strcpy_s(a.ExecOrderRef, g_NewExecOrderRef);
		a.FrontID = g_NewFrontID;
		a.SessionID = g_NewSessionID;
		strcpy_s(a.ExchangeID, g_chExchangeID);
		strcpy_s(a.ExecOrderSysID, g_NewExecOrderSysID);
		a.ActionFlag = THOST_FTDC_AF_Delete;//???
		strcpy_s(a.UserID, g_chUserID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		//strcpy_s(a.InvestUnitID, "");
		//strcpy_s(a.IPAddress, "");
		//strcpy_s(a.MacAddress, "");
		int b = m_pUserApi->ReqExecOrderAction(&a, 1);
		LOG((b == 0) ? "??????????????......??????\n" : "??????????????......???????????????=[%d]\n", b);
	}

	//????????????????
	void ReqBatchOrderAction()
	{
		CThostFtdcInputBatchOrderActionField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		//a.OrderActionRef = 1;
		a.FrontID = g_chFrontID;
		//a.SessionID = g_chSessionID;
		strcpy_s(a.ExchangeID,g_chExchangeID);
		strcpy_s(a.UserID,g_chUserID);
		int ab = m_pUserApi->ReqBatchOrderAction(&a, nRequestID++);
		LOG((ab == 0) ? "????????????????......??????\n" : "????????????????......???????????=[%d]\n", ab);
	}

	///??????????
	void ReqQryOrder()
	{
		action_number = 0;
		vector_OrderSysID.clear();
		vector_ExchangeID.clear();
		vector_InstrumentID.clear();
		vector_OrderRef.clear();
		vector_FrontID.clear();
		vector_SessionID.clear();
		vector_OrderStatus.clear();
		CThostFtdcQryOrderField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		//strcpy_s(a.InstrumentID, g_chInstrumentID);
		//strcpy_s(a.ExchangeID, g_chExchangeID);
		int ab = m_pUserApi->ReqQryOrder(&a, nRequestID++);
		LOG((ab == 0) ? "??????????......??????\n" : "??????????......???????????=[%d]\n", ab);
	}

	static bool IsCancellableOrderStatus(char s)
	{
		return s == THOST_FTDC_OST_NoTradeQueueing ||
			s == THOST_FTDC_OST_PartTradedQueueing ||
			s == THOST_FTDC_OST_NotTouched ||
			s == THOST_FTDC_OST_Touched;
	}

	// One-key: query orders then cancel all active/unfilled ones.
	void OneKeyCancelAllActiveOrders()
	{
		LOG("[ONE_KEY_CANCEL] start: query orders then cancel active ones\n");

		// Best-effort wait for query completion (OnRspQryOrder bIsLast -> SetEvent)
		ResetEvent(g_hEvent);
		ReqQryOrder();
		DWORD waitRet = WaitForSingleObject(g_hEvent, 8000);
		if (waitRet != WAIT_OBJECT_0)
		{
			LOG("[ONE_KEY_CANCEL] warning: query wait timeout or failed, proceed with current list\n");
		}

		int total = static_cast<int>(vector_OrderSysID.size());
		int toCancel = 0;
		for (int i = 0; i < total; ++i)
		{
			if (i < static_cast<int>(vector_OrderStatus.size()) && IsCancellableOrderStatus(vector_OrderStatus[i]))
			{
				++toCancel;
			}
		}

		LOG("[ONE_KEY_CANCEL] orders: total=%d active_to_cancel=%d\n", total, toCancel);

		int sent = 0;
		for (int i = 0; i < total; ++i)
		{
			if (i < static_cast<int>(vector_OrderStatus.size()) && !IsCancellableOrderStatus(vector_OrderStatus[i]))
			{
				continue;
			}

			ReqOrderAction_forqry(i + 1);
			++sent;

			// Avoid flooding the front; keep it simple and conservative.
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		LOG("[ONE_KEY_CANCEL] done: cancel_requests_sent=%d\n", sent);
	}

	///???????????
	void ReqOrderInsert_Condition(int select_num)
	{
		string limit_price;
		LOG("????????????(limitprice):\n");
		cin >> limit_price;

		string stop_price;
		LOG("???????????(stopprice):\n");
		cin >> stop_price;

		CThostFtdcInputOrderField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		strcpy_s(a.UserID, g_chUserID);
		a.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
		a.Direction = THOST_FTDC_D_Buy;//??
		//a.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		a.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		strcpy_s(a.CombOffsetFlag, "0");
		strcpy_s(a.CombHedgeFlag, "1");
		a.LimitPrice = atoi(limit_price.c_str());
		a.VolumeTotalOriginal = 1;
		a.TimeCondition = THOST_FTDC_TC_GFD;
		//a.VolumeCondition = THOST_FTDC_VC_AV;
		a.VolumeCondition = THOST_FTDC_VC_MV;
		a.MinVolume = 0;
		if (select_num == 1)
		{
			a.ContingentCondition = THOST_FTDC_CC_LastPriceGreaterThanStopPrice;
		}
		else if (select_num == 2)
		{
			a.ContingentCondition = THOST_FTDC_CC_LastPriceGreaterEqualStopPrice;
		}
		else if (select_num == 3)
		{
			a.ContingentCondition = THOST_FTDC_CC_LastPriceLesserThanStopPrice;
		}
		else if (select_num == 4)
		{
			a.ContingentCondition = THOST_FTDC_CC_LastPriceLesserEqualStopPrice;
		}
		else if (select_num == 5)
		{
			a.ContingentCondition = THOST_FTDC_CC_AskPriceGreaterThanStopPrice;
		}
		else if (select_num == 6)
		{
			a.ContingentCondition = THOST_FTDC_CC_AskPriceGreaterEqualStopPrice;
		}
		else if (select_num == 7)
		{
			a.ContingentCondition = THOST_FTDC_CC_AskPriceLesserThanStopPrice;
		}
		else if (select_num == 8)
		{
			a.ContingentCondition = THOST_FTDC_CC_AskPriceLesserEqualStopPrice;
		}
		else if (select_num == 9)
		{
			a.ContingentCondition = THOST_FTDC_CC_BidPriceGreaterThanStopPrice;
		}
		else if (select_num == 10)
		{
			a.ContingentCondition = THOST_FTDC_CC_BidPriceGreaterEqualStopPrice;
		}
		else if (select_num == 11)
		{
			a.ContingentCondition = THOST_FTDC_CC_BidPriceLesserThanStopPrice;
		}
		else if (select_num == 12)
		{
			a.ContingentCondition = THOST_FTDC_CC_BidPriceLesserEqualStopPrice;
		}
		a.StopPrice = atoi(stop_price.c_str());
		//itoa(a.StopPrice, const_cast<char *>(getConfig("config", "StopPrice").c_str()), 10);
		a.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		a.IsAutoSuspend = 0;
		strcpy_s(a.ExchangeID, g_chExchangeID);
		int ab = m_pUserApi->ReqOrderInsert(&a, nRequestID++);
		LOG((ab == 0) ? "????????????......??????\n" : "????????????......???????????=[%d]\n", ab);
	}

	///????????????
	void ReqOrderAction_Condition()
	{
		CThostFtdcInputOrderActionField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		strcpy_s(a.UserID, g_chUserID);
		if (chioce_action == 0)
		{
			a.FrontID = g_chFrontID;
			a.SessionID = g_chSessionID;
			strcpy_s(a.OrderRef, g_chOrderRef);
		}
		if (chioce_action == 1)
		{
			strcpy_s(a.OrderSysID, g_chOrderSysID);
		}
		strcpy_s(a.ExchangeID, g_chExchangeID);
		a.ActionFlag = THOST_FTDC_AF_Delete;
		int ab = m_pUserApi->ReqOrderAction(&a, nRequestID++);
        if (ab == 0)
        {
            TrackCancelCount();
            TrackOrderRateAndDuplicate(string("CANCEL|") + a.ExchangeID + "|" + a.InstrumentID + "|" + a.OrderRef + "|" + a.OrderSysID);
        }
		LOG((ab == 0) ? "????????????......??????\n" : "????????????......???????????=[%d]\n", ab);
	}

	//????????????
	void ReqOrderAction_forqry(int action_num)
	{
		CThostFtdcInputOrderActionField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		//strcpy_s(a.UserID, g_chUserID);

		strcpy_s(a.OrderSysID, vector_OrderSysID.at(action_num - 1).c_str());
		strcpy_s(a.ExchangeID, vector_ExchangeID.at(action_num - 1).c_str());
		strcpy_s(a.InstrumentID, vector_InstrumentID.at(action_num - 1).c_str());

		strcpy_s(a.OrderRef, vector_OrderRef.at(action_num - 1).c_str());
		a.FrontID = vector_FrontID.at(action_num - 1);
		a.SessionID = vector_SessionID.at(action_num - 1);

		a.ActionFlag = THOST_FTDC_AF_Delete;
		int ab = m_pUserApi->ReqOrderAction(&a, nRequestID++);
        if (ab == 0)
        {
            TrackCancelCount();
            TrackOrderRateAndDuplicate(string("CANCEL|") + a.ExchangeID + "|" + a.InstrumentID + "|" + a.OrderRef + "|" + a.OrderSysID);
        }
		LOG((ab == 0) ? "????????????......??????\n" : "????????????......???????????????=[%d]\n", ab);
	}

	///?????????
	void ReqQryTrade()
	{
		CThostFtdcQryTradeField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		string instr;
		instr.clear();
		LOG("????????????(???????????)\n");
		cin.ignore();
		getline(cin, instr);
		strcpy_s(a.InstrumentID, instr.c_str());

		string Exch;
		Exch.clear();
		LOG("??????????????(???????????)\n");
		//cin.ignore();
		getline(cin, Exch);
		strcpy_s(a.ExchangeID, Exch.c_str());
		/*strcpy_s(a.TradeID, "");
		strcpy_s(a.TradeTimeStart, "");
		strcpy_s(a.TradeTimeEnd, "");*/
		int b = m_pUserApi->ReqQryTrade(&a, nRequestID++);
		LOG((b == 0) ? "?????????......??????\n" : "?????????......???????????????=[%d]\n", b);
	}

	///?????????
	void ReqQryParkedOrder()
	{
		CThostFtdcQryParkedOrderField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		//strcpy_s(a.InstrumentID, g_chInstrumentID);
		strcpy_s(a.ExchangeID, g_chExchangeID);
		int ab = m_pUserApi->ReqQryParkedOrder(&a, nRequestID++);
		LOG((ab == 0) ? "?????????......??????\n" : "?????????......???????????=[%d]\n", ab);
	}

	//????????????????
	void ReqQryParkedOrderAction()
	{
		CThostFtdcQryParkedOrderActionField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		strcpy_s(a.ExchangeID, g_chExchangeID);
		int ab = m_pUserApi->ReqQryParkedOrderAction(&a, nRequestID++);
		LOG((ab == 0) ? "????????????????......??????\n" : "????????????????......???????????=[%d]\n", ab);
	}

	//????????????
	void ReqQryTradingAccount()
	{
		CThostFtdcQryTradingAccountField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.CurrencyID, "CNY");
		int ab = m_pUserApi->ReqQryTradingAccount(&a, nRequestID++);
		LOG((ab == 0) ? "????????????......??????\n" : "????????????......???????????=[%d]\n", ab);
	}

	///???????????????
	virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		CTraderSpi::OnRspQryTradingAccount(pTradingAccount, pRspInfo, nRequestID, bIsLast);
		LogTraderErrorDetail("OnRspQryTradingAccount", pRspInfo);
		//ReqQryTradingAccount();
	};

	//?????????????
	void ReqQryInvestorPosition()
	{
		CThostFtdcQryInvestorPositionField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
	/*	string instr;
		instr.clear();
		cin.ignore();
		LOG("????????????(???????????)??\n");
		getline(cin, instr);
		strcpy_s(a.InstrumentID, instr.c_str());*/
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		/*string exch;
		exch.clear();
		cin.ignore();
		LOG("??????????????(???????????)??\n");
		getline(cin, exch);
		strcpy_s(a.ExchangeID, exch.c_str());*/
		//strcpy_s(a.InstrumentID, "SPD");
		int b = m_pUserApi->ReqQryInvestorPosition(&a, nRequestID++);
		LOG((b == 0) ? "?????????????......??????\n" : "?????????????......???????????????=[%d]\n", b);
	}

	//????????????????
	void ReqQryInvestorPositionDetail()
	{
		CThostFtdcQryInvestorPositionDetailField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		/*string instr;
		instr.clear();
		cin.ignore();
		LOG("????????????(???????????)\n");
		getline(cin, instr);
		strcpy_s(a.InstrumentID, instr.c_str());*/

		//strcpy_s(a.InstrumentID, g_chInstrumentID);
		/*string exch;
		exch.clear();
		cin.ignore();
		LOG("??????????????(???????????)??\n");
		getline(cin, exch);
		strcpy_s(a.ExchangeID, exch.c_str());*/

		//strcpy_s(a.InstrumentID, g_chInstrumentID);
		int b = m_pUserApi->ReqQryInvestorPositionDetail(&a, nRequestID++);
		LOG((b == 0) ? "????????????????......??????\n" : "????????????????......???????????????=[%d]\n", b);
	}

	//???????????????????
	void ReqQryExchangeMarginRate()
	{
		CThostFtdcQryExchangeMarginRateField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		a.HedgeFlag = THOST_FTDC_HF_Speculation;//???
		int b = m_pUserApi->ReqQryExchangeMarginRate(&a, nRequestID++);
		LOG((b == 0) ? "???????????????????......??????\n" : "???????????????????......???????????????=[%d]\n", b);
	}

	//????????????????
	void ReqQryInstrumentMarginRate()
	{
		CThostFtdcQryInstrumentMarginRateField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.ExchangeID,g_chExchangeID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		a.HedgeFlag = THOST_FTDC_HF_Speculation;//???
		int b = m_pUserApi->ReqQryInstrumentMarginRate(&a, nRequestID++);
		LOG((b == 0) ? "????????????????......??????\n" : "????????????????......???????????????=[%d]\n", b);
	}

	//?????????????????
	void ReqQryInstrumentCommissionRate()
	{
		CThostFtdcQryInstrumentCommissionRateField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		strcpy_s(a.ExchangeID, g_chExchangeID);
		int b = m_pUserApi->ReqQryInstrumentCommissionRate(&a, nRequestID++);
		LOG((b == 0) ? "?????????????????......??????\n" : "?????????????????......???????????????=[%d]\n", b);
	}

	//??????????????????????
	void ReqQryMMInstrumentCommissionRate()
	{
		CThostFtdcQryMMInstrumentCommissionRateField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		int b = m_pUserApi->ReqQryMMInstrumentCommissionRate(&a, nRequestID++);
		LOG((b == 0) ? "??????????????????????......??????\n" : "??????????????????????......???????????????=[%d]\n", b);
	}

	//????????????????????????
	void ReqQryMMOptionInstrCommRate()
	{
		CThostFtdcQryMMOptionInstrCommRateField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		int b = m_pUserApi->ReqQryMMOptionInstrCommRate(&a, nRequestID++);
		LOG((b == 0) ? "????????????????????????......??????\n" : "????????????????????????......???????????????=[%d]\n", b);
	}

	//????????????????
	void ReqQryInstrumentOrderCommRate()
	{
		CThostFtdcQryInstrumentOrderCommRateField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		int b = m_pUserApi->ReqQryInstrumentOrderCommRate(&a, nRequestID++);
		LOG((b == 0) ? "????????????????......??????\n" : "????????????????......???????????????=[%d]\n", b);
	}

	//??????????????????
	void ReqQryOptionInstrCommRate()
	{
		CThostFtdcQryOptionInstrCommRateField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		string Inst;
		string Exch;
		string InvestUnit;
		LOG("????????????:(?????????)");
		cin >> Inst;
		LOG("??????????????:(?????????)");
		cin >> Exch;
		LOG("?????????????????:(?????????)");
		cin >> InvestUnit;
		strcpy_s(a.InstrumentID, Inst.c_str());
		strcpy_s(a.ExchangeID, Exch.c_str());
		strcpy_s(a.InvestUnitID, InvestUnit.c_str());
		int b = m_pUserApi->ReqQryOptionInstrCommRate(&a, nRequestID++);
		LOG((b == 0) ? "??????????????????......??????\n" : "??????????????????......???????????????=[%d]\n", b);
	}

	//?????????
	void ReqQryInstrument()
	{
		CThostFtdcQryInstrumentField a = { 0 };
		strcpy_s(a.ExchangeID, g_chExchangeID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		//strcpy_s(a.ExchangeInstID,"");
		//strcpy_s(a.ProductID, "IO");
		int b = m_pUserApi->ReqQryInstrument(&a, nRequestID++);
		LOG((b == 0) ? "?????????......??????\n" : "?????????......???????????????=[%d]\n", b);
	}

	///????????????
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		CTraderSpi::OnRspQryInstrument(pInstrument, pRspInfo, nRequestID, bIsLast);
		LogTraderErrorDetail("OnRspQryInstrument", pRspInfo);
		if (pInstrument)
		{
			md_InstrumentID.push_back(pInstrument->InstrumentID);
			SimpleInstrumentInfo info;
			info.valid = true;
			info.exchangeId = pInstrument->ExchangeID;
			info.instrumentId = pInstrument->InstrumentID;
			info.priceTick = pInstrument->PriceTick;
			info.maxLimitOrderVolume = pInstrument->MaxLimitOrderVolume;
			info.minLimitOrderVolume = pInstrument->MinLimitOrderVolume;
			g_instrumentInfoCache[info.instrumentId] = info;
		}
		if (bIsLast)
		{
			SetEvent(xinhao);
		}
	}

	virtual void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField* pDepthMarketData, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		CTraderSpi::OnRspQryDepthMarketData(pDepthMarketData, pRspInfo, nRequestID, bIsLast);
		LogTraderErrorDetail("OnRspQryDepthMarketData", pRspInfo);
		if (pDepthMarketData)
		{
			SimpleDepthLimits lim;
			lim.valid = true;
			lim.exchangeId = pDepthMarketData->ExchangeID;
			lim.instrumentId = pDepthMarketData->InstrumentID;
			lim.upperLimitPrice = pDepthMarketData->UpperLimitPrice;
			lim.lowerLimitPrice = pDepthMarketData->LowerLimitPrice;
			lim.bandingUpperPrice = pDepthMarketData->BandingUpperPrice;
			lim.bandingLowerPrice = pDepthMarketData->BandingLowerPrice;
			g_depthLimitCache[lim.instrumentId] = lim;
		}
		if (bIsLast)
		{
			SetEvent(xinhao);
		}
	}

	bool EnsureInstrumentInfo(const string& instrumentId, const string& exchangeId, SimpleInstrumentInfo& outInfo)
	{
		map<string, SimpleInstrumentInfo>::const_iterator it = g_instrumentInfoCache.find(instrumentId);
		if (it != g_instrumentInfoCache.end() && it->second.valid)
		{
			if (exchangeId.empty() || it->second.exchangeId == exchangeId)
			{
				outInfo = it->second;
				return true;
			}
		}

		CThostFtdcQryInstrumentField q = { 0 };
		if (!exchangeId.empty())
		{
			strcpy_s(q.ExchangeID, exchangeId.c_str());
		}
		strcpy_s(q.InstrumentID, instrumentId.c_str());
		TrackQueryRate("ReqQryInstrument(Validate)");
		int ret = m_pUserApi->ReqQryInstrument(&q, nRequestID++);
		LogRequestReturnStatus("TraderApi::ReqQryInstrument(Validate)", ret);
		if (ret != 0)
		{
			return false;
		}
		WaitForSingleObject(xinhao, 5000);

		it = g_instrumentInfoCache.find(instrumentId);
		if (it == g_instrumentInfoCache.end() || !it->second.valid)
		{
			return false;
		}
		if (!exchangeId.empty() && it->second.exchangeId != exchangeId)
		{
			return false;
		}
		outInfo = it->second;
		return true;
	}

	bool EnsureDepthLimits(const string& instrumentId, const string& exchangeId, SimpleDepthLimits& outLim)
	{
		map<string, SimpleDepthLimits>::const_iterator it = g_depthLimitCache.find(instrumentId);
		if (it != g_depthLimitCache.end() && it->second.valid)
		{
			if (exchangeId.empty() || it->second.exchangeId == exchangeId)
			{
				outLim = it->second;
				return true;
			}
		}

		CThostFtdcQryDepthMarketDataField q = { 0 };
		if (!exchangeId.empty())
		{
			strcpy_s(q.ExchangeID, exchangeId.c_str());
		}
		strcpy_s(q.InstrumentID, instrumentId.c_str());
		TrackQueryRate("ReqQryDepthMarketData(Validate)");
		int ret = m_pUserApi->ReqQryDepthMarketData(&q, nRequestID++);
		LogRequestReturnStatus("TraderApi::ReqQryDepthMarketData(Validate)", ret);
		if (ret != 0)
		{
			return false;
		}
		WaitForSingleObject(xinhao, 5000);

		it = g_depthLimitCache.find(instrumentId);
		if (it == g_depthLimitCache.end() || !it->second.valid)
		{
			return false;
		}
		if (!exchangeId.empty() && it->second.exchangeId != exchangeId)
		{
			return false;
		}
		outLim = it->second;
		return true;
	}

	// Order entry with local validation (instrument, tick size, price band, volume limits).
	// Used for test cases that require "reject wrong instruction before sending to CTP".
	void ReqOrderInsert_Ordinary_Checked()
	{
		system("cls");

		string instInput;
		string exchInput;
		string priceInput;
		int volume = 0;

		LOG("InstrumentID ('.' = use config %s):\n", g_chInstrumentID);
		cin >> instInput;
		if (instInput == ".") instInput = g_chInstrumentID;

		LOG("ExchangeID ('.' = use config %s):\n", g_chExchangeID);
		cin >> exchInput;
		if (exchInput == ".") exchInput = g_chExchangeID;

		LOG("LimitPrice:\n");
		cin >> priceInput;
		LOG("Volume:\n");
		cin >> volume;

		if (instInput.empty() || exchInput.empty())
		{
			LOG("[LOCAL_REJECT] reason=EMPTY_FIELD InstrumentID=[%s] ExchangeID=[%s]\n", instInput.c_str(), exchInput.c_str());
			return;
		}
		if (volume <= 0)
		{
			LOG("[LOCAL_REJECT] reason=BAD_VOLUME volume=[%d]\n", volume);
			return;
		}

		char* endp = nullptr;
		double limitPrice = std::strtod(priceInput.c_str(), &endp);
		if (endp == nullptr || *endp != '\0' || !std::isfinite(limitPrice) || limitPrice <= 0.0)
		{
			LOG("[LOCAL_REJECT] reason=BAD_PRICE price=[%s]\n", priceInput.c_str());
			return;
		}

		if (!IsWithinChinaTradingWindow())
		{
			// Use a standard CTP error code so it maps via error.xml.
			LogLocalApiError("ReqOrderInsert_Ordinary", 17, "LOCAL: Not in trading time (CN 08:30-15:30)");
			return;
		}

		// Simplified mode for alert-threshold testing: don't block order submission on
		// instrument/tick/limit checks (those checks are validated by CTP asynchronously).

		CThostFtdcInputOrderField ord = { 0 };
		strcpy_s(ord.InvestUnitID, "InvestUnitID");
		strcpy_s(ord.AccountID, "1");
		strcpy_s(ord.BrokerID, g_chBrokerID);
		strcpy_s(ord.InvestorID, g_chInvestorID);
		strcpy_s(ord.InstrumentID, instInput.c_str());
		strcpy_s(ord.UserID, g_chUserID);
		OrderRef_num++;
		itoa(OrderRef_num, ord.OrderRef, 10);
		ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;

		int num1;
	Direction2:
		LOG("Direction: 1=Buy 2=Sell\n");
		cin >> num1;
		if (num1 == 1) ord.Direction = THOST_FTDC_D_Buy;
		else if (num1 == 2) ord.Direction = THOST_FTDC_D_Sell;
		else { LOG("Bad direction.\n"); _getch(); goto Direction2; }

		int num2;
	Offset2:
		LOG("Offset: 1=Open 2=Close 4=CloseToday 5=CloseYesterday\n");
		cin >> num2;
		if (num2 == 1) ord.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
		else if (num2 == 2) ord.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
		else if (num2 == 4) ord.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
		else if (num2 == 5) ord.CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;
		else { LOG("Bad offset.\n"); _getch(); goto Offset2; }

		ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
		ord.LimitPrice = limitPrice;
		ord.VolumeTotalOriginal = volume;
		ord.TimeCondition = THOST_FTDC_TC_GFD;
		ord.VolumeCondition = THOST_FTDC_VC_AV;
		ord.MinVolume = 1;
		ord.ContingentCondition = THOST_FTDC_CC_Immediately;
		ord.StopPrice = 0;
		ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
		ord.IsAutoSuspend = 0;
		strcpy_s(ord.ExchangeID, exchInput.c_str());

		std::ostringstream orderFingerprint;
		orderFingerprint << ord.ExchangeID << "|" << ord.InstrumentID << "|"
			<< ord.Direction << "|" << ord.CombOffsetFlag[0] << "|"
			<< ord.LimitPrice << "|" << ord.VolumeTotalOriginal;

		int ret = m_pUserApi->ReqOrderInsert(&ord, 1);
		LogRequestReturnStatus("TraderApi::ReqOrderInsert(Checked)", ret);
		if (ret == 0)
		{
			TrackOrderCount();
			TrackOrderRateAndDuplicate(orderFingerprint.str());
		}
	}

	//????????????????
	void ReqQrySettlementInfo()
	{
		CThostFtdcQrySettlementInfoField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		string Traday;
		LOG("???????????????????路?(????:20180101,?路???201801):");
		cin >> Traday;
		strcpy_s(a.TradingDay, Traday.c_str());
		int b = m_pUserApi->ReqQrySettlementInfo(&a, nRequestID++);
		LOG((b == 0) ? "????????????????......??????\n" : "????????????????......???????????????=[%d]\n", b);
	}

	//????????????
	void ReqQryTransferSerial()
	{
		CThostFtdcQryTransferSerialField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.AccountID, g_chInvestorID);
	cir1:int bankid;
		LOG("?????????????????????\n");
		LOG("1.????????\n");
		LOG("2.??????\n");
		LOG("3.?泄?????\n");
		LOG("5.???????\n");
		LOG("6.????????\n");
		LOG("7.???????\n");
		LOG("8.???????\n");
		LOG("9.????????\n");
		LOG("10.???????\n");
		LOG("11.????????\n");
		LOG("12.???????\n");
		LOG("13.???????\n");
		LOG("14.???????\n");
		LOG("15.???????\n");
		LOG("16.??????\n");
		cin >> bankid;
		if (bankid == 1 | 2 | 3 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16)
		{
			//strcpy_s(a.BankID, itoa(bankid, a.BankID, 10));///???写???
			itoa(bankid, a.BankID, 10);
		}
		else
		{
			LOG("?????????????写???\n");
			goto cir1;
		}
		int choos;
	curr:LOG("????????????\t1.CNY\t2.USD\n");
		cin >> choos;
		switch (choos)
		{
		case 1:
			strcpy_s(a.CurrencyID, "CNY");
			break;
		case 2:
			strcpy_s(a.CurrencyID, "USD");
			break;
		default:
			LOG("??????????????\n");
			_getch();
			goto curr;
		}
		int b = m_pUserApi->ReqQryTransferSerial(&a, nRequestID++);
		LOG((b == 0) ? "????????????......??????\n" : "????????????......???????????????=[%d]\n", b);
	}

	//?????????
	void ReqQryProduct()
	{
		CThostFtdcQryProductField a = { 0 };
		strcpy_s(a.ProductID, "sc");
		a.ProductClass = THOST_FTDC_PC_Futures;
		strcpy_s(a.ExchangeID, g_chExchangeID);
		int ret = m_pUserApi->ReqQryProduct(&a, nRequestID++);
		LogRequestReturnStatus("TraderApi::ReqQryProduct", ret);
	}

	//?????????????
	void ReqQryTransferBank()
	{
		CThostFtdcQryTransferBankField a = { 0 };
		strcpy_s(a.BankID,"3");
		int b = m_pUserApi->ReqQryTransferBank(&a, nRequestID++);
		LOG((b == 0) ? "?????????????......??????\n" : "?????????????......???????????????=[%d]\n", b);
	}

	//????????????
	void ReqQryTradingNotice()
	{
		CThostFtdcQryTradingNoticeField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		int b = m_pUserApi->ReqQryTradingNotice(&a, nRequestID++);
		LOG((b == 0) ? "????????????......??????\n" : "????????????......???????????????=[%d]\n", b);
	}

	//???????????
	void ReqQryNotice()
	{
		CThostFtdcQryNoticeField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		int b = m_pUserApi->ReqQryNotice(&a, nRequestID++);
		LOG((b == 0) ? "???????????......??????\n" : "???????????......???????????????=[%d]\n", b);
	}

	//?????????????
	void ReqQryTradingCode()
	{
		CThostFtdcQryTradingCodeField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.ExchangeID, g_chExchangeID);
		a.ClientIDType = THOST_FTDC_CIDT_Speculation;
		int b = m_pUserApi->ReqQryTradingCode(&a, nRequestID++);
		LOG((b == 0) ? "?????????????......??????\n" : "?????????????......???????????????=[%d]\n", b);
	}

	//????????????????
	void ReqQrySettlementInfoConfirm()
	{
		CThostFtdcQrySettlementInfoConfirmField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		//strcpy_s(a.AccountID, g_chInvestorID);
		strcpy_s(a.CurrencyID, "CNY");
		int b = m_pUserApi->ReqQrySettlementInfoConfirm(&a, nRequestID++);
		LOG((b == 0) ? "????????????????......??????\n" : "????????????????......???????????????=[%d]\n", b);
	}

	//???????????
	void ReqQryProductGroup()
	{
		CThostFtdcQryProductGroupField a = { 0 };
		strcpy_s(a.ExchangeID, g_chExchangeID);
		//strcpy_s(a.ProductID, g_chInstrumentID);
		int b = m_pUserApi->ReqQryProductGroup(&a, nRequestID++);
		LOG((b == 0) ? "???????????......??????\n" : "???????????......???????????????=[%d]\n", b);
	}

	//?????????????
	void ReqQryInvestUnit()
	{
		CThostFtdcQryInvestUnitField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		//strcpy_s(a.InvestorID, g_chUserID);
		//strcpy_s(a.InvestUnitID, "");
		int b = m_pUserApi->ReqQryInvestUnit(&a, nRequestID++);
		LOG((b == 0) ? "?????????????......??????\n" : "?????????????......???????????????=[%d]\n", b);
	}

	//???????????????????
	void ReqQryBrokerTradingParams()
	{
		CThostFtdcQryBrokerTradingParamsField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.CurrencyID, "CNY");
		int b = m_pUserApi->ReqQryBrokerTradingParams(&a, nRequestID++);
		LOG((b == 0) ? "???????????????????......??????\n" : "???????????????????......???????????????=[%d]\n", b);
	}

	//?????????
	void ReqQryForQuote()
	{
		CThostFtdcQryForQuoteField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		strcpy_s(a.ExchangeID, g_chExchangeID);
		strcpy_s(a.InsertTimeStart, "");
		strcpy_s(a.InsertTimeEnd, "");
		strcpy_s(a.InvestUnitID, "");
		int b = m_pUserApi->ReqQryForQuote(&a, nRequestID++);
		LOG((b == 0) ? "?????????......??????\n" : "?????????......???????????????=[%d]\n", b);
	}

	//??????????
	void ReqQryQuote()
	{
		CThostFtdcQryQuoteField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		strcpy_s(a.ExchangeID, g_chExchangeID);
		strcpy_s(a.QuoteSysID, "");
		strcpy_s(a.InsertTimeStart, "");
		strcpy_s(a.InsertTimeEnd, "");
		strcpy_s(a.InvestUnitID, "");
		int b = m_pUserApi->ReqQryQuote(&a, nRequestID++);
		LOG((b == 0) ? "?????????......??????\n" : "?????????......???????????????=[%d]\n", b);
	}

	///??????????
	void ReqForQuoteInsert()
	{
		CThostFtdcInputForQuoteField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		//strcpy_s(a.ForQuoteRef, "");
		strcpy_s(a.UserID, g_chUserID);
		strcpy_s(a.ExchangeID, g_chExchangeID);
		//strcpy_s(a.InvestUnitID, "");
		//strcpy_s(a.IPAddress, "");
		//strcpy_s(a.MacAddress, "");
		int b = m_pUserApi->ReqForQuoteInsert(&a, nRequestID++);
		LOG((b == 0) ? "??????????......??????\n" : "??????????......???????????????=[%d]\n", b);
	}

	///????????????????
	void ReqQuoteInsert()
	{
	choose:int choose_Flag;
		LOG("??????????\t1.????\t2.???\n");
		cin >> choose_Flag;

		if (choose_Flag != 1 && choose_Flag!=2)
		{
			LOG("?????????????\n");
			_getch();
			choose_Flag = NULL;
			goto choose;
		}

		int price_bid;
		LOG("????????????\n");
		cin >> price_bid;

		int price_ask;
		LOG("??????????????\n");
		cin >> price_ask;
		LOG("?????????????1??\n");
		string quoteref;
		LOG("??????quoteref????\n");
		cin >> quoteref;
		string AskOrderRef;
		string BidOrderRef;
		LOG("??????AskOrderRef???:\n");
		cin >> AskOrderRef;
		LOG("??????BidOrderRef???:\n");
		cin >> BidOrderRef;
		_getch();
		CThostFtdcInputQuoteField t = { 0 };
		strcpy_s(t.BrokerID, g_chBrokerID);
		strcpy_s(t.InvestorID, g_chInvestorID);
		strcpy_s(t.InstrumentID, g_chInstrumentID);
		strcpy_s(t.ExchangeID, g_chExchangeID);
		
		strcpy_s(t.QuoteRef, quoteref.c_str());
		strcpy_s(t.UserID, g_chUserID);
		t.AskPrice = price_ask;
		t.BidPrice = price_bid;
		t.AskVolume = 1;
		t.BidVolume = 1;
		if (choose_Flag ==1)
		{
			t.AskOffsetFlag = THOST_FTDC_OF_Open;///????????
			t.BidOffsetFlag = THOST_FTDC_OF_Open;///??????
		}
		else if (choose_Flag ==2)
		{
			t.AskOffsetFlag = THOST_FTDC_OF_Close;///????????
			t.BidOffsetFlag = THOST_FTDC_OF_Close;///??????
		}
		t.AskHedgeFlag = THOST_FTDC_HF_Speculation;///???????????
		t.BidHedgeFlag = THOST_FTDC_HF_Speculation;///???????????

		strcpy_s(t.AskOrderRef, AskOrderRef.c_str());///??????????????
		strcpy_s(t.BidOrderRef, BidOrderRef.c_str());///???????????
		//strcpy_s(t.ForQuoteSysID, "");///?????
		//strcpy_s(t.InvestUnitID, "1");///?????????
		int a = m_pUserApi->ReqQuoteInsert(&t, 1);
		LOG((a == 0) ? "????????????????......??????\n" : "????????????????......???????????????=[%d]\n", a);
	}

	///??????
	virtual void OnRtnQuote(CThostFtdcQuoteField *pQuote) 
	{
		if (pQuote && strcmp(pQuote->InvestorID, g_chInvestorID) != 0)
		{
			return;
		}
		else
		{
			CTraderSpi::OnRtnQuote(pQuote);
			//SetEvent(g_hEvent);
		}
	}

	//???????
	void ReqQuoteAction()
	{
		CThostFtdcInputQuoteActionField t = { 0 };
		strcpy_s(t.BrokerID, g_chBrokerID);
		strcpy_s(t.InvestorID, "00001");
		//strcpy_s(t.UserID, g_chUserID);
		strcpy_s(t.ExchangeID, "SHFE");
		strcpy_s(t.QuoteRef, "           8");
		t.FrontID = 7;
		t.SessionID = 1879781311;
		t.ActionFlag = THOST_FTDC_AF_Delete;
		strcpy_s(t.InstrumentID, "cu1905C55000");
		int a = m_pUserApi->ReqQuoteAction(&t, 1);
		printf("m_pUserApi->ReqQuoteAction = [%d]", a);
	}

	//???????????????
	void ReqQryMaxOrderVolume()
	{
		CThostFtdcQryMaxOrderVolumeField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		a.Direction = THOST_FTDC_D_Buy;
		a.OffsetFlag = THOST_FTDC_OF_Open;
		a.HedgeFlag = THOST_FTDC_HF_Speculation;
		a.MaxVolume = 1;
		strcpy_s(a.BrokerID, g_chBrokerID);
		int b = m_pUserApi->ReqQryMaxOrderVolume(&a, nRequestID++);
		LOG((b == 0) ? "???????????????......??????\n" : "???????????????......???????????????=[%d]\n", b);
	}

	//????????????????????
	void ReqQueryCFMMCTradingAccountToken()
	{
		CThostFtdcQueryCFMMCTradingAccountTokenField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		//strcpy_s(a.InvestUnitID, "");
		int b = m_pUserApi->ReqQueryCFMMCTradingAccountToken(&a, nRequestID++);
		LOG((b == 0) ? "????????????????????......??????\n" : "????????????????????......???????????????=[%d]\n", b);
	}

	///??????????????????
	virtual void OnRtnCFMMCTradingAccountToken(CThostFtdcCFMMCTradingAccountTokenField* pCFMMCTradingAccountToken)
	{
		string web_address;
		web_address = "https://investorservice.cfmmc.com/loginByKey.do?companyID=";
		web_address.append(pCFMMCTradingAccountToken->ParticipantID);
		web_address.append("&userid=");
		web_address.append(pCFMMCTradingAccountToken->AccountID);
		web_address.append("&keyid=");
		int Key = pCFMMCTradingAccountToken->KeyID;
		std::stringstream kk;
		std::string k;
		kk << Key;
		kk >> k;
		web_address.append(k);
		web_address.append("&passwd=");
		web_address.append(pCFMMCTradingAccountToken->Token);
		LOG("web????:%s\n",web_address.c_str());
	};

	//???????????????
	void ReqQryOptionInstrTradeCost()
	{
		CThostFtdcQryOptionInstrTradeCostField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		a.HedgeFlag = THOST_FTDC_HF_Speculation;
		a.InputPrice = 1000;
		a.UnderlyingPrice = 0;
		strcpy_s(a.ExchangeID, g_chExchangeID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		int b = m_pUserApi->ReqQryOptionInstrTradeCost(&a, nRequestID++);
		LogRequestReturnStatus("TraderApi::ReqQryOptionInstrTradeCost", b);
	}

	void ReqQryCombLeg()
	{
		LOG("ReqQryCombLeg is not supported in current API header version.\n");
	}

	///??????????????
	virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
	{
		if (pOrderAction && strcmp(pOrderAction->InvestorID, g_chInvestorID) != 0)
		{
			return;
		}
		else
		{
			CTraderSpi::OnErrRtnOrderAction(pOrderAction,pRspInfo);
			LogTraderErrorDetail("OnErrRtnOrderAction", pRspInfo);
			SetEvent(g_hEvent);
		}
	}

	///??????????????
	virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo,
		int nRequestID, bool bIsLast)
	{
		if (pInputOrder && strcmp(pInputOrder->InvestorID, g_chInvestorID) != 0)
		{
			return;
		}
		else
		{
			CTraderSpi::OnRspOrderInsert(pInputOrder,pRspInfo,nRequestID,bIsLast);
			LogTraderErrorDetail("OnRspOrderInsert", pRspInfo);
		}
	}

	///????????????
	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
	{
		if (pInputOrder && strcmp(pInputOrder->InvestorID, g_chInvestorID) != 0)
		{
			return;
		}
		else
		{
			CTraderSpi::OnErrRtnOrderInsert(pInputOrder, pRspInfo);
			LogTraderErrorDetail("OnErrRtnOrderInsert", pRspInfo);
			SetEvent(g_hEvent);
		}
	}

	///??????
	virtual void OnRtnOrder(CThostFtdcOrderField *pOrder)
	{
		if (pOrder && strcmp(pOrder->InvestorID, g_chInvestorID) != 0)
		{
			return;
		}
		else
		{
			CTraderSpi::OnRtnOrder(pOrder);
			strcpy_s(g_chOrderSysID, pOrder->OrderSysID);
			g_chFrontID = pOrder->FrontID;
			g_chSessionID = pOrder->SessionID;
			strcpy_s(g_chOrderRef, pOrder->OrderRef);
			strcpy_s(g_chExchangeID, pOrder->ExchangeID);
			if (pOrder->OrderStatus == THOST_FTDC_OST_AllTraded)///??????
			{
				LOG("??????????\n\n");
				//SetEvent(g_hEvent);
			}if (pOrder->OrderStatus == THOST_FTDC_OST_PartTradedQueueing)///???????????????
			{
				LOG("???????????????\n\n");
			}if (pOrder->OrderStatus == THOST_FTDC_OST_PartTradedNotQueueing)///???????????????
			{
				LOG("???????????????\n\n");
			}if (pOrder->OrderStatus == THOST_FTDC_OST_NoTradeQueueing)///未????????????
			{
				chioce_action = 0;
				LOG("未????????????\n\n");

				
			}if (pOrder->OrderStatus == THOST_FTDC_OST_NoTradeNotQueueing)///未????????????
			{
				LOG("未????????????\n\n");
			}if (pOrder->OrderStatus == THOST_FTDC_OST_Canceled)///????
			{
				LOG("????\n\n");
				//SetEvent(g_hEvent);
				//CSimpleHandler::ReqQryTradingAccount();
			}if (pOrder->OrderStatus == THOST_FTDC_OST_Unknown)///未?
			{
				LOG("未?\n\n");
				//CSimpleHandler::ReqQryTradingAccount();
			}if (pOrder->OrderStatus == THOST_FTDC_OST_NotTouched)///??未????
			{
				chioce_action = 1;
				LOG("??未????\n\n");
			}if (pOrder->OrderStatus == THOST_FTDC_OST_Touched)///?????
			{
				LOG("?????\n\n");
			}
		}
	}

	virtual void OnRtnTrade(CThostFtdcTradeField* pTrade)
	{
		if (pTrade && strcmp(pTrade->InvestorID, g_chInvestorID) != 0)
		{
			return;
		}
		CTraderSpi::OnRtnTrade(pTrade);
		TrackTradeCount();
	}

	///?????????
	virtual void OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField *pRemoveParkedOrder, CThostFtdcRspInfoField *pRspInfo,
		int nRequestID, bool bIsLast)
	{
		if (pRemoveParkedOrder && strcmp(pRemoveParkedOrder->InvestorID, g_chInvestorID) != 0)
		{
			return;
		}
		else
		{
			strcpy_s(g_chParkedOrderID1, pRemoveParkedOrder->ParkedOrderID);
			CTraderSpi::OnRspRemoveParkedOrder(pRemoveParkedOrder, pRspInfo, nRequestID, bIsLast);
			LogTraderErrorDetail("OnRspRemoveParkedOrder", pRspInfo);
			SetEvent(g_hEvent);
		}
	}

	///??????????
	virtual void OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField *pRemoveParkedOrderAction, CThostFtdcRspInfoField *pRspInfo,
		int nRequestID, bool bIsLast)
	{
		if (pRemoveParkedOrderAction && strcmp(pRemoveParkedOrderAction->InvestorID, g_chInvestorID) != 0)
		{
			return;
		}
		else
		{
			strcpy_s(g_chParkedOrderActionID1, pRemoveParkedOrderAction->ParkedOrderActionID);
			CTraderSpi::OnRspRemoveParkedOrderAction(pRemoveParkedOrderAction, pRspInfo, nRequestID, bIsLast);
			LogTraderErrorDetail("OnRspRemoveParkedOrderAction", pRspInfo);
			SetEvent(g_hEvent);
		}
	}

	///?????????????
	virtual void OnRspParkedOrderInsert(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo,
		int nRequestID, bool bIsLast)
	{
		if (pParkedOrder && strcmp(pParkedOrder->InvestorID, g_chInvestorID) != 0)
		{
			return;
		}
		else
		{
			strcpy_s(g_chParkedOrderID1, pParkedOrder->ParkedOrderID);
			CTraderSpi::OnRspParkedOrderInsert(pParkedOrder, pRspInfo, nRequestID, bIsLast);
			LogTraderErrorDetail("OnRspParkedOrderInsert", pRspInfo);
			SetEvent(g_hEvent);
		}
	}

	///??????????????
	virtual void OnRspParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo,
		int nRequestID, bool bIsLast)
	{
		if (pParkedOrderAction && strcmp(pParkedOrderAction->InvestorID, g_chInvestorID) != 0)
		{
			return;
		}
		else
		{
			strcpy_s(g_chParkedOrderActionID1, pParkedOrderAction->ParkedOrderActionID);
			CTraderSpi::OnRspParkedOrderAction(pParkedOrderAction,pRspInfo,nRequestID,bIsLast);
			LogTraderErrorDetail("OnRspParkedOrderAction", pRspInfo);
			SetEvent(g_hEvent);
		}
	}

	///?????????????
	virtual void OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField *pParkedOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		CTraderSpi::OnRspQryParkedOrderAction(pParkedOrderAction, pRspInfo, nRequestID, bIsLast);
		LogTraderErrorDetail("OnRspQryParkedOrderAction", pRspInfo);
	}

	///????????????
	virtual void OnRspQryParkedOrder(CThostFtdcParkedOrderField *pParkedOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		CTraderSpi::OnRspQryParkedOrder(pParkedOrder,pRspInfo,nRequestID,bIsLast);
		LogTraderErrorDetail("OnRspQryParkedOrder", pRspInfo);
	}

	///?????????????
	virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pOrder) {
			vector_OrderSysID.push_back(pOrder->OrderSysID);
			vector_ExchangeID.push_back(pOrder->ExchangeID);
			vector_InstrumentID.push_back(pOrder->InstrumentID);
			vector_OrderRef.push_back(pOrder->OrderRef);
			vector_FrontID.push_back(pOrder->FrontID);
			vector_SessionID.push_back(pOrder->SessionID);
			vector_OrderStatus.push_back(pOrder->OrderStatus);
		}
		CTraderSpi::OnRspQryOrder(pOrder,pRspInfo,nRequestID,bIsLast);
		LogTraderErrorDetail("OnRspQryOrder", pRspInfo);
		action_number++;
		LOG("\n???????\"%d\"\n\n", action_number);
		if (bIsLast)
		{
			SetEvent(g_hEvent);
		}
	}

	///?????????
	virtual void OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder) 
	{
		if (pExecOrder) {
			strcpy_s(g_NewExecOrderRef, pExecOrder->ExecOrderRef);
			strcpy_s(g_NewExecOrderSysID, pExecOrder->ExecOrderSysID);
			g_NewFrontID = pExecOrder->FrontID;
			g_NewSessionID = pExecOrder->SessionID;
		}
		CTraderSpi::OnRtnExecOrder(pExecOrder);
	}

	//????????????????????
	void ReqQueryBankAccountMoneyByFuture()
	{
		CThostFtdcReqQueryAccountField a = { 0 };
		int b = m_pUserApi->ReqQueryBankAccountMoneyByFuture(&a, nRequestID++);
		LOG((b == 0) ? "????????????????????......??????\n" : "????????????????????......???????????????=[%d]\n", b);
	}

	//??????????????????????
	void ReqFromBankToFutureByFuture()
	{
		int output_num;
		LOG("???????????:");
		cin >> output_num;

		CThostFtdcReqTransferField a = { 0 };
		//strcpy_s(a.TradeCode, "202001");///???????
	int bankid = 0;
		while (bankid != 1 & 2 & 3 & 5 & 6 & 7 & 8 & 9 & 10 & 11 & 12 & 13 & 14 & 15 & 16) {
			LOG("?????????????????????\n");
			LOG("1.????????\n");
			LOG("2.??????\n");
			LOG("3.?泄?????\n");
			LOG("5.???????\n");
			LOG("6.????????\n");
			LOG("7.???????\n");
			LOG("8.???????\n");
			LOG("9.????????\n");
			LOG("10.???????\n");
			LOG("11.????????\n");
			LOG("12.???????\n");
			LOG("13.???????\n");
			LOG("14.???????\n");
			LOG("15.???????\n");
			LOG("16.??????\n");
			cin >> bankid;
			if (bankid == 1 | 2 | 3 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16)
			{
				//strcpy_s(a.BankID, itoa(bankid, a.BankID, 10));///???写???
				itoa(bankid, a.BankID, 10);
			}
			else
			{
				LOG("?????????????写???\n");
				_getch();
			}
		}
		
		
		strcpy_s(a.BankBranchID, "0000");///???????
		strcpy_s(a.BrokerID, g_chBrokerID);
		//strcpy_s(a.TradeDate, "20170829");///????????
		//strcpy_s(a.TradeTime, "09:00:00");
		//strcpy_s(a.BankSerial, "6889");///?????????
		//strcpy_s(a.TradingDay, "20170829");///?????????? 
		//a.PlateSerial = 5;///??????????????
		a.LastFragment = THOST_FTDC_LF_Yes;///???????? '0'=???????
		a.SessionID = SessionID;
		//strcpy_s(a.CustomerName, "");///???????
		a.IdCardType = THOST_FTDC_ICT_IDCard;///???????
		a.CustType = THOST_FTDC_CUSTT_Person;///???????
		//strcpy_s(a.IdentifiedCardNo, "310115198706241914");///???????
		/*strcpy_s(a.BankAccount, "123456789");
		strcpy_s(a.BankPassWord, "123456");///????????*/
		//strcpy_s(a.BankAccount, "621485212110187");
		//strcpy_s(a.BankPassWord, "092812");///????????--????????锌?????
		strcpy_s(a.AccountID, g_chInvestorID);///????????
		//strcpy_s(a.Password, "092812");///???????--???????
		strcpy_s(a.Password, "yangweitao_654321");///???????--???????
		//a.InstallID = 1;///??????
		//a.FutureSerial = 0;///???????????
		a.VerifyCertNoFlag = THOST_FTDC_YNI_No;///???????????????
		strcpy_s(a.CurrencyID, "CNY");///???????
		a.TradeAmount = output_num;///?????
		//a.FutureFetchAmount = 0;///?????????
		//a.CustFee = 0;///?????????
		//a.BrokerFee = 0;///?????????????
		strcpy_s(a.Message, "wweqe");
		a.SecuPwdFlag = THOST_FTDC_BPWDF_BlankCheck;///??????????????
		//a.RequestID = 0;///??????
		//a.TID = 0;///????ID
		int b = m_pUserApi->ReqFromBankToFutureByFuture(&a, 1);
		LOG((b == 0) ? "??????????????????????......??????\n" : "??????????????????????......???????????????=[%d]\n", b);
	}

	//??????????????????????
	void ReqFromFutureToBankByFuture()
	{
		int output_num;
		LOG("???????????:");
		cin >> output_num;

		CThostFtdcReqTransferField a = { 0 };
		strcpy_s(a.TradeCode, "202002");///???????
		bankid_new:int bankid = 0;
		LOG("?????????????????????\n");
		LOG("1.????????\n");
		LOG("2.??????\n");
		LOG("3.?泄?????\n");
		LOG("5.???????\n");
		LOG("6.????????\n");
		LOG("7.???????\n");
		LOG("8.???????\n");
		LOG("9.????????\n");
		LOG("10.???????\n");
		LOG("11.????????\n");
		LOG("12.???????\n");
		LOG("13.???????\n");
		LOG("14.???????\n");
		LOG("15.???????\n");
		LOG("16.??????\n");
		cin >> bankid;
		if (bankid == 1 | 2 | 3 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16)
		{
			//strcpy_s(a.BankID, itoa(bankid, a.BankID, 10));///???写???
			itoa(bankid, a.BankID, 10);
		}
		else {
			LOG("?????????????\n");
			_getch();
			goto bankid_new;
		}
		strcpy_s(a.BankBranchID, "0000");///???????
		strcpy_s(a.BrokerID, g_chBrokerID);
		//strcpy_s(a.BankBranchID, "0000");///???蟹??????????
		//strcpy_s(a.TradeDate, "20170829");///????????
		//strcpy_s(a.TradeTime, "09:00:00");
		//strcpy_s(a.BankSerial, "");///?????????
		//strcpy_s(a.TradingDay, "20170829");///?????????? 
		//a.PlateSerial= 0;///??????????????
		a.LastFragment = THOST_FTDC_LF_Yes;///???????? '0'=???????
		a.SessionID = SessionID;
		//strcpy_s(a.CustomerName, "");///???????
		a.IdCardType = THOST_FTDC_ICT_IDCard;///???????
		strcpy_s(a.IdentifiedCardNo, "310115198706241914");///???????
		strcpy_s(a.BankAccount, "123456789");///???????
		//strcpy_s(a.BankPassWord, "123456");///????????
		strcpy_s(a.AccountID, g_chInvestorID);///????????
		strcpy_s(a.Password, "123456");///???????
		a.InstallID = 1;///??????
		a.CustType = THOST_FTDC_CUSTT_Person;
		//a.FutureSerial = 0;///???????????
		a.VerifyCertNoFlag = THOST_FTDC_YNI_No;///???????????????
		strcpy_s(a.CurrencyID, "CNY");///???????
		a.TradeAmount = output_num;///?????
		a.FutureFetchAmount = 0;///?????????
		a.CustFee = 0;///?????????
		a.BrokerFee = 0;///?????????????
		//a.SecuPwdFlag = THOST_FTDC_BPWDF_BlankCheck;///??????????????
		a.RequestID = 0;///??????
		a.TID = 0;///????ID
		strcpy_s(a.Digest, "test_yang");///??
		int b = m_pUserApi->ReqFromFutureToBankByFuture(&a, 1);
		LOG((b == 0) ? "??????????????????????......??????\n" : "??????????????????????......???????????????=[%d]\n", b);
	}

	//??????????????
	void ReqOptionSelfCloseInsert()
	{
		CThostFtdcInputOptionSelfCloseField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		strcpy_s(a.OptionSelfCloseRef, "1");
		strcpy_s(a.UserID, g_chUserID);
		a.Volume = 10;
		
		int choose_1 = 0;
		while (choose_1 != 1 && choose_1 != 2 && choose_1 != 3 && choose_1 != 4) {
			LOG("??????????????\n1.???\t2.????\t3.???\t4.??????\n");
			cin >> choose_1;
			if (choose_1 == 1) { a.HedgeFlag = THOST_FTDC_HF_Speculation; }
			else if (choose_1 == 2) { a.HedgeFlag = THOST_FTDC_HF_Arbitrage; }
			else if (choose_1 == 3) { a.HedgeFlag = THOST_FTDC_HF_Hedge; }
			else if (choose_1 == 4) { a.HedgeFlag = THOST_FTDC_HF_MarketMaker; }
			else {
				LOG("???????????????\n");
				_getch();
			}
		}
		
		int choose_2 = 0;
		while (choose_2 != 1 && choose_2 != 2 && choose_2 != 3) {
			LOG("?????????????????????????\n1.?????????位\t2.?????????位\t3.???????????????????位\n");
			cin >> choose_2;
			if (choose_2 == 1) { a.OptSelfCloseFlag = THOST_FTDC_OSCF_CloseSelfOptionPosition; }
			else if (choose_2 == 2) { a.OptSelfCloseFlag = THOST_FTDC_OSCF_ReserveOptionPosition; }
			else if (choose_2 == 3) { a.OptSelfCloseFlag = THOST_FTDC_OSCF_SellCloseSelfFuturePosition; }
			else {
				LOG("???????????????\n");
				_getch();
				continue;
			}
		}

		strcpy_s(a.ExchangeID, g_chExchangeID);
		string accountid_new;
		LOG("????????????:\n");
		cin >> accountid_new;
		strcpy_s(a.AccountID, accountid_new.c_str());
		strcpy_s(a.CurrencyID, "CNY");
		int b = m_pUserApi->ReqOptionSelfCloseInsert(&a, 1);
		LOG((b == 0) ? "??????????????......??????\n" : "??????????????......???????????????=[%d]\n", b);
	}

	///?????????
	virtual void OnRtnOptionSelfClose(CThostFtdcOptionSelfCloseField *pOptionSelfClose)
	{
		if (pOptionSelfClose) {
			g_chFrontID = pOptionSelfClose->FrontID;
			g_chSessionID = pOptionSelfClose->SessionID;
			strcpy_s(g_chOptionSelfCloseSysID, pOptionSelfClose->OptionSelfCloseSysID);//?????????
			strcpy_s(g_chOptionSelfCloseRef, pOptionSelfClose->OptionSelfCloseRef);//???????????
		}
		CTraderSpi::OnRtnOptionSelfClose(pOptionSelfClose);
	}

	//??????????????
	void ReqOptionSelfCloseAction()
	{
		CThostFtdcInputOptionSelfCloseActionField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		//strcpy_s(a.OptionSelfCloseSysID, g_chOptionSelfCloseSysID);//?????????
		strcpy_s(a.OptionSelfCloseRef, g_chOptionSelfCloseRef);//???????????
		//a.FrontID = g_chFrontID;
		//a.SessionID = g_chSessionID;
		strcpy_s(a.ExchangeID, g_chExchangeID);
		a.ActionFlag = THOST_FTDC_AF_Delete;
		strcpy_s(a.UserID, g_chUserID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		int b = m_pUserApi->ReqOptionSelfCloseAction(&a, 1);
		LOG((b == 0) ? "??????????????......??????\n" : "??????????????......???????????????=[%d]\n", b);
	}

	//?????????????
	void ReqQryOptionSelfClose()
	{
		CThostFtdcQryOptionSelfCloseField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		strcpy_s(a.ExchangeID, g_chExchangeID);
		int b = m_pUserApi->ReqQryOptionSelfClose(&a, 1);
		LOG((b == 0) ? "?????????????......??????\n" : "?????????????......???????????????=[%d]\n", b);
	}

	///????????????????
	virtual void OnRspQryOptionSelfClose(CThostFtdcOptionSelfCloseField *pOptionSelfClose, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pOptionSelfClose) {
			g_chFrontID = pOptionSelfClose->FrontID;
			g_chSessionID = pOptionSelfClose->SessionID;
			strcpy_s(g_chOptionSelfCloseSysID, pOptionSelfClose->OptionSelfCloseSysID);//?????????
			strcpy_s(g_chOptionSelfCloseRef, pOptionSelfClose->OptionSelfCloseRef);//???????????
		}
		CTraderSpi::OnRspQryOptionSelfClose(pOptionSelfClose, pRspInfo, nRequestID, bIsLast);
		LogTraderErrorDetail("OnRspQryOptionSelfClose", pRspInfo);
	}

	///?????????????
	void ReqQryExecOrder()
	{
		CThostFtdcQryExecOrderField a = { 0 }; 
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID, g_chInstrumentID);
		strcpy_s(a.ExchangeID, g_chExchangeID);
		strcpy_s(a.ExecOrderSysID, "");
		strcpy_s(a.InsertTimeStart, "");
		strcpy_s(a.InsertTimeEnd, "");
		int b = m_pUserApi->ReqQryExecOrder(&a, 1);
		LOG((b == 0) ? "?????????......??????\n" : "?????????......???????????????=[%d]\n", b);
	}

	///?????????????
	void ReqQrySecAgentTradingAccount()
	{
		CThostFtdcQryTradingAccountField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.CurrencyID, "CNY");
		a.BizType = THOST_FTDC_BZTP_Future;
		strcpy_s(a.AccountID, g_chInvestorID);
		int b = m_pUserApi->ReqQrySecAgentTradingAccount(&a, 1);
		LOG((b == 0) ? "?????????????......??????\n" : "?????????????......???????????????=[%d]\n", b);
	}

	///???????????????????校????
	void ReqQrySecAgentCheckMode()
	{
		CThostFtdcQrySecAgentCheckModeField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		int b = m_pUserApi->ReqQrySecAgentCheckMode(&a, 1);
		LOG((b == 0) ? "???????????????????校????......??????\n" : "???????????????????校????......???????????????=[%d]\n", b);
	}

	///???????????????????屑??????????????
	///??????????????????????????????
	void RegisterUserSystemInfo()
	{
		char pSystemInfo[344];
		int len;
		CTP_GetSystemInfo(pSystemInfo, len);

		CThostFtdcUserSystemInfoField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.UserID, g_chUserID);
		memcpy(a.ClientSystemInfo, pSystemInfo, len);
		a.ClientSystemInfoLen = len;

		/*string ip;
		ip.clear();
		cin.ignore();
		LOG("??????ip???(???????????)\n");
		getline(cin, ip);
		strcpy_s(a.ClientPublicIP, ip.c_str());*/
		strcpy_s(a.ClientPublicIP, "192.168.0.1");//ip???

		//int Port;
		//Port = 0;
		//cin.ignore();
		//LOG("?????????\n");
		//cin >> Port;
		//a.ClientIPPort = Port;//????
		a.ClientIPPort = 51305;//????

		/*string LoginTime;
		LoginTime.clear();
		cin.ignore();
		LOG("???????????(???????????)\n");
		getline(cin, LoginTime);
		strcpy_s(a.ClientPublicIP, LoginTime.c_str());*/
		//strcpy_s(a.ClientLoginTime, "20190121");
		strcpy_s(a.ClientAppID, g_chAppID);
		int b = m_pUserApi->RegisterUserSystemInfo(&a);
		LOG((b == 0) ? "????????????......??????\n" : "????????????......???????????????=[%d]\n", b);
	}

	///???????????????????屑????????????????
	///??????????????蔚???????????????
	void SubmitUserSystemInfo()
	{
		char pSystemInfo[344];
		int len;
		CTP_GetSystemInfo(pSystemInfo, len);

		CThostFtdcUserSystemInfoField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.UserID, g_chUserID);
		memcpy(a.ClientSystemInfo, pSystemInfo, len);
		a.ClientSystemInfoLen = len;

		/*string ip;
		ip.clear();
		cin.ignore();
		LOG("??????ip???(???????????)\n");
		getline(cin, ip);
		strcpy_s(a.ClientPublicIP, ip.c_str());*/
		strcpy_s(a.ClientPublicIP, "192.168.0.1");//ip???

		//int Port;
		//Port = 0;
		//cin.ignore();
		//LOG("?????????\n");
		//cin >> Port;
		//a.ClientIPPort = Port;//????
		a.ClientIPPort = 51305;//????

		/*string LoginTime;
		LoginTime.clear();
		cin.ignore();
		LOG("???????????(???????????)\n");
		getline(cin, LoginTime);
		strcpy_s(a.ClientPublicIP, LoginTime.c_str());*/
		strcpy_s(a.ClientLoginTime, "20190121");
		strcpy_s(a.ClientAppID, g_chAppID);
		int b = m_pUserApi->SubmitUserSystemInfo(&a);
		LOG((b == 0) ? "????????????......??????\n" : "????????????......???????????????=[%d]\n", b);
	}

	///??????????????????
	void ReqUserAuthMethod()
	{
		CThostFtdcReqUserAuthMethodField a = { 0 };
		strcpy_s(a.TradingDay, "20190308");
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.UserID, g_chUserID);
		int b = m_pUserApi->ReqUserAuthMethod(&a, nRequestID++);
		LOG((b == 0) ? "??????????????????......??????\n" : "??????????????????......???????????????=[%d]\n", b);
	}

	//????????????????
	void ReqQryTraderOffer()
	{
		CThostFtdcQryTraderOfferField a = {0};
		strcpy_s(a.ExchangeID,"SHFE");
		//strcpy_s(a.ParticipantID, "SHFE");
		//strcpy_s(a.TraderID, "SHFE");
		//int b = m_pUserApi->ReqQryTraderOffer(&a, nRequestID++);
		//LOG((b == 0) ? "????????????????......??????\n" : "????????????????......???????????????=[%d]\n", b);
	}

	///??????????????????????
	void ReqGenUserCaptcha()
	{
		CThostFtdcReqGenUserCaptchaField a = { 0 };
		strcpy_s(a.TradingDay, "");
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.UserID, g_chUserID);
		int b = m_pUserApi->ReqGenUserCaptcha(&a, nRequestID++);
		LOG((b == 0) ? "??????????????????????......??????\n" : "??????????????????????......???????????????=[%d]\n", b);
	}

	///???????????????????????
	void ReqGenUserText()
	{
		CThostFtdcReqGenUserTextField a = { 0 };
		strcpy_s(a.TradingDay, "");
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.UserID, g_chUserID);
		int b = m_pUserApi->ReqGenUserText(&a, nRequestID++);
		LOG((b == 0) ? "???????????????????????......??????\n" : "???????????????????????......???????????????=[%d]\n", b);
	}

	///?????????????????????????
	void ReqUserLoginWithCaptcha()
	{
		CThostFtdcReqUserLoginWithCaptchaField a = { 0 };
		strcpy_s(a.TradingDay, "");
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.UserID, g_chUserID);
		strcpy_s(a.Password, g_chPassword);
		strcpy_s(a.UserProductInfo, "");
		strcpy_s(a.InterfaceProductInfo, "");
		strcpy_s(a.ProtocolInfo, "");//协?????
		strcpy_s(a.MacAddress, "");//Mac???
		strcpy_s(a.ClientIPAddress, "");//???IP???
		strcpy_s(a.LoginRemark, "");//???????
		strcpy_s(a.Captcha, "");//?????????????????
		a.ClientIPPort = 10203;
		int b = m_pUserApi->ReqUserLoginWithCaptcha(&a, nRequestID++);
		LOG((b == 0) ? "?????????????????????????......??????\n" : "?????????????????????????......???????????????=[%d]\n", b);
	}

	///??????????卸???????????????
	void ReqUserLoginWithText()
	{
		CThostFtdcReqUserLoginWithTextField a = { 0 };
		strcpy_s(a.TradingDay, "");
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.UserID, g_chUserID);
		strcpy_s(a.Password, g_chPassword);
		strcpy_s(a.UserProductInfo, "");
		strcpy_s(a.InterfaceProductInfo, "");
		strcpy_s(a.MacAddress, "");
		strcpy_s(a.ClientIPAddress, "");
		strcpy_s(a.LoginRemark, "");
		strcpy_s(a.Text, "");
		a.ClientIPPort = 10000;
		int b = m_pUserApi->ReqUserLoginWithText(&a, nRequestID++);
		LOG((b == 0) ? "??????????卸???????????????......??????\n" : 
			"??????????卸???????????????......???????????????=[%d]\n", b);
	}

	///??????????卸?????????????
	void ReqUserLoginWithOTP()
	{
		CThostFtdcReqUserLoginWithOTPField a = { 0 };
		strcpy_s(a.TradingDay, "");
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.UserID, g_chUserID);
		strcpy_s(a.Password, g_chPassword);
		strcpy_s(a.UserProductInfo, "");
		strcpy_s(a.InterfaceProductInfo, "");
		strcpy_s(a.MacAddress, "");
		strcpy_s(a.ClientIPAddress, "");
		strcpy_s(a.LoginRemark, "");
		strcpy_s(a.OTPPassword, "");
		a.ClientIPPort = 10000;
		int b = m_pUserApi->ReqUserLoginWithOTP(&a, nRequestID++);
		LOG((b == 0) ? "??????????卸?????????????......??????\n" : "??????????卸?????????????......???????????????=[%d]\n", b);
	}

	///???????????????????
	void ReqQrySecAgentTradeInfo()
	{
		CThostFtdcQrySecAgentTradeInfoField a = { 0 };
		strcpy_s(a.BrokerID, "");
		strcpy_s(a.BrokerSecAgentID, "");
		int b = m_pUserApi->ReqQrySecAgentTradeInfo(&a, nRequestID++);
		LOG((b == 0) ? "???????????????????......??????\n" : "???????????????????......???????????????=[%d]\n", b);
	}

	//????????????
	void ReqQryClassifiedInstrument()
	{
		CThostFtdcQryClassifiedInstrumentField a = { 0 };
		//strcpy_s(a.InstrumentID,"");
		//strcpy_s(a.ExchangeID,"INE");
		//strcpy_s(a.ExchangeInstID,"");
		//strcpy_s(a.ProductID,"nr");
		a.TradingType = THOST_FTDC_TD_TRADE;
		a.ClassType = THOST_FTDC_INS_FUTURE;
		int b = m_pUserApi->ReqQryClassifiedInstrument(&a, nRequestID++);
		LOG((b == 0) ? "????????????......??????\n" : "????????????......???????????????=[%d]\n", b);
	}

	//?????????????
	void ReqQryCombPromotionParam()
	{
		CThostFtdcQryCombPromotionParamField a = { 0 };
		strcpy_s(a.ExchangeID,"DCE");
		strcpy_s(a.InstrumentID,"SPD m_o&m_o");
		int b = m_pUserApi->ReqQryCombPromotionParam(&a, nRequestID++);
		LOG((b == 0) ? "?????????????......??????\n" : "?????????????......???????????????=[%d]\n", b);
	}

	//??????????????/?????????
	void ReqQryInvestorProductGroupMargin()
	{
		CThostFtdcQryInvestorProductGroupMarginField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		a.HedgeFlag = THOST_FTDC_HF_Speculation;
		strcpy_s(a.ExchangeID, g_chExchangeID);
		strcpy_s(a.ProductGroupID, g_chInstrumentID);
		strcpy_s(a.reserve1,g_chInstrumentID);
		int b = m_pUserApi->ReqQryInvestorProductGroupMargin(&a, nRequestID++);
		LOG((b == 0) ? "??????????????/?????????......??????\n" : "??????????????/?????????......???????????????=[%d]\n", b);
	}

	//???????????????????????
	void ReqQryExchangeMarginRateAdjust()
	{
		CThostFtdcQryExchangeMarginRateAdjustField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		//strcpy_s(a.reserve1, "");
		a.HedgeFlag = THOST_FTDC_HF_Speculation;
		//strcpy_s(a.InstrumentID,g_chInstrumentID);
		int b = m_pUserApi->ReqQryExchangeMarginRateAdjust(&a, nRequestID++);
		LOG((b == 0) ? "???????????????????????......??????\n" : "???????????????????????......???????????????=[%d]\n", b);
	}

	///???????????????
	void ReqQryRiskSettleInvstPosition()
	{
		CThostFtdcQryRiskSettleInvstPositionField a = { 0 };
		strcpy_s(a.BrokerID, g_chBrokerID);
		strcpy_s(a.InvestorID, g_chInvestorID);
		strcpy_s(a.InstrumentID,g_chInstrumentID);
		int b = m_pUserApi->ReqQryRiskSettleInvstPosition(&a, nRequestID++);
		LOG((b == 0) ? "???????????????......??????\n" : "???????????????......???????????????=[%d]\n", b);
	}

	///????????????
	void ReqQryRiskSettleProductStatus()
	{
		CThostFtdcQryRiskSettleProductStatusField a = { 0 };
		strcpy_s(a.ProductID, g_chBrokerID);
		int b = m_pUserApi->ReqQryRiskSettleProductStatus(&a, nRequestID++);
		LOG((b == 0) ? "????????????......??????\n" : "????????????......???????????????=[%d]\n", b);
	}

	///???????????
	virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField* pInstrumentStatus)
	{
		LOG("<OnRtnInstrumentStatus>\n");
		if (pInstrumentStatus)
		{
			LOG("\tExchangeID [%s]\n", pInstrumentStatus->ExchangeID);
			LOG("\treserve1 [%s]\n", pInstrumentStatus->reserve1);
			LOG("\tSettlementGroupID [%s]\n", pInstrumentStatus->SettlementGroupID);
			LOG("\treserve2 [%s]\n", pInstrumentStatus->reserve2);
			LOG("\tEnterTime [%s]\n", pInstrumentStatus->EnterTime);
			LOG("\tExchangeInstID [%s]\n", pInstrumentStatus->ExchangeInstID);
			LOG("\tInstrumentID [%s]\n", pInstrumentStatus->InstrumentID);
			LOG("\tTradingSegmentSN [%d]\n", pInstrumentStatus->TradingSegmentSN);
			LOG("\tInstrumentStatus [%c]\n", pInstrumentStatus->InstrumentStatus);
			LOG("\tEnterReason [%c]\n", pInstrumentStatus->EnterReason);
		}
		LOG("</OnRtnInstrumentStatus>\n");
	};

private:
	CThostFtdcTraderApi *m_pUserApi;
};
