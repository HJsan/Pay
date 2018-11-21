#pragma once
#include <string>

namespace SAPay {

template<typename T>
class CPayError
{
public:
	CPayError(T ret) :m_ret(ret), m_iLastNetWorkCode(0) {}

	CPayError(T ret, const std::string& strLastReq, const std::string& strLastResps) :
		m_ret(ret),
		m_iLastNetWorkCode(0),
		m_strLastReq(strLastReq),
		m_strLastResps(strLastResps) {}

	CPayError(T ret, const std::string& strLastReq, const std::string& strLastResps, int iNetWorkCode) :
		m_ret(ret),
		m_iLastNetWorkCode(iNetWorkCode),
		m_strLastReq(strLastReq),
		m_strLastResps(strLastResps) {}

	CPayError(T ret, const std::string& strLastReq, const std::string& strLastResps, const std::string& strErrorInfo) :
		m_ret(ret),
		m_iLastNetWorkCode(0),
		m_strLastReq(strLastReq),
		m_strLastResps(strLastResps),
		m_strErrInfo(strErrorInfo) {}

	T getErrorCode() const { return m_ret; }
	int getNetWorkCode() const { return m_iLastNetWorkCode; }
	const std::string& getLastReq() const { return m_strLastReq; }
	const std::string& getLastResps() const { return m_strLastResps; }
	const std::string& getErrInfo() const { return m_strErrInfo; }

private:
	T m_ret;
	int m_iLastNetWorkCode;
	std::string m_strLastReq;
	std::string m_strLastResps;
	std::string m_strErrInfo;
};

}