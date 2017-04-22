#pragma once
#ifndef liuzianglib_IOCP
#define liuzianglib_IOCP
#include <winsock2.h>
#include <MSWSock.h>
#include <vector>
#include "liuzianglib.h"
#include "DC_WinSock.h"
#include "DC_ThreadPool.h"
#include "DC_timer.h"
#pragma comment(lib,"ws2_32.lib")
//Version 2.4.2V21
//20170422

namespace DC {

	namespace IOCP {

		class Server;

		namespace IOCPSpace {

			class unique_id {
			public:
				unique_id() :uniqueid(DC::randomer(0, 21476836476)) {}

				unique_id(const int32_t input) :uniqueid(input) {}

				unique_id(const unique_id&) = default;

			public:
				bool operator==(const unique_id& input)const {
					if (uniqueid == input.uniqueid) return true;
					return false;
				}

			public:
				inline int32_t getUniqueID()const {
					return uniqueid;
				}

			private:
				const int32_t uniqueid;
			};

		}

		enum OperationType { ACCEPT_POSTED, SEND_POSTED, RECV_POSTED, NULL_POSTED };

		struct PerIOContext {//只能在堆上构造，使用destroy成员函数来释放内存
			OVERLAPPED m_overlapped;
			OperationType m_opType;
			SOCKET m_sock;
			WSABUF m_wsabuf;
			IOCPSpace::unique_id uniqueid;

		public:
			PerIOContext(const std::size_t& buffersize) {
				memset(&m_overlapped, NULL, sizeof(m_overlapped));
				m_sock = INVALID_SOCKET;
				m_opType = OperationType::NULL_POSTED;

				m_wsabuf.len = buffersize;
				if (buffersize != 0) {
					m_wsabuf.buf = new char[buffersize];
					resetBuffer();
				}
				else m_wsabuf.buf = nullptr;
			}

			PerIOContext(const PerIOContext& input) = delete;

		public:
			bool operator==(const PerIOContext& input)const {
				if (input.uniqueid == uniqueid) return true;
				return false;
			}

		private:
			~PerIOContext() {}

		public:
			inline void resetBuffer() {
				memset(m_wsabuf.buf, NULL, m_wsabuf.len);
			}

			inline void CloseSock() {
				if (m_sock != INVALID_SOCKET) {
					closesocket(m_sock);
					m_sock = INVALID_SOCKET;
				}
			}

			void destroy() {
				CloseSock();

				if (m_wsabuf.buf != nullptr) {
					delete[] m_wsabuf.buf;
				}
				delete this;
			}
		};

		class PerSocketContext;

		namespace IOCPSpace {

			struct PICdeleter {
			public:
				inline void operator()(IOCP::PerIOContext *ptr) {
					ptr->destroy();
				}
			};

			template <typename T, typename deleter = void>
			class Pool {
				friend class PerSocketContext;
			public:
				Pool() :removeTime(0), cleanLimit(1) {}

				virtual ~Pool() = default;

			public:
				inline void setCleanLimit(const int32_t& input) {
					if (this == nullptr) return;
					if (input < 1) { cleanLimit.store(1, std::memory_order_release); return; }
					cleanLimit.store(input, std::memory_order_release);
				}

				template <typename ...U>
				inline T* make(U&& ...args)noexcept {
					if (this == nullptr) return nullptr;
					Cleanif();
					std::unique_lock<std::mutex> LwriteLock(writeLock);
					try {
						m.emplace_back(new T(std::forward<U>(args)...));
					}
					catch (...) {
						return nullptr;
					}
					return m.rbegin()->get();
				}

				template <typename ...U>
				inline T* put(U&& ...args)noexcept {//only unique_ptr
					if (this == nullptr) return nullptr;
					Cleanif();
					std::unique_lock<std::mutex> LwriteLock(writeLock);
					try {
						m.emplace_back(std::forward<U>(args)...);
					}
					catch (...) {
						return nullptr;
					}
					return m.rbegin()->get();
				}

				T* release(const T *input) {
					if (this == nullptr) return nullptr;
					std::unique_lock<std::mutex> LwriteLock(writeLock);
					for (auto i = m.begin(); i != m.end(); i++) {
						if (*input == *(i->get())) {
							removeTime++;
							return i->release();
						}
					}
					return nullptr;
				}

				bool drop(const T *input) {
					if (this == nullptr) return false;
					std::unique_lock<std::mutex> LwriteLock(writeLock);
					auto fres = std::find_if(m.begin(), m.end(), std::bind(&Pool::compare, this, std::placeholders::_1, input));

					if (fres == m.end())
						return false;

					removeTime++;
					fres->get_deleter()(fres->release());
					return true;
				}

				void clean() {
					if (this == nullptr) return;
					std::unique_lock<std::mutex> LwriteLock(writeLock);
					realClean();
					removeTime.store(0, std::memory_order_release);
				}

				inline void clear() {
					if (this == nullptr) return;
					std::unique_lock<std::mutex> LwriteLock(writeLock);
					m.clear();
					removeTime.store(0, std::memory_order_release);
				}

			private:
				void realClean() {
					for (auto i = m.begin(); i != m.end();) {
						if (!(*i)) { i = m.erase(i); continue; }
						i++;
					}
				}

				inline void Cleanif() {
					if (removeTime.load(std::memory_order_acquire) >= cleanLimit.load(std::memory_order_acquire)) {
						clean();
					}
				}

				inline bool compare(const std::unique_ptr<T, deleter>& target, const T* const value) {
					if (target.get() == value) return true;
					return false;
				}

			private:
				std::vector<std::unique_ptr<T, deleter>> m;
				std::mutex writeLock;
				std::atomic_int32_t removeTime, cleanLimit;
			};

			template <typename T>
			class Pool<T, void> {
				friend class Server;
			public:
				Pool() :removeTime(0), cleanLimit(1) {}

			public:
				inline void setCleanLimit(const int32_t& input) {
					if (this == nullptr) return;
					if (input < 1) { cleanLimit.store(1, std::memory_order_release); return; }
					cleanLimit.store(input, std::memory_order_release);
				}

				template <typename ...U>
				inline T* make(U&& ...args)noexcept {
					if (this == nullptr) return nullptr;
					Cleanif();
					std::unique_lock<std::mutex> LwriteLock(writeLock);
					try {
						m.emplace_back(new T(std::forward<U>(args)...));
					}
					catch (...) {
						return nullptr;
					}
					return m.rbegin()->get();
				}

				template <typename ...U>
				inline T* put(U&& ...args)noexcept {//only unique_ptr
					if (this == nullptr) return nullptr;
					Cleanif();
					std::unique_lock<std::mutex> LwriteLock(writeLock);
					try {
						m.emplace_back(std::forward<U>(args)...);
					}
					catch (...) {
						return nullptr;
					}
					return m.rbegin()->get();
				}

				T* release(const T *input) {
					if (this == nullptr) return nullptr;
					std::unique_lock<std::mutex> LwriteLock(writeLock);
					for (auto i = m.begin(); i != m.end(); i++) {
						if (*input == *(i->get())) {
							removeTime++;
							return i->release();
						}
					}
					return nullptr;
				}

				bool drop(const T *input) {
					if (this == nullptr) return false;
					std::unique_lock<std::mutex> LwriteLock(writeLock);
					auto fres = std::find_if(m.begin(), m.end(), std::bind(&Pool::compare, this, std::placeholders::_1, input));

					if (fres == m.end())
						return false;

					removeTime++;
					fres->get_deleter()(fres->release());
					return true;
				}

				void clean() {
					if (this == nullptr) return;
					std::unique_lock<std::mutex> LwriteLock(writeLock);
					realClean();
					removeTime.store(0, std::memory_order_release);
				}

				inline void clear() {
					if (this == nullptr) return;
					std::unique_lock<std::mutex> LwriteLock(writeLock);
					m.clear();
					removeTime.store(0, std::memory_order_release);
				}

			private:
				void realClean() {
					for (auto i = m.begin(); i != m.end();) {
						if (!(*i)) { i = m.erase(i); continue; }
						i++;
					}
				}

				inline void Cleanif() {
					if (removeTime.load(std::memory_order_acquire) >= cleanLimit.load(std::memory_order_acquire)) {
						clean();
					}
				}

				inline bool compare(const std::unique_ptr<T>& target, const T* const value) {
					if (target.get() == value) return true;
					return false;
				}

				bool drop_nolock(const T *input) {
					auto fres = std::find_if(m.begin(), m.end(), std::bind(&Pool::compare, this, std::placeholders::_1, input));

					if (fres == m.end())
						return false;

					removeTime++;
					fres->get_deleter()(fres->release());
					return true;
				}

			private:
				std::vector<std::unique_ptr<T>> m;
				std::mutex writeLock;
				std::atomic_int32_t removeTime, cleanLimit;
			};

		}

		class PerSocketContext :public IOCPSpace::Pool<PerIOContext, IOCPSpace::PICdeleter> {
		public:
			PerSocketContext(const int32_t& PoolCleanLimit) {
				setCleanLimit(PoolCleanLimit);
				m_sock = INVALID_SOCKET;
				timer.start();
			}

			~PerSocketContext() {
				CloseSock();
			}

		public:
			bool operator==(const PerSocketContext& input)const {
				if (input.uniqueid == uniqueid) return true;
				return false;
			}

		public:
			inline void CloseSock() {
				if (m_sock != INVALID_SOCKET) {
					closesocket(m_sock);
					m_sock = INVALID_SOCKET;
				}
			}

			inline time_t getTimerSeconds() {
				std::unique_lock<std::mutex> LtimerM(timerM);
				return timer.getsecond();
			}

		public:
			SOCKET m_sock;
			SOCKADDR_IN m_clientAddr;
			IOCPSpace::unique_id uniqueid;
		private:
			std::mutex timerM;
			DC::timer timer;
		};

		using PICptr = std::unique_ptr<PerIOContext, IOCPSpace::PICdeleter>;
		using PSCptr = std::unique_ptr<PerSocketContext>;

		class Server {
		public:
			Server(const std::size_t& inputThreadNumber) :TP(nullptr), ThreadNumber(inputThreadNumber), m_listen(INVALID_SOCKET), m_iocp(nullptr), PoolCleanLimit(1), CleanTimeInterval(1), stopFlag(true) {
				//以下是默认设置
				SetMemoryPoolCleanLimit(20);//内存池待清理数量上限
				SetCleanTimeInterval(60);//清理线程唤醒间隔(秒)
				SetConnectionMaxLiveTime(30);//客户端连接最长存活时间(秒)
			}

			Server(const Server&) = delete;

			virtual ~Server()noexcept {
				try {
					Stop();
				}
				catch (...) {}//宁愿内存泄漏也不要未定义行为
			}

		public:
			static inline bool LoadSocketLib()noexcept {
				try {
					return WinSock::Startup(2, 2);
				}
				catch (...) { return false; }
			}

			static inline void UnloadSocketLib()noexcept {
				try {
					WinSock::Cleanup();
				}
				catch (...) {}
			}

			inline void SetListenAddr(const WinSock::Address& input)noexcept {
				bindAddr = input;
			}

			inline void SetListenAddr(const std::string& ip, const int32_t& port)noexcept {
				bindAddr = WinSock::MakeAddr(ip, port);
			}

			inline void SetMemoryPoolCleanLimit(const int32_t& input) {
				PoolCleanLimit.store(input, std::memory_order_release);
				PSC_Pool.setCleanLimit(input);
			}

			inline void SetCleanTimeInterval(const int32_t& input) {
				CleanTimeInterval.store(input, std::memory_order_release);
			}

			inline void SetConnectionMaxLiveTime(const int32_t& input) {
				ConnectionTimeOut.store(input, std::memory_order_release);
			}

			bool Start(int32_t waitlimit = 1) {//可以多次调用，不会出现错误
				try {
					Stop();
				}
				catch (...) { return false; }

				if (waitlimit < 1) waitlimit = 1;

				if (isNull(m_iocp))
					m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
				if (isNull(m_iocp)) return false;

				if (isNull(TP))
					TP = new DC::ThreadPool(ThreadNumber + 2);//cleaner和listener
				TP->start();

				auto wait = TP->async(&Server::ListenerThread, this);
				if (wait.wait_for(std::chrono::seconds(waitlimit)) != std::future_status::timeout)
					return false;

				TP->async(&Server::CleanerThread, this, std::chrono::seconds(CleanTimeInterval.load(std::memory_order_acquire)));

				for (std::size_t i = 0; i < ThreadNumber; i++)
					TP->async(&Server::WorkerThread, this);

				stopFlag.store(false, std::memory_order_release);
				return true;
			}

			inline void Stop() {//可以多次调用，不会出现错误
                //向工作线程发送停止信号
				std::vector<DC::IOCP::PICptr> temp;
				for (int i = 0; i < ThreadNumber; i++)
					temp.emplace_back(new(std::nothrow) DC::IOCP::PerIOContext(1));
				for (auto& p : temp)
					PostExit(p.get());

				//停止监听
				if (m_listen != INVALID_SOCKET)
					closesocket(m_listen);

				//向清洁线程发送停止信号
				stopFlag.store(true, std::memory_order_release);
				cleanerCV.notify_all();

				//关闭完成端口
				if (!isNull(m_iocp)) { CloseHandle(m_iocp); m_iocp = nullptr; }

				//关闭并删除线程池
				if (!isNull(TP)) { delete TP; TP = nullptr; }

				//清空客户端SOCKET
				PSC_Pool.clear();
			}

			static inline void loop()noexcept {//永远不会返回
				while (true)
					std::this_thread::sleep_for(std::chrono::minutes(9710));
			}

		protected:
			virtual inline const std::size_t getRecvBufferSize()noexcept {//recv 缓冲区
				return 1024;
			}

			virtual void OnRecv(DC::IOCP::PerSocketContext*, const std::string&, const DC::WinSock::Address&) {}

			virtual void OnSend(DC::IOCP::PerSocketContext*, const std::string&, const DC::WinSock::Address&) {}

			virtual void OnError(const DC::DC_ERROR&) {}

			inline bool Send(PerSocketContext *client, const std::string& sendthis)noexcept {
				return PostSend(client, client->make(sendthis.size() + 1), sendthis);
			}

		private:
			bool PostRecv(PerSocketContext *PSC, PerIOContext *PIC)noexcept {
				if (isNull(PSC) || isNull(PIC)) return false;

				DWORD dwFlags = 0;
				DWORD dwBytes = 0;

				PIC->resetBuffer();
				PIC->m_opType = RECV_POSTED;

				int nBytesRecv = WSARecv(PSC->m_sock, &PIC->m_wsabuf, 1, &dwBytes, &dwFlags, &PIC->m_overlapped, NULL);

				if (SOCKET_ERROR == nBytesRecv) {
					if (WSA_IO_PENDING != WSAGetLastError())
						return false;
				}
				return true;
			}

			inline bool DoRecv(PerSocketContext *PSC, PerIOContext *PIC)noexcept {
				if (isNull(PSC) || isNull(PIC)) return false;

				try {
					OnRecv(PSC, PIC->m_wsabuf.buf, PSC->m_clientAddr);
				}
				catch (const DC::DC_ERROR& err) {
					this->OnError(err);
				}
				catch (...) {
					this->OnError(DC::DC_ERROR("OnRecv", "uncaught exception", -1));
				}

				return PostRecv(PSC, PIC);
			}

			bool PostSend(PerSocketContext *PSC, PerIOContext *PIC, const std::string& sendthis)noexcept {
				if (isNull(PSC) || isNull(PIC)) return false;

				DWORD dwFlags = 0;
				DWORD dwBytes = sendthis.size();

				if (sendthis.size() > PIC->m_wsabuf.len) return false;

				PIC->resetBuffer();
				memcpy(PIC->m_wsabuf.buf, sendthis.c_str(), sendthis.size());
				PIC->m_opType = SEND_POSTED;

				int nBytesRecv = WSASend(PSC->m_sock, &PIC->m_wsabuf, 1, &dwBytes, dwFlags, &PIC->m_overlapped, NULL);

				if (SOCKET_ERROR == nBytesRecv) {
					if (WSA_IO_PENDING != WSAGetLastError())
						return false;
				}
				return true;
			}

			inline bool DoSend(PerSocketContext *PSC, PerIOContext *PIC)noexcept {
				if (isNull(PSC) || isNull(PIC)) return false;

				try {
					OnSend(PSC, PIC->m_wsabuf.buf, PSC->m_clientAddr);
					PSC->drop(PIC);
				}
				catch (const DC::DC_ERROR& err) {
					this->OnError(err);
				}
				catch (...) {
					this->OnError(DC::DC_ERROR("OnSend", "uncaught exception", -1));
				}
				return true;
			}

			inline bool PostExit(PerIOContext *PIC)noexcept {
				if (isNull(PIC)) return false;

				PerIOContext *AcceptIoContext = PIC;
				DWORD dwBytes = 0;
				AcceptIoContext->m_opType = OperationType::NULL_POSTED;

				if (0 != PostQueuedCompletionStatus(m_iocp, dwBytes, reinterpret_cast<ULONG_PTR>(AcceptIoContext), &AcceptIoContext->m_overlapped)) return true;
				return false;
			}

			void WorkerThread()noexcept {
				DWORD dwBytesTransfered = 0;
				OVERLAPPED *pOverlapped = NULL;
				PerSocketContext *pSocketContext = NULL;

				while (true) {
					BOOL GQrv = GetQueuedCompletionStatus(m_iocp, &dwBytesTransfered,
						reinterpret_cast<PULONG_PTR>(&pSocketContext), &pOverlapped, INFINITE);

					//PSC为空(一般意味着完成端口已关闭)
					if (isNull(pSocketContext)) break;
					//完成端口已关闭
					if (isNull(m_iocp)) break;
					//遇到了错误
					if (!GQrv) continue;

					PerIOContext *pIOContext = CONTAINING_RECORD(pOverlapped, PerIOContext, m_overlapped);

					//客户端已经断开连接
					if (dwBytesTransfered == 0 && (pIOContext->m_opType == OperationType::SEND_POSTED || pIOContext->m_opType == OperationType::RECV_POSTED)) {
						pSocketContext->CloseSock();
						continue;
					}

					switch (pIOContext->m_opType) {
					case OperationType::RECV_POSTED: {
						DoRecv(pSocketContext, pIOContext);
					}break;
					case OperationType::SEND_POSTED: {
						DoSend(pSocketContext, pIOContext);
					}break;
					case OperationType::NULL_POSTED: {
						return; 
					}break;
					}
				}
			}

			bool ListenerThread()noexcept {
				WinSock::SocketInit_TCP(m_listen);
				if (m_listen == SOCKET_ERROR) return false;

				if (!WinSock::Bind(m_listen, bindAddr)) { closesocket(m_listen); return false; }
				if (!WinSock::Listen(m_listen, SOMAXCONN)) { closesocket(m_listen); return false; }

				SOCKET acceptSocket;
				DC::WinSock::Address ClientAddr;

				while (true) {
					acceptSocket = INVALID_SOCKET;
					if (!DC::WinSock::Accept(acceptSocket, m_listen, ClientAddr)) { closesocket(m_listen); return false; }

					PSCptr ptr(new(std::nothrow) PerSocketContext(PoolCleanLimit.load(std::memory_order_acquire)));
					if (!ptr) { closesocket(acceptSocket); continue; }//new失败
					ptr->m_sock = acceptSocket;
					ptr->m_clientAddr = ClientAddr;
					if (!AssociateWithIOCP(ptr.get())) { closesocket(acceptSocket); continue; }//绑定到完成端口失败
					if (!PostRecv(ptr.get(), ptr->make(getRecvBufferSize()))) { closesocket(acceptSocket); continue; }//PostRecv失败
					if (nullptr == PSC_Pool.put(std::move(ptr))) { closesocket(acceptSocket); continue; }//记录客户端失败
				}

				return true;
			}

			void CleanerThread(const std::chrono::seconds limits)noexcept {
				try {
					DC::timer timer;
					std::chrono::seconds templimits;
					while (true) {
						templimits = limits;
						timer.reset();
						timer.start();
						std::unique_lock<std::mutex> lock(cleanerMut);
						while (true) {
							if (cleanerCV.wait_for(lock, templimits) == std::cv_status::timeout)
								if (stopFlag.load(std::memory_order_acquire) == true)
									return;//退出信号
								else
									break;//时间到，开始工作

							timer.stop();

							if (stopFlag.load(std::memory_order_acquire) == true)
								return;//退出信号

							if (std::chrono::seconds(timer.getsecond()) >= templimits)
								break;//睡过头了

							//假唤醒，计算剩余时间开始新一轮等待
							templimits = templimits - std::chrono::seconds(timer.getsecond());
							timer.reset();
							timer.start();
						}

						std::unique_lock<std::mutex> lockPSC(PSC_Pool.writeLock);
						for (auto& p : PSC_Pool.m) {
							if (p.get() == nullptr) continue;
							if (p->getTimerSeconds() >= ConnectionTimeOut.load(std::memory_order_acquire)) {
								p->CloseSock();
								PSC_Pool.drop_nolock(p.get());
							}
						}
						lockPSC.unlock();
						PSC_Pool.clean();
					}
				}
				//抛异常的话直接退出
				catch (const DC_ERROR& err) {
					OnError(err);
					return;
				}
				catch (...) {
					OnError(DC::DC_ERROR("CleanerThread", "uncaught exception", -1));
					return; 
				}
			}

			inline bool AssociateWithIOCP(PerSocketContext* input)noexcept {
				if (isNull(input)) return false;

				HANDLE hTemp = CreateIoCompletionPort(reinterpret_cast<HANDLE>(input->m_sock), m_iocp, reinterpret_cast<DWORD>(input), 0);

				if (NULL == hTemp)
					return false;
				return true;
			}

			template <typename T>
			inline bool isNull(T ptr)noexcept {
				if (ptr == nullptr || ptr == NULL) return true;
				return false;
			}

		private:
			HANDLE m_iocp;
			SOCKET m_listen;
			ThreadPool *TP;
			const std::size_t ThreadNumber;
			std::atomic_int32_t PoolCleanLimit, CleanTimeInterval, ConnectionTimeOut;//内存池待清理数量上限，检查客户端连接是否超时间隔，客户端连接最长存活时间
			std::atomic_bool stopFlag;
			std::condition_variable cleanerCV;
			std::mutex cleanerMut;

			WinSock::Address bindAddr;

			IOCPSpace::Pool<PerSocketContext> PSC_Pool;
		};

	}

}

#endif