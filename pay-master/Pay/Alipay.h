#pragma once

#include <map>
#include <vector>
#include <functional>
#include "rapidjson/document.h"
#include "Pay/PayError.h"

namespace SAPay{

enum CAlipayRet
{
	//未知错误
	ALIPAY_RET_UNKNOW_ERROR,

	//sub code 错误
	ALIPAY_RET_SUB_CODE_ERROR,

	//网络错误
	ALIPAY_RET_NETWORK_ERROR,

	//返回信息解析错误
	ALIPAY_RET_PARSE_ERROR,

	//验签错误
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
	//将json通知解析为map
	static void parseAlipayNotify(
		const std::string& strNotify,
		std::map<std::string, std::string>& mapKeyValue
	);

	//对通知验签 0-sucess other-failed
	static int verifyAlipayNotify(
		const std::map<std::string, std::string>& mapNotify,
		const std::string& strPubKey
	);

	//对返回结果验签 0-sucess other-failed
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
	* @param strPubKey						支付宝公钥
	* @param strPrivKey						商户私钥
	* @param bIsDevMode						是否开启沙箱模式,如果是,请求地址为 https://openapi.alipaydev.com/gateway.do
	*										否则为 https://openapi.alipay.com/gateway.do
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
	* @param iAmount						交易金额,单位为分
	* @param strTradingCode					商户订单号
	* @param strSubject						subject
	* @param strCallBackAddr				回调地址
	* @param strTimeOut						生效时间 https://docs.open.alipay.com/204/105465/
	* @param strPassBackParams				自定义消息
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
	*										输出字段包括 strTradeNo, strOutTradeNo, strBuyerLogonId, 
	*										strBuyerUserId, strGmtRefundPay, strFundChange, strRefundFee
	*
	* @param iAmount						退款金额
	* @param strTradingCode					退款订单号
	* @param strOutTradingCode				需要退款的订单号						
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
	* @brief								提现、单笔转账
	*
	* @note									see https://docs.open.alipay.com/api_28/alipay.fund.trans.toaccount.transfer
	*										输出字段包括 strOutBizNo, strPayDate, strOrderId
	*
	* @param iAmount						转账金额
	* @param strTradingCode					订单号
	* @param strAlipayAccount				用户支付宝账号
	* @param strTrueName					用户真实姓名		
	* @param strRemarks						备注
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
	* @brief								查询订单
	*
	* @note									see https://docs.open.alipay.com/api_1/alipay.trade.query/
	*										输出字段包括 strTradeNo, strOutTradeNo, strBuyerLogonId, 
	*										strBuyerUserId, strTradeStatus, strTotalAmount
	*
	* @param strOutTradingCode				需要查询的订单号				
	*/
	void queryPayStatus(
		const std::string& strOutTradingCode,
		CAlipayResps& alipayResps
	);

	/**
	* @name queryRefund
	*
	* @brief								查询订单退款状态
	*
	* @note
	*
	* @param strOutTradingCode				被退款的订单号
	* @param strRefundTradingCode			退款订单号
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

	//返回信息解析
	virtual void parseTransferResps(const std::string& strReq, const std::string& strResps, rapidjson::Value& respsContent, CAlipayResps* pAlipayResps);

	virtual void parseRefundResps(const std::string& strReq, const std::string& strResps, rapidjson::Value& respsContent, CAlipayResps* pAlipayResps);

	virtual void parseQueryStatusResps(const std::string& strReq, const std::string& strResps, rapidjson::Value& respsContent, CAlipayResps* pAlipayResps);

	virtual void parseQueryRefundResps(const std::string& strReq, const std::string& strResps, rapidjson::Value& respsContent, CAlipayResps* pAlipayResps);
	
	void sendReqAndParseResps(
		const std::string& strReq,
		const std::string& strRespsName,
		ParseFunc func
	);

	//拼接请求内容
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