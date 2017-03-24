#pragma once
#ifndef liuzianglib_json
#define liuzianglib_json
#include <tuple>
#include <vector>
#include <limits>
#include <type_traits>
#include "DC_STR.h"
//Version 2.4.1V12
//20170325

namespace DC {

	namespace json {

		namespace jsonSpace {

			typedef std::tuple<std::size_t, std::size_t> PosPair;
			typedef std::vector<std::tuple<jsonSpace::PosPair, jsonSpace::PosPair>> ObjTable;

			inline bool SybolValid(const std::vector<std::size_t>& AllStartSymbol, const std::vector<std::size_t>& EndStartSymbol) {//�жϿ�ʼ���źͽ������������Ƿ�һ��
				return AllStartSymbol.size() == EndStartSymbol.size();
			}

			std::vector<PosPair> getSymbolPair(const std::string& str, const char* const StartSymbol, const char* const EndSymbol) {//�ҳ���Եķ��ţ�������Խ���((2+2)*(1+1)*6)���ֶ�����
																																	//����ֵ��std::vector<PosPair>��ÿһ��PosPair����һ�Է��ŵ�λ�ã�PosPair<0>�ǿ�ʼ���ŵ�λ�ã�PosPair<1>�ǽ������ŵ�λ��
				                                                                                                                    //��֧��StartSymbol==EndSymbol�����
				std::vector<std::size_t> AllStartSymbol = DC::STR::find(str, StartSymbol).getplace_ref(), AllEndSymbol = DC::STR::find(str, EndSymbol).getplace_ref();
				std::vector<PosPair> returnvalue;

				if (!SybolValid(AllStartSymbol, AllEndSymbol)) throw DC::DC_ERROR("invalid string", "symbols can not be paired", 0);//�жϿ�ʼ���źͽ������������Ƿ�һ��				
				//����㷨�������ڡ�����AllStartSymbol�е����һ��Ԫ�������������AllEndSymbolԪ�ر�Ȼ������֮��ԡ���
				for (auto i = AllStartSymbol.rbegin(); i != AllStartSymbol.rend(); i = AllStartSymbol.rbegin()) {
					std::size_t minimal = INT_MAX;//int�������ֵ
					auto iter = AllEndSymbol.end();
					for (auto i2 = AllEndSymbol.begin(); i2 != AllEndSymbol.end(); i2++) {
						if ((!(*i2 < *i)) && (*i2 - *i) < minimal) { minimal = *i2 - *i; iter = i2; }//�ҳ��͵�ǰ��ʼ��������Ľ�������
					}
					if (iter == AllEndSymbol.end())
						throw DC::DC_ERROR("undefined behavior", 0);//������Ӧ�ò����׳�������
					returnvalue.emplace_back(*i, *iter);
					AllStartSymbol.erase(--i.base());
					AllEndSymbol.erase(iter);
				}
				return returnvalue;
			}

			std::vector<PosPair> getStringSymbolPair(const std::string& str) {//�ҳ���Ե�""
				                                                              //����Ƕ��Ŷ
				std::vector<std::size_t> AllSymbol = DC::STR::find(str, "\"").getplace_ref();
				std::vector<PosPair> returnvalue;

				if (AllSymbol.size() % 2 != 0) throw DC::DC_ERROR("invalid string", "symbols can not be paired", 0);
				while(!AllSymbol.empty()) {
					returnvalue.emplace_back(*AllSymbol.begin(), *(AllSymbol.begin() + 1));
					AllSymbol.erase(AllSymbol.begin());
					AllSymbol.erase(AllSymbol.begin());
				}
				return returnvalue;
			}

			inline std::string GetJsStr(const std::string& input) {
				return "\"" + input + "\"";
			}

			ObjTable GetObjTable(const std::string& rawStr) {
				//д�����˼·:����ַ�����raw��������ʽ�����ַ��绻��tab֮���ֱ������
				return ObjTable();
			}

		}

		class value {
		public:
			value() = default;

			template<typename T>
			value(T&& input)noexcept {
				try {
					set(std::forward<T>(input));
				}
				catch (...) {
					//�쳣�����Ѿ���set�ﱣ֤���ˣ����ﲻ�����������������ֹ�쳣��ɢ������
				}
			}

			virtual ~value() = default;

		public:
			template<typename T>
			inline void set(T&& input) {
				static_assert(std::is_same<std::string, std::decay<T>::type>::value, "input type should be std::string");
				rawStr = std::forward<T>(input);
				try {
					refresh();
				}
				catch (const DC::DC_ERROR& ex) {
					if (!rawStr.empty()) rawStr.clear();
					throw ex;
				}
			}

			value at(const std::string& input) {

			}

		protected:
			std::string rawStr;
			std::vector<jsonSpace::PosPair> ObjectSymbolPair, ArraySymbolPair, StringSymbolPair;
			json::jsonSpace::ObjTable ObjectTable;//��ǰ��Ķ����б���һ���Ƕ��������ڶ����Ƕ������ݡ�

		private:
			void refresh() {
				try {
					ObjectSymbolPair = jsonSpace::getSymbolPair(rawStr, "{", "}");
					ArraySymbolPair = jsonSpace::getSymbolPair(rawStr, "[", "]");
					StringSymbolPair = jsonSpace::getSymbolPair(rawStr, "\"", "\"");
				}
				catch (const DC::DC_ERROR& ex) {
					if (!ObjectSymbolPair.empty()) ObjectSymbolPair.clear();
					if (!ArraySymbolPair.empty()) ArraySymbolPair.clear();
					if (!ObjectTable.empty()) ObjectTable.clear();
					throw ex;
				}
			}
		};

	}
}

#endif