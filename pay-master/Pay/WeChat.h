#pragma once
#include <map>
#include <vector>
#include <functional>
#include "Pay/PayError.h"

namespace SAPay{

enum CWeChatRet
{
	//未知错误
	WECHAT_RET_UNKNOW_ERROR,

	//return code错误
	WECHAT_RET_ERR_CODE_ERROR,

	//return msg错误
	WECHAT_RET_RET_MSG_ERROR,

	//网络错误
	WECHAT_RET_NETWORK_ERROR,

	//返回信息解析错误
	WECHAT_RET_PARSE_ERROR,

	//验签错误
	WECHAT_RET_VERIFY_ERROR,

	//无证书信息
	WECHAT_RET_MISSING_CERT_INFO,

	//无app secret信息
	WECHAT_RET_MISSING_APP_SECRET
};

using CWeChatError = CPayError<CWeChatRet>;




enum CWeChatRespsTradeState
{
	WECHAT_TRADE_STATE_SUCCESS,
	WECHAT_TRADE_STATE_REFUND,
	WECHAT_TRADE_STATE_NOTPAY,
	WECHAT_TRADE_STATE_CLOSED,
	WECHAT_TRADE_STATE_REVOKED,
	WECHAT_TRADE_STATE_USERPAYING,
	WECHAT_TRADE_STATE_PAYERROR,
	WECHAT_TRADE_STATE_UNKNOW
};

struct CWeChatResps
{
	CWeChatResps() :iTradeState(WECHAT_TRADE_STATE_UNKNOW) {}

	//small program login
	std::string strSessionKey;
	std::string strOpenId;

	//prepay content
	std::string strTradeType;
	std::string strPrepayId;
	std::string strPrepaySignedContent;

	//query
	CWeChatRespsTradeState iTradeState;
	std::string strBankType;
	std::string strTotalFee;
	std::string strCashFee;
	std::string strTransactionId;
	std::string strOutTradeNo;
	std::string strTimeEnd;
	std::string strTradeStateDesc;

	//refund
	std::string strRefundFee;
	std::string strRefundId;
};





class CWeChat
{
public:
	//将xml解析为map
	static void parseWechatRespsAndNotify(
		const std::string& strNotify,
		std::map<std::string, std::string>& mapNameValue
	);

	//验签函数
	static int verifyWechatRespsAndNotify(
		const std::map<std::string, std::string>& mapResps, 
		const std::string& strMchKey
	);

public:
	CWeChat() = delete;

	/**
	* @name CWeChat
	*
	* @param strAppId						wechat app id
	* @param strMchId						wechat mch id
	* @param strMchKey						wechat mch key
	* @param strAppSecret					wechat app secret 
	* @param strCertPath					cert path
	* @param strKeyPath						key path
	*/
	CWeChat(
		const std::string& strAppId,
		const std::string& strMchId,
		const std::string& strMchKey,
		const std::string& strAppSecret = std::string(""),
		const std::string& strCertPath = std::string(""),
		const std::string& strKeyPath = std::string("")
	);

	virtual ~CWeChat() {}

	/*@param bIsApp							true-APP false-small program*/
	void setIsApp(bool bIsApp) { m_bIsApp = bIsApp; }

	/**
	* @name queryPayStatus
	*
	* @brief								query bill status
	*
	* @note									see https://pay.weixin.qq.com/wiki/doc/api/app/app.php?chapter=9_2&index=4
	*										输出字段包括 strTradeState, strBankType, strTotalFee,
	*										strCashFee,strTransactionId, strOutTradeNo,strTimeEnd, strTradeStateDesc
	*
	* @param strOutTradingCode				需要查询的订单号			
	*/
	void queryPayStatus(const std::string& strOutTradingCode, CWeChatResps& wechatResps);

	/**
	* @name smallProgramLogin
	*
	* @brief								
	*
	* @note									see https://developers.weixin.qq.com/miniprogram/dev/api/api-login.html?t=20161122
	*										输出字段包括 strSessionKey, strOpenId
	*
	* @param strJsCode						客户端生成的jsCode				
	*/
	void smallProgramLogin(const std::string& strJsCode, CWeChatResps& wechatResps);

	/**
	* @name prepay
	*
	* @brief								get prepayId
	*
	* @note									see https://pay.weixin.qq.com/wiki/doc/api/wxa/wxa_api.php?chapter=9_1&index=1
	*										输出字段包括 strTradeType, strPrepayId
	*
	* @param iAmount						交易金额,单位为分
	* @param llValidTime					失效时间,单位为秒
	* @param strTradingCode					商户交易单号
	* @param strRemoteIP					客户端地址
	* @param strBody						body
	* @param strCallBackAddr				回调地址
	* @param strAttach						自定义信息
	* @param strOpenId						openId(如果是小程序,则必传此字段)
	*										
	*/
	void prepay(
		int iAmount,
		long long llValidTime,
		const std::string& strTradingCode,
		const std::string& strRemoteIP,
		const std::string& strBody,
		const std::string& strCallBackAddr,
		CWeChatResps& wechatResps,
		const std::string& strAttach = std::string(""),
		const std::string& strOpenId = std::string("")
	);

	/**
	* @name prepayWithSign
	*
	*
	* @note									see https://pay.weixin.qq.com/wiki/doc/api/wxa/wxa_api.php?chapter=7_7&index=3
	*										and https://pay.weixin.qq.com/wiki/doc/api/app/app.php?chapter=8_3
	*										输出字段包括 strPrepaySignedContent
	*
	*										如果想自定义重新签名的格式,
	*										请创建一个新的类继承自CWeChat,并重写signSmallProgramPrepayInfo与signAppPrepayInfo方法
	*
	* @param iAmount						交易金额,单位为分
	* @param llValidTime					失效时间,单位为秒
	* @param strTradingCode					商户交易单号
	* @param strRemoteIP					客户端地址
	* @param strBody						body
	* @param strCallBackAddr				回调地址
	* @param strAttach						自定义信息
	* @param strOpenId						openId(如果是小程序,则必传此字段)
	*
	*/
	void prepayWithSign(
		int iAmount,
		long long llValidTime,
		const std::string& strTradingCode,
		const std::string& strRemoteIP,
		const std::string& strBody,
		const std::string& strCallBackAddr,
		CWeChatResps& wechatResps,
		const std::string& strAttach = std::string(""),
		const std::string& strOpenId = std::string("")
	);

	/*
	* @name refund
	*
	* @note									see https://pay.weixin.qq.com/wiki/doc/api/app/app.php?chapter=9_4&index=6
	*
	* @param iTotalAmount					订单总金额
	* @param iRefundAmount					需要退款的金额
	* @param strOutTradeNo					需要退款的订单号
	* @param strOutRefundNo					退款订单号					
	* @param strRemarks						备注
	* @param strCallBackAddr				回调地址
	*
	*/
	void refund(
		int iTotalAmount,
		int iRefundAmount,
		const std::string& strOutTradeNo,
		const std::string& strOutRefundNo,
		CWeChatResps& wechatResps,
		const std::string& strRemarks = "",
		const std::string& strCallBackAddr = ""
	);

protected:

	//token
	bool m_bIsApp;
	std::string m_strAppId;
	std::string m_strMchId;
	std::string m_strMchKey;
	std::string m_strAppSecret;
	std::string m_strCertPath;
	std::string m_strKeyPath;

protected:
	using ParseFunc = std::function<void(const std::string& strReq, const std::string& strResps, std::map<std::string, std::string>&)>;

	//返回信息解析
	virtual void parseQueryStatusResps(const std::string& strReq, const std::string& strResps, std::map<std::string, std::string>& mapResps, CWeChatResps* pWechatResps);

	virtual void parsePrepayResps(const std::string& strReq, const std::string& strResps, std::map<std::string, std::string>& mapResps, CWeChatResps* pWechatResps);

	virtual void parseRefundResps(const std::string& strReq, const std::string& strResps, std::map<std::string, std::string>& mapResps, CWeChatResps* pWechatResps);

	/**
	* @name signSmallProgramPrepayInfo
	*
	* @note									可以重写此方法以实现自定义小程序重新签名的内容
	*
	* @param strNonceStr					随机字符串
	* @param strTimeStamp					时间戳
	* @param strPrepayId					prepayId
	* @param strSignResult					签名结果
	*/
	virtual void appendSmallProgramPrepayInfo(
		const std::string& strNonceStr,
		const std::string& strTimeStamp,
		const std::string& strPrepayId,
		const std::string& strSignResult,
		std::string& strPrepaySignedContent
	);
	/**
	* @name appendAppPrepayInfo
	*
	* @note									可以重写此方法以实现自定义APP重新签名的内容
	*
	* @param strNonceStr					随机字符串
	* @param strTimeStamp					时间戳
	* @param strPrepayId					prepayId
	* @param strSignResult					签名结果
	*/
	virtual void appendAppPrepayInfo(
		const std::string& strNonceStr,
		const std::string& strTimeStamp,
		const std::string& strPrepayId,
		const std::string& strSignResult,
		std::string& strPrepaySignedContent
	);




	//签名
	void signPrepay(
		std::string& strSign,
		const std::string& strNonceStr,
		const std::string& strTimeStamp,
		const std::string& strPrepayId
	);

	void sendReqAndParseResps(
		const std::string& strReq,
		const std::string& strHref,
		ParseFunc func,
		bool bPostWithCert = false
	);

	//拼接请求
	void appendSmallProgramLoginContent(std::string& strReq, const std::string& strJsCode);

	void appendQueryStatusContent(std::string& strReq, const std::string& strOutTradingCode);

	void appendPrepayContent(
		std::string& strReq,
		int iAmount,
		long long llValidTime,
		const std::string& strTradingCode,
		const std::string& strRemoteIP,
		const std::string& strBody,
		const std::string& strCallBackAddr,
		const std::string& strAttach = "",
		const std::string& strOpenId = ""
	);

	void appendRefundContent(
		std::string& strReq,
		int iTotalAmount,
		int iRefundAmount,
		const std::string& strOutTradeNo,
		const std::string& strOutRefundNo,
		const std::string& strRemarks = "",
		const std::string& strCallBackAddr = ""
	);
};

}