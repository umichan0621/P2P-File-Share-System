#include "net_receiver_win.h"
#include <string.h>
#include <ws2tcpip.h>
#include <base/config.hpp>
#include <base/buffer_pool.hpp>
#include <base/logger/logger.h>

#pragma warning(disable:4244)

namespace net
{
	IoData::IoData() :
		IoType(IO_OPERATION_TYPE::NULL_POSTED)
	{
		memset(&OverLapped, 0, sizeof(OverLapped));
		WsaBuf.buf = g_pBufferPoolMgr->allocate();
		WsaBuf.len = MIN_MTU;
	}

	IoData::~IoData()
	{
		if (WsaBuf.buf != nullptr)
		{
			g_pBufferPoolMgr->release(WsaBuf.buf);
			WsaBuf.buf = nullptr;
		}
	}

	NetReceiverWin::NetReceiverWin() :
		m_ListenFd(0),
		m_ListenFd6(0),
		m_ListenFdNAT(0),
		m_AddrLen6(26),
		m_ThreadNum(0),
		m_pThreadPool(nullptr),
		m_Iocp(INVALID_HANDLE_VALUE),
		m_StopEv(CreateEvent(NULL, TRUE, FALSE, NULL)) {}

	NetReceiverWin::~NetReceiverWin()
	{
		closesocket(m_ListenFd);
		if (0 != m_ListenFd6)
		{
			closesocket(m_ListenFd6);
		}
		closesocket(m_ListenFdNAT);
		_stop();
		CloseHandle(m_Iocp);
		CloseHandle(m_StopEv);
		WSACleanup();
	}

	bool NetReceiverWin::init(uint16_t MaxConnectionNum, uint16_t Port, uint16_t Port6)
	{
		//windows网络初始化
		WSADATA WsaData;
		if (0 != WSAStartup((MAKEWORD(2, 2)), &WsaData))
		{
			LOG_ERROR << "WSA init failed";
			return false;
		}
		//启动IPv4端口
		if (false == _init_socket(Port))
		{
			LOG_ERROR << "socket init failed";
			return false;
		}
		LOG_TRACE << "UDP IPv4 init successfully, port:" << Port;
		//生成随机的NAT端口，直到当前随机端口可用
		srand(time(NULL));
		int PortNAT = 10000 + rand() % (0xffff - 10000);
		bool Res = _init_socket_nat(PortNAT);
		while (false == Res)
		{
			PortNAT = 10000 + rand() % (0xffff - 10000);
			Res = _init_socket_nat(PortNAT);
		}
		LOG_TRACE << "UDP NAT init successfully, port:" << PortNAT;
		//允许使用IPv6
		if (Port6 > 0)
		{
			if (false == _init_socket6(Port6))
			{
				LOG_ERROR << "socket init failed";
				return false;
			}
			LOG_TRACE << "UDP IPv6 init successfully, port:" << Port6;
		}

		if (false == _init_completionport())
		{
			LOG_ERROR << "completionport init failed";
			return false;
		}
		return true;
	}

	void NetReceiverWin::listen(ThreadPool* pThreadPool)
	{
		m_pThreadPool = pThreadPool;
		SYSTEM_INFO SysInfo;
		GetSystemInfo(&SysInfo);
		//创建和CPU核心数一致的工作线程
		m_ThreadNum = SysInfo.dwNumberOfProcessors;
		for (uint16_t i = 0; i < m_ThreadNum; ++i)
		{
			m_pThreadPool->add_task(std::bind(&NetReceiverWin::_worker_thread, this));
		}
		//初始化IO对象池，预创建工作线程数*4的对象
		m_IoDataPool.init(10 * m_ThreadNum);
		//用IoDataPool预先创建线程数4倍Recv的IoData
		//然后按IPv4:IPv6=3:1的比例投递出去
		IoData* pIoData = nullptr;
		for (uint16_t i = 0; i < 9 * m_ThreadNum; ++i)
		{
			pIoData = m_IoDataPool.allocate();
			//IoData，recv的只用来recv，只在这里初始化
			pIoData->IoType = IO_OPERATION_TYPE::RECV_POSTED;
			sockaddr* pSockaddr = (sockaddr*)&pIoData->PeerAddr;
			pSockaddr->sa_family = AF_INET;
			_post_recv(pIoData);
		}
		for (uint16_t i = 0; i < m_ThreadNum; ++i)
		{
			pIoData = m_IoDataPool.allocate();
			//IoData，recv的只用来recv，只在这里初始化
			pIoData->IoType = IO_OPERATION_TYPE::RECV_POSTED;
			sockaddr* pSockaddr = (sockaddr*)&pIoData->PeerAddr;
			pSockaddr->sa_family = AF_INET6;
			_post_recv(pIoData);
		}
	}

	bool NetReceiverWin::_init_socket(uint16_t Port)
	{
		sockaddr_in Sockaddr = { 0 };
		Sockaddr.sin_family = AF_INET;
		Sockaddr.sin_port = ::htons(Port);
		Sockaddr.sin_addr.S_un.S_addr = ADDR_ANY;
		//创建socket IPv4
		m_ListenFd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (SOCKET_ERROR == m_ListenFd)
		{
			LOG_ERROR << "create socket error!";
			return false;
		}
		u_long mode = 1;
		if (SOCKET_ERROR == ::ioctlsocket(m_ListenFd, FIONBIO, &mode))
		{
			closesocket(m_ListenFd);
			LOG_ERROR << "set ioctl error!";
			return false;
		}

		int32_t BufSize = 1024 * 1024;//缓存设定为1M
		if (SOCKET_ERROR == ::setsockopt(m_ListenFd, SOL_SOCKET, SO_RCVBUF, (char*)&BufSize, sizeof(BufSize)))
		{
			closesocket(m_ListenFd);
			LOG_ERROR << "set recvbuf error!";
			return false;
		}

		if (SOCKET_ERROR == ::bind(m_ListenFd, (sockaddr*)&Sockaddr, sizeof(sockaddr_in)))
		{
			closesocket(m_ListenFd);
			LOG_ERROR << "bind socket error!";
			return false;
		}
		return true;
	}

	bool NetReceiverWin::_init_socket6(uint16_t Port6)
	{
		sockaddr_in6 Sockaddr = { 0 };
		Sockaddr.sin6_family = AF_INET6;
		Sockaddr.sin6_port = ::htons(Port6);
		inet_pton(AF_INET6, "0:0:0:0:0:0:0:0", (sockaddr_in6*)&Sockaddr.sin6_addr);
		//创建socket IPv6
		m_ListenFd6 = ::socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
		if (SOCKET_ERROR == m_ListenFd6)
		{
			LOG_ERROR << "create socket error!";
			return false;
		}
		int32_t BufSize = 1024 * 1024;//缓存设定为1M
		if (SOCKET_ERROR == ::setsockopt(m_ListenFd6, SOL_SOCKET, SO_RCVBUF, (char*)&BufSize, sizeof(BufSize)))
		{
			closesocket(m_ListenFd6);
			LOG_ERROR << "set recvbuf error!";
			return false;
		}
		if (SOCKET_ERROR == ::setsockopt(m_ListenFd6, SOL_SOCKET, SO_SNDBUF, (char*)&BufSize, sizeof(BufSize)))
		{
			closesocket(m_ListenFd6);
			LOG_ERROR << "set sendbuf error!";
			return false;
		}
		if (SOCKET_ERROR == ::bind(m_ListenFd6, (sockaddr*)&Sockaddr, sizeof(sockaddr_in6)))
		{
			closesocket(m_ListenFd6);
			LOG_ERROR << "bind socket error!";
			return false;
		}
		return true;
	}

	bool NetReceiverWin::_init_socket_nat(uint16_t PortNAT)
	{
		sockaddr_in Sockaddr = { 0 };
		Sockaddr.sin_family = AF_INET;
		Sockaddr.sin_port = ::htons(PortNAT);
		Sockaddr.sin_addr.S_un.S_addr = ADDR_ANY;
		//创建socket IPv4
		m_ListenFdNAT = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (SOCKET_ERROR == m_ListenFdNAT)
		{
			return false;
		}
		if (SOCKET_ERROR == ::bind(m_ListenFdNAT, (sockaddr*)&Sockaddr, sizeof(sockaddr_in)))
		{
			closesocket(m_ListenFdNAT);
			return false;
		}
		return true;
	}

	bool NetReceiverWin::_init_completionport(bool bIPv6)
	{
		m_Iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		if (!m_Iocp)
		{
			LOG_ERROR << "CreateIoCompletionPort failed";
			return false;
		}

		CreateIoCompletionPort((HANDLE)m_ListenFd, m_Iocp, (DWORD)m_ListenFd, 0);

		//启动IPv6
		if (true==bIPv6)
		{
			CreateIoCompletionPort((HANDLE)m_ListenFd6, m_Iocp, (DWORD)m_ListenFd6, 0);
		}
		return true;
	}

	void NetReceiverWin::_post_recv(IoData* pIoData)
	{
		DWORD Flags = 0, RecvLen = 0;
		sockaddr* pSockaddr = (sockaddr*)&pIoData->PeerAddr;
		SOCKET RecvFd = (pSockaddr->sa_family == AF_INET ? m_ListenFd : m_ListenFd6);
		if (SOCKET_ERROR == WSARecvFrom(
			RecvFd, &(pIoData->WsaBuf), 1, &RecvLen, &Flags,
			pSockaddr, &m_AddrLen6, &pIoData->OverLapped, NULL))
		{
			//if (ERROR_IO_PENDING != WSAGetLastError())
			//{
				//LOG_ERROR << "WSARecvFrom() failed with error " << WSAGetLastError();
			//}
		}
	}

	void NetReceiverWin::_post_send()
	{
		//::WSASendTo()
	}

	void NetReceiverWin::_gateway(const PeerAddress PeerAddr, char* pMessage, uint16_t Len)
	{
		//触发gateway的回调，返回true才允许建立连接
		if (true == on_gateway(PeerAddr, pMessage, Len))
		{
			uint16_t SessionId = g_pPeerManager->connect_peer(PeerAddr);
			on_accept(SessionId, PeerAddr);
		}
		g_pBufferPoolMgr->release(pMessage);
	}

	void NetReceiverWin::_recv(uint16_t SessionId, char* pMessage, uint16_t Len)
	{
		on_recv(SessionId, pMessage, Len);
		g_pBufferPoolMgr->release(pMessage);
	}

	void NetReceiverWin::_stop()
	{
		//激活关闭事件
		SetEvent(m_StopEv);
		for (uint16_t i = 0; i < m_ThreadNum; ++i)
		{
			//通知所有完成端口退出
			PostQueuedCompletionStatus(m_Iocp, 0, (DWORD)(-1), NULL);
		}
	}

	void NetReceiverWin::_worker_thread()
	{
		IoData* pIoData = nullptr;
		OVERLAPPED* pOverLapped = nullptr;
		DWORD Len;

		while (WAIT_OBJECT_0 != WaitForSingleObject(m_StopEv, 0))
		{
			//发送或接受发生问题
			if (false == GetQueuedCompletionStatus(m_Iocp, &Len, (PULONG_PTR)&pIoData, &pOverLapped, INFINITE))
			{
				//LOG_ERROR << "GetQueuedCompletionStatus failed with error:" << GetLastError();
				pIoData = m_IoDataPool.allocate();
				pIoData->IoType = IO_OPERATION_TYPE::RECV_POSTED;
				sockaddr* pSockaddr = (sockaddr*)&pIoData->PeerAddr;
				pSockaddr->sa_family = AF_INET;
				_post_recv(pIoData);
			}
			pIoData = CONTAINING_RECORD(pOverLapped, IoData, OverLapped);
			//为空指针，表示停止循环并退出
			if (nullptr == pIoData)
			{
				continue;
			}
			
			//读消息
			if (pIoData->IoType == IO_OPERATION_TYPE::RECV_POSTED)
			{
				//收到消息，处理
				uint16_t SessionId = g_pPeerManager->session_id(pIoData->PeerAddr);

				if (ERROR_SESSION_ID != SessionId)
				{
					char* pMessage = pIoData->WsaBuf.buf;
					pIoData->WsaBuf.buf = g_pBufferPoolMgr->allocate();
					//新的连接
					if (0 == SessionId)
					{
						m_pThreadPool->add_task(std::bind(&NetReceiverWin::_gateway, 
							this, pIoData->PeerAddr, pMessage, static_cast<uint16_t>(Len)));
					}
					//触发recv回调
					else
					{
						m_pThreadPool->add_task(std::bind(&NetReceiverWin::_recv,
							this,SessionId, pMessage, static_cast<uint16_t>(Len)));
					}
				}
				//用post_recv重新投递出去
				_post_recv(pIoData);
			}
		}
		LOG_TRACE << "Worker Thread Stopped";
	}
}