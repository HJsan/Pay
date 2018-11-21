#include "WeChat.h"
#include "PayHeader.h"
#include "Tinyxml/tinyxml.h"
#include "rapidjson/document.h"
#include "Utils/Utils.h"
#include "Utils/Md5Utils.h"
#include "Utils/HttpClient.h"
#include <boost/format.hpp>

using namespace std;
using namespace boost;
using namespace SAPay;

void CWeChat::parseWechatRespsAndNotify(
	const string& strNotify,
	map<string, string>& mapNameValue
)
{
	TiXmlDocument document;
	document.Parse(strNotify.c_str());
	TiXmlNode* pRoot = document.FirstChild();
	if (pRoot)
	{
		for (TiXmlNode* pChild = pRoot->FirstChild(); pChild != nullptr; pChild = pChild->NextSibling())
		{
			TiXmlNode* pTextChild = pChild->FirstChild();
			if (pChild->Type() == TiXmlNode::TINYXML_ELEMENT &&
				pTextChild &&
				pTextChild->Type() == TiXmlNode::TINYXML_TEXT)
			{
				mapNameValue[pChild->Value()] = pTextChild->ToText()->Value();
			}
		}
	}
}

int CWeChat::verifyWechatRespsAndNotify(
	const map<string, string>& mapResps,
	const string& strMchKey
)
{
	string content(""), sign("");
	vector<string>& vecDictionary = CUtils::createDictionaryWithMap(mapResps);
	for (auto itr = vecDictionary.cbegin(); itr != vecDictionary.cend(); ++itr)
	{
		auto itrr = mapResps.find(*itr);
		if (itrr != mapResps.end())
		{
			if (*itr == WECHAT_RESPS_SIGN)
			{
				sign = itrr->second;
				continue;
			}

			if (!itrr->second.empty())
				CUtils::AppendContentWithoutUrlEncode(*itr, itrr->second, content, !content.empty());
		}
	}
	CUtils::AppendContentWithoutUrlEncode("key", strMchKey, content);
	Md5Utils md5;
	string signResult;
	md5.encStr32(content.c_str(), signResult);
	return signResult == sign ? 1 : -1;
}

static void addXmlChild(TiXmlElement* pRoot, const string& key, const string& value)
{
	TiXmlText* pValue = new TiXmlText(value.c_str());
	pValue->SetCDATA(true);
	TiXmlElement* pKey = new TiXmlElement(key.c_str());
	pKey->LinkEndChild(pValue);
	pRoot->LinkEndChild(pKey);
}

CWeChat::CWeChat(
	const string& strAppId,
	const string& strMchId,
	const string& strMchKey,
	const string& strAppSecret /*= string("")*/,
	const string& strCertPath /*= string("")*/,
	const string& strKeyPath /*= string("")*/
) :
	m_bIsApp(true),
	m_strAppId(strAppId),
	m_strMchId(strMchId),
	m_strMchKey(strMchKey),
	m_strAppSecret(strAppSecret),
	m_strCertPath(strCertPath),
	m_strKeyPath(strKeyPath)
{
}

bool CWeChat::sendReqAndParseResps(
	const string& strReq,
	const string& strHref,
	function<CWechatRet(map<string, string>&)> func,
	bool bPostWithCert /*= false*/
)
{
	string strResps("");
	int iNetWorkRet = 0;
	if (bPostWithCert)
	{
		if (m_strCertPath.empty() || m_strKeyPath.empty())
		{
			throw CWechatError(WECHAT_RET_MISSING_CERT_INFO);
		}
		iNetWorkRet = CHttpClient::postWithCert(strHref, strReq, m_strCertPath, m_strKeyPath, strResps);
	}
	else
	{
		iNetWorkRet = CHttpClient::post(strHref, strReq, strResps);;
	}

	if (iNetWorkRet)
	{
		throw CWechatError(WECHAT_RET_NETWORK_ERROR, strReq, strResps, iNetWorkRet);
	}

	map<string, string> mapResps;
	parseWechatRespsAndNotify(strResps, mapResps);
	auto itrReturnCode = mapResps.find(WECHAT_RESPS_RETURN_CODE);
	auto itrResultCode = mapResps.find(WECHAT_RESPS_RESULT_CODE);
	if (itrReturnCode == mapResps.end() ||
		itrResultCode == mapResps.end() ||
		itrReturnCode->second != "SUCCESS" ||
		itrResultCode->second != "SUCCESS")
	{
		auto itrErrCode = mapResps.find(WECHAT_RESPS_ERR_CODE);
		auto itrReturnMsg = mapResps.find(WECHAT_RESPS_RETURN_MSG);
		if (itrErrCode != mapResps.end())
		{
			throw CWechatError(WECHAT_RET_ERR_CODE_ERROR, strReq, strResps, itrErrCode->second);
		}
		else if (itrReturnMsg != mapResps.end())
		{
			throw CWechatError(WECHAT_RET_RET_MSG_ERROR, strReq, strResps, itrReturnMsg->second);
		}
		else
		{
			throw CWechatError(WECHAT_RET_UNKNOW_ERROR, strReq, strResps);
		}
	}

	if (verifyWechatRespsAndNotify(mapResps, m_strMchKey) < 0)
	{
		throw CWechatError(WECHAT_RET_VERIFY_ERROR, strReq, strResps);
	}

	CWechatRet iRet = func(mapResps);
	if (iRet != WECHAT_RET_OK)
	{
		throw CWechatError(iRet, strReq, strResps);
	}
	return true;
}

bool CWeChat::queryPayStatus(
	const string& strOutTradingCode, 
	CWeChatResps& wechatResps
)
{
	return sendReqAndParseResps(
		appendQueryStatusContent(strOutTradingCode),
		WECHAT_HREF_QUERY,
		[&wechatResps](map<string, string>& mapResps) -> CWechatRet
	{
		auto itrOpenId = mapResps.find(WECHAT_RESPS_OPEN_ID);
		auto itrTradeType = mapResps.find(WECHAT_RESPS_TRADE_TYPE);
		auto itrTradeState = mapResps.find(WECHAT_RESPS_TRADE_STATE);
		auto itrBankType = mapResps.find(WECHAT_RESPS_BANK_TYPE);
		auto itrTotalFee = mapResps.find(WECHAT_RESPS_TOTAL_FEE);
		auto itrCashFee = mapResps.find(WECHAT_RESPS_CASH_FEE);
		auto itrTransactionId = mapResps.find(WECHAT_RESPS_TRANSACTION_ID);
		auto itrOutTradeNo = mapResps.find(WECHAT_RESPS_OUT_TRADE_NO);
		auto itrTimeEnd = mapResps.find(WECHAT_RESPS_TIME_END);
		auto itrTradeStateDesc = mapResps.find(WECHAT_RESPS_TRADE_STATE_DESC);
		if (itrOpenId == mapResps.end() ||
			itrTradeType == mapResps.end() ||
			itrTradeState == mapResps.end() ||
			itrBankType == mapResps.end() ||
			itrTotalFee == mapResps.end() ||
			itrCashFee == mapResps.end() ||
			itrTransactionId == mapResps.end() ||
			itrOutTradeNo == mapResps.end() ||
			itrTimeEnd == mapResps.end() ||
			itrTradeStateDesc == mapResps.end())
		{
			return WECHAT_RET_PARSE_ERROR;
		}

		wechatResps.strOpenId = itrOpenId->second;
		wechatResps.strTradeType = itrTradeType->second;
		wechatResps.strBankType = itrBankType->second;
		wechatResps.strTotalFee = itrTotalFee->second;
		wechatResps.strCashFee = itrCashFee->second;
		wechatResps.strTransactionId = itrTransactionId->second;
		wechatResps.strOutTradeNo = itrOutTradeNo->second;
		wechatResps.strTimeEnd = itrTimeEnd->second;
		wechatResps.strTradeStateDesc = itrTradeStateDesc->second;

		if (itrTradeState->second == WECHAT_TRADE_STATE_SUCCESS_STR)
			wechatResps.iTradeState = WECHAT_TRADE_STATE_SUCCESS;
		else if(itrTradeState->second == WECHAT_TRADE_STATE_REFUND_STR)
			wechatResps.iTradeState = WECHAT_TRADE_STATE_REFUND;
		else if (itrTradeState->second == WECHAT_TRADE_STATE_NOTPAY_STR)
			wechatResps.iTradeState = WECHAT_TRADE_STATE_NOTPAY;
		else if (itrTradeState->second == WECHAT_TRADE_STATE_CLOSED_STR)
			wechatResps.iTradeState = WECHAT_TRADE_STATE_CLOSED;
		else if (itrTradeState->second == WECHAT_TRADE_STATE_REVOKED_STR)
			wechatResps.iTradeState = WECHAT_TRADE_STATE_REVOKED;
		else if (itrTradeState->second == WECHAT_TRADE_STATE_USERPAYING_STR)
			wechatResps.iTradeState = WECHAT_TRADE_STATE_USERPAYING;
		else if (itrTradeState->second == WECHAT_TRADE_STATE_PAYERROR_STR)
			wechatResps.iTradeState = WECHAT_TRADE_STATE_PAYERROR;
		else
			wechatResps.iTradeState = WECHAT_TRADE_STATE_UNKNOW;
		return WECHAT_RET_OK;
	}
	);
}

bool CWeChat::refund(
	int iTotalAmount,
	int iRefundAmount,
	const std::string& strOutTradeNo,
	const std::string& strOutRefundNo,
	CWeChatResps& wechatResps,
	const std::string& strRemarks /*= ""*/,
	const std::string& strCallBackAddr /*= ""*/
)
{
	return sendReqAndParseResps(
		appendRefundContent(iTotalAmount, iRefundAmount, strOutTradeNo, strOutRefundNo, strRemarks, strCallBackAddr),
		WECHAT_HREF_REFUND,
		[&wechatResps](map<string, string>& mapResps) -> CWechatRet
	{
		auto itrRefundId = mapResps.find(WECHAT_RESPS_REFUND_ID);
		auto itrRefundFee = mapResps.find(WECHAT_RESPS_REFUND_FEE);
		if (itrRefundId == mapResps.end() || itrRefundFee == mapResps.end())
		{
			return WECHAT_RET_PARSE_ERROR;
		}

		wechatResps.strRefundFee = itrRefundFee->second;
		wechatResps.strRefundId = itrRefundId->second;
		return WECHAT_RET_OK;
	},
		true
	);
}

bool CWeChat::smallProgramLogin(
	const string& strJsCode,
	CWeChatResps& wechatResps
)
{
	if (m_strAppSecret.empty())
	{
		throw CWechatError(WECHAT_RET_MISSING_APP_SECRET);
	}

	string& strReq = appendSmallProgramLoginContent(strJsCode);
	string strResps("");
	int iNetWorkRet = CHttpClient::get(WECHAT_HREF_SMALL_PROGRAM_LOGIN + strReq, strResps);
	if (iNetWorkRet)
	{
		throw CWechatError(WECHAT_RET_NETWORK_ERROR, strReq, strResps, iNetWorkRet);
	}

	rapidjson::Document respsDocument;
	respsDocument.Parse(strResps.c_str(), strResps.length());
	if (!respsDocument.IsObject() ||
		!respsDocument.HasMember(WECHAT_RESPS_SESSION_KEY) ||
		!respsDocument.HasMember(WECHAT_RESPS_OPEN_ID) ||
		!respsDocument[WECHAT_RESPS_SESSION_KEY].IsString() ||
		!respsDocument[WECHAT_RESPS_OPEN_ID].IsString())
	{
		throw CWechatError(WECHAT_RET_PARSE_ERROR, strReq, strResps);
	}

	wechatResps.strSessionKey = respsDocument[WECHAT_RESPS_SESSION_KEY].GetString();
	wechatResps.strOpenId = respsDocument[WECHAT_RESPS_OPEN_ID].GetString();
	return true;
}

bool CWeChat::prepay(
	int iAmount,
	long long llValidTime,
	const string& strTradingCode,
	const string& strRemoteIP,
	const string& strBody,
	const string& strCallBackAddr,
	CWeChatResps& wechatResps,
	const string& strAttach /*= string("")*/,
	const string& strOpenId /*= string("")*/
)
{
	return sendReqAndParseResps(
		appendPrepayContent(iAmount, llValidTime, strTradingCode, strRemoteIP, 
			strBody, strCallBackAddr, strAttach, strOpenId),
		WECHAT_HREF_PREPAY,
		[&wechatResps](map<string, string>& mapResps) -> CWechatRet
	{
		auto itrTradeType = mapResps.find(WECHAT_RESPS_TRADE_TYPE);
		auto itrPrepayId = mapResps.find(WECHAT_RESPS_PREPAY_ID);
		if (itrTradeType == mapResps.end() ||
			itrPrepayId == mapResps.end())
		{
			return WECHAT_RET_PARSE_ERROR;
		}

		wechatResps.strTradeType = itrTradeType->second;
		wechatResps.strPrepayId = itrPrepayId->second;
		return WECHAT_RET_OK;
	}
	);
}

bool CWeChat::prepayWithSign(
	int iAmount,
	long long llValidTime,
	const string& strTradingCode,
	const string& strRemoteIP,
	const string& strBody,
	const string& strCallBackAddr,
	CWeChatResps& wechatResps,
	const string& strAttach /*= string("")*/,
	const string& strOpenId /*= string("")*/
)
{
	bool bRet = prepay(iAmount, llValidTime, strTradingCode, strRemoteIP,
		strBody, strCallBackAddr, wechatResps, strAttach, strOpenId);
	if (bRet)
	{
		string& strNonceStr = CUtils::generate_unique_string(32);
		string& strTimeStamp = CUtils::getCurentTimeStampStr();
		string& strSignResult = signPrepay(strNonceStr, strTimeStamp, wechatResps.strPrepayId);
		m_bIsApp ?
			appendAppPrepayInfo(strNonceStr, strTimeStamp, wechatResps.strPrepayId, strSignResult, wechatResps.strPrepaySignedContent) :
			appendSmallProgramPrepayInfo(strNonceStr, strTimeStamp, wechatResps.strPrepayId, strSignResult, wechatResps.strPrepaySignedContent);
	}
	return bRet;
}

string CWeChat::signPrepay(
	const string& strNonceStr,
	const string& strTimeStamp,
	const string& strPrepayId
)
{
	string signContent("");
	if (m_bIsApp)
	{
		CUtils::AppendContentWithoutUrlEncode("appid", m_strAppId, signContent, false);
		CUtils::AppendContentWithoutUrlEncode("noncestr", strNonceStr, signContent);
		CUtils::AppendContentWithoutUrlEncode("package", "Sign=WXPay", signContent);
		CUtils::AppendContentWithoutUrlEncode("partnerid", m_strMchId, signContent);
		CUtils::AppendContentWithoutUrlEncode("prepayid", strPrepayId, signContent);
		CUtils::AppendContentWithoutUrlEncode("timestamp", strTimeStamp, signContent);
		CUtils::AppendContentWithoutUrlEncode("key", m_strMchKey, signContent);
	}
	else
	{
		string packageContent = "prepay_id=" + strPrepayId;
		CUtils::AppendContentWithoutUrlEncode("appId", m_strAppId, signContent, false);
		CUtils::AppendContentWithoutUrlEncode("nonceStr", strNonceStr, signContent);
		CUtils::AppendContentWithoutUrlEncode("package", packageContent, signContent);
		CUtils::AppendContentWithoutUrlEncode("signType", "MD5", signContent);
		CUtils::AppendContentWithoutUrlEncode("timeStamp", strTimeStamp, signContent);
		CUtils::AppendContentWithoutUrlEncode("key", m_strMchKey, signContent);
	}
	string signResult("");
	Md5Utils m5;
	m5.encStr32(signContent.c_str(), signResult);
	return signResult;
}

void CWeChat::appendAppPrepayInfo(
	const string& strNonceStr,
	const string& strTimeStamp,
	const string& strPrepayId,
	const string& strSignResult,
	string& strPrepaySignedContent
)
{
	format f("{\"timeStamp\":\"%s\",\"nonceStr\":\"%s\",\"package\":\"Sign=WXPay\",\"paySign\":\"%s\","\
		"\"prepayid\":\"%s\",\"partnerid\":\"%s\",\"appid\":\"%s\"}");
	f % strTimeStamp.c_str() % strNonceStr.c_str() % strSignResult.c_str() 
		% strPrepayId.c_str() % m_strMchId.c_str() % m_strAppId.c_str();
	strPrepaySignedContent = f.str();
};

void CWeChat::appendSmallProgramPrepayInfo(
	const string& strNonceStr,
	const string& strTimeStamp,
	const string& strPrepayId,
	const string& strSignResult,
	string& strPrepaySignedContent
)
{
	format f("{\"timeStamp\":\"%s\",\"nonceStr\":\"%s\",\"package\":\"prepay_id=%s\",\"signType\":\"MD5\",\"paySign\":\"%s\"}");
	f % strTimeStamp.c_str() % strNonceStr.c_str() % strPrepayId.c_str() % strSignResult.c_str();
	strPrepaySignedContent = f.str();
};

string CWeChat::appendSmallProgramLoginContent(const string& strJsCode)
{
	string strTotalContent("");
	CUtils::AppendContentWithUrlEncode(WECHAT_REQ_APP_ID, m_strAppId, strTotalContent, false);
	CUtils::AppendContentWithUrlEncode(WECHAT_REQ_SECRET, m_strAppSecret, strTotalContent);
	CUtils::AppendContentWithUrlEncode(WECHAT_REQ_JS_CODE, strJsCode, strTotalContent);
	CUtils::AppendContentWithUrlEncode(WECHAT_REQ_GRANT_TYPE, "authorization_code", strTotalContent);
	return strTotalContent;
}

string CWeChat::appendQueryStatusContent(const string& strOutTradingCode)
{
	string& strNonceStr = CUtils::generate_unique_string(32);
	string signContent("");
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_APP_ID, m_strAppId, signContent, false);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_MCH_ID, m_strMchId, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_NONCE_STR, strNonceStr, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_OUT_TRADE_NO, strOutTradingCode, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_MCH_KEY, m_strMchKey, signContent);
	string strSignResult("");
	Md5Utils m5;
	m5.encStr32(signContent.c_str(), strSignResult);

	TiXmlElement* root = new TiXmlElement(WECHAT_XML_ROOT);
	addXmlChild(root, WECHAT_REQ_APP_ID, m_strAppId);
	addXmlChild(root, WECHAT_REQ_MCH_ID, m_strMchId);
	addXmlChild(root, WECHAT_REQ_NONCE_STR, strNonceStr);
	addXmlChild(root, WECHAT_REQ_OUT_TRADE_NO, strOutTradingCode);
	addXmlChild(root, WECHAT_REQ_SIGN, strSignResult);

	TiXmlDocument document;
	document.LinkEndChild(root);
	TiXmlPrinter printer;
	document.Accept(&printer);
	return printer.CStr();
}

string CWeChat::appendPrepayContent(
	int iAmount,
	long long llValidTime,
	const string& strTradingCode,
	const string& strRemoteIP,
	const string& strBody,
	const string& strCallBackAddr,
	const string& strAttach, /*= ""*/
	const string& strOpenId /*= ""*/
)
{
	string& strNonceStr = CUtils::generate_unique_string(32);
	string& strTimeExpire = CUtils::getDelayTime(llValidTime, false);
#ifdef CHECK_INPUT_STRING_TYPE
	const string& u8Body = ch_trans::is_utf8(strBody.c_str()) ? strBody : ch_trans::ascii_to_utf8(strBody);

	string u8Attach("");
	if (!strAttach.empty())
		u8Attach = ch_trans::is_utf8(strAttach.c_str()) ? strAttach : ch_trans::ascii_to_utf8(strAttach);
#else
	const string& u8Body = strBody;

	const string& u8Attach = strAttach;
#endif

	const string& strTradeType = m_bIsApp ? "APP" : "JSAPI";

	string signContent("");
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_APP_ID, m_strAppId, signContent, false);
	//add attach id if exist
	if (!u8Attach.empty())
		CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_ATTACH, u8Attach, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_BODY, u8Body, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_MCH_ID, m_strMchId, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_NONCE_STR, strNonceStr, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_NOTIFY_URL, strCallBackAddr, signContent);
	//add open id if exist
	if (!strOpenId.empty())
		CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_OPEN_ID, strOpenId, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_OUT_TRADE_NO, strTradingCode, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_SPBILL_CREATE_IP, strRemoteIP, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_TIME_EXPIRE, strTimeExpire, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_TOTAL_FEE, CUtils::i2str(iAmount), signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_TRADE_TYPE, strTradeType, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_MCH_KEY, m_strMchKey, signContent);
	string signResult("");
	Md5Utils m5;
	m5.encStr32(signContent.c_str(), signResult);

	TiXmlElement* root = new TiXmlElement(WECHAT_XML_ROOT);
	addXmlChild(root, WECHAT_REQ_APP_ID, m_strAppId);
	addXmlChild(root, WECHAT_REQ_MCH_ID, m_strMchId);
	addXmlChild(root, WECHAT_REQ_NONCE_STR, strNonceStr);
	addXmlChild(root, WECHAT_REQ_BODY, u8Body);
	addXmlChild(root, WECHAT_REQ_OUT_TRADE_NO, strTradingCode);
	addXmlChild(root, WECHAT_REQ_TOTAL_FEE, CUtils::i2str(iAmount));
	addXmlChild(root, WECHAT_REQ_SPBILL_CREATE_IP, strRemoteIP);
	addXmlChild(root, WECHAT_REQ_TIME_EXPIRE, strTimeExpire);
	addXmlChild(root, WECHAT_REQ_NOTIFY_URL, strCallBackAddr);
	addXmlChild(root, WECHAT_REQ_TRADE_TYPE, strTradeType);
	addXmlChild(root, WECHAT_REQ_SIGN, signResult);
	//add attach id if exist
	if (!u8Attach.empty())
		addXmlChild(root, WECHAT_REQ_ATTACH, u8Attach);
	//add open id if exist
	if (!strOpenId.empty())
		addXmlChild(root, WECHAT_REQ_OPEN_ID, strOpenId);

	TiXmlDocument document;
	document.LinkEndChild(root);
	TiXmlPrinter printer;
	document.Accept(&printer);
	return printer.CStr();
}

std::string CWeChat::appendRefundContent(
	int iTotalAmount,
	int iRefundAmount,
	const std::string& strOutTradeNo,
	const std::string& strOutRefundNo,
	const std::string& strRemarks /*= ""*/,
	const std::string& strCallBackAddr /*= ""*/
)
{
	string& strNonceStr = CUtils::generate_unique_string(32);
#ifdef CHECK_INPUT_STRING_TYPE
	const string& u8Remarks = ch_trans::is_utf8(strRemarks.c_str()) ? strRemarks : ch_trans::ascii_to_utf8(strRemarks);
#else
	const string& u8Remarks = strRemarks;
#endif

	string signContent("");
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_APP_ID, m_strAppId, signContent, false);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_MCH_ID, m_strMchId, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_NONCE_STR, strNonceStr, signContent);
	if (!strCallBackAddr.empty())
		CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_NOTIFY_URL, strCallBackAddr, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_OUT_REFUND_NO, strOutRefundNo, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_OUT_TRADE_NO, strOutTradeNo, signContent);
	if (!u8Remarks.empty())
		CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_REFUND_DESC, u8Remarks, signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_REFUND_FEE, CUtils::i2str(iRefundAmount), signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_TOTAL_FEE, CUtils::i2str(iTotalAmount), signContent);
	CUtils::AppendContentWithoutUrlEncode(WECHAT_REQ_MCH_KEY, m_strMchKey, signContent);
	string signResult("");
	Md5Utils m5;
	m5.encStr32(signContent.c_str(), signResult);

	TiXmlElement* root = new TiXmlElement(WECHAT_XML_ROOT);
	addXmlChild(root, WECHAT_REQ_APP_ID, m_strAppId);
	addXmlChild(root, WECHAT_REQ_MCH_ID, m_strMchId);
	addXmlChild(root, WECHAT_REQ_NONCE_STR, strNonceStr);
	if (!strCallBackAddr.empty())
		addXmlChild(root, WECHAT_REQ_NOTIFY_URL, strCallBackAddr);
	addXmlChild(root, WECHAT_REQ_OUT_TRADE_NO, strOutTradeNo);
	addXmlChild(root, WECHAT_REQ_OUT_REFUND_NO, strOutRefundNo);
	addXmlChild(root, WECHAT_REQ_TOTAL_FEE, CUtils::i2str(iTotalAmount));
	addXmlChild(root, WECHAT_REQ_REFUND_FEE, CUtils::i2str(iRefundAmount));
	if (!u8Remarks.empty())
		addXmlChild(root, WECHAT_REQ_REFUND_DESC, u8Remarks);
	addXmlChild(root, WECHAT_REQ_SIGN, signResult);

	TiXmlDocument document;
	document.LinkEndChild(root);
	TiXmlPrinter printer;
	document.Accept(&printer);
	return printer.CStr();
}