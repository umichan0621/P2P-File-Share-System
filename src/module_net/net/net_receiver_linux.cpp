#include "net_receiver_linux.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <netdb.h>
#include <base/config.hpp>
#include <base/logger/logger.h>
#include <module_peer/peer_manager.h>

#define SOCKET_ERROR -1
constexpr int32_t MAX_EVENTS = 128;			//每次能处理的事件数
constexpr int32_t FD_SIZE = 1024;			//epoll描述符上限

namespace net
{
	NetReceiverLinux::NetReceiverLinux() :
		m_ListenFd(SOCKET_ERROR),
		m_ListenFd6(SOCKET_ERROR),
		m_ListenFdNAT(SOCKET_ERROR),
		m_AddrLen6(26),
		m_EpollFd(SOCKET_ERROR){}

	NetReceiverLinux::~NetReceiverLinux()
	{
		close(m_ListenFd);
		if (0 != m_ListenFd6)
		{
			close(m_ListenFd6);
		}
		close(m_ListenFdNAT);
		close(m_ListenFd);
		close(m_EpollFd);
	}

	bool NetReceiverLinux::_init_epoll(bool bIPv6)
	{
		//创建epoll
		m_EpollFd = epoll_create(FD_SIZE);
		if (-1 == m_EpollFd)
		{
			LOG_ERROR << "create listen epoll failed";
			return false;
		}
		//添加epoll监听事件 IPv4
		struct epoll_event stListener = { 0 };
		stListener.events = EPOLLIN;
		stListener.data.fd = m_ListenFd;
		/*
		//如果是ET模式，设置EPOLLET
		ev.events |= EPOLLET;
		//设置是否阻塞
		int flags = fcntl(fd, F_GETFL);
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
		*/
		if (-1 == epoll_ctl(m_EpollFd, EPOLL_CTL_ADD, m_ListenFd, &stListener))
		{
			LOG_ERROR << "add listen epoll failed";
			return false;
		}
		if (true==bIPv6)
		{
			//添加epoll监听事件 IPv6
			struct epoll_event  stListener6 = { 0 };
			stListener6.events = EPOLLIN;
			stListener6.data.fd = m_ListenFd6;

			if (-1 == epoll_ctl(m_EpollFd, EPOLL_CTL_ADD,m_ListenFd6, &stListener6))
			{
				LOG_ERROR << "add listen6 epoll failed";
				return false;
			}
		}
		return true;
	}

	bool NetReceiverLinux::init(uint16_t MaxConnectionNum, uint16_t Port, uint16_t Port6)
	{
		if (false == _init_socket(Port))
		{
			LOG_ERROR << "socket init failed";
			return false;
		}
		LOG_TRACE << "UDP IPv4 init successfully, port:" << Port;
		//生成随机的NAT端口，直到当前随机端口可用
		srand(static_cast<uint32_t>(time(NULL)));
		uint16_t PortNAT = static_cast <uint16_t>(10000 + rand() % (0xffff - 10000));
		bool Res = _init_socket_nat(PortNAT);
		while (false == Res)
		{
			PortNAT = static_cast <uint16_t>(10000 + rand() % (0xffff - 10000));
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
		if (false == _init_epoll())
		{
			LOG_ERROR << "epoll init failed";
			return false;
		}
		return true;
	}

	void NetReceiverLinux::listen(ThreadPool* pThreadPool)
	{
		/*工作模式：一个Reactor线程循环运行
		* Reactor线程接收消息，生成任务
		* accept任务和recv任务加入线程池
		* 加入任务后立刻返回Reactor接受下一个消息
		* 禁止加入死循环任务或长时间任务占用工作线程
		* 其他工作线程执行任务*/
		//获取外部线程池对象
		m_pThreadPool = pThreadPool;
		//预分配128*548的缓存
		m_BufPool.init(128, MIN_MTU);
		//添加Epoll循环任务
		m_pThreadPool->add_task(std::bind(&NetReceiverLinux::_reactor, this));
	}

	bool NetReceiverLinux::_init_socket(uint16_t Port)
	{
		addrinfo Hints = { 0 };
		addrinfo* pListenAddr = new addrinfo();
		Hints.ai_flags = AI_PASSIVE;
		Hints.ai_socktype = SOCK_DGRAM;		//UDP
		Hints.ai_family = AF_INET;			//IPv4
		if (0 != ::getaddrinfo(NULL, std::to_string(Port).c_str(), &Hints, &pListenAddr))
		{
			delete pListenAddr;
			LOG_ERROR << "getaddrinfo failed";
			return false;
		}
		//创建socket IPv4
		m_ListenFd = ::socket(pListenAddr->ai_family, pListenAddr->ai_socktype, pListenAddr->ai_protocol);
		if (SOCKET_ERROR == m_ListenFd)
		{
			delete pListenAddr;
			LOG_ERROR << "create socket error!";
			return false;
		}
		int32_t BufSize = 1024 * 1024;//缓存设定为1M
		if (SOCKET_ERROR == ::setsockopt(m_ListenFd, SOL_SOCKET, SO_RCVBUF, (char*)&BufSize, sizeof(BufSize)))
		{
			delete pListenAddr;
			close(m_ListenFd);
			LOG_ERROR << "set recvbuf error!";
			return false;
		}
		if (SOCKET_ERROR == ::setsockopt(m_ListenFd, SOL_SOCKET, SO_SNDBUF, (char*)&BufSize, sizeof(BufSize)))
		{
			delete pListenAddr;
			close(m_ListenFd);
			LOG_ERROR << "set sendbuf error!";
			return false;
		}
		//绑定IP地址端口信息
		if (SOCKET_ERROR == ::bind(m_ListenFd, pListenAddr->ai_addr, pListenAddr->ai_addrlen))
		{
			delete pListenAddr;
			close(m_ListenFd);
			LOG_ERROR << "bind socket error!";
			return false;
		}
		delete pListenAddr;
		return true;
	}

	bool NetReceiverLinux::_init_socket6(uint16_t Port6)
	{
		addrinfo Hints = { 0 };
		addrinfo* pListenAddr = new addrinfo();
		Hints.ai_flags = AI_PASSIVE;
		Hints.ai_socktype = SOCK_DGRAM;		//UDP
		Hints.ai_family = AF_INET6;			//IPv6
		if (0 != ::getaddrinfo(NULL, std::to_string(Port6).c_str(), &Hints, &pListenAddr))
		{
			delete pListenAddr;
			LOG_ERROR << "getaddrinfo failed";
		}
		//创建socket IPv6
		m_ListenFd6 = ::socket(pListenAddr->ai_family, pListenAddr->ai_socktype, pListenAddr->ai_protocol);
		if (SOCKET_ERROR == m_ListenFd6)
		{
			delete pListenAddr;
			LOG_ERROR << "create socket6 error!";
			return false;
		}
		int32_t BufSize = 1024 * 1024;//缓存设定为1M
		if (SOCKET_ERROR == ::setsockopt(m_ListenFd6, SOL_SOCKET, SO_RCVBUF, (char*)&BufSize, sizeof(BufSize)))
		{
			delete pListenAddr;
			close(m_ListenFd6);
			LOG_ERROR << "set recvbuf error!";
			return false;
		}
		if (SOCKET_ERROR == ::setsockopt(m_ListenFd6, SOL_SOCKET, SO_SNDBUF, (char*)&BufSize, sizeof(BufSize)))
		{
			delete pListenAddr;
			close(m_ListenFd6);
			LOG_ERROR << "set sendbuf error!";
			return false;
		}
		int ipv6only = 1;
		if (SOCKET_ERROR == setsockopt(m_ListenFd6, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&ipv6only, sizeof(ipv6only)))
		{
			delete pListenAddr;
			close(m_ListenFd6);
			LOG_ERROR << "set ipv6only failed!";
			return false;
		}
		//绑定IP地址端口信息
		if (SOCKET_ERROR == ::bind(m_ListenFd6, pListenAddr->ai_addr, pListenAddr->ai_addrlen))
		{
			delete pListenAddr;
			close(m_ListenFd6);
			LOG_ERROR << "bind socket6 error!";
			return false;
		}
		delete pListenAddr;
		return true;
	}

	bool NetReceiverLinux::_init_socket_nat(uint16_t PortNAT)
	{
		addrinfo Hints = { 0 };
		addrinfo* pListenAddr = new addrinfo();
		Hints.ai_flags = AI_PASSIVE;
		Hints.ai_socktype = SOCK_DGRAM;		//UDP
		Hints.ai_family = AF_INET;			//IPv4
		if (0 != ::getaddrinfo(NULL, std::to_string(PortNAT).c_str(), &Hints, &pListenAddr))
		{
			delete pListenAddr;
			return false;
		}
		//创建socket IPv4
		m_ListenFdNAT = ::socket(pListenAddr->ai_family, pListenAddr->ai_socktype, pListenAddr->ai_protocol);
		if (SOCKET_ERROR == m_ListenFdNAT)
		{
			delete pListenAddr;
			return false;
		}

		//绑定IP地址端口信息
		if (SOCKET_ERROR == ::bind(m_ListenFdNAT, pListenAddr->ai_addr, pListenAddr->ai_addrlen))
		{
			delete pListenAddr;
			close(m_ListenFdNAT);
			return false;
		}
		delete pListenAddr;
		return true;
	}

	void NetReceiverLinux::_recv(uint16_t SessionId, char* pMessage, uint16_t Len)
	{
		on_recv(SessionId, pMessage, Len);
		m_BufPool.release(pMessage);
	}

	void NetReceiverLinux::_gateway(const PeerAddress PeerAddr, char* pMessage, uint16_t Len)
	{
		//返回true表示同意建立连接
		if (true == on_gateway(PeerAddr,pMessage, Len))
		{
			
			uint16_t SessionId = g_pPeerManager->connect_peer(PeerAddr);
			on_accept(SessionId, PeerAddr);
		}
		m_BufPool.release(pMessage);
	}

	void NetReceiverLinux::_reactor()
	{
		struct epoll_event EpollEvent[MAX_EVENTS];
		int32_t SocketFd = SOCKET_ERROR;
		uint32_t TriggerNum = 0;
		int64_t RecvLen = -1;
		PeerAddress PeerAddr = { 0 };
		for (;; )
		{
			//获取已经准备好的描述符事件
			/*
			如果要设置read超时
			1.设置socket非阻塞
			2.设置epoll_wait超时1秒
			3.每次进入epoll_wait之前，遍历在线用户列表，踢出长时间没有请求的用户
			*/
			TriggerNum = epoll_wait(m_EpollFd, EpollEvent, MAX_EVENTS, -1);
			//处理事件

			for (uint32_t i = 0; i < TriggerNum; ++i)
			{
				SocketFd = EpollEvent[i].data.fd;
				if (EpollEvent[i].events & EPOLLIN)
				{
					//IPv4或IPv6监听Socket
					if (SocketFd == m_ListenFd || SocketFd == m_ListenFd6)
					{
						PeerAddr = { 0 };
						sockaddr* pSockaddr = (sockaddr*)&PeerAddr;
						char* pBuf = m_BufPool.allocate();
						RecvLen = ::recvfrom(SocketFd, pBuf, MIN_MTU, 0, pSockaddr, &m_AddrLen6);

						if (RecvLen <= 0)
							continue;
						
						uint16_t SessionId = g_pPeerManager->session_id(PeerAddr);
						if (ERROR_SESSION_ID == SessionId)
							continue;
						//新的连接
						if (0 == SessionId)
						{
							//加入任务队列，内部会回收之前分配的资源
							m_pThreadPool->add_task(std::bind(&NetReceiverLinux::_gateway, this, PeerAddr, pBuf, static_cast<uint16_t>(RecvLen)));
						}
						else
						{
							//加入任务队列，内部会回收之前分配的资源
							m_pThreadPool->add_task(std::bind(&NetReceiverLinux::_recv, this, SessionId, pBuf, static_cast<uint16_t>(RecvLen)));
						}
					}
					else
					{
						::close(SocketFd);
					}
				}

			}
		}
	}
}