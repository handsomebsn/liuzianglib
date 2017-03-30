#pragma once
#ifndef liuzianglib_Encrypt
#define liuzianglib_Encrypt
#include <string>
#include <mutex>
#include "Crypto++\aes.h"
#include "Crypto++\filters.h"
#include "Crypto++\modes.h"
#include "Crypto++\randpool.h"
#include "Crypto++\rsa.h" 
#include "Crypto++\hex.h" 
#include "Crypto++\files.h" 
#include <sstream>
#include "md5.cpp"
//Version 2.4.2
//20170330

namespace DC {

	class AES {//AES �ڸ��߲�η�װ�� Crypto++ ���е� AES �ӽ����㷨����ʹ��������ʹ��
			   //�ӽ��ܲ���ǰ�����ڹ��캯���� InputPassword ��������Կ���ӽ��ܲ���ǰ�������Ƿ��Ѿ���������Կ�����û����Կ�ӽ��ܲ����᷵��һ����ֵ
			   //����Ҫ����ƫ����(IV)��InputPassword() ���Զ������һ��ƫ��������ƫ��������Կ�Ĳ�ͬ����ͬ
			   //�ӽ���һ�����ַ����᷵��һ����ֵ
			   //ʹ�ô����������н��ܻ᷵��һ����ֵ
			   //AES ���̰߳�ȫ��
	public:
		AES(const std::string password) {//�ڹ��캯���м������ƫ����
			AMD5 = new MD5;
			if (password.empty()) return;
			AMD5->reset();
			AMD5->update(password);
			std::string TEMP_IV = AMD5->toString();
			TEMP_IV.resize(16);
			InsertKey(password.c_str(), TEMP_IV.c_str());
		}

		AES() {
			AMD5 = new MD5;
		}

		~AES() {
			delete AMD5;
		}

	private:
		AES(const AES& inputAES) {}

		AES& operator=(const AES& inputAES) {}

	public:
		void InputKey(const std::string& password) {//������Կ
			if (password.empty()) return;
			AMD5->reset();
			AMD5->update(password);
			std::string TEMP_IV = AMD5->toString();
			TEMP_IV.resize(16);
			InsertKey(password.c_str(), TEMP_IV.c_str());
		}

	public:
		std::string encrypt(const std::string& plainText) {
			if (Password != 1) return "";
			if (plainText.empty()) return "";
			std::lock_guard<std::mutex> GMCR(MCR);
			std::string cipherText;
			CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
			CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv);
			CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(cipherText));
			stfEncryptor.Put(reinterpret_cast<const unsigned char*>(plainText.c_str()), plainText.length() + 1);
			stfEncryptor.MessageEnd();
			std::string cipherTextHex;
			for (int i = 0; i < cipherText.size(); i++)
			{
				char ch[3] = { 0 };
				sprintf(ch, "%02x", static_cast<byte>(cipherText[i]));
				cipherTextHex += ch;
			}

			return cipherTextHex;
		}

		std::string decrypt(const std::string& cipherTextHex) {
			if (Password != 1) return "";
			if (cipherTextHex.empty()) return "";
			std::lock_guard<std::mutex> GMCR(MCR);
			std::string cipherText;
			std::string decryptedText;
			int i = 0;
			try {
				while (true)
				{
					char c;
					int x;
					std::stringstream ss;
					ss << std::hex << cipherTextHex.substr(i, 2).c_str();
					ss >> x;
					c = (char)x;
					cipherText += c;
					if (i >= cipherTextHex.length() - 2)break;
					i += 2;
				}
			}
			catch (std::out_of_range) {
				return "";
			}
			CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
			CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);
			CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedText));
			stfDecryptor.Put(reinterpret_cast<const unsigned char*>(cipherText.c_str()), cipherText.size());
			try {
				stfDecryptor.MessageEnd();
				decryptedText.resize(decryptedText.size() - 1);
				return decryptedText;
			}
			catch (...) {
				return "";
			}
		}

	private:
		void InsertKey(const char *inputKey, const char *inputIV) {
			if (strlen(inputIV) != 16) return;
			std::lock_guard<std::mutex> GMCR(MCR);
			Password = 1;
			for (int j = 0; j < CryptoPP::AES::DEFAULT_KEYLENGTH; ++j)
			{
				key[j] = inputKey[j];
			}

			for (int i = 0; i < CryptoPP::AES::BLOCKSIZE; ++i)
			{
				iv[i] = inputIV[i];
			}
		}

	private:
		byte key[CryptoPP::AES::DEFAULT_KEYLENGTH], iv[CryptoPP::AES::BLOCKSIZE];
		std::mutex MCR;
		MD5 *AMD5;
		int32_t Password = 0;
	};

	class RSA {
		//���ĳ��ȹ������ܵ���δ������Ϊ�����飺1024λ��Կ�����86λ�����ǰ�ȫ�ģ����ĳ�����256��2048λ��Կ�����214λ�����ǰ�ȫ�ģ����ĳ�����512��4096λ��Կ�����470λ�����ǰ�ȫ�ģ����ĳ�����1024��8192λ��Կ�����982λ�����ǰ�ȫ�ģ����ĳ�����2048��
		//�ӽ���ʧ��(������������:�ӽ���һ�����ַ�����ʹ�ô������Կ���н���)�᷵��һ����ֵ
	public:
		void GenerateKey(int32_t KeyLength, std::string Seed, std::string &strPriv, std::string &strPub) {//����һ�� RSA ��Կ
			CryptoPP::RandomPool RandPool;
			RandPool.IncorporateEntropy((byte *)Seed.c_str(), Seed.size());

			//generate private key  
			CryptoPP::RSAES_OAEP_SHA_Decryptor Priv(RandPool, KeyLength);
			CryptoPP::HexEncoder PrivateEncoder(new CryptoPP::StringSink(strPriv));
			Priv.DEREncode(PrivateEncoder);
			PrivateEncoder.MessageEnd();

			//generate public key using private key  
			CryptoPP::RSAES_OAEP_SHA_Encryptor Pub(Priv);
			CryptoPP::HexEncoder PublicEncoder(new CryptoPP::StringSink(strPub));
			Pub.DEREncode(PublicEncoder);
			PublicEncoder.MessageEnd();
		}

		std::string encrypt(const std::string &publicKEY, const std::string& seed, const std::string &message) {
			std::string TEMP_result;
			try {
				TEMP_result = RSAEncryptString(publicKEY.c_str(), seed.c_str(), message.c_str());
			}
			catch (...) {
				return std::string();
			}
			return TEMP_result;
		}

		std::string decrypt(const std::string &privateKEY, const std::string &message) {
			std::string TEMP_result;
			try {
				TEMP_result = RSADecryptString(privateKEY.c_str(), message.c_str());
			}
			catch (...) {
				return std::string();
			}
			return TEMP_result;
		}

		std::string Sign(const std::string &privateKEY, const std::string &message) {
			if (privateKEY.size() % 2 != 0) return "";//��Ҫɾ����
			std::string result;
			try {
				RSASignFile(privateKEY.c_str(), message.c_str(), result);
			}
			catch (...) {
				return std::string();
			}
			return result;
		}

		int32_t Ver(const std::string &publicKEY, const std::string &message, const std::string &Verstr) {
			if (publicKEY.size() % 2 != 0) return -3;//��Ҫɾ����
			int32_t returnvalue = 0;
			try {
				returnvalue = RSAVerifyFile(publicKEY.c_str(), message.c_str(), Verstr.c_str());
			}
			catch (...) {
				return -2;
			}
			return returnvalue;
		}

	private:
		std::string RSAEncryptString(const char *pubkey, const char *seed, const char *message)
		{
			CryptoPP::StringSource pubstr(pubkey, true, new CryptoPP::HexDecoder);
			CryptoPP::RSAES_OAEP_SHA_Encryptor pub(pubstr);

			CryptoPP::RandomPool randPool;
			randPool.Put((byte *)seed, strlen(seed));

			std::string result;
			CryptoPP::StringSource(message, true, new CryptoPP::PK_EncryptorFilter(randPool, pub, new CryptoPP::HexEncoder(new CryptoPP::StringSink(result))));
			return result;
		}

		std::string RSADecryptString(const char *privkey, const char *ciphertext)
		{
			CryptoPP::StringSource privstr(privkey, true, new CryptoPP::HexDecoder);

			CryptoPP::RSAES_OAEP_SHA_Decryptor priv(privstr);

			std::string result;
			CryptoPP::StringSource(ciphertext, true, new CryptoPP::HexDecoder(new CryptoPP::PK_DecryptorFilter(GlobalRNG(), priv, new CryptoPP::StringSink(result))));
			return result;
		}

		CryptoPP::RandomPool & GlobalRNG()
		{
			static CryptoPP::RandomPool randomPool;
			return randomPool;
		}

		//������������������Կλ�����Ե�ʱ���abort()
		void RSASignFile(const char *privateKey, const char *message, std::string &signature)
		{
			CryptoPP::StringSource privKey(privateKey, true, new CryptoPP::HexDecoder);
			CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA>::Signer priv(privKey);
			CryptoPP::StringSource f(message, true, new CryptoPP::SignerFilter(GlobalRNG(), priv, new CryptoPP::HexEncoder(new CryptoPP::StringSink(signature))));
		}

		bool RSAVerifyFile(const char *publicKEY, const char *messageStr, const char *signatureStr)
		{
			CryptoPP::StringSource pubFile(publicKEY, true, new CryptoPP::HexDecoder);
			CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA>::Verifier pub(pubFile);

			CryptoPP::StringSource signatureFile(signatureStr, true, new CryptoPP::HexDecoder);
			if (signatureFile.MaxRetrievable() != pub.SignatureLength())
				return false;
			CryptoPP::SecByteBlock signature(pub.SignatureLength());
			signatureFile.Get(signature, signature.size());

			CryptoPP::VerifierFilter *verifierFilter = new CryptoPP::VerifierFilter(pub);
			verifierFilter->Put(signature, pub.SignatureLength());
			CryptoPP::StringSource f(messageStr, true, verifierFilter);

			return verifierFilter->GetLastResult();
		}
	};

}

#endif