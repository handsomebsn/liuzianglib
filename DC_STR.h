#pragma once
#ifndef liuzianglib_STR
#define liuzianglib_STR
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <sstream>
#include "DC_ERROR.h"
//Version 2.4.1V11
//20170324

namespace DC {

	namespace STR {

		namespace STRSpace {

			std::vector<std::size_t> KMPSearch(const char *pat, const char *txt) {
				std::vector<std::size_t> returnvalue;
				std::size_t M = strlen(pat), N = strlen(txt), j = 0, i = 0;
				std::unique_ptr<std::size_t[]> lps(new(std::nothrow) std::size_t[M]);
				if (lps.get() == nullptr) return returnvalue;
				auto computeLPSArray = [&pat, &M, &lps] {
					std::size_t len = 0, i = 1;  // ��¼ǰһ��[�ƥ���ǰ׺�ͺ�׺]�ĳ���
					lps[0] = 0; // lps[0] ������ 0
					while (i < M)
					{
						if (pat[i] == pat[len])
						{
							len++;
							lps[i] = len;
							i++;
						}
						else // (pat[i] != pat[len])
						{
							if (len != 0)
							{
								// ����ط�������. ����������� AAACAAAA ,i = 7.
								len = lps[len - 1];

								// ����, ע�� i ������ط���û������
							}
							else // ��� (len == 0)
							{
								lps[i] = 0; //û��һ��ƥ���
								i++;
							}
						}
					}
				};
				computeLPSArray();
				while (i < N) {
					if (pat[j] == txt[i])
					{
						j++;
						i++;
					}
					if (j == M)
					{
						returnvalue.push_back(i - j);
						j = lps[j - 1];
					}
					else if (pat[j] != txt[i])
					{
						if (j != 0)
							j = lps[j - 1];
						else
							i = i + 1;
					}
				}
				return returnvalue;
			}

			class SetLocal final {//ʹ��ʱ��ע��:һ����������ֻ�ܳ���һ��SetLocal���󣬷�������˳����ܲ�ȷ������������localû�б����û�Ĭ�ϵģ���Ȼ��chs
								  //��Ϊ�﷨�����޷�����ReplaceInfo�������﷨���浥��ģʽ
			public:
				SetLocal() :curLocale(setlocale(LC_ALL, NULL)) {
					setlocale(LC_ALL, "chs");
				}

				~SetLocal() {
					setlocale(LC_ALL, curLocale.c_str());
				}

				SetLocal(const SetLocal&) = delete;

				SetLocal& operator=(const SetLocal&) = delete;

			private:
				std::string curLocale;
			};

			class ReplaceInfo final {
			public:
				ReplaceInfo() = default;

				ReplaceInfo(const ReplaceInfo& input) :whererp(input.whererp), bei_ti_huan_howlong(input.bei_ti_huan_howlong) {}

				ReplaceInfo(ReplaceInfo&& input)noexcept : whererp(std::move(input.whererp)), bei_ti_huan_howlong(input.bei_ti_huan_howlong) {}//����size_t�Ͳ�ת������Ϊ�����ƶ��������ǿ���������תһ�λ��ж��������ת������

				ReplaceInfo& operator=(const ReplaceInfo& input) {
					whererp = input.whererp;
					bei_ti_huan_howlong = input.bei_ti_huan_howlong;
					return *this;
				}

				ReplaceInfo& operator=(ReplaceInfo&& input)noexcept {
					whererp = std::move(input.whererp);
					bei_ti_huan_howlong = input.bei_ti_huan_howlong;
					return *this;
				}

			public:
				void setplace(const std::vector<std::size_t>& input) {
					whererp = input;
				}

				void setplace(std::vector<std::size_t>&& input) {
					whererp = std::move(input);
				}

				void setsize(const std::size_t& input) {
					bei_ti_huan_howlong = input;
				}

				const std::vector<std::size_t>& getplace_ref()const {
					return whererp;
				}

				std::size_t getsize()const {
					return bei_ti_huan_howlong;
				}

			private:
				std::vector<std::size_t> whererp;
				std::size_t bei_ti_huan_howlong;
			};

		}

		std::string insert(const std::string& str, const std::string& input, const std::size_t& wheretoput) {//�� std::string str[wheretoput] ֮����� std::string input
																											 //wheretoput�Ƿ�ʱ���쳣
																											 //��������
			auto GetBefore = [&str, &wheretoput] {
				std::string rv;
				rv.reserve(wheretoput);
				for (std::size_t i = 0; i < wheretoput; i++)
					rv.push_back(str[i]);
				return rv;
			};
			auto GetAfter = [&str, &wheretoput] {
				std::string rv;
				rv.reserve(str.size() - wheretoput);
				for (std::size_t i = wheretoput; i < str.size(); i++)
					rv.push_back(str[i]);
				return rv;
			};
			if (wheretoput<0 || wheretoput>str.size()) throw DC::DC_ERROR("index illegal", 0);
			return GetBefore() + input + GetAfter();
		}

		std::string insertCHS(const std::string& str, const std::string& input, const std::size_t& wheretoput) {//�� std::string str[wheretoput] ֮����� std::string input
																												//wheretoput�Ƿ�ʱ���쳣
																												//��������
			if (wheretoput < 0) throw DC::DC_ERROR("index illegal", 0);
			std::size_t realput = 0, count_ = 0;
			std::string before, after;
			for (std::size_t i = 0; i < str.size(); i++) {
				if (count_ == wheretoput) { realput = i; }
				if ((str[i] < 0 || str[i]>127) && (str[i + 1] < 0 || str[i + 1]>127)) { i++; count_++; }
				else count_++;
			}
			if (wheretoput >= count_) {
				if (wheretoput == count_) return str + input;
				throw DC::DC_ERROR("index illegal", 0);
			}
			before.reserve(realput);
			for (std::size_t i = 0; i < realput; i++) {
				if (str[i] != NULL) before.push_back(str[i]);
			}
			after.reserve(str.size() - realput);
			for (std::size_t i = 0; i < str.size() - realput; i++) {
				if (str[(realput + i)] != NULL) after.push_back(str[(realput)+i]);
			}
			return before + input + after;
		}

		std::string toUpper(std::string str) {//�� std::string str �е�Сд�ַ�תΪ��д
			transform(str.begin(), str.end(), str.begin(), ::toupper);
			return str;
		}

		std::string toLower(std::string str) {//�� std::string str �еĴ�д�ַ�תΪСд
			transform(str.begin(), str.end(), str.begin(), ::tolower);
			return str;
		}

		inline STRSpace::ReplaceInfo find(const std::string& str, const std::string& findstr) {//ʹ�� KMP �ַ���ƥ���㷨�ҵ����д����� std::string str �е� std::string find�����������ǵ�λ��
			STRSpace::ReplaceInfo rv;
			rv.setsize(findstr.size());
			rv.setplace(STRSpace::KMPSearch(findstr.c_str(), str.c_str()));
			return rv;
		}

		std::string replace(const std::string& str, const STRSpace::ReplaceInfo& info, const std::string& rpword) {//��ȷ�� whererp �е�Ԫ��(��ʾλ�õ�����)�Ǵ�С�����������еġ�
																												   //���ܲ����滻ѭ�����ַ������������hh����hh�滻Ϊh�������work
			if (info.getplace_ref().empty() || str.empty()) return str;
			const std::size_t endsize = str.size() + info.getplace_ref().size()*(rpword.size() - info.getsize());
			std::vector<std::size_t>::const_iterator wherepit = info.getplace_ref().begin();
			std::string TEMP_str;
			for (std::size_t index = 0; TEMP_str.size() < endsize;) {
				if (wherepit != info.getplace_ref().end()) {
					if (index == *wherepit) {
						TEMP_str += rpword;
						index += info.getsize();
						wherepit++;
						continue;
					}
				}
				TEMP_str += str[index];
				index++;
			}
			return TEMP_str;
		}

		template <typename TYPE>
		TYPE toType(const std::string &str) {
			TYPE rv;
			std::stringstream sstr(str);
			sstr >> rv;
			if (sstr.fail()) {
				//����Ҫclear����Ϊsstr�����ٴ�ʹ��
				throw DC::DC_ERROR("std::stringstream::fail()==true", 0);
			}
			return rv;
		}

		template <>
		const char* toType<const char*>(const std::string &str) {
			return str.c_str();
		}

		template <>
		char* toType<char*>(const std::string &str) = delete;

		template <>
		std::wstring toType<std::wstring>(const std::string& s) {
			STRSpace::SetLocal sl;
			const char* _Source = s.c_str();
			size_t _Dsize = s.size() + 1;
			std::unique_ptr<wchar_t[]> _Dest(new wchar_t[_Dsize]);
			wmemset(_Dest.get(), NULL, _Dsize);
			mbstowcs(_Dest.get(), _Source, _Dsize);
			std::wstring result = _Dest.get();
			return result;
		}

		template <class T>
		std::string toString(const T& value) {
			std::stringstream sstr;
			sstr << value;
			if (sstr.fail()) {
				//����Ҫclear����Ϊsstr�����ٴ�ʹ��
				throw DC::DC_ERROR("std::stringstream::fail()==true", 0);
			}
			return sstr.str();
		}

		template <>
		std::string toString<std::wstring>(const std::wstring& ws) {
			STRSpace::SetLocal sl;
			const wchar_t* _Source = ws.c_str();
			size_t _Dsize = 2 * ws.size() + 1;
			std::unique_ptr<char[]> _Dest(new char[_Dsize]);
			memset(_Dest.get(), NULL, _Dsize);
			wcstombs(_Dest.get(), _Source, _Dsize);
			std::string result = _Dest.get();
			return result;
		}

	}

}

#endif