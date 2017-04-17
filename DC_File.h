#pragma once
#ifndef liuzianglib_File
#define liuzianglib_File
#include <string>
#include "DC_ERROR.h"
//Version 2.4.2V16
//20170417

#define ERROR_CANTOPENFILE DC::DC_ERROR(filename,"CAN NOT OPEN FILE", -1)
#define ERROR_CANTGETSIZE DC::DC_ERROR(filename,"CAN NOT GET FILE SIZE", -1)

namespace DC {

	namespace File {
		//ĳЩ������ptr���õ����ذ汾
		//��ʧ�����쳣

		class text;
		class binary;

		class file_ptr final {//����C�����ļ�ָ���������ڵİ�װ��
							  //����ͨ��if(file_ptr)�ķ�ʽ�ж�file_ptr�Ƿ��Ѿ���
							  //������ʹ��ʱ�ֶ�fclose(file_ptr::get())���ⲻ�ᵼ��δ������Ϊ
							  //��֧���ƶ�����
							  //ĳЩʱ������ʹ�ô���Ĺ�������1(VS�ڴ�鿴���еġ�����(����)��)�ڴ�û�л��գ��ⲻ���ڴ�й©��
		public:
			file_ptr() :fp(nullptr) {}

			file_ptr(FILE* input) :fp(input) {}

			file_ptr(const file_ptr&) = delete;

			file_ptr(file_ptr&& input)noexcept {
				clear();
				this->fp = input.fp;
				input.fp = nullptr;
			}

			~file_ptr() {
				clear();
			}

			file_ptr& operator=(const file_ptr&) = delete;

			file_ptr& operator=(file_ptr&& input)noexcept {
				clear();
				this->fp = input.fp;
				input.fp = nullptr;
				return *this;
			}

			operator bool()const {
				if (fp == NULL || fp == nullptr) return false;
				return true;
			}

			inline FILE* get()const {
				return fp;
			}

			inline void reset(FILE* input) {
				clear();
				fp = input;
			}

		private:
			inline void clear() {
				if (fp == NULL || fp == nullptr) return;
				fclose(fp);
			}

		private:
			FILE *fp;
		};

		inline void del(const std::string& filename)noexcept {
			remove(filename.c_str());
		}

		inline bool exists(const std::string& filename)noexcept {
			file_ptr ptr(fopen(filename.c_str(), "r"));
			return (bool)ptr;
		}

		template<typename FLAG>
		inline bool exists(const std::string& filename, file_ptr& inputptr)noexcept;

		template<>
		inline bool exists<text>(const std::string& filename, file_ptr& inputptr)noexcept {//ͬʱ�ж��Ƿ���ں��Ƿ��ܴ�
			file_ptr ptr(fopen(filename.c_str(), "r"));
			inputptr = std::move(ptr);
			return (bool)inputptr;
		}

		template<>
		inline bool exists<binary>(const std::string& filename, file_ptr& inputptr)noexcept {//ͬʱ�ж��Ƿ���ں��Ƿ��ܴ�
			file_ptr ptr(fopen(filename.c_str(), "rb"));
			inputptr = std::move(ptr);
			return (bool)inputptr;
		}

		std::size_t getSize(const std::string& filename) {//��ȡ�ļ�����
			file_ptr ptr;
			if (!exists<text>(filename, ptr)) throw ERROR_CANTOPENFILE;
			fseek(ptr.get(), 0L, SEEK_END);
			const auto& rv = ftell(ptr.get());
			if (rv < 0) throw ERROR_CANTGETSIZE;
			return rv;//�з����޷��Ų�ƥ����Ժ��ԣ������Ѿ���������ִ�����
		}

		template<typename FLAG = text>
		std::string read(const std::string& filename) {
			static_assert(std::is_same<std::decay_t<FLAG>, text>::value || std::is_same<std::decay_t<FLAG>, binary>::value, "flag illegal");
			file_ptr ptr;
			if (!exists<FLAG>(filename, ptr)) throw ERROR_CANTOPENFILE;
			std::string returnvalue;
			auto&& ch = fgetc(ptr.get());
			while (ch != EOF) {
				returnvalue.push_back(ch);
				ch = fgetc(ptr.get());
			}
			return returnvalue;
		}

		template<typename FLAG = text>
		bool write(const std::string& filename, const std::string& write)noexcept;

		template<>
		bool write<text>(const std::string& filename, const std::string& write)noexcept {//����д��
			file_ptr ptr(fopen(filename.c_str(), "w"));
			if (!ptr) return false;
			if (fprintf(ptr.get(), "%s", write.c_str()) == -1) return false;
			return true;
		}

		template<>
		bool write<binary>(const std::string& filename, const std::string& write)noexcept {//����д��
			file_ptr ptr(fopen(filename.c_str(), "wb"));
			if (!ptr) return false;
			if (fwrite(write.c_str(), 1, write.size(), ptr.get()) == -1) return false;
			return true;
		}

		template<typename FLAG = text>
		bool writeAppend(const std::string& filename, const std::string& write)noexcept;

		template<>
		bool writeAppend<text>(const std::string& filename, const std::string& write)noexcept {
			file_ptr ptr(fopen(filename.c_str(), "a"));
			if (!ptr) return false;
			if (fprintf(ptr.get(), "%s", write.c_str()) == -1) return false;
			return true;
		}

		template<>
		bool writeAppend<binary>(const std::string& filename, const std::string& write)noexcept {
			file_ptr ptr(fopen(filename.c_str(), "ab"));
			if (!ptr) return false;
			if (fwrite(write.c_str(), 1, write.size(), ptr.get()) == -1) return false;
			return true;
		}

	}

}

#endif