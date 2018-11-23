#pragma once

#include <map>
#include <vector>
#include <functional>
#include "rapidjson/document.h"
#include "Pay/PayError.h"

namespace SAPay{

enum CAlipayRet
{
	//δ֪����
	ALIPAY_RET_UNKNOW_ERROR,

	//sub code ����
	ALIPAY_RET_SUB_CODE_ERROR,

	//�������
	ALIPAY_RET_NETWORK_ERROR,

	//������Ϣ��������
	ALIPAY_RET_PARSE_ERROR,

	//��ǩ����
	ALIPAY_RET_VERIFY_ERROR
};

using CAlipayError = CPayError<CAlipayRet>;





enum CAlipayTradeStatus
{
	ALIPAY_TRADE_STATUS_SUCCESS,
	ALIPAY_TRADE_STATUS_CLOSED,
	ALIPAY_TRADE_STATUS_FINISHED,
	ALIPAY_TRADE_STATUS_WAIT_BUYER_PAY,
	ALIPAY_TRADE_STATUS_UNKONW
};

struct CAlipayResps
{
	CAlipayResps() :iTradeStatus(ALIPAY_TRADE_STATUS_UNKONW) {}
	//withdraw
	std::string strOutBizNo;
	std::string strPayDate;
	std::string strOrderId;

	//refund and query
	std::string strTradeNo;
	std::string strOutTradeNo;
	std::string strBuyerLogonId;
	std::string strBuyerUserId;

	//refund
	std::string strGmtRefundPay;
	std::string strFundChange;
	std::string strRefundFee;

	//query
	CAlipayTradeStatus iTradeStatus;
	std::string strTotalAmount;

	//query refund
	std::string strOutRequestNo;
	std::string strRefundAmount;
};





class CAlipay
{
public:
	//��json֪ͨ����Ϊmap
	static void parseAlipayNotify(
		const std::string& strNotify,
		std::map<std::string, std::string>& mapKeyValue
	);

	//��֪ͨ��ǩ 0-sucess other-failed
	static int verifyAlipayNotify(
		const std::map<std::string, std::string>& mapNotify,
		const std::string& strPubKey
	);

	//�Է��ؽ����ǩ 0-sucess other-failed
	static int verifyAlipayResps(
		const std::string& strRespsContent, 
		const std::string& strSign, 
		const std::string& strPubKey
	);

public:
	CAlipay() = delete;

	/**
	* @name CAlipay
	*
	* @param strAppId						alipay app id
	* @param strPubKey						֧������Կ
	* @param strPrivKey						�̻�˽Կ
	* @param bIsDevMode						�Ƿ���ɳ��ģʽ,�����,�����ַΪ https://openapi.alipaydev.com/gateway.do
	*										����Ϊ https://openapi.alipay.com/gateway.do
	*/
	CAlipay(
		const std::string& strAppId,
		const std::string& strPubKey,
		const std::string& strPrivKey,
		bool bIsDevMode = false
	);
	virtual ~CAlipay() {}

	

	/**
	* @name appendPayContent
	*
	* @brief								
	*
	* @param iAmount						���׽��,��λΪ��
	* @param strTradingCode					�̻�������
	* @param strSubject						subject
	* @param strCallBackAddr				�ص���ַ
	* @param strTimeOut						��Чʱ�� https://docs.open.alipay.com/204/105465/
	* @param strPassBackParams				�Զ�����Ϣ
	*/
	void appendPayContent(
		std::string& strContent,
		int iAmount,
		const std::string& strTradingCode,
		const std::string& strSubject,
		const std::string& strCallBackAddr,
		const std::string& strTimeOut = std::string("30m"),
		const std::string& strPassBackParams = std::string("")
	);

	/**
	* @name refund
	*
	* @brief								
	*
	* @note									see https://docs.open.alipay.com/api_1/alipay.trade.refund
	*										����ֶΰ��� strTradeNo, strOutTradeNo, strBuyerLogonId, 
	*										strBuyerUserId, strGmtRefundPay, strFundChange, strRefundFee
	*
	* @param iAmount						�˿���
	* @param strTradingCode					�˿����
	* @param strOutTradingCode				��Ҫ�˿�Ķ�����						
	*/
	void refund(
		int iAmount,
		const std::string& strTradingCode,
		const std::string& strOutTradingCode,
		CAlipayResps& alipayResps
	);

	/**
	* @name withdraw
	*
	* @brief								���֡�����ת��
	*
	* @note									see https://docs.open.alipay.com/api_28/alipay.fund.trans.toaccount.transfer
	*										����ֶΰ��� strOutBizNo, strPayDate, strOrderId
	*
	* @param iAmount						ת�˽��
	* @param strTradingCode					������
	* @param strAlipayAccount				�û�֧�����˺�
	* @param strTrueName					�û���ʵ����		
	* @param strRemarks						��ע
	*/
	void withdraw(
		int iAmount,
		const std::string& strTradingCode,
		const std::string& strAlipayAccount,
		const std::string& strTrueName,
		CAlipayResps& alipayResps,
		const std::string& strRemarks = std::string("")
	);

	/**
	* @name queryPayStatus
	*
	* @brief								��ѯ����
	*
	* @note									see https://docs.open.alipay.com/api_1/alipay.trade.query/
	*										����ֶΰ��� strTradeNo, strOutTradeNo, strBuyerLogonId, 
	*										strBuyerUserId, strTradeStatus, strTotalAmount
	*
	* @param strOutTradingCode				��Ҫ��ѯ�Ķ�����				
	*/
	void queryPayStatus(
		const std::string& strOutTradingCode,
		CAlipayResps& alipayResps
	);

	/**
	* @name queryRefund
	*
	* @brief								��ѯ�����˿�״̬
	*
	* @note
	*
	* @param strOutTradingCode				���˿�Ķ�����
	* @param strRefundTradingCode			�˿����
	*/
	void queryRefund(
		const std::string& strOutTradingCode,
		const std::string& strRefundTradingCode,
		CAlipayResps& alipayResps
	);

protected:
	//token
	bool m_bIsDevMode;
	std::string m_strAppId;
	std::string m_strPubKey;
	std::string m_strPrivKey;

protected:
	using ParseFunc = std::function<void(const std::string&, const std::string&, rapidjson::Value&)>;

	//������Ϣ����
	virtual void parseTransferResps(const std::string& strReq, const std::string& strResps, rapidjson::Value& respsContent, CAlipayResps* pAlipayResps);

	virtual void parseRefundResps(const std::string& strReq, const std::string& strResps, rapidjson::Value& respsContent, CAlipayResps* pAlipayResps);

	virtual void parseQueryStatusResps(const std::string& strReq, const std::string& strResps, rapidjson::Value& respsContent, CAlipayResps* pAlipayResps);

	virtual void parseQueryRefundResps(const std::string& strReq, const std::string& strResps, rapidjson::Value& respsContent, CAlipayResps* pAlipayResps);
	
	void sendReqAndParseResps(
		const std::string& strReq,
		const std::string& strRespsName,
		ParseFunc func
	);

	//ƴ����������
	void appendContentAndSign(
		std::string& totalString, 
		const std::string& biz_content, 
		const std::string& strMethodName,
		const std::string& strCharset = "utf-8",
		const std::string& strCallBack = ""
	);

	void appendTransferContent(
		std::string& strReq,
		int iAmount,
		const std::string& strAlipayAccount,
		const std::string& strTrueName,
		const std::string& strTradingCode,
		const std::string& strRemarks = std::string("")
	);
	void appendRefundContent(
		std::string& strReq,
		int iAmount,
		const std::string& strTradingCode,
		const std::string& strOutTradingCode
	);
	void appendQueryStatusContent(
		std::string& strReq,
		const std::string& strOutTradingCode
	);
	void appendQueryRefundContent(
		std::string& strReq,
		const std::string& strOutTradingCode,
		const std::string& strRefundTradingCode
	);
};

}