#pragma once
#include <map>
#include <vector>
#include <string>

#define HTTPCLIENT_DEFAULT_TOME_OUT 30

namespace SAPay{

class CHttpClient
{
public:
	static std::vector<std::string> makeHeaderWithMap(const std::map<std::string, std::string>& mapHeader);


	static int get(
		const std::string &strHref,
		std::string& strRespsContent,
		int iTimeOut = HTTPCLIENT_DEFAULT_TOME_OUT
	);


	static int post(
		const std::string& strHref,
		const std::string& strData,
		std::string& strRespsContent,
		std::string& strRespsHeader = std::string(""),
		int iTimeOut = HTTPCLIENT_DEFAULT_TOME_OUT,
		const std::vector<std::string>& vecHeader = std::vector<std::string>()
	);


	static int postWithCert(
		const std::string& strHref,
		const std::string& strData,
		const std::string& strCertPath,
		const std::string& strKeyPath,
		std::string& strRespsContent,
		std::string& strRespsHeader = std::string(""),
		int iTimeOut = HTTPCLIENT_DEFAULT_TOME_OUT,
		const std::vector<std::string>& vecHeader = std::vector<std::string>()
	);
};

}
