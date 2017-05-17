#pragma once
#ifndef liuzianglib_http
#define liuzianglib_http
#include "liuzianglib.h"
#include "DC_STR.h"
#include <vector>
#include <string>
#include <cctype>
#include <functional>
//Version 2.4.2V35
//20170517

namespace DC {

	namespace Web {

		namespace http {

			class request;
			class response;
			class headers;

			using body = std::string;
			using method = const std::string;
			using status_code = const unsigned short;

			namespace methods {

				static http::method GET = "GET";
				static http::method POST = "POST";
				static http::method HEAD = "HEAD";

			}

			namespace status_codes {

				static http::status_code OK = 200;
				static http::status_code BadRequest = 400;
				static http::status_code Forbidden = 403;
				static http::status_code NotFound = 404;
				static http::status_code MethodNotAllowed = 405;
				static http::status_code InternalError = 500;
				static http::status_code ServiceUnavailable = 503;

			}

			namespace httpSpace {

				static const char *emptyline = "\r\n\r\n", *nextline = "\r\n", *Key_TransferEncoding = "Transfer-Encoding", *Key_ContentLength = "Content-Length";

				class header final :public DC::KeyValuePair {
				public:
					header() = default;

					header(const std::string& input) :KeyValuePair() {
						Set(input);
					}

					header(const std::string& key, const std::string& inputvalue) :KeyValuePair() {
						name = key;
						value = inputvalue;
						OK = true;
					}

				public:
					virtual inline char GetSeparator()const noexcept override {
						return ':';
					}

					virtual void Set(const std::string& input)override {
						DC::KeyValuePair::Set(input);
						if (!value.empty()) if (*value.begin() == ' ') value.erase(value.begin());
					}

				protected:
					virtual inline bool isSeparator(const char& ch)const noexcept override {
						if (ch == ':') return true;
						return false;
					}
				};

				const char* getSC(const http::status_code& input) {
					if (input == 200) { return "OK"; }
					if (input == 400) { return "Bad Request"; }
					if (input == 403) { return "Forbidden"; }
					if (input == 404) { return "Not Found"; }
					if (input == 405) { return "Method Not Allowed"; }
					if (input == 500) { return "Internal Error"; }
					if (input == 503) { return "Service Unavailable"; }
					throw DC::DC_ERROR("getSC", "unknown status code");
				}

				class base;

				class title final {
					friend class request;
					friend class response;
					friend class httpSpace::base;
				public:
					title() = default;

					title(const double& inputVersion, const http::method& inputmethod, const std::string& URI) :version(DC::STR::toString(inputVersion)), second(inputmethod), third(URI) {}

					title(const std::string& inputVersion, const std::string& inputmethod, const std::string& URI) :version(inputVersion), second(inputmethod), third(URI) {}

					title(const double& inputVersion, const http::status_code& inputStatusCode) :version(DC::STR::toString(inputVersion)), second(DC::STR::toString(inputStatusCode)), third(httpSpace::getSC(inputStatusCode)) {}

					title(const std::string& inputVersion, const http::status_code& inputStatusCode) :version(inputVersion), second(DC::STR::toString(inputStatusCode)), third(httpSpace::getSC(inputStatusCode)) {}

				public:
					template<typename T>
					inline std::string toStr()const;

					template<>
					inline std::string toStr<DC::Web::http::request>()const {
						return second + " " + third + " " + GetVersionStr();
					}

					template<>
					inline std::string toStr<DC::Web::http::response>()const {
						return GetVersionStr() + " " + second + " " + third;
					}

				private:
					std::string version, second, third;//second:GET\200 third:URI\OK

				private:
					inline std::string GetVersionStr()const {
						return "HTTP/" + version;
					}

					inline std::string& method() {
						return second;
					}

					inline std::string& StatusCode() {
						return second;
					}

					inline std::string& StatusCodeDes() {
						return third;
					}

					inline std::string& URI() {
						return third;
					}
				};

				inline bool vec_header_find_func(const header& obj, const std::string& name) {
					if (obj.GetName() == name&&obj.isSetOK()) return true;
					return false;
				}

			}

			class headers final {
			public:
				headers() = default;

				template<typename T, typename ...argsType>
				headers(T&& first, argsType&& ...args) {//�÷�:http_header(addHeader("name", "value"), addHeader("name2", "value2"), addHeader("name3", "value3")......);
					auto args_value = DC::GetArgs(std::forward<argsType>(args)...);
					m_data.push_back(std::move(first));
					for (const auto& p : args_value) {
						m_data.push_back(std::move(p.get<httpSpace::header>()));
					}
				}

				headers(const headers&) = default;

				headers(headers&& input) :m_data(std::move(input.m_data)) {}

			public:
				headers& operator=(const headers&) = default;

				headers& operator=(headers&& input) {
					m_data = std::move(input.m_data);
					return *this;
				}

			public:
				template<typename T, typename ...argsType, class = typename std::enable_if_t<!std::is_same<std::decay_t<T>, std::string>::value>>
				void add(T&& first, argsType&& ...args) {//�÷�:add(addHeader("name", "value"), addHeader("name2", "value2"), addHeader("name3", "value3")......);
					auto args_value = DC::GetArgs(std::forward<argsType>(args)...);
					m_data.emplace_back(std::forward<T>(first));
					for (const auto& p : args_value) {
						m_data.push_back(std::move(p.get<httpSpace::header>()));
					}
				}

				inline void add(const std::string& input) {
					m_data.emplace_back(input);
				}

				inline bool hasKey(const std::string& input)const noexcept {
					return std::find_if(m_data.begin(), m_data.end(), std::bind(httpSpace::vec_header_find_func, std::placeholders::_1, input)) != m_data.end();
				}

				inline httpSpace::header getHeader(const std::string& key)const {//����ж��ƥ���key������������ǰ��
					auto it = std::find_if(m_data.begin(), m_data.end(), std::bind(httpSpace::vec_header_find_func, std::placeholders::_1, key));
					if (it == m_data.end()) throw DC::DC_ERROR("get", "key not found");
					return *it;
				}

				inline std::string getValue(const std::string& key)const {
					return getHeader(key).GetValue();
				}

				inline void drop(const std::string& key) {//����ж��header��key���ǣ���ô������ȫ��ɾ��
					while (true) {
						auto it = std::find_if(m_data.begin(), m_data.end(), std::bind(httpSpace::vec_header_find_func, std::placeholders::_1, key));
						if (it == m_data.end()) return;
						m_data.erase(it);
					}
				}

				inline void clear() {
					m_data.clear();
				}

				inline bool empty()const {
					return m_data.empty();
				}

				std::string toStr()const {
					std::string returnvalue;
					for (const auto& p : m_data) {
						returnvalue += p.GetName() += p.GetSeparator() + p.GetValue() + httpSpace::nextline;
					}
					returnvalue.erase(--returnvalue.rbegin().base());
					returnvalue.erase(--returnvalue.rbegin().base());
					return returnvalue;
				}

			private:
				std::vector<httpSpace::header> m_data;
			};

			namespace httpSpace {

				class base {
				public:
					base() = default;

					base(const base&) = default;

					base(base&& input) :m_title(std::move(input.m_title)), m_headers(std::move(input.m_headers)), m_body(std::move(input.m_body)) {}

					virtual ~base()noexcept = default;

				public:
					base& operator=(const base&) = default;

					base& operator=(base&& input) {
						m_title = std::move(input.m_title);
						m_headers = std::move(input.m_headers);
						m_body = std::move(input.m_body);
						return *this;
					}

				public:
					inline std::string HTTPVersion() { return m_title.version; }

					inline void setHTTPVersion(const double& input) { m_title.version = DC::STR::toString(input); }

					inline void setHTTPVersion(const std::string& input) { m_title.version = input; }

					inline httpSpace::title& Title() {
						return m_title;
					}

					inline http::headers& Headers() {
						return m_headers;
					}

					inline http::body& Body() {
						return m_body;
					}

					virtual std::string toStr()const = 0;

				protected:
					DC::Web::http::httpSpace::title m_title;
					headers m_headers;
					body m_body;
				};

				template<typename titleType, typename headersType, typename bodyType>
				inline void Derived_construct(base& object, titleType&& inputtitle, headersType&& inputheaders, bodyType&& inputbody) {
					static_assert(std::is_same<std::decay_t<titleType>, title>::value, "first arg's type should be title");
					static_assert(std::is_same<std::decay_t<headersType>, headers>::value, "second arg's type should be headers");
					static_assert(std::is_same<std::decay_t<bodyType>, body>::value, "third arg's type should be body");
					object.Title() = std::forward<title>(inputtitle);
					object.Headers() = std::forward<headers>(inputheaders);
					object.Body() = std::forward<body>(inputbody);
				}

				template<typename T>
				httpSpace::title title_deserialization(const std::string& input);

				template<>
				inline httpSpace::title title_deserialization<http::request>(const std::string& input) {
					auto loca = DC::STR::find(input, " ");
					if (loca.getplace_ref().empty()) throw DC::DC_ERROR("title_deserialization", "method not found");
					auto GetVersionNumStr = [](const std::string& input) {
						std::string rv;
						for (const auto& p : input) {
							if (std::isdigit(p) || p == '.') rv.push_back(p);
						}
						return rv;
					};
					auto methodAndURI = DC::STR::getSub(input, -1, *loca.getplace_ref().rbegin());
					auto frs = methodAndURI.find_first_of(' ');
					if (frs == std::string::npos) throw DC::DC_ERROR("title_deserialization", "space not found");

					return httpSpace::title(GetVersionNumStr(DC::STR::getSub(input, *loca.getplace_ref().rbegin(), input.size())), DC::STR::getSub(methodAndURI, -1, frs), DC::STR::getSub(methodAndURI, frs, methodAndURI.size()));
				}

				template<>
				inline httpSpace::title title_deserialization<http::response>(const std::string& input) {
					auto loca = DC::STR::find(input, " ");
					if (loca.getplace_ref().empty()) throw DC::DC_ERROR("title_deserialization", "method not found");
					auto GetVersionNumStr = [](const std::string& input) {
						std::string rv;
						for (const auto& p : input) {
							if (std::isdigit(p) || p == '.') rv.push_back(p);
						}
						return rv;
					};
					auto frs = DC::STR::find(input, " ");
					if (frs.getplace_ref().empty()) throw DC::DC_ERROR("title_deserialization", "space not found");
					return httpSpace::title(GetVersionNumStr(DC::STR::getSub(input, -1, *loca.getplace_ref().begin())), (http::status_code)(DC::STR::toType<int32_t>(DC::STR::getSub(input, *frs.getplace_ref().begin(), *frs.getplace_ref().rbegin()))));
				}

				http::headers headers_deserialization(const std::string& input) {
					http::headers rv;
					auto frs = DC::STR::find(input, http::httpSpace::nextline);
					if (frs.getplace_ref().empty()) {
						if (input.empty()) return rv;
						//ֻ��һ�е����
						rv.add(input);
						return rv;
					}
					rv.add(DC::STR::getSub(input, -1, *frs.getplace_ref().begin()));
					for (std::size_t i = 1; i < frs.getplace_ref().size(); i++) {
						rv.add(DC::STR::getSub(input, frs.getplace_ref()[i - 1] + 1, frs.getplace_ref()[i]));
					}
					rv.add(DC::STR::getSub(input, frs.getplace_ref()[frs.getplace_ref().size() - 1] + 1, input.size()));
					return rv;
				}

			}

			class request final :public httpSpace::base {
			public:
				request() = default;

				template<typename ...argsType>
				request(argsType ...args) {
					httpSpace::Derived_construct(reinterpret_cast<httpSpace::base&>(*this), std::forward<argsType>(args)...);
				}

			public:
				virtual inline std::string toStr()const override {
					auto temp = m_title.toStr<request>() + httpSpace::nextline + m_headers.toStr();
					if (m_body.empty()) return temp + httpSpace::emptyline;
					else return temp + httpSpace::emptyline + m_body;
				}

				inline method& Method() {
					return m_title.method();
				}

				inline std::string& URI() {
					return m_title.URI();
				}
			};

			class response final :public httpSpace::base {
			public:
				response() = default;

				template<typename ...argsType>
				response(argsType ...args) {
					httpSpace::Derived_construct(reinterpret_cast<httpSpace::base&>(*this), std::forward<argsType>(args)...);
				}

			public:
				virtual inline std::string toStr()const override {
					auto temp = m_title.toStr<response>() + httpSpace::nextline + m_headers.toStr();
					if (m_body.empty()) return temp + httpSpace::emptyline;
					else return temp + httpSpace::emptyline + m_body;
				}

				inline void setStatusCode(const http::status_code& input) {
					m_title.StatusCode() = DC::STR::toString(input);
					m_title.StatusCodeDes() = httpSpace::getSC(input);
				}

				inline http::status_code StatusCode() {
					return DC::STR::toType<int32_t>(m_title.StatusCode());
				}
			};

			template<typename ...argsType>
			httpSpace::header addHeader(argsType&& ...args) {
				return httpSpace::header(std::forward<argsType>(args)...);
			}

			request request_deserialization(const std::string& input) {
				std::string titleraw, headersraw, bodyraw;
				std::size_t titleend = 0, headersend = 0;

				for (std::size_t i = 0; i < input.size(); i++) {
					if (input[i] == '\n') { titleend = i; break; }
					titleraw.push_back(input[i]);
				}

				auto emptylineLoca = DC::STR::find(input, httpSpace::emptyline);
				try {
					if (emptylineLoca.getplace_ref().empty())
						headersraw = DC::STR::getSub(input, titleend, input.size());
					else
						headersraw = DC::STR::getSub(input, titleend, *emptylineLoca.getplace_ref().begin());
				}
				catch (...) {}

				try {
					if (!emptylineLoca.getplace_ref().empty()) bodyraw = DC::STR::getSub(input, *emptylineLoca.getplace_ref().begin() + sizeof(httpSpace::emptyline) - 1, input.size());
				}
				catch (...) {}

				return request(httpSpace::title(httpSpace::title_deserialization<request>(titleraw)), httpSpace::headers_deserialization(headersraw), bodyraw);
			}

			response response_deserialization(const std::string& input) {
				std::string titleraw, headersraw, bodyraw;
				std::size_t titleend = 0, headersend = 0;

				for (std::size_t i = 0; i < input.size(); i++) {
					if (input[i] == '\r') { titleend = i; break; }
					titleraw.push_back(input[i]);
				}

				auto emptylineLoca = DC::STR::find(input, httpSpace::emptyline);
				try {
					if (emptylineLoca.getplace_ref().empty())
						headersraw = DC::STR::getSub(input, titleend, input.size());
					else
						headersraw = DC::STR::getSub(input, titleend, *emptylineLoca.getplace_ref().begin());
				}
				catch (...) {}

				try {
					if (!emptylineLoca.getplace_ref().empty()) bodyraw = DC::STR::getSub(input, *emptylineLoca.getplace_ref().begin() + sizeof(httpSpace::emptyline) - 1, input.size());
				}
				catch (...) {}

				return response(httpSpace::title(httpSpace::title_deserialization<response>(titleraw)), httpSpace::headers_deserialization(headersraw), bodyraw);
			}

			inline httpSpace::title Set_Version_Statuscode(const double& inputVersion, const http::status_code& inputStatusCode) {
				return httpSpace::title(inputVersion, inputStatusCode);
			}

			inline httpSpace::title Set_Version_Method_URI(const double& inputVersion, const http::method& inputmethod, const std::string& inputURI) {
				return httpSpace::title(inputVersion, inputmethod, inputURI);
			}

			inline std::string get(const httpSpace::base& input, const std::string& key) {
				return const_cast<httpSpace::base&>(input).Headers().getValue(key);
			}

			inline bool has_body(const httpSpace::base& input)noexcept {
				return !const_cast<httpSpace::base&>(input).Body().empty();
			}

			inline bool has_TransferEncoding(const httpSpace::base& input)noexcept {
				return const_cast<httpSpace::base&>(input).Headers().hasKey(httpSpace::Key_TransferEncoding);
			}

			inline std::string get_TransferEncoding(const httpSpace::base& input) {
				return get(input, httpSpace::Key_TransferEncoding);
			}

			inline bool has_ContentLength(const httpSpace::base& input)noexcept {
				return const_cast<httpSpace::base&>(input).Headers().hasKey(httpSpace::Key_ContentLength);
			}

			inline std::string get_ContentLength(const httpSpace::base& input) {
				return get(input, httpSpace::Key_ContentLength);
			}

			inline std::string get_URL(const std::string& uri) {
				auto pos = uri.find_first_of('?');
				if (pos == std::string::npos || uri.empty()) return uri;
				return DC::STR::getSub(uri, -1, pos);
			}

			inline std::string get_Parameter(const std::string& uri) {
				auto pos = uri.find_first_of('?');
				if (pos == std::string::npos || uri.empty()) throw DC::Exception("DC::http::get_Parameter", "? not found");
				return DC::STR::getSub(uri, pos, uri.size());
			}

		}

	}

}

#endif