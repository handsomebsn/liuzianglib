#pragma once
#ifndef liuzianglib_json
#define liuzianglib_json
#include <tuple>
#include <vector>
#include <limits>
#include <type_traits>
#include "DC_STR.h"
#include "liuzianglib.h"
//Version 2.4.1V13
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

			inline std::string GetJsStr(const std::string& input) {
				return "\"" + input + "\"";
			}

		}

		class value {
		public:
			value() = default;

			value(const value& input) = default;

			value(value&& input) :rawStr(std::move(input.rawStr)), ObjectSymbolPair(std::move(input.ObjectSymbolPair)), ArraySymbolPair(std::move(input.ArraySymbolPair)), StringSymbolPair(std::move(input.StringSymbolPair)) {}

			value(const std::string& input) {//ע������ǿ����������Ҫ�ƶ�������ʹ��set������һ���ĵ���set�ƶ�����
				set(input);
			}

			virtual ~value() = default;

		public:
			value& operator=(const value&) = default;

			value& operator=(value&& input) {
				if (this == &input)
					return *this;
				rawStr = std::move(input.rawStr);
				ObjectSymbolPair = std::move(input.ObjectSymbolPair);
				ArraySymbolPair = std::move(input.ArraySymbolPair);
				StringSymbolPair = std::move(input.StringSymbolPair);
				return *this;
			}

		public:
			template<typename T>
			inline void set(T&& input) {
				static_assert(std::is_same<std::string, std::decay<T>::type>::value, "input type should be std::string");
				rawStr = std::forward<T>(input);
				try {
					RemoveOutsideSymbol();
					refresh();
				}
				catch (const DC::DC_ERROR& ex) {
					if (!rawStr.empty()) rawStr.clear();
					throw ex;
				}
			}

			value at(const std::string& key) {
				auto findResult_Full = DC::STR::find(rawStr, jsonSpace::GetJsStr(key));
				auto findResult = findResult_Full.getplace_ref();
				for (auto i = findResult.begin(); i != findResult.end();) {
					if (isInsideStr(*i) || isInsideObj(*i)) {
						i = findResult.erase(i);
						continue;
					}
					i++;
				}
				if (findResult.size() < 1) {
					throw DC::DC_ERROR("value::at", "cant find key", 0);
				}
				if (findResult.size() > 1) {
					throw DC::DC_ERROR("value::at", "too much key", 0);
				}
				std::size_t startpos = 0, endpos = 0;
				try {
					startpos = findNeXTchar(findNeXTchar(':', *findResult.begin() + findResult_Full.getsize()) + 1);
				}
				catch (...) {
					throw DC::DC_ERROR("value::at", "can not find separator", 0);//�Ҳ���ð��
				}
				switch (rawStr[startpos]) {
				case '{': {
					for (const auto& p : ObjectSymbolPair) {
						if (std::get<0>(p) == startpos) {
							endpos = std::get<1>(p);
							break;
						}
					}
				}break;
				case '[': {
					for (const auto& p : ArraySymbolPair) {
						if (std::get<0>(p) == startpos) {
							endpos = std::get<1>(p);
							break;
						}
					}
				}break;
				case '\"': {
					for (const auto& p : StringSymbolPair) {
						if (std::get<0>(p) == startpos) {
							endpos = std::get<1>(p);
							break;
						}
					}
				}break;
				default: {
					//�ֱ��ҳ����ֽ������������
					std::size_t close0 = INT_MAX, close1 = INT_MAX, close2 = INT_MAX;
					int32_t flag = 0;
					try {
						close0 = findNeXTchar('}', startpos);
					}
					catch (...) {
						flag++;
					}
					try {
						close1 = findNeXTchar(']', startpos);
					}
					catch (...) {
						flag++;
					}
					try {
						close2 = findNeXTchar(',', startpos);
					}
					catch (...) {
						flag++;
					}
					
					//���������û��
					if (flag == 3) {
						for (endpos = startpos; endpos < rawStr.size(); endpos++) {
							if (rawStr[endpos] == ' ' || rawStr[endpos] == '\n' || rawStr[endpos] == '\r' || rawStr[endpos] == '\t') break;
						}
						endpos--;
						break;
					}

					//������С�Ĵ���rv��
					std::size_t rv = close0;
					if (close1 < rv) rv = close1;
					if (close2 < rv) rv = close2;

					endpos = rv - 1;
				}break;
				}

				//�жϺϷ�
				if(startpos>endpos)
					throw DC::DC_ERROR("value::at", "substring length illegal", 0);//�ִ����Ȳ��Ϸ�

				//��ȡ�Ӵ�
				std::string rv;
				rv.reserve(endpos - startpos);
				for (std::size_t i = startpos; i <= endpos&&i < rawStr.size(); i++) {
					rv.push_back(rawStr[i]);
				}

				return value(rv);
			}

		protected:
			virtual void RemoveOutsideSymbol() {//ɾ��������ķ��Ŷ�
				bool flag = false;

				for (auto i = rawStr.begin(); i != rawStr.end(); i++) {
					if (*i == '{' || *i == '[') {
						rawStr.erase(i);
						flag = true;
						break;
					}
				}

				if (flag != true) return;

				for (auto i = rawStr.rbegin(); i != rawStr.rend(); i++) {
					if (*i == '}' || *i == ']') {
						rawStr.erase(--i.base());
						break;
					}
				}
			}

		protected:
			std::string rawStr;
			std::vector<jsonSpace::PosPair> ObjectSymbolPair, ArraySymbolPair, StringSymbolPair;

		private:
			void refresh() {
				try {
					StringSymbolPair = this->getStringSymbolPair(rawStr);
					ObjectSymbolPair = getSymbolPair("{", "}");
					ArraySymbolPair = getSymbolPair("[", "]");					
				}
				catch (const DC::DC_ERROR& ex) {
					if (!StringSymbolPair.empty()) StringSymbolPair.clear();
					if (!ObjectSymbolPair.empty()) ObjectSymbolPair.clear();
					if (!ArraySymbolPair.empty()) ArraySymbolPair.clear();
					throw ex;
				}
			}

			bool isInsideStr(const std::size_t& input) {//inputλ���Ƿ���js�ַ���������js�û��ַ�������
				for (const auto& p : StringSymbolPair) {
					if (input > std::get<0>(p) && input < std::get<1>(p)) return true;
				}
				return false;
			}

			bool isInsideObj(const std::size_t& input) {//inputλ���Ƿ���js�ַ���������js�û��ַ�������
				for (const auto& p : ObjectSymbolPair) {
					if (input > std::get<0>(p) && input < std::get<1>(p)) return true;
				}
				return false;
			}

			std::vector<jsonSpace::PosPair> getStringSymbolPair(std::string str) {//�ҳ���Ե�""
																				  //����Ƕ��Ŷ
																				  //��һ�β�����������
				str = DC::STR::replace(str, DC::STR::find(str, R"(\")"), "  ");//��\"���������ո񣬼ȱ�֤�˳��Ȳ��䣬�ֱ�֤��ȥ���û��ַ������������
				std::vector<std::size_t> AllSymbol = DC::STR::find(str, "\"").getplace_ref();
				std::vector<jsonSpace::PosPair> returnvalue;

				if (AllSymbol.size() % 2 != 0) 
					throw DC::DC_ERROR("invalid string", "symbols can not be paired", 0);
				while (!AllSymbol.empty()) {
					returnvalue.emplace_back(*AllSymbol.begin(), *(AllSymbol.begin() + 1));
					AllSymbol.erase(AllSymbol.begin());
					AllSymbol.erase(AllSymbol.begin());
				}
				return returnvalue;
			}

			std::size_t findNeXTchar(std::size_t startpos) {//�ҵ���һ���ַ�������"name:  s"����5��ʼ�ң��ҵ�s����������;�����и�ʽ���Ʒ�
															//�Ҳ������쳣
				for (; startpos < rawStr.size(); startpos++)
					if (rawStr[startpos] != ' '&&rawStr[startpos] != '\n'&&rawStr[startpos] != '\r'&&rawStr[startpos] != '\t')
						return startpos;
				throw false;
			}

			std::size_t findNeXTchar(const char& findthis, std::size_t startpos) {//�ҵ���һ���ַ�
																				  //�Ҳ������쳣
				for (; startpos < rawStr.size(); startpos++)
					if (rawStr[startpos] == findthis) 
						return startpos;
				throw false;
			}

			std::vector<jsonSpace::PosPair> getSymbolPair(const char* const StartSymbol, const char* const EndSymbol) {//��ֹjs�ַ�����ķ���Ӱ��
				std::vector<std::size_t> AllStartSymbol = DC::STR::find(rawStr, StartSymbol).getplace_ref(), AllEndSymbol = DC::STR::find(rawStr, EndSymbol).getplace_ref();
				std::vector<jsonSpace::PosPair> returnvalue;

				for (auto i = AllStartSymbol.begin(); i != AllStartSymbol.end(); ) {
					if (isInsideStr(*i)) {
						i = AllStartSymbol.erase(i);
						continue;
					}
					i++;
				}
				for (auto i = AllEndSymbol.begin(); i != AllEndSymbol.end(); ) {
					if (isInsideStr(*i)) {
						i = AllEndSymbol.erase(i);
						continue;
					}
					i++;
				}

				if (!jsonSpace::SybolValid(AllStartSymbol, AllEndSymbol)) throw DC::DC_ERROR("invalid string", "symbols can not be paired", 0);//�жϿ�ʼ���źͽ������������Ƿ�һ��				
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
		};

	}
}

#endif