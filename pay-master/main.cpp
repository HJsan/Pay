#include <string>
#include <iostream>
#include "Pay/Alipay.h"
#include "Pay/WeChat.h"
#include "Pay/PayError.h"
#include "PayUtils/HttpClient.h"

#define ALIPAY_APP_ID			"your alipay app id"
#define ALIPAY_PUB_KEY			"your alipay pub key"
#define ALIPAY_PRIV_KEY			"your alipay priv key"

#define WECHAT_APP_ID			"your wechat app id"
#define WECHAT_MCH_ID			"your wechat mch id"
#define WECHAT_MCH_KEY			"your wechat mch key"
#define WECHAT_APP_SECRET		"your wechat app secret"

using namespace std;
using namespace SAPay;


static bool testAlipayRefund()
{
	CAlipay alipay(
		ALIPAY_APP_ID,
		ALIPAY_PUB_KEY,
		ALIPAY_PRIV_KEY,
		true
	);

	CAlipayResps alipayResps;
	try
	{
		//refund
		alipay.refund(
			10000,
			"your refund trading code",
			"trading code",
			alipayResps
		);
	}
	catch (const CAlipayError& e)
	{
		CAlipayRet iAlpayRet = e.getErrorCode();
		if (iAlpayRet == ALIPAY_RET_NETWORK_ERROR)
			cout << "alipay net work error, code : " << e.getNetWorkCode() << endl;
		else if (iAlpayRet == ALIPAY_RET_VERIFY_ERROR)
			cout << "alipay verify error, req : " << e.getLastReq() << " resps : " << e.getLastResps() << endl;
		else if (iAlpayRet == ALIPAY_RET_PARSE_ERROR)
			cout << "alipay resps parse error, req : " << e.getLastReq() << " resps : " << e.getLastResps() << endl;
		else if (iAlpayRet == ALIPAY_RET_SUB_CODE_ERROR)
			cout << "alipay sub code error, sub_code : " << e.getErrInfo() << " req : " << e.getLastReq() << " resps : " << e.getLastResps() << endl;
		return false;
	}
	cout << "success" << endl;
	cout << alipayResps.strRefundFee << endl;
	cout << alipayResps.strGmtRefundPay << endl;
	cout << endl << endl << endl;
	return true;
};



static bool testWechatPrepay()
{
	CWeChat wechat(
		WECHAT_APP_ID,
		WECHAT_MCH_ID,
		WECHAT_MCH_KEY,
		WECHAT_APP_SECRET,
		"cert/apiclient_cert.pem",
		"cert/apiclient_key.pem"
	);

	//prepay and sign again
	CWeChatResps wechatResps;
	try
	{
		wechat.prepayWithSign(
			10000,
			5000,
			"trade no",
			"remote ip",
			"body",
			"call back address",
			wechatResps
		);
	}
	catch (const CWeChatError& e)
	{
		CWeChatRet iWeChatRet = e.getErrorCode();
		if (iWeChatRet == WECHAT_RET_NETWORK_ERROR)
			cout << "wechat net work error, code : " << e.getNetWorkCode() << endl;
		else if (iWeChatRet == WECHAT_RET_VERIFY_ERROR)
			cout << "wechat verify error, req : " << e.getLastReq() << " resps : " << e.getLastResps() << endl;
		else if (iWeChatRet == WECHAT_RET_PARSE_ERROR)
			cout << "wechat resps parse error, req : " << e.getLastReq() << " resps : " << e.getLastResps() << endl;
		else if (iWeChatRet == WECHAT_RET_ERR_CODE_ERROR)
			cout << "wechat sub code error, err_code : " << e.getErrInfo() << " req : " << e.getLastReq() << " resps : " << e.getLastResps() << endl;
		else if (iWeChatRet == WECHAT_RET_RET_MSG_ERROR)
			cout << "wechat return msg error, return_msg : " << e.getErrInfo() << " req : " << e.getLastReq() << " resps : " << e.getLastResps() << endl;
		return false;
	}
	cout << "success" << endl;
	cout << wechatResps.strPrepaySignedContent << endl;
	cout << endl << endl << endl;
	return true;
}



int main()
{
	testAlipayRefund();
	testWechatPrepay();
	return 0;
}