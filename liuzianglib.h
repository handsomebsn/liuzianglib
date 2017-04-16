#pragma once
#ifndef liuzianglib_liuzianglib
#define liuzianglib_liuzianglib
#include <iostream>
#include <random>
#include <queue>
#include "DC_Any.h"
//Version 2.4.2V15
//20170416

#define GET_FIRST_PARAMETERS 0//������GetCommandLineParameters

namespace DC {

	typedef std::queue<std::string> PARS_V;
	typedef std::vector<DC::Any> ARGS_V;

	static inline int32_t randomer(int32_t s, int32_t b) {//���ɽ���s��b֮��������(����s��b)
		                                                  //��û���ڴ�й©����Ϊ�ظ�����û��ռ�ø�����ڴ�
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(s, b);
		return dis(gen);
	}

	template <typename numtype> std::string::size_type Howmuchdig(numtype num) {//����num��λ��������num=1000ʱ������4
		int32_t i = 0;
		while (num > 1) { num /= 10; i++; }
		if (num == 1) return i + 1; else return i;
	}

	template <typename itemtype> inline void swap(itemtype& a, itemtype& b) {
		itemtype TEMP(std::move(a));
		a = std::move(b);
		b = std::move(TEMP);
	}

	template<typename ...ARGS>
	void GetCommandLineParameters(const char* argv[], ARGS& ...args) {//��ʼ
		PARS_V pars;
		for (std::size_t i = 1; true; i++) {
			if (argv[i] != NULL) pars.push(argv[i]);
			else break;
		}
		GetCommandLineParameters(pars, args...);
	}

	template<typename ...ARGS>
	void GetCommandLineParameters(const int32_t& flag, const char* argv[], ARGS& ...args) {//��ѡ��Ŀ�ʼ
		PARS_V pars;
		for (std::size_t i = flag; true; i++) {
			if (argv[i] != NULL) pars.push(argv[i]);
			else break;
		}
		GetCommandLineParameters(pars, args...);
	}

	template<typename T, typename ...ARGS>
	void GetCommandLineParameters(PARS_V& pars, T& item, ARGS& ...args) {//�ݹ���
		if (!pars.empty()) {
			item = pars.front();
			pars.pop();
		}
		GetCommandLineParameters(pars, args...);
	}

	template<typename T>
	void GetCommandLineParameters(PARS_V& pars, T& item) {//��ֹ����
		if (!pars.empty()) item = pars.front();
	}

	inline ARGS_V GetArgs() {//�޲���
		ARGS_V returnvalue;
		return returnvalue;
	}

	template<typename T>
	ARGS_V GetArgs(const T& arg) {//��������
		ARGS_V returnvalue;
		returnvalue.push_back(arg);
		return returnvalue;
	}

	template<typename T,typename ...ARGS>
	ARGS_V GetArgs(const T& arg, const ARGS& ...args) {//�������
		ARGS_V returnvalue;
		returnvalue.push_back(arg);
		GetArgs(returnvalue, args...);
		return returnvalue;
	}

	template<typename T, typename ...ARGS>
	void GetArgs(ARGS_V& rv, const T& arg, const ARGS& ...args) {//�ݹ���
		rv.push_back(arg);
		GetArgs(rv, args...);
	}

	template<typename T>
	void GetArgs(ARGS_V& rv, const T& arg) {//��ֹ����
		rv.push_back(arg);
	}

	std::vector<std::string> GetKeyValuePairStr(const std::string& str) {//��ȡ���ո�ָ�����ֵ���ַ���
		std::string pushthis;
		std::vector<std::string> rv;
		for (const auto&p : str) {
			if (p != ' ')
				pushthis.push_back(p);
			else {
				rv.push_back(pushthis);
				pushthis = "";
			}
		}
		if (!pushthis.empty()) rv.push_back(pushthis);
		return rv;
	}

	class KeyValuePair {//���浥����ֵ��
		                //Ҫ�趨�ָ�������Ӵ�����������д isSeparator �麯��
	public:
		KeyValuePair() :OK(false) {}

		KeyValuePair(const KeyValuePair&) = default;

		KeyValuePair(KeyValuePair&& input) :name(std::move(input.name)), value(std::move(input.value)), OK(input.OK) {
			input.OK = false;
		}

		virtual ~KeyValuePair()noexcept = default;

	public:
		KeyValuePair& operator=(const KeyValuePair&) = default;

		KeyValuePair& operator=(KeyValuePair&& input) {
			name = std::move(input.name);
			value = std::move(input.value);
			OK = input.OK;
			input.OK = false;
			return *this;
		}

	public:		
		virtual void Set(const std::string& input) {
			bool OKflag = false;
			std::size_t whereis = 0;

			//�ҷָ���λ��
			for (const auto& p : input) {
				if (isSeparator(p)) {
					OKflag = true;
					break;
				}
				whereis++;
			}
			if (OKflag != true) { OK = false; return; }

			//��ȡ�ַ���
			name = value = "";
			for (std::size_t i = 0; i < whereis; i++) {
				if (input[i] != NULL) name += input[i];
			}
			for (std::size_t i = whereis + 1; i < input.size(); i++) {
				if (input[i] != NULL) value += input[i];
			}
			OK = true;
		}

		inline bool isSetOK()const {
			return OK;
		}

		virtual inline std::string GetName()const {
			return name;
		}

		virtual inline std::string GetValue()const {
			return value;
		}

		virtual inline char GetSeparator()const noexcept = 0;

	protected:
		virtual inline bool isSeparator(const char& ch)const noexcept = 0;

	protected:
		bool OK;
		std::string name, value;
	};

}

#endif