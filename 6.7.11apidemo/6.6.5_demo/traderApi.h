#pragma once
#include "ThostFtdcTraderApi.h"
#include <vector>
#include <string>
#include <map>

using namespace std;

class CTraderApi : public CThostFtdcTraderApi
{
public:
	CTraderApi() {};
	~CTraderApi() {};

private:
	CThostFtdcTraderApi *m_pApi;

public:
	///����TraderApi
	///@param pszFlowPath ����������Ϣ�ļ���Ŀ¼��Ĭ��Ϊ��ǰĿ¼
	///@return ��������UserApi
	CThostFtdcTraderApi* CreateFtdcTraderApi(const char* pszFlowPath = "");

	///��ȡAPI�İ汾��Ϣ
	///@retrun ��ȡ���İ汾��
	const char* GetApiVersion();

	///ɾ���ӿڶ�����
	///@remark ����ʹ�ñ��ӿڶ���ʱ,���øú���ɾ���ӿڶ���
	virtual void Release();

	///��ʼ��
	///@remark ��ʼ�����л���,ֻ�е��ú�,�ӿڲſ�ʼ����
	virtual void Init();

	///�ȴ��ӿ��߳̽�������
	///@return �߳��˳�����
	virtual int Join();

	///��ȡ��ǰ������
	///@retrun ��ȡ���Ľ�����
	///@remark ֻ�е�¼�ɹ���,���ܵõ���ȷ�Ľ�����
	virtual const char* GetTradingDay();

	///ע��ǰ�û������ַ
	///@param pszFrontAddress��ǰ�û������ַ��
	///@remark �����ַ�ĸ�ʽΪ����protocol://ipaddress:port�����磺��tcp://127.0.0.1:17001���� 
	///@remark ��tcp����������Э�飬��127.0.0.1��������������ַ����17001�������������˿ںš�
	virtual void RegisterFront(char* pszFrontAddress);

	///ע�����ַ����������ַ
	///@param pszNsAddress�����ַ����������ַ��
	///@remark �����ַ�ĸ�ʽΪ����protocol://ipaddress:port�����磺��tcp://127.0.0.1:12001���� 
	///@remark ��tcp����������Э�飬��127.0.0.1��������������ַ����12001�������������˿ںš�
	///@remark RegisterNameServer������RegisterFront
	virtual void RegisterNameServer(char* pszNsAddress);

	///ע�����ַ������û���Ϣ
	///@param pFensUserInfo���û���Ϣ��
	virtual void RegisterFensUserInfo(CThostFtdcFensUserInfoField* pFensUserInfo);

	///ע��ص��ӿ�
	///@param pSpi �����Իص��ӿ����ʵ��
	virtual void RegisterSpi(CThostFtdcTraderSpi* pSpi);

	///����˽������
	///@param nResumeType ˽�����ش���ʽ  
	///        THOST_TERT_RESTART:�ӱ������տ�ʼ�ش�
	///        THOST_TERT_RESUME:���ϴ��յ�������
	///        THOST_TERT_QUICK:ֻ���͵�¼��˽����������
	///@remark �÷���Ҫ��Init����ǰ���á����������򲻻��յ�˽���������ݡ�
	virtual void SubscribePrivateTopic(THOST_TE_RESUME_TYPE nResumeType);

	///���Ĺ�������
	///@param nResumeType �������ش���ʽ  
	///        THOST_TERT_RESTART:�ӱ������տ�ʼ�ش�
	///        THOST_TERT_RESUME:���ϴ��յ�������
	///        THOST_TERT_QUICK:ֻ���͵�¼�󹫹���������
	///        THOST_TERT_NONE:ȡ�����Ĺ�����
	///@remark �÷���Ҫ��Init����ǰ���á����������򲻻��յ������������ݡ�
	virtual void SubscribePublicTopic(THOST_TE_RESUME_TYPE nResumeType);

	///�ͻ�����֤����
	virtual int ReqAuthenticate(CThostFtdcReqAuthenticateField* pReqAuthenticateField, int nRequestID);

	///ע���û��ն���Ϣ�������м̷�����������ģʽ
	///��Ҫ���ն���֤�ɹ����û���¼ǰ���øýӿ�
	virtual int RegisterUserSystemInfo(CThostFtdcUserSystemInfoField* pUserSystemInfo);

	///�ϱ��û��ն���Ϣ�������м̷���������Ա��¼ģʽ
	///����Ա��¼�󣬿��Զ�ε��øýӿ��ϱ��ͻ���Ϣ
	virtual int SubmitUserSystemInfo(CThostFtdcUserSystemInfoField* pUserSystemInfo);
	///�û���¼����
	virtual int ReqUserLogin(CThostFtdcReqUserLoginField* pReqUserLoginField, int nRequestID);

	///�ǳ�����
	virtual int ReqUserLogout(CThostFtdcUserLogoutField* pUserLogout, int nRequestID);

	///�û������������
	virtual int ReqUserPasswordUpdate(CThostFtdcUserPasswordUpdateField* pUserPasswordUpdate, int nRequestID);

	///�ʽ��˻������������
	virtual int ReqTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField* pTradingAccountPasswordUpdate, int nRequestID);

	///��ѯ�û���ǰ֧�ֵ���֤ģʽ
	virtual int ReqUserAuthMethod(CThostFtdcReqUserAuthMethodField* pReqUserAuthMethod, int nRequestID);

	///�û�������ȡͼ����֤������
	virtual int ReqGenUserCaptcha(CThostFtdcReqGenUserCaptchaField* pReqGenUserCaptcha, int nRequestID);

	///�û�������ȡ������֤������
	virtual int ReqGenUserText(CThostFtdcReqGenUserTextField* pReqGenUserText, int nRequestID);

	///�û���������ͼƬ��֤��ĵ�½����
	virtual int ReqUserLoginWithCaptcha(CThostFtdcReqUserLoginWithCaptchaField* pReqUserLoginWithCaptcha, int nRequestID);

	///�û��������ж�����֤��ĵ�½����
	virtual int ReqUserLoginWithText(CThostFtdcReqUserLoginWithTextField* pReqUserLoginWithText, int nRequestID);

	///�û��������ж�̬����ĵ�½����
	virtual int ReqUserLoginWithOTP(CThostFtdcReqUserLoginWithOTPField* pReqUserLoginWithOTP, int nRequestID);

	///����¼������
	virtual int ReqOrderInsert(CThostFtdcInputOrderField* pInputOrder, int nRequestID);

	///Ԥ��¼������
	virtual int ReqParkedOrderInsert(CThostFtdcParkedOrderField* pParkedOrder, int nRequestID);

	///Ԥ�񳷵�¼������
	virtual int ReqParkedOrderAction(CThostFtdcParkedOrderActionField* pParkedOrderAction, int nRequestID);

	///������������
	virtual int ReqOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, int nRequestID);

	///��ѯ��󱨵���������
	virtual int ReqQryMaxOrderVolume(CThostFtdcQryMaxOrderVolumeField* pQryMaxOrderVolume, int nRequestID);

	///Ͷ���߽�����ȷ��
	virtual int ReqSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, int nRequestID);

	///����ɾ��Ԥ��
	virtual int ReqRemoveParkedOrder(CThostFtdcRemoveParkedOrderField* pRemoveParkedOrder, int nRequestID);

	///����ɾ��Ԥ�񳷵�
	virtual int ReqRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField* pRemoveParkedOrderAction, int nRequestID);

	///ִ������¼������
	virtual int ReqExecOrderInsert(CThostFtdcInputExecOrderField* pInputExecOrder, int nRequestID);

	///ִ�������������
	virtual int ReqExecOrderAction(CThostFtdcInputExecOrderActionField* pInputExecOrderAction, int nRequestID);

	///ѯ��¼������
	virtual int ReqForQuoteInsert(CThostFtdcInputForQuoteField* pInputForQuote, int nRequestID);

	///����¼������
	virtual int ReqQuoteInsert(CThostFtdcInputQuoteField* pInputQuote, int nRequestID);

	///���۲�������
	virtual int ReqQuoteAction(CThostFtdcInputQuoteActionField* pInputQuoteAction, int nRequestID);

	///����������������
	virtual int ReqBatchOrderAction(CThostFtdcInputBatchOrderActionField* pInputBatchOrderAction, int nRequestID);

	///��Ȩ�ԶԳ�¼������
	virtual int ReqOptionSelfCloseInsert(CThostFtdcInputOptionSelfCloseField* pInputOptionSelfClose, int nRequestID);

	///��Ȩ�ԶԳ��������
	virtual int ReqOptionSelfCloseAction(CThostFtdcInputOptionSelfCloseActionField* pInputOptionSelfCloseAction, int nRequestID);

	///�������¼������
	virtual int ReqCombActionInsert(CThostFtdcInputCombActionField* pInputCombAction, int nRequestID);

	///�����ѯ����
	virtual int ReqQryOrder(CThostFtdcQryOrderField* pQryOrder, int nRequestID);

	///�����ѯ�ɽ�
	virtual int ReqQryTrade(CThostFtdcQryTradeField* pQryTrade, int nRequestID);

	///�����ѯͶ���ֲ߳�
	virtual int ReqQryInvestorPosition(CThostFtdcQryInvestorPositionField* pQryInvestorPosition, int nRequestID);

	///�����ѯ�ʽ��˻�
	virtual int ReqQryTradingAccount(CThostFtdcQryTradingAccountField* pQryTradingAccount, int nRequestID);

	///�����ѯͶ����
	virtual int ReqQryInvestor(CThostFtdcQryInvestorField* pQryInvestor, int nRequestID);

	///�����ѯ���ױ���
	virtual int ReqQryTradingCode(CThostFtdcQryTradingCodeField* pQryTradingCode, int nRequestID);

	///�����ѯ��Լ��֤����
	virtual int ReqQryInstrumentMarginRate(CThostFtdcQryInstrumentMarginRateField* pQryInstrumentMarginRate, int nRequestID);

	///�����ѯ��Լ��������
	virtual int ReqQryInstrumentCommissionRate(CThostFtdcQryInstrumentCommissionRateField* pQryInstrumentCommissionRate, int nRequestID);

	///�����ѯ������
	virtual int ReqQryExchange(CThostFtdcQryExchangeField* pQryExchange, int nRequestID);

	///�����ѯ��Ʒ
	virtual int ReqQryProduct(CThostFtdcQryProductField* pQryProduct, int nRequestID);

	///�����ѯ��Լ
	virtual int ReqQryInstrument(CThostFtdcQryInstrumentField* pQryInstrument, int nRequestID);

	///�����ѯ����
	virtual int ReqQryDepthMarketData(CThostFtdcQryDepthMarketDataField* pQryDepthMarketData, int nRequestID);

	///�����ѯ����Ա���̻�
	virtual int ReqQryTraderOffer(CThostFtdcQryTraderOfferField* pQryTraderOffer, int nRequestID);

	///�����ѯͶ���߽�����
	virtual int ReqQrySettlementInfo(CThostFtdcQrySettlementInfoField* pQrySettlementInfo, int nRequestID);

	///�����ѯת������
	virtual int ReqQryTransferBank(CThostFtdcQryTransferBankField* pQryTransferBank, int nRequestID);

	///�����ѯͶ���ֲ߳���ϸ
	virtual int ReqQryInvestorPositionDetail(CThostFtdcQryInvestorPositionDetailField* pQryInvestorPositionDetail, int nRequestID);

	///�����ѯ�ͻ�֪ͨ
	virtual int ReqQryNotice(CThostFtdcQryNoticeField* pQryNotice, int nRequestID);

	///�����ѯ������Ϣȷ��
	virtual int ReqQrySettlementInfoConfirm(CThostFtdcQrySettlementInfoConfirmField* pQrySettlementInfoConfirm, int nRequestID);

	///�����ѯͶ���ֲ߳���ϸ
	virtual int ReqQryInvestorPositionCombineDetail(CThostFtdcQryInvestorPositionCombineDetailField* pQryInvestorPositionCombineDetail, int nRequestID);

	///�����ѯ��֤����ϵͳ���͹�˾�ʽ��˻���Կ
	virtual int ReqQryCFMMCTradingAccountKey(CThostFtdcQryCFMMCTradingAccountKeyField* pQryCFMMCTradingAccountKey, int nRequestID);

	///�����ѯ�ֵ��۵���Ϣ
	virtual int ReqQryEWarrantOffset(CThostFtdcQryEWarrantOffsetField* pQryEWarrantOffset, int nRequestID);

	///�����ѯͶ����Ʒ��/��Ʒ�ֱ�֤��
	virtual int ReqQryInvestorProductGroupMargin(CThostFtdcQryInvestorProductGroupMarginField* pQryInvestorProductGroupMargin, int nRequestID);

	///�����ѯ��������֤����
	virtual int ReqQryExchangeMarginRate(CThostFtdcQryExchangeMarginRateField* pQryExchangeMarginRate, int nRequestID);

	///�����ѯ������������֤����
	virtual int ReqQryExchangeMarginRateAdjust(CThostFtdcQryExchangeMarginRateAdjustField* pQryExchangeMarginRateAdjust, int nRequestID);

	///�����ѯ����
	virtual int ReqQryExchangeRate(CThostFtdcQryExchangeRateField* pQryExchangeRate, int nRequestID);

	///�����ѯ������������Ա����Ȩ��
	virtual int ReqQrySecAgentACIDMap(CThostFtdcQrySecAgentACIDMapField* pQrySecAgentACIDMap, int nRequestID);

	///�����ѯ��Ʒ���ۻ���
	virtual int ReqQryProductExchRate(CThostFtdcQryProductExchRateField* pQryProductExchRate, int nRequestID);

	///�����ѯ��Ʒ��
	virtual int ReqQryProductGroup(CThostFtdcQryProductGroupField* pQryProductGroup, int nRequestID);

	///�����ѯ�����̺�Լ��������
	virtual int ReqQryMMInstrumentCommissionRate(CThostFtdcQryMMInstrumentCommissionRateField* pQryMMInstrumentCommissionRate, int nRequestID);

	///�����ѯ��������Ȩ��Լ������
	virtual int ReqQryMMOptionInstrCommRate(CThostFtdcQryMMOptionInstrCommRateField* pQryMMOptionInstrCommRate, int nRequestID);

	///�����ѯ����������
	virtual int ReqQryInstrumentOrderCommRate(CThostFtdcQryInstrumentOrderCommRateField* pQryInstrumentOrderCommRate, int nRequestID);

	///�����ѯ�ʽ��˻�
	virtual int ReqQrySecAgentTradingAccount(CThostFtdcQryTradingAccountField* pQryTradingAccount, int nRequestID);

	///�����ѯ�����������ʽ�У��ģʽ
	virtual int ReqQrySecAgentCheckMode(CThostFtdcQrySecAgentCheckModeField* pQrySecAgentCheckMode, int nRequestID);

	///�����ѯ������������Ϣ
	virtual int ReqQrySecAgentTradeInfo(CThostFtdcQrySecAgentTradeInfoField* pQrySecAgentTradeInfo, int nRequestID);

	///�����ѯ��Ȩ���׳ɱ�
	virtual int ReqQryOptionInstrTradeCost(CThostFtdcQryOptionInstrTradeCostField* pQryOptionInstrTradeCost, int nRequestID);

	///�����ѯ��Ȩ��Լ������
	virtual int ReqQryOptionInstrCommRate(CThostFtdcQryOptionInstrCommRateField* pQryOptionInstrCommRate, int nRequestID);

	///�����ѯִ������
	virtual int ReqQryExecOrder(CThostFtdcQryExecOrderField* pQryExecOrder, int nRequestID);

	///�����ѯѯ��
	virtual int ReqQryForQuote(CThostFtdcQryForQuoteField* pQryForQuote, int nRequestID);

	///�����ѯ����
	virtual int ReqQryQuote(CThostFtdcQryQuoteField* pQryQuote, int nRequestID);

	///�����ѯ��Ȩ�ԶԳ�
	virtual int ReqQryOptionSelfClose(CThostFtdcQryOptionSelfCloseField* pQryOptionSelfClose, int nRequestID);

	///�����ѯͶ�ʵ�Ԫ
	virtual int ReqQryInvestUnit(CThostFtdcQryInvestUnitField* pQryInvestUnit, int nRequestID);

	///�����ѯ��Ϻ�Լ��ȫϵ��
	virtual int ReqQryCombInstrumentGuard(CThostFtdcQryCombInstrumentGuardField* pQryCombInstrumentGuard, int nRequestID);

	///�����ѯ�������
	virtual int ReqQryCombAction(CThostFtdcQryCombActionField* pQryCombAction, int nRequestID);

	///�����ѯת����ˮ
	virtual int ReqQryTransferSerial(CThostFtdcQryTransferSerialField* pQryTransferSerial, int nRequestID);

	///�����ѯ����ǩԼ��ϵ
	virtual int ReqQryAccountregister(CThostFtdcQryAccountregisterField* pQryAccountregister, int nRequestID);

	///�����ѯǩԼ����
	virtual int ReqQryContractBank(CThostFtdcQryContractBankField* pQryContractBank, int nRequestID);

	///�����ѯԤ��
	virtual int ReqQryParkedOrder(CThostFtdcQryParkedOrderField* pQryParkedOrder, int nRequestID);

	///�����ѯԤ�񳷵�
	virtual int ReqQryParkedOrderAction(CThostFtdcQryParkedOrderActionField* pQryParkedOrderAction, int nRequestID);

	///�����ѯ����֪ͨ
	virtual int ReqQryTradingNotice(CThostFtdcQryTradingNoticeField* pQryTradingNotice, int nRequestID);

	///�����ѯ���͹�˾���ײ���
	virtual int ReqQryBrokerTradingParams(CThostFtdcQryBrokerTradingParamsField* pQryBrokerTradingParams, int nRequestID);

	///�����ѯ���͹�˾�����㷨
	virtual int ReqQryBrokerTradingAlgos(CThostFtdcQryBrokerTradingAlgosField* pQryBrokerTradingAlgos, int nRequestID);

	///�����ѯ��������û�����
	virtual int ReqQueryCFMMCTradingAccountToken(CThostFtdcQueryCFMMCTradingAccountTokenField* pQueryCFMMCTradingAccountToken, int nRequestID);

	///�ڻ����������ʽ�ת�ڻ�����
	virtual int ReqFromBankToFutureByFuture(CThostFtdcReqTransferField* pReqTransfer, int nRequestID);

	///�ڻ������ڻ��ʽ�ת��������
	virtual int ReqFromFutureToBankByFuture(CThostFtdcReqTransferField* pReqTransfer, int nRequestID);

	///�ڻ������ѯ�����������
	virtual int ReqQueryBankAccountMoneyByFuture(CThostFtdcReqQueryAccountField* pReqQueryAccount, int nRequestID);

	///�����ѯ�����Լ
	virtual int ReqQryClassifiedInstrument(CThostFtdcQryClassifiedInstrumentField* pQryClassifiedInstrument, int nRequestID);

	///��������Żݱ���
	virtual int ReqQryCombPromotionParam(CThostFtdcQryCombPromotionParamField* pQryCombPromotionParam, int nRequestID);

	///Ͷ���߷��ս���ֲֲ�ѯ
	virtual int ReqQryRiskSettleInvstPosition(CThostFtdcQryRiskSettleInvstPositionField* pQryRiskSettleInvstPosition, int nRequestID);

	///���ս����Ʒ��ѯ
	virtual int ReqQryRiskSettleProductStatus(CThostFtdcQryRiskSettleProductStatusField* pQryRiskSettleProductStatus, int nRequestID);

	///SPBM�ڻ���Լ������ѯ
	virtual int ReqQrySPBMFutureParameter(CThostFtdcQrySPBMFutureParameterField* pQrySPBMFutureParameter, int nRequestID);

	///SPBM��Ȩ��Լ������ѯ
	virtual int ReqQrySPBMOptionParameter(CThostFtdcQrySPBMOptionParameterField* pQrySPBMOptionParameter, int nRequestID);

	///SPBMƷ���ڶ������ۿ۲�����ѯ
	virtual int ReqQrySPBMIntraParameter(CThostFtdcQrySPBMIntraParameterField* pQrySPBMIntraParameter, int nRequestID);

	///SPBM��Ʒ�ֵֿ۲�����ѯ
	virtual int ReqQrySPBMInterParameter(CThostFtdcQrySPBMInterParameterField* pQrySPBMInterParameter, int nRequestID);

	///SPBM��ϱ�֤���ײͲ�ѯ
	virtual int ReqQrySPBMPortfDefinition(CThostFtdcQrySPBMPortfDefinitionField* pQrySPBMPortfDefinition, int nRequestID);

	///Ͷ����SPBM�ײ�ѡ���ѯ
	virtual int ReqQrySPBMInvestorPortfDef(CThostFtdcQrySPBMInvestorPortfDefField* pQrySPBMInvestorPortfDef, int nRequestID);

	///Ͷ����������ϱ�֤��ϵ����ѯ
	virtual int ReqQryInvestorPortfMarginRatio(CThostFtdcQryInvestorPortfMarginRatioField* pQryInvestorPortfMarginRatio, int nRequestID);

	///Ͷ���߲�ƷSPBM��ϸ��ѯ
	virtual int ReqQryInvestorProdSPBMDetail(CThostFtdcQryInvestorProdSPBMDetailField* pQryInvestorProdSPBMDetail, int nRequestID);

};
