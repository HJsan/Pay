#pragma once
#include <string>
#include <openssl/pem.h>
#include <openssl/rsa.h>

namespace SAPay {

class CRSAUtils
{
public:
	//��ǩ
	static bool rsa_verify_from_pubKey_with_base64(const std::string &content, const std::string &sign, const std::string &key);

	//��ǩ
	static std::string rsa_sign_from_privKey_with_base64(const std::string& content, const std::string& key);

	//��Կ���ܡ�
	static std::string rsa_encrypt_from_pubKey(const std::string& plainText, const std::string& pubKeyBuffer);

	//˽Կ���ܡ�
	static std::string rsa_decrypt_from_privKey(const std::string& encryptedText, const std::string& privKeyBuffer);

	//˽Կ���ܡ�
	static std::string rsa_encrypt_from_privKey(const std::string& plainText, const std::string& privKeyBuffer);

	//��Կ���ܡ�
	static std::string rsa_decrypt_from_pubKey(const std::string& encryptedText, const std::string& pubKeyBuffer);


private:
	//��ȡ��Կ/˽Կ��
	static RSA* rsa_key_from_buffer(const std::string& keyBuff, bool isPublic);

	//��Կ/˽Կ���ܡ�
	static std::string rsa_encrypt(RSA* rsaPubKey, RSA* rsaPrivKey, const std::string& plainText);

	//˽Կ/��Կ���ܡ�
	static std::string rsa_decrypt(RSA* rsaPubKey, RSA* rsaPrivKey, const std::string& encryptedText);

	//base64����
	static std::string base64Encode(const unsigned char *bytes, int len);

	//base64����
	static bool base64Decode(const std::string &str, unsigned char *bytes, int &len);
};

}


