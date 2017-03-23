#pragma once
#ifndef liuzianglib_ERROR
#define liuzianglib_ERROR
#include <string>
#include <time.h>
#include "md5.cpp"
//Version 2.4.1V10
//20170323

namespace DC {

	class DC_ERROR {//DC_ERROR ���ڼ�¼��������ʱ�����Ĵ���
					//��¼�Ĵ�����ϸ��Ϣ����:������⡢���������������¼ʱ��(UNIXʱ���)�������ϣֵ
					//ͨ�����캯������¼��������
					//DC_ERROR ���ṩ�ƶ�����
	public:
		DC_ERROR(const std::string& inputTitle, const int32_t& inputvalue) {
			Error_Time = time(0);
			Title = inputTitle;
			value = inputvalue;
		}

		DC_ERROR(const std::string& inputTitle, const std::string& inputDescription, const int32_t& inputvalue) {
			Error_Time = time(0);
			Title = inputTitle;
			Description = inputDescription;
			value = inputvalue;
		}

		DC_ERROR(const DC_ERROR& input) {
			Title = input.Title;
			Description = input.Description;
			Error_Time = input.Error_Time;
			value = input.value;
		}

	public:
		DC_ERROR& operator=(const DC_ERROR& input) {
			Title = input.Title;
			Description = input.Description;
			Error_Time = input.Error_Time;
			value = input.value;
			return *this;
		}

		std::string GetTitle()const {
			return Title;
		}

		std::string GetDescription()const {
			return Description;
		}

		time_t GetTime()const {
			return Error_Time;
		}

		std::string GetHash()const {
			MD5 TEMP(Title + Description + toString(Error_Time));
			return TEMP.toString();
		}

		int32_t GetValue()const {
			return value;
		}

	private:
		std::string toString(const time_t& value)const {
			std::stringstream sstr;
			sstr << value;
			if (sstr.fail()) {
				return std::string("");
			}
			return sstr.str();
		}

	private:
		std::string Title, Description;
		time_t Error_Time;
		int32_t value;
	};

}

#endif