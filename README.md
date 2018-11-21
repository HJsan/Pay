# Pay
用c++简单实现支付宝、微信支付服务端api，支持APP及微信小程序支付
包括：
支付宝prepay字符串生成、退款、单笔转账、查询接口
微信统一下单及再次签名、退款、小程序登录、查询接口

如有任何问题,请联系邮箱samlior@foxmail.com

# include
```
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"  
#include "rapidjson/stringbuffer.h"

#include "Tinyxml/tinyxml.h"

#include <curl/curl.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
```

# 支付宝退款接口调用示例
```
//创建接口对象
CAlipay alipay(
	//app id
	ALIPAY_APP_ID,
	//支付宝公钥
	ALIPAY_PUB_KEY,
	//商户私钥
	ALIPAY_PRIV_KEY,
	//true-沙箱模式 false-真实环境
	true
);

//创建对象保存结果
CAlipayResps alipayResps;
try
{
	//调用退款接口
	alipay.refund(
		//退款金额,单位为分
		10000,
		//退款订单号
		"your refund trading code",
		//订单号
		"trading code",
		alipayResps
	);
}
catch (const CAlipayError& e)
{
	//处理异常
	CAlipayRet iAlpayRet = e.getErrorCode();
	//网络错误(curl错误)
	if (iAlpayRet == ALIPAY_RET_NETWORK_ERROR)
		cout << "alipay net work error, code : " << e.getNetWorkCode() << endl;
	//验签失败
	else if (iAlpayRet == ALIPAY_RET_VERIFY_ERROR)
		cout << "alipay verify error, req : " << e.getLastReq() << " resps : " << e.getLastResps() << endl;
	//返回信息解析错误
	else if (iAlpayRet == ALIPAY_RET_PARSE_ERROR)
		cout << "alipay resps parse error, req : " << e.getLastReq() << " resps : " << e.getLastResps() << endl;
	//返回错误
	else if (iAlpayRet == ALIPAY_RET_SUB_CODE_ERROR)
		cout << "alipay sub code error, sub_code : " << e.getErrInfo() << " req : " << e.getLastReq() << " resps : " << e.getLastResps() << endl;
	return false;
}

cout << "success" << endl;
//退款金额
cout << alipayResps.strRefundFee << endl;
//退款时间
cout << alipayResps.strGmtRefundPay << endl;
cout << endl << endl << endl;
```

# 微信统一下单接口调用示例
```
//创建接口对象
CWeChat wechat(
	//app id
	WECHAT_APP_ID,
	//商户id
	WECHAT_MCH_ID,
	//商户key
	WECHAT_MCH_KEY,
	//app secret
	WECHAT_APP_SECRET,
	//证书地址(调用退款接口时需要证书,pem格式)
	"cert/apiclient_cert.pem",
	//key地址(调用退款接口时需要证书,pem格式)
	"cert/apiclient_key.pem"
);

//创建对象保存结果
CWeChatResps wechatResps;
try
{
	wechat.prepayWithSign(
		//交易金额,单位为分
		10000,
		//超时时长,单位为秒
		5000,
		//交易单号
		"trade no",
		//客户端ip
		"remote ip",
		//body
		"body",
		//回调地址
		"call back address",
		wechatResps
	);
}
catch (const CWechatError& e)
{
	//处理异常
	CWechatRet iWeChatRet = e.getErrorCode();
	//网络错误(curl错误)
	if (iWeChatRet == WECHAT_RET_NETWORK_ERROR)
		cout << "wechat net work error, code : " << e.getNetWorkCode() << endl;
	//验签失败
	else if (iWeChatRet == WECHAT_RET_VERIFY_ERROR)
		cout << "wechat verify error, req : " << e.getLastReq() << " resps : " << e.getLastResps() << endl;
	//返回信息解析错误
	else if (iWeChatRet == WECHAT_RET_PARSE_ERROR)
		cout << "wechat resps parse error, req : " << e.getLastReq() << " resps : " << e.getLastResps() << endl;
	//return code 错误
	else if (iWeChatRet == WECHAT_RET_ERR_CODE_ERROR)
		cout << "wechat sub code error, err_code : " << e.getErrInfo() << " req : " << e.getLastReq() << " resps : " << e.getLastResps() << endl;
	//return msg 错误
	else if (iWeChatRet == WECHAT_RET_RET_MSG_ERROR)
		cout << "wechat return msg error, return_msg : " << e.getErrInfo() << " req : " << e.getLastReq() << " resps : " << e.getLastResps() << endl;
	return false;
}

//输出结果
cout << "success" << endl;
//统一下单后再次签名结果
cout << wechatResps.strPrepaySignedContent << endl;
cout << endl << endl << endl;
return true;
```