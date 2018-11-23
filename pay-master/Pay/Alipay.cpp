#include "Alipay.h"
#include "rapidjson/prettywriter.h"  
#include "rapidjson/stringbuffer.h"
#include "PayUtils/Utils.h"
#include "PayUtils/RSAUtils.h"
#include "PayUtils/HttpClient.h"
#include "PayHeader.h"
#include <boost/format.hpp>

using namespace std;
using namespace boost;
using namespace SAPay;


void CAlipay::parseAlipayNotify(const string& strNotify, map<string, string>& mapKeyValue)
{
	vector<string> vecNotify;
	CUtils::split_string(strNotify, "&", vecNotify);
	for (auto itr = vecNotify.begin(); itr != vecNotify.end(); ++itr)
	{
		size_t pos = itr->find('=');
		if (pos != string::npos && itr->size() > pos + 1)
			mapKeyValue[itr->substr(0, pos)] = itr->substr(pos + 1);
	}
}

int CAlipay::verifyAlipayResps(const string& strRespsContent, const string& strSign, const string& pubKey)
{
	return CRSAUtils::rsa_verify_from_pubKey_with_base64(strRespsContent, strSign, pubKey) ? 0 : -1;
}

int CAlipay::verifyAlipayNotify(const map<string, string>& mapNotify, const string& pubKey)
{
	string content(""), sign("");
	vector<string>& vecDictionary = CUtils::createDictionaryWithMap(mapNotify);
	for (auto itr = vecDictionary.begin(); itr != vecDictionary.end(); ++itr)
	{
		if (*itr == ALIPAY_NOTIFY_SIGN_TYPE)
			continue;
		auto itrr = mapNotify.find(*itr);
		if (itrr != mapNotify.end())
		{
			if (*itr == ALIPAY_NOTIFY_SIGN)
			{
				sign = itrr->second;
				continue;
			}
			CUtils::AppendContentWithoutUrlEncode(*itr, itrr->second, content, !content.empty());
		}
	}
	return CRSAUtils::rsa_verify_from_pubKey_with_base64(content, sign, pubKey) ? 0 : -1;
}

template<typename T>
static string convertJsonToString(const T& tValue)
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	tValue.Accept(writer);
	return buffer.GetString();
}

CAlipay::CAlipay(
	const string& strAppId,
	const string& strPubKey,
	const string& strPrivKey,
	bool bIsDevMode /*= false*/
) :
	m_strAppId(strAppId),
	m_strPubKey(strPubKey),
	m_strPrivKey(strPrivKey),
	m_bIsDevMode(bIsDevMode)
{
}

void CAlipay::sendReqAndParseResps(
	const string& strReq,
	const string& strRespsName,
	ParseFunc func
)
{
	string strResps("");
	int iNetWorkRet = CHttpClient::post(m_bIsDevMode ? ALIPAY_HREF_DEV : ALIPAY_HREF, strReq, strResps);
	if (iNetWorkRet)
	{
		throw CAlipayError(ALIPAY_RET_NETWORK_ERROR, strReq, strResps, iNetWorkRet);
	}

	rapidjson::Document respsDocument;
	respsDocument.Parse(strResps.c_str(), strResps.length());
	if (!respsDocument.IsObject() ||
		!respsDocument.HasMember(strRespsName.c_str()) ||
		!respsDocument[strRespsName.c_str()].IsObject())
	{
		throw CAlipayError(ALIPAY_RET_PARSE_ERROR, strReq, strResps, iNetWorkRet);
	}


	rapidjson::Value& respsContent = respsDocument[strRespsName.c_str()];
	if (!respsDocument.HasMember(ALIPAY_RESPS_SIGN) ||
		!respsDocument[ALIPAY_RESPS_SIGN].IsString())
	{
		if (respsContent.HasMember(ALIPAY_RESPS_SUB_CODE) &&
			respsContent[ALIPAY_RESPS_SUB_CODE].IsString())
		{
			throw CAlipayError(ALIPAY_RET_SUB_CODE_ERROR, strReq, strResps, respsContent[ALIPAY_RESPS_SUB_CODE].GetString());
		}
		else
		{
			throw CAlipayError(ALIPAY_RET_UNKNOW_ERROR, strReq, strResps);
		}
	}

	//check sign
	if (verifyAlipayResps(
		convertJsonToString(respsContent),
		respsDocument[ALIPAY_RESPS_SIGN].GetString(),
		m_strPubKey) < 0)
	{
		throw CAlipayError(ALIPAY_RET_VERIFY_ERROR, strReq, strResps);
	}

	if (!respsContent.HasMember(ALIPAY_RESPS_CODE) ||
		!respsContent[ALIPAY_RESPS_CODE].IsString() ||
		!respsContent.HasMember(ALIPAY_RESPS_MSG) ||
		!respsContent[ALIPAY_RESPS_MSG].IsString())
	{
		throw CAlipayError(ALIPAY_RET_PARSE_ERROR, strReq, strResps);
	}

	//check code and msg
	const char* code = respsContent[ALIPAY_RESPS_CODE].GetString();
	const char* msg = respsContent[ALIPAY_RESPS_MSG].GetString();
	if (strcmp(code, "10000") != 0 || strcmp(msg, "Success") != 0)
	{
		if (respsContent.HasMember(ALIPAY_RESPS_SUB_CODE) &&
			respsContent[ALIPAY_RESPS_SUB_CODE].IsString())
		{
			throw CAlipayError(ALIPAY_RET_SUB_CODE_ERROR, strReq, strResps, respsContent[ALIPAY_RESPS_SUB_CODE].GetString());
		}
		else
		{
			throw CAlipayError(ALIPAY_RET_UNKNOW_ERROR, strReq, strResps);
		}
	}

	func(strReq, strResps, respsContent);
}

void CAlipay::refund(
	int iAmount,
	const string& strTradingCode,
	const string& strOutTradingCode,
	CAlipayResps& alipayResps
)
{
	string strReq;
	appendRefundContent(strReq, iAmount, strTradingCode, strOutTradingCode);
	sendReqAndParseResps(strReq, ALIPAY_RESPS_RFND, bind(&CAlipay::parseRefundResps, this, placeholders::_1, placeholders::_2, placeholders::_3, &alipayResps));
}

void CAlipay::parseRefundResps(const string& strReq, const string& strResps, rapidjson::Value& respsContent, CAlipayResps* pAlipayResps)
{
	CAlipayResps& alipayResps = *pAlipayResps;
	if (!respsContent.HasMember(ALIPAY_RESPS_BUYER_LOGON_ID) ||
		!respsContent[ALIPAY_RESPS_BUYER_LOGON_ID].IsString() ||
		!respsContent.HasMember(ALIPAY_RESPS_BUYER_USER_ID) ||
		!respsContent[ALIPAY_RESPS_BUYER_USER_ID].IsString() ||
		!respsContent.HasMember(ALIPAY_RESPS_FUND_CHANGE) ||
		!respsContent[ALIPAY_RESPS_FUND_CHANGE].IsString() ||
		!respsContent.HasMember(ALIPAY_RESPS_GMT_REFUND_PAY) ||
		!respsContent[ALIPAY_RESPS_GMT_REFUND_PAY].IsString() ||
		!respsContent.HasMember(ALIPAY_RESPS_OUT_TRADE_NO) ||
		!respsContent[ALIPAY_RESPS_OUT_TRADE_NO].IsString() ||
		!respsContent.HasMember(ALIPAY_RESPS_TRADE_NO) ||
		!respsContent[ALIPAY_RESPS_TRADE_NO].IsString() ||
		!respsContent.HasMember(ALIPAY_RESPS_REFUND_FEE) ||
		!respsContent[ALIPAY_RESPS_REFUND_FEE].IsString())
	{
		throw CAlipayError(ALIPAY_RET_PARSE_ERROR, strReq, strResps);
	}

	alipayResps.strBuyerLogonId = respsContent[ALIPAY_RESPS_BUYER_LOGON_ID].GetString();
	alipayResps.strBuyerUserId = respsContent[ALIPAY_RESPS_BUYER_USER_ID].GetString();
	alipayResps.strFundChange = respsContent[ALIPAY_RESPS_FUND_CHANGE].GetString();
	alipayResps.strGmtRefundPay = respsContent[ALIPAY_RESPS_GMT_REFUND_PAY].GetString();
	alipayResps.strRefundFee = respsContent[ALIPAY_RESPS_REFUND_FEE].GetString();
	alipayResps.strOutTradeNo = respsContent[ALIPAY_RESPS_OUT_TRADE_NO].GetString();
	alipayResps.strTradeNo = respsContent[ALIPAY_RESPS_TRADE_NO].GetString();
}

void CAlipay::withdraw(
	int iAmount,
	const string& strTradingCode,
	const string& strAlipayAccount,
	const string& strTrueName,
	CAlipayResps& alipayResps,
	const string& strRemarks /*= string("")*/
)
{
	string strReq;
	appendTransferContent(strReq, iAmount, strAlipayAccount, strTrueName, strTradingCode, strRemarks);
	sendReqAndParseResps(strReq, ALIPAY_RESPS_TRSFR, bind(&CAlipay::parseTransferResps, this, placeholders::_1, placeholders::_2, placeholders::_3, &alipayResps));
}

void CAlipay::parseTransferResps(const string& strReq, const string& strResps, rapidjson::Value& respsContent, CAlipayResps* pAlipayResps)
{
	CAlipayResps& alipayResps = *pAlipayResps;
	if (!respsContent.HasMember(ALIPAY_RESPS_ORDER_ID) ||
		!respsContent[ALIPAY_RESPS_ORDER_ID].IsString() ||
		!respsContent.HasMember(ALIPAY_RESPS_OUT_BIZ_NO) ||
		!respsContent[ALIPAY_RESPS_OUT_BIZ_NO].IsString() ||
		!respsContent.HasMember(ALIPAY_RESPS_PAY_DATE) ||
		!respsContent[ALIPAY_RESPS_PAY_DATE].IsString())
	{
		throw CAlipayError(ALIPAY_RET_PARSE_ERROR, strReq, strResps);
	}

	alipayResps.strOrderId = respsContent[ALIPAY_RESPS_ORDER_ID].GetString();
	alipayResps.strPayDate = respsContent[ALIPAY_RESPS_PAY_DATE].GetString();
	alipayResps.strOutBizNo = respsContent[ALIPAY_RESPS_OUT_BIZ_NO].GetString();
}

void CAlipay::queryPayStatus(const string& strOutTradingCode, CAlipayResps& alipayResps)
{
	string strReq;
	appendQueryStatusContent(strReq, strOutTradingCode);
	sendReqAndParseResps(strReq, ALIPAY_RESPS_QUERY, bind(&CAlipay::parseQueryStatusResps, this, placeholders::_1, placeholders::_2, placeholders::_3, &alipayResps));
}

void CAlipay::parseQueryStatusResps(const string& strReq, const string& strResps, rapidjson::Value& respsContent, CAlipayResps* pAlipayResps)
{
	CAlipayResps& alipayResps = *pAlipayResps;
	if (!respsContent.HasMember(ALIPAY_RESPS_BUYER_LOGON_ID) ||
		!respsContent[ALIPAY_RESPS_BUYER_LOGON_ID].IsString() ||
		!respsContent.HasMember(ALIPAY_RESPS_BUYER_USER_ID) ||
		!respsContent[ALIPAY_RESPS_BUYER_USER_ID].IsString() ||
		!respsContent.HasMember(ALIPAY_RESPS_OUT_TRADE_NO) ||
		!respsContent[ALIPAY_RESPS_OUT_TRADE_NO].IsString() ||
		!respsContent.HasMember(ALIPAY_RESPS_TRADE_NO) ||
		!respsContent[ALIPAY_RESPS_TRADE_NO].IsString() ||
		!respsContent.HasMember(ALIPAY_RESPS_TRADE_STATUS) ||
		!respsContent[ALIPAY_RESPS_TRADE_STATUS].IsString() ||
		!respsContent.HasMember(ALIPAY_RESPS_TOTAL_AMOUNT) ||
		!respsContent[ALIPAY_RESPS_TOTAL_AMOUNT].IsString())
	{
		throw CAlipayError(ALIPAY_RET_PARSE_ERROR, strReq, strResps);
	}

	alipayResps.strBuyerLogonId = respsContent[ALIPAY_RESPS_BUYER_LOGON_ID].GetString();
	alipayResps.strBuyerUserId = respsContent[ALIPAY_RESPS_BUYER_USER_ID].GetString();
	alipayResps.strOutTradeNo = respsContent[ALIPAY_RESPS_OUT_TRADE_NO].GetString();
	alipayResps.strTradeNo = respsContent[ALIPAY_RESPS_TRADE_NO].GetString();
	alipayResps.strTotalAmount = respsContent[ALIPAY_RESPS_TOTAL_AMOUNT].GetString();

	string strTradeStatus = respsContent[ALIPAY_RESPS_TRADE_STATUS].GetString();
	if (strTradeStatus == ALIPAY_TRADE_STATUS_SUCCESS_STR)
		alipayResps.iTradeStatus = ALIPAY_TRADE_STATUS_SUCCESS;
	else if (strTradeStatus == ALIPAY_TRADE_STATUS_CLOSED_STR)
		alipayResps.iTradeStatus = ALIPAY_TRADE_STATUS_CLOSED;
	else if (strTradeStatus == ALIPAY_TRADE_STATUS_FINISHED_STR)
		alipayResps.iTradeStatus = ALIPAY_TRADE_STATUS_FINISHED;
	else if (strTradeStatus == ALIPAY_TRADE_STATUS_WAIT_BUYER_PAY_STR)
		alipayResps.iTradeStatus = ALIPAY_TRADE_STATUS_WAIT_BUYER_PAY;
	else
		alipayResps.iTradeStatus = ALIPAY_TRADE_STATUS_UNKONW;
}

void CAlipay::queryRefund(
	const std::string& strOutTradingCode, 
	const std::string& strRefundTradingCode, 
	CAlipayResps& alipayResps
)
{
	string strReq;
	appendQueryRefundContent(strReq, strOutTradingCode, strRefundTradingCode);
	sendReqAndParseResps(strReq, ALIPAY_RESPS_QUERY_REFUND, bind(&CAlipay::parseQueryRefundResps, this, placeholders::_1, placeholders::_2, placeholders::_3, &alipayResps));
}

void CAlipay::parseQueryRefundResps(const string& strReq, const string& strResps, rapidjson::Value& respsContent, CAlipayResps* pAlipayResps)
{
	CAlipayResps& alipayResps = *pAlipayResps;
	if (respsContent.HasMember(ALIPAY_RESPS_OUT_REQ_NO) &&
		respsContent[ALIPAY_RESPS_OUT_REQ_NO].IsString())
	{
		alipayResps.strOutRequestNo = respsContent[ALIPAY_RESPS_OUT_REQ_NO].GetString();
	}

	if (respsContent.HasMember(ALIPAY_RESPS_OUT_TRADE_NO) &&
		respsContent[ALIPAY_RESPS_OUT_TRADE_NO].IsString())
	{
		alipayResps.strOutTradeNo = respsContent[ALIPAY_RESPS_OUT_TRADE_NO].GetString();
	}

	if (respsContent.HasMember(ALIPAY_RESPS_TRADE_NO) &&
		respsContent[ALIPAY_RESPS_TRADE_NO].IsString())
	{
		alipayResps.strTradeNo = respsContent[ALIPAY_RESPS_TRADE_NO].GetString();
	}

	if (respsContent.HasMember(ALIPAY_RESPS_TOTAL_AMOUNT) &&
		respsContent[ALIPAY_RESPS_TOTAL_AMOUNT].IsString())
	{
		alipayResps.strTotalAmount = respsContent[ALIPAY_RESPS_TOTAL_AMOUNT].GetString();
	}

	if (respsContent.HasMember(ALIPAY_RESPS_REFUND_AMOUNT) &&
		respsContent[ALIPAY_RESPS_REFUND_AMOUNT].IsString())
	{
		alipayResps.strRefundAmount = respsContent[ALIPAY_RESPS_REFUND_AMOUNT].GetString();
	}
}



void CAlipay::appendContentAndSign(
	string& totalString,
	const string& biz_content,
	const string& strMethodName,
	const std::string& strCharset /*= "utf-8"*/,
	const string& strCallBack /*= ""*/
)
{
	string clearString;
	CUtils::AppendContent(ALIPAY_REQ_APP_ID, m_strAppId, totalString, clearString, false);
	CUtils::AppendContent(ALIPAY_REQ_BIZ_CONTENT, biz_content, totalString, clearString);
	CUtils::AppendContent(ALIPAY_REQ_CHARSET, strCharset, totalString, clearString);
	CUtils::AppendContent(ALIPAY_REQ_METHOD, strMethodName, totalString, clearString);
	if (!strCallBack.empty())
		CUtils::AppendContent(ALIPAY_REQ_NOTIFY_URL, strCallBack, totalString, clearString);
	CUtils::AppendContent(ALIPAY_REQ_SIGN_TYPE, "RSA2", totalString, clearString);
	CUtils::AppendContent(ALIPAY_REQ_TIMESTAMP, CUtils::getCurentTime(), totalString, clearString);
	CUtils::AppendContent(ALIPAY_REQ_VERSION, "1.0", totalString, clearString);
	string& signContent = CRSAUtils::rsa_sign_from_privKey_with_base64(clearString, m_strPrivKey);
	CUtils::AppendContent(ALIPAY_REQ_SIGN, signContent, totalString);
}

void CAlipay::appendPayContent(
	string& strContent,
	int iAmount,
	const string& strTradingCode,
	const string& strSubject,
	const string& strCallBack,
	const string& strTimeOut /*= string("30m")*/,
	const string& strPassBackParams /*= string("")*/
)
{
#ifdef CHECK_INPUT_STRING_TYPE
	const string& u8Subject = ch_trans::is_utf8(strSubject.c_str()) ? strSubject : ch_trans::ascii_to_utf8(strSubject);

	string u8PassBackParams("");
	if (!strPassBackParams.empty())
		u8PassBackParams = ch_trans::is_utf8(strPassBackParams.c_str()) ? strPassBackParams : ch_trans::ascii_to_utf8(strPassBackParams);
#else
	const string& u8Subject = strSubject;

	const string& u8PassBackParams = strPassBackParams;
#endif

	format f("{\"timeout_express\":\"%s\",\"product_code\":\"QUICK_MSECURITY_PAY\",\"total_amount\":\"%.2f\",\"subject\":\"%s\",\"out_trade_no\":\"%s\"%s}");
	f % strTimeOut.c_str() % (((float)iAmount) / 100.0) % u8Subject.c_str() % strTradingCode.c_str();
	if (!u8PassBackParams.empty())
	{
		format f2(",\"passback_params\":\"%s\"");
		f2 % u8PassBackParams.c_str();
		f % f2.str().c_str();
	}
	else
		f % "";
	string& biz_content = f.str();

	appendContentAndSign(strContent, biz_content, "alipay.trade.app.pay", "utf-8", strCallBack);
}

void CAlipay::appendTransferContent(
	std::string& strReq,
	int iAmount,
	const string& strAlipayAccount,
	const string& strTrueName,
	const string& strTradingCode,
	const string& strRemarks /*= string("")*/
)
{
#ifdef CHECK_INPUT_STRING_TYPE
	const string& asciiTrueName = ch_trans::is_utf8(strTrueName.c_str()) ? ch_trans::utf8_to_ascii(strTrueName) : strTrueName;

	const string& asciiRemarks = ch_trans::is_utf8(strRemarks.c_str()) ? ch_trans::utf8_to_ascii(strRemarks) : strRemarks;
#else
	const string& asciiTrueName = strTrueName;

	const string& asciiRemarks = strRemarks;
#endif

	format f("{\"out_biz_no\":\"%s\",\"payee_type\":\"ALIPAY_LOGONID\",\"payee_account\":\"%s\",\"amount\":\"%.2f\",\"payee_real_name\":\"%s\",\"remark\":\"%s\"}");
	f % strTradingCode.c_str() % strAlipayAccount.c_str() % (((float)iAmount) / 100.0) % asciiTrueName.c_str() % asciiRemarks.c_str();
	string& biz_content = f.str();

	appendContentAndSign(strReq, biz_content, "alipay.fund.trans.toaccount.transfer", "gb2312");
}

void CAlipay::appendRefundContent(
	std::string& strReq,
	int iAmount,
	const string& strTradingCode,
	const string& strOutTradingCode
)
{
	format f("{\"out_trade_no\":\"%s\",\"refund_amount\":%.2f,\"out_request_no\":\"%s\"}");
	f % strOutTradingCode.c_str() % (((float)iAmount) / 100.0) % strTradingCode.c_str();
	string& biz_content = f.str();

	appendContentAndSign(strReq, biz_content, "alipay.trade.refund");
}

void CAlipay::appendQueryStatusContent(string& strReq, const string& strOutTradingCode)
{
	format f("{\"out_trade_no\":\"%s\"}");
	f % strOutTradingCode.c_str();
	string& biz_content = f.str();

	appendContentAndSign(strReq, biz_content, "alipay.trade.query");
}

void CAlipay::appendQueryRefundContent(string& strReq, const string& strOutTradingCode, const string& strRefundTradingCode)
{
	format f("{\"out_trade_no\":\"%s\",\"out_request_no\":\"%s\"}");
	f % strOutTradingCode.c_str() % strRefundTradingCode.c_str();
	string& biz_content = f.str();

	appendContentAndSign(strReq, biz_content, "alipay.trade.fastpay.refund.query");
}