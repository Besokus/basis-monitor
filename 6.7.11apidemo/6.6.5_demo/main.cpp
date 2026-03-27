#include "stdafx.h"
#include "main.h"
#include <thread>

#ifdef _WIN32
#define FLOW_PATH ".\\flow\\"
#else
#define FLOW_PATH "../flow/"
#endif

int main()
{
#ifdef _WIN32
	system("COLOR 0A");
#endif
	logfile = fopen("syslog.txt", "w");

	while (true)
	{
		LOG("��ѡ����������/����:\n");
		LOG("1.���ӽ���\n");
		LOG("2.��������\n");
		int trade_md;
		cin >> trade_md;
		switch (trade_md) {
		case 2:
		{
			string g_chFrontMdaddr = getConfig("config", "FrontMdAddr");
			string instrumentId = getConfig("config", "InstrumentID");
			LOG("g_chFrontMdaddr = %s\n", g_chFrontMdaddr.c_str());
			LOG("InstrumentID = %s\n", instrumentId.c_str());

			/*int g_chbIsUsingUdp = atoi(getConfig("config", "bIsUsingUdp").c_str());
			int g_chbIsMulticast = atoi(getConfig("config", "bIsMulticast").c_str());
			bool UsingUdp = false;
			bool Multicast = false;
			if (g_chbIsUsingUdp == 0) UsingUdp = false;
			if (g_chbIsUsingUdp == 1) UsingUdp = true;
			if (g_chbIsMulticast == 0) Multicast = false;
			if (g_chbIsMulticast == 1) Multicast = true;
			LOG("bIsUsingUdp = [%d],bIsMulticast = [%d]\n", UsingUdp, Multicast);*/

			CThostFtdcMdApi  *pUserMdApi =
				CThostFtdcMdApi::CreateFtdcMdApi(FLOW_PATH, false, false);
			CSimpleMdHandler ash(pUserMdApi);
			pUserMdApi->RegisterSpi(&ash);
			/*ash.RegisterFensUserInfo();
			pUserMdApi->RegisterNameServer(const_cast<char *>(g_chFrontMdaddr.c_str()));*/
			pUserMdApi->RegisterFront(const_cast<char *>(g_chFrontMdaddr.c_str()));
			pUserMdApi->Init();
			WaitForSingleObject(xinhao, INFINITE);
			ash.ReqUserLogin();
			WaitForSingleObject(xinhao, INFINITE);
			md_InstrumentID.clear();
			if (instrumentId.empty())
			{
				LOG("config.ini InstrumentID is empty, cannot subscribe market data.\n");
				_getch();
				pUserMdApi->Release();
				return -1;
			}
			//?????????
			/*md_InstrumentID.clear();
			string New_instrument;
			LOG("?????????????????????':'???:\n");
			cin >> New_instrument;
			md_InstrumentID = split(New_instrument, ":");*/
			md_InstrumentID.push_back(instrumentId);

			ash.SubscribeMarketData();
			//ash.UnSubscribeMarketData();//�˶�����
			//ash.UnSubscribeForQuoteRsp();//�˶�ѯ��
			//ash.ReqQryMulticastInstrument();//�����ѯ�鲥��Լ
			//WaitForSingleObject(xinhao, INFINITE);
			_getch();
			pUserMdApi->Release();
			return 0;
			exit(-1);
		}
		case 1:
		{
			string g_chFrontaddr = getConfig("config", "FrontAddr");
			cout << "g_chFrontaddr = " << g_chFrontaddr << "\n" << endl;
			CTraderApi *pUserApi = new CTraderApi;
			pUserApi->CreateFtdcTraderApi(FLOW_PATH);
			LOG(pUserApi->GetApiVersion());
			cout << endl;
			CSimpleHandler sh(pUserApi);
		cir:pUserApi->RegisterSpi(&sh);
			pUserApi->SubscribePrivateTopic(THOST_TERT_QUICK);
			pUserApi->SubscribePublicTopic(THOST_TERT_QUICK);
			/*sh.RegisterFensUserInfo();
			pUserApi->RegisterNameServer(const_cast<char *>(g_chFrontaddr.c_str()));*/
			pUserApi->RegisterFront(const_cast<char *>(g_chFrontaddr.c_str()));
			pUserApi->Init();
			WaitForSingleObject(g_hEvent, INFINITE);
			

			while (true)
			{
				LOG("��ȷ������ģ??\n");
				LOG("1.ֱ��ģʽ\n");
				LOG("2.??�̷���������Աģ??(һ��???ģ??)\n");
				LOG("3.??�̷������ǲ���Աģ???��??????ģ??\n");
				int mode_num;
				cin >> mode_num;
				switch (mode_num)
				{
				case 1://?????
				{
					sh.ReqAuthenticate();
					WaitForSingleObject(g_hEvent, INFINITE);
					sh.ReqUserLogin();
					WaitForSingleObject(g_hEvent, INFINITE);
					break;
				}
				case 2://???????
				{
					sh.ReqAuthenticate();
					WaitForSingleObject(g_hEvent, INFINITE);
					sh.ReqUserLogin();
					WaitForSingleObject(g_hEvent, INFINITE);
					sh.SubmitUserSystemInfo();
					break;
				}
				case 3://????????
				{
					sh.ReqAuthenticate();
					WaitForSingleObject(g_hEvent, INFINITE);
					sh.RegisterUserSystemInfo();
					sh.ReqUserLogin();
					WaitForSingleObject(g_hEvent, INFINITE);
					break;
				}
				default:
					LOG("选择的模式有�?，�?�重新输入！\n");
					_getch();
					system("cls");
					continue;
				}
				break;
			}


			while (true)
			{
			loop:int input1;
				system("cls");
				
				LOG("201.�ϱ��û���???��Ϣ\n");
				LOG("110,���ײ�???��Լ???����???��\n");
				LOG("101.�û���¼����\n");
				LOG("102.�ͻ�??��֤\n");
				LOG("103.����ǳ�\n");
				LOG("1.���㵥ȷ��???��\n");
				LOG("2.�û������������\n");
				LOG("3.�ʽ��˻������������\n");
				LOG("/////////////����////////////\n");
				LOG("4.����ģ��\n");
				LOG("////////////��???/////////////\n");
				LOG("5.��???ģ��\n");
				LOG("/////////////��Ȩ&����??///////////\n");
				LOG("6.��Ȩ&������\n");
				LOG("///////////����??////////////\n");
				LOG("7.??��ָ??\n");
				LOG("///////////ͭ��Ȩ��??///////////\n");
				LOG("8.ͭ��Ȩ����\n");
				LOG("9.�汾6.6.5�¼ӽӿ�\n");
				LOG("0.��ս���\n");
				LOG("100.�˳�����\n");
				LOG("����������Ҫ�Ĳ������??");
				cin >> input1;
				switch (input1)
				{
				case 201:
				{
					sh.SubmitUserSystemInfo();
					_getch();
					break;
				}
				case 110:
				{
					string g_chFrontMdaddr = getConfig("config", "FrontMdAddr");
					cout << "g_chFrontMdaddr = " << g_chFrontMdaddr << "\n" << endl;
					CThostFtdcMdApi  *pUserMdApi =
						CThostFtdcMdApi::CreateFtdcMdApi(FLOW_PATH);
					CSimpleMdHandler ash(pUserMdApi);
					pUserMdApi->RegisterSpi(&ash);
					pUserMdApi->RegisterFront(const_cast<char *>(g_chFrontMdaddr.c_str()));
					pUserMdApi->Init();
					WaitForSingleObject(xinhao, INFINITE);
					ash.ReqUserLogin();
					WaitForSingleObject(xinhao, INFINITE);
					sh.ReqQryInstrument();//??????
					WaitForSingleObject(xinhao, INFINITE);
					ash.SubscribeMarketData();//????????
					_getch();
					pUserMdApi->Release();
					break;
				}
				case 101:
				{
					sh.ReqUserLogin();
					_getch();
					break;
				}
				case 102:
				{
					sh.ReqAuthenticate();
					_getch();
					break;
				}
				case 103:
				{
					sh.ReqUserLogout();
					_getch();
					break;
					//goto cir;
				}
				case 1://???????????
				{
					sh.ReqSettlementInfoConfirm();
					WaitForSingleObject(g_hEvent, INFINITE);
					_getch();
					system("cls");
					break;
				}
				case 2://??????????????
				{
					sh.ReqUserPasswordUpdate();
					WaitForSingleObject(g_hEvent, INFINITE);
					_getch();
					system("cls");
					break;
				}
				case 3://?????????????????
				{
					sh.ReqTradingAccountPasswordUpdate();
					WaitForSingleObject(g_hEvent, INFINITE);
					_getch();
					system("cls");
					break;
				}
				case 4://???????????
				{
				orderinsert:system("cls");
					int orderinsert_num;
					LOG("4.报入一笔立即单\n");
					LOG("5.撤销上一笔报单\n");
					LOG("6.报入预埋�?限价单立即单\n");
					LOG("7.撤销预埋�?(上一�?预埋�?)\n");
					LOG("8.报入预埋撤单\n");
					LOG("9.撤销预埋撤单-(上一�?预埋撤�??\n");
					LOG("10.报入条件单\n");
					LOG("11.撤销条件�?(上一�?条件�?)\n");
					LOG("24.大商所限价止损单\n");
					LOG("25.大商所市价止损单\n");
					LOG("26.大商所止盈单\n");
					LOG("27.FOK全成全撤\n");
					LOG("28.FAK部成部撤\n");
					LOG("29.市价单\n");
					LOG("30.套利指令\n");
					LOG("31.互换单\n");
					LOG("32.申�?�组合\n");
					LOG("33.对冲设置请求\n");
					LOG("34.取消对冲设置请求\n");
					LOG("35.查�?��?�冲设置请求\n");
					LOG("0.返回上一层\n");
					cin >> orderinsert_num;
					switch (orderinsert_num)
					{
					case 0:
						goto loop;
					case 4://?????????????
					{
						sh.ReqOrderInsert_Ordinary();
						_getch();
						break;
					}
					case 5://????????????
					{
						sh.ReqOrderAction_Ordinary();
						_getch();
						break;
					}
					case 6://???????
					{
						sh.ReqParkedOrderInsert();
						_getch();
						break;
					}
					case 7://??????
					{
						sh.ReqRemoveParkedOrder();
						_getch();
						break;
					}
					case 8://????????
					{
						sh.ReqParkedOrderAction();
						_getch();
						break;
					}
					case 9://?????????
					{
						sh.ReqRemoveParkedOrderAction();
						_getch();
						break;
					}
					case 10://??????????
					{
					it:LOG("1.最新价大于条件价\n");
						LOG("2.最新价大于等于条件价\n");
						LOG("3.最新价小于条件价\n");
						LOG("4.最新价小于等于条件价\n");
						LOG("5.卖一价大于条件价\n");
						LOG("6.卖一价大于等于条件价\n");
						LOG("7.卖一价小于条件价\n");
						LOG("8.卖一价小于等于条件价\n");
						LOG("9.买一价大于条件价\n");
						LOG("10.买一价大于等于条件价\n");
						LOG("11.买一价小于条件价\n");
						LOG("12.买一价小于等于条件价\n");
						LOG("13.返回上一层\n");
						LOG("请输入你需要报入的条件单类�?\n");
						int num;
						cin >> num;
						if (num < 1 || num>13)
						{
							LOG("输入的序号有�?请重新输�?\n");
							_getch();
							goto it;
						}
						else if (num == 13)
						{
							goto orderinsert;
						}
						else
						{
							sh.ReqOrderInsert_Condition(num);//??????????
							_getch();
							break;
						}
					}
					case 11://??????????????
					{
						sh.ReqOrderAction_Condition();
						_getch();
						break;
					}
					case 24://????????????
					{
						sh.ReqOrderInsert_Touch1();
						_getch();
						break;
					}
					case 25://???????锟斤�????
					{
						sh.ReqOrderInsert_Touch();
						_getch();
						break;
					}
					case 26://??????????
					{
						sh.ReqOrderInsert_TouchProfit();
						_getch();
						break;
					}
					case 27://FOK??????
					{
						sh.ReqOrderInsert_VC_CV();
						_getch();
						break;
					}
					case 28://FAK???????
					{
						sh.ReqOrderInsert_VC_AV();
						_getch();
						break;
					}
					case 29://?锟斤�??
					{
						sh.ReqOrderInsert_AnyPrice();
						_getch();
						break;
					}
					case 30://???????
					{
						sh.ReqOrderInsert_Arbitrage();
						_getch();
						break;
					}
					case 31://??????
					{
						sh.ReqOrderInsert_IsSwapOrder();
						_getch();
						break;
					}
					case 32://???????
					{
						sh.ReqCombActionInsert();
						_getch();
						break;
					}
					case 33://???????????
					{
						sh.ReqOffsetSetting();
						_getch();
						break;
					}
					case 34://??????????????
					{
						sh.ReqCancelOffsetSetting();
						_getch();
						break;
					}
					case 35://??????????????
					{
						sh.ReqQryOffsetSetting();
						_getch();
						break;
					}
					default:
						LOG("??????????????\n");
						_getch();
						goto orderinsert;
					}
					goto orderinsert;
				}
				case 5://???????
				{
				search:system("cls");
					int choose_num;
					LOG("4.请求查�??交易通知\n");
					LOG("5.请求查�?��?�户通知\n");
					LOG("11.查�?�成�?\n");
					LOG("12.查�?��?�埋单\n");
					LOG("13.查�?��?�埋撤单\n");
					LOG("14.查�?�报单\n");
					LOG("15.撤单对应查�?�编号\n");
					LOG("16.请求查�?�资金账户\n");//ReqQryTradingAccount
					LOG("17.请求查�?�投资者持仓\n");//ReqQryInvestorPosition
					LOG("18.请求查�?�投资者持仓明细\n");//ReqQryInvestorPositionDetail
					LOG("19.请求查�??交易所保证金率\n");//ReqQryExchangeMarginRate
					LOG("20.请求查�?�合约保证金率\n");//ReqQryInstrumentMarginRate
					LOG("21.请求查�?�合约手�?费率\n");//ReqQryInstrumentCommissionRate
					LOG("22.请求查�?�做市商合约手续费率\n");//ReqQryMMInstrumentCommissionRate
					LOG("23.请求查�?�做市商期权合约手续费\n"); //ReqQryMMOptionInstrCommRate
					LOG("24.请求查�?�报单手�?费\n");//ReqQryInstrumentOrderCommRate
					LOG("25.请求查�?�期权合约手�?费\n");//ReqQryOptionInstrCommRate
					LOG("26.请求查�?�合�?\n");//ReqQryInstrument
					LOG("27.请求查�?�投资者结算结果\n");//ReqQrySettlementInfo
					LOG("28.请求查�?�转帐流水\n");//ReqQryTransferSerial
					LOG("29.请求查�?��??价\n");
					LOG("30.请求查�?�报价\n");
					LOG("31.请求查�?�执行�?�告\n");
					LOG("32.请求查�?�转帐银行\n");
					LOG("33.请求查�??交易通知\n");
					LOG("34.请求查�??交易编码\n");
					LOG("35.请求查�?�结算信�?�?�?\n");
					LOG("36.请求查�??产品组\n");
					LOG("37.请求查�?�投资者单元\n");
					LOG("38.期货发起查�?�银行余额�?�求\n");
					LOG("39.请求查�?�经�?�?司交易参数\n");
					LOG("40.查�?�最大报单数量�?�求\n");
					LOG("41.请求查�?�分类合�?\n");
					LOG("42.请求组合优惠比例\n");
					LOG("43.请求查�?�投资者品�?跨品种保证金\n");
					LOG("44.请求查�??交易所调整保证金率\n");
					LOG("45.投资者�?�险结算持仓查�??\n");
					LOG("46.风险结算产品查�??\n");
					LOG("47.查�?�银期�?�约关系\n");
					LOG("48.请求查�?��?�约银�?�\n");
					LOG("49.请求查�?�监控中心用户令牌\n");
					LOG("50.请求查�?�期权交易成本\n");
					LOG("51.请求查�?�组合腿信息\n");
					LOG("52.One-key cancel all active orders\n");					
					LOG("0.返回上一层\n");
					LOG("请输入选择的序�?\n");
					cin >> choose_num;
					switch (choose_num)
					{
					case 4://????????????
					{
						sh.ReqQryTradingNotice();
						_getch();
						break;
					}
					case 5://???????????
					{
						sh.ReqQryNotice();
						_getch();
						break;
					}
					case 11://?????????
					{
						sh.ReqQryTrade();
						_getch();
						break;
					}
					case 12://???????????????
					{
						sh.ReqQryParkedOrder();
						_getch();
						break;
					}
					case 13://????????????????
					{
						sh.ReqQryParkedOrderAction();
						_getch();
						break;
					}
					case 14://??????????
					{
						sh.ReqQryOrder();
						_getch();
						break;
					}
					case 15://???????????????
					{
					action:int action_num;
						LOG("请输入需要撤单的序号：\n");
						cin >> action_num;
						LOG("%d\n", action_num);
						if (action_num < 1 || action_num > static_cast<int>(vector_OrderSysID.size()))
						{
							LOG("输入的序号有�?请重新输�?\n");
							_getch();
							goto action;
						}
						sh.ReqOrderAction_forqry(action_num);
						_getch();
						break;
					}
					case 16://????????????
					{
						sh.ReqQryTradingAccount();
						_getch();
						break;
					}
					case 52:
						{
							// sh.OneKeyCancelAllActiveOrders();
							_getch();
							break;
						}case 17://?????????????
					{
						sh.ReqQryInvestorPosition();
						_getch();
						break;
					}
					case 18://????????????????
					{
						sh.ReqQryInvestorPositionDetail();
						_getch();
						break;
					}
					case 19://???????????????????
					{
						sh.ReqQryExchangeMarginRate();
						_getch();
						break;
					}
					case 20://????????????????
					{
						sh.ReqQryInstrumentMarginRate();
						_getch();
						break;
					}
					case 21://?????????????????
					{
						sh.ReqQryInstrumentCommissionRate();
						_getch();
						break;
					}
					case 22://??????????????????????
					{
						sh.ReqQryMMInstrumentCommissionRate();
						_getch();
						break;
					}
					case 23://????????????????????????
					{
						sh.ReqQryMMOptionInstrCommRate();
						_getch();
						break;
					}
					case 24://????????????????
					{
						sh.ReqQryInstrumentOrderCommRate();
						_getch();
						break;
					}
					case 25://??????????????????
					{
						sh.ReqQryOptionInstrCommRate();
						_getch();
						break;
					}
					case 26://?????????
					{
						sh.ReqQryInstrument();
						_getch();
						break;
					}
					case 27://????????????????
					{
						sh.ReqQrySettlementInfo();
						_getch();
						break;
					}
					case 28://????????????
					{
						sh.ReqQryTransferSerial();
						_getch();
						break;
					}
					case 29://?????????
					{
						sh.ReqQryForQuote();
						_getch();
						break;
					}
					case 30://??????????
					{
						sh.ReqQryQuote();
						_getch();
						break;
					}
					case 31://?????????????
					{
						sh.ReqQryExecOrder();
						_getch();
						break;
					}
					case 32://?????????????
					{
						sh.ReqQryTransferBank();
						_getch();
						break;
					}
					case 33://????????????
					{
						sh.ReqQryTradingNotice();
						_getch();
						break;
					}
					case 34://?????????????
					{
						sh.ReqQryTradingCode();
						_getch();
						break;
					}
					case 35://????????????????
					{
						sh.ReqQrySettlementInfoConfirm();
						_getch();
						break;
					}
					case 36://???????????
					{
						sh.ReqQryProductGroup();
						_getch();
						break;
					}
					case 37://?????????????
					{
						sh.ReqQryInvestUnit();
						_getch();
						break;
					}
					case 38://????????????????????
					{
						sh.ReqQueryBankAccountMoneyByFuture();
						_getch();
						break;
					}
					case 39://???????????????????
					{
						sh.ReqQryBrokerTradingParams();
						_getch();
						break;
					}
					case 40://???????????????
					{
						sh.ReqQryMaxOrderVolume();
						_getch();
						break;
					}
					case 41://????????????
					{
						sh.ReqQryClassifiedInstrument();
						_getch();
						break;
					}
					case 42://?????????????
					{
						sh.ReqQryCombPromotionParam();
						_getch();
						break;
					}
					case 43://??????????????/?????????
					{
						sh.ReqQryInvestorProductGroupMargin();
						_getch();
						break;
					}
					case 44://???????????????????????
					{
						sh.ReqQryExchangeMarginRateAdjust();
						_getch();
						break;
					}
					case 45://???????????????????????
					{
						sh.ReqQryRiskSettleInvstPosition();
						_getch();
						break;
					}
					case 46://???????????????????????
					{
						sh.ReqQryRiskSettleProductStatus();
						_getch();
						break;
					}
					case 47://????????????
					{
						sh.ReqQryAccountregister();
						_getch();
						break;
					}
					case 48://????????????
					{
						sh.ReqQryContractBank();
						_getch();
						break;
					}
					case 49://????????????????????
					{
						sh.ReqQueryCFMMCTradingAccountToken();
						_getch();
						break;
					}
					case 50:
					{
						sh.ReqQryOptionInstrTradeCost();
						_getch();
						break;
					}
					case 51:
					{
						sh.ReqQryCombLeg();
						_getch();
						break;
					}
					case 0:
					{
						goto loop;
					}
					default: {
						LOG("请输入�?�确的序号\n");
						_getch();
						goto search;
					}
					}
					goto search;
				}
				case 6://???&??????
				{
				Exec:system("cls");
					int num_xingquan;
					LOG("32.执�?��?�告录入请求\n");
					LOG("33.执�?��?�告操作请求\n");
					LOG("34.放弃行权\n");
					LOG("35.�?价录入�?�求\n");
					LOG("36.做市商报价录入�?�求\n");
					LOG("37.做市商报价撤销请求\n");
					LOG("0.返回上一�\n");
					LOG("请选择你需要的编码\n");
					cin >> num_xingquan;
					switch (num_xingquan)
					{
					case 32://??????????????
					{
						sh.ReqExecOrderInsert(0);
						_getch();
						break;
					}
					case 33://??????????????
					{
						sh.ReqExecOrderAction();
						_getch();
						break;
					}
					case 34://???????
					{
						sh.ReqExecOrderInsert(1);
						_getch();
						break;
					}
					case 35://??????????
					{
						string g_chFrontMdaddr = getConfig("config", "FrontMdAddr");
						cout << "g_chFrontMdaddr = " << g_chFrontMdaddr << "\n" << endl;
						CThostFtdcMdApi  *pUserMdApi =
							CThostFtdcMdApi::CreateFtdcMdApi();
						CSimpleMdHandler ash(pUserMdApi);
						pUserMdApi->RegisterSpi(&ash);
						pUserMdApi->RegisterFront(const_cast<char *>(g_chFrontMdaddr.c_str()));
						pUserMdApi->Init();
						WaitForSingleObject(xinhao, INFINITE);
						ash.ReqUserLogin();
						WaitForSingleObject(xinhao, INFINITE);
						ash.SubscribeMarketData();//???锟紽?????????
						sh.ReqForQuoteInsert();//???????????
						_getch();
						pUserMdApi->Release();
						break;
					}
					case 36://????????????????
					{
						sh.ReqQuoteInsert();
						_getch();
						break;
					}
					case 37://????????????????
					{
						sh.ReqQuoteAction();
						_getch();
						break;
					}
					case 0:
					{
						goto loop;
					}
					default:
						LOG("输入的编码有�?，�?�重新输�?\n");
						_getch();
						//goto Exec;
					}
					goto Exec;
				}
				case 7://?????????
				{
				futrue:system("cls");
					int num_future;
					LOG("38.期货发起银�?�资金转期货请求\n");
					LOG("39.期货发起期货资金�?银�?��?�求\n");
					LOG("0.返回上一层\\n");
					LOG("请输入你需要的操作序号�\n");
					cin >> num_future;
					switch (num_future)
					{
					case 38://??????????????????????
					{
						sh.ReqFromBankToFutureByFuture();
						_getch();
						break;
					}
					case 39://??????????????????????
					{
						sh.ReqFromFutureToBankByFuture();
						_getch();
						break;
					}
					case 0:
					{
						goto loop;
					}
					default:
						LOG("输入的编码有�?，�?�重新输�?\n");
						_getch();
						//goto futrue;
					}
					goto futrue;
				}
				case 8://????????
				{
				qiquan:system("cls");
					int num_qiquan;
					LOG("//////////////铜期权测�?///////////\n");
					LOG("1.期权�?对冲录入请求\n");
					LOG("2.期权�?对冲操作请求\n");
					LOG("3.请求查�?�期权自对冲\n");
					LOG("0.返回上一层\n");
					LOG("请选择你需要的编码:\n");
					cin >> num_qiquan;
					switch (num_qiquan)
					{
					case 1://??????????????
					{
						sh.ReqOptionSelfCloseInsert();
						_getch();
						break;
					}
					case 2://??????????????
					{
						sh.ReqOptionSelfCloseAction();
						_getch();
						break;
					}
					case 3://?????????????
					{
						sh.ReqQryOptionSelfClose();
						_getch();
						break;
					}
					case 0:
						goto loop;
						break;
					default:
						LOG("锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟�??。\n\n");
						_getch();
						goto qiquan;
					}
					goto qiquan;
				}
				case 9://?锟斤�?.6.5?????
				{
				NewVersion:
					system("cls");
					int num_Newversion;
					LOG("新版�?测试\n");
					LOG("1.请求查�??交易员报盘机\n");
					LOG("0.返回上一层\n");
					LOG("请选择你需要的编码:\n");
					cin >> num_Newversion;
					switch (num_Newversion)
					{
					case 1://????????????????
					{
						sh.ReqQryTraderOffer();
						_getch();
						break;
					}
					case 0:
						goto loop;
						break;
					default:
						LOG("输入的序号有�?，�?�重新输入。\\n");
						_getch();
						goto NewVersion;
					}
					goto NewVersion;
				}
				case 0:
					system("cls");
					break;
				case 100:
					pUserApi->Release();
					exit(-1);
				}
			}
			return 0;
		}
		default: {
			LOG("请输入�?�确的序号�\n");
			_getch();
		}
		}
	}
}
