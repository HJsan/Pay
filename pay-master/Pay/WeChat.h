#pragma once
#include <map>
#include <vector>
#include <functional>
#include "Pay/PayError.h"

namespace SAPay{

enum CWeChatRet
{
	//δ֪����
	WECHAT_RET_UNKNOW_ERROR,

	//return code����
	WECHAT_RET_ERR_CODE_ERROR,

	//return msg����
	WECHAT_RET_RET_MSG_ERROR,

	//�������
	WECHAT_RET_NETWORK_ERROR,

	//������Ϣ��������
	WECHAT_RET_PARSE_ERROR,

	//��ǩ����
	WECHAT_RET_VERIFY_ERROR,

	//��֤����Ϣ
	WECHAT_RET_MISSING_CERT_INFO,

	//��app secret��Ϣ
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
	//��xml����Ϊmap
	static void parseWechatRespsAndNotify(
		const std::string& strNotify,
		std::map<std::string, std::string>& mapNameValue
	);

	//��ǩ����
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
	*										����ֶΰ��� strTradeState, strBankType, strTotalFee,
	*										strCashFee,strTransactionId, strOutTradeNo,strTimeEnd, strTradeStateDesc
	*
	* @param strOutTradingCode				��Ҫ��ѯ�Ķ�����			
	*/
	void queryPayStatus(const std::string& strOutTradingCode, CWeChatResps& wechatResps);

	/**
	* @name smallProgramLogin
	*
	* @brief								
	*
	* @note									see https://developers.weixin.qq.com/miniprogram/dev/api/api-login.html?t=20161122
	*										����ֶΰ��� strSessionKey, strOpenId
	*
	* @param strJsCode						�ͻ������ɵ�jsCode				
	*/
	void smallProgramLogin(const std::string& strJsCode, CWeChatResps& wechatResps);

	/**
	* @name prepay
	*
	* @brief								get prepayId
	*
	* @note									see https://pay.weixin.qq.com/wiki/doc/api/wxa/wxa_api.php?chapter=9_1&index=1
	*										����ֶΰ��� strTradeType, strPrepayId
	*
	* @param iAmount						���׽��,��λΪ��
	* @param llValidTime					ʧЧʱ��,��λΪ��
	* @param strTradingCode					�̻����׵���
	* @param strRemoteIP					�ͻ��˵�ַ
	* @param strBody						body
	* @param strCallBackAddr				�ص���ַ
	* @param strAttach						�Զ�����Ϣ
	* @param strOpenId						openId(�����С����,��ش����ֶ�)
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
	*										����ֶΰ��� strPrepaySignedContent
	*
	*										������Զ�������ǩ���ĸ�ʽ,
	*										�봴��һ���µ���̳���CWeChat,����дsignSmallProgramPrepayInfo��signAppPrepayInfo����
	*
	* @param iAmount						���׽��,��λΪ��
	* @param llValidTime					ʧЧʱ��,��λΪ��
	* @param strTradingCode					�̻����׵���
	* @param strRemoteIP					�ͻ��˵�ַ
	* @param strBody						body
	* @param strCallBackAddr				�ص���ַ
	* @param strAttach						�Զ�����Ϣ
	* @param strOpenId						openId(�����С����,��ش����ֶ�)
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
	* @param iTotalAmount					�����ܽ��
	* @param iRefundAmount					��Ҫ�˿�Ľ��
	* @param strOutTradeNo					��Ҫ�˿�Ķ�����
	* @param strOutRefundNo					�˿����					
	* @param strRemarks						��ע
	* @param strCallBackAddr				�ص���ַ
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

	//������Ϣ����
	virtual void parseQueryStatusResps(const std::string& strReq, const std::string& strResps, std::map<std::string, std::string>& mapResps, CWeChatResps* pWechatResps);

	virtual void parsePrepayResps(const std::string& strReq, const std::string& strResps, std::map<std::string, std::string>& mapResps, CWeChatResps* pWechatResps);

	virtual void parseRefundResps(const std::string& strReq, const std::string& strResps, std::map<std::string, std::string>& mapResps, CWeChatResps* pWechatResps);

	/**
	* @name signSmallProgramPrepayInfo
	*
	* @note									������д�˷�����ʵ���Զ���С��������ǩ��������
	*
	* @param strNonceStr					����ַ���
	* @param strTimeStamp					ʱ���
	* @param strPrepayId					prepayId
	* @param strSignResult					ǩ�����
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
	* @note									������д�˷�����ʵ���Զ���APP����ǩ��������
	*
	* @param strNonceStr					����ַ���
	* @param strTimeStamp					ʱ���
	* @param strPrepayId					prepayId
	* @param strSignResult					ǩ�����
	*/
	virtual void appendAppPrepayInfo(
		const std::string& strNonceStr,
		const std::string& strTimeStamp,
		const std::string& strPrepayId,
		const std::string& strSignResult,
		std::string& strPrepaySignedContent
	);




	//ǩ��
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

	//ƴ������
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