#include "session.h"
#include <chrono>
#include <algorithm>
#include <module_net/net_receiver.h>
#include <base/logger/logger.h>
#include <base/timer.h>
#include <base/protocol/protocol_base.h>
#include <module_peer/peer_manager.h>

using namespace std::chrono;
#pragma warning(disable:4244)

namespace net
{
	Session::Session() :
		m_ListenFd(0),
		m_ListenFdNAT(0),
		m_pSockaddr(nullptr),
		m_RecvBuffer(48 * 1024),//缓存设定为Fragment的1.5倍
		m_Kcp(nullptr),
		m_TimeoutCount(0),
		m_SessionStatus(SessionStatus::STATUS_DISCONNECT) {}
		//m_pPeerInfo(new PeerInfo()) {}

	Session::~Session()
	{
		if (nullptr != m_pSockaddr)
		{
			delete m_pSockaddr;
		}
		if (nullptr != m_Kcp)
		{
			ikcp_release(m_Kcp);
		}
		if (nullptr != m_pPeerInfo)
		{
			delete m_pPeerInfo;
		}
	}

	void Session::reset_timeout()
	{
		m_TimeoutCount = 0;
	}

	uint8_t Session::timeout()
	{
		++m_TimeoutCount;
		return m_TimeoutCount;
	}

	SessionStatus Session::status()
	{
		return m_SessionStatus;
	}

	void Session::set_status(SessionStatus Status)
	{
		m_SessionStatus = Status;
	}

	sockaddr* Session::get_sockaddr()
	{
		return m_pSockaddr;
	}

	bool Session::init(sockaddr* pSockaddr, SOCKET ListenFd, SOCKET ListenFdNAT)
	{
		if (nullptr == pSockaddr)
		{
			return false;
		}
		m_pSockaddr = pSockaddr;
		//设定当前Session的SocketFd
		m_ListenFd = ListenFd;
		//设定NAT的SocketFd
		m_ListenFdNAT = ListenFdNAT;
		init_kcp(114514);
		return true;
	}

	void Session::init_kcp(uint32_t Conv)
	{
		m_Kcp = ::ikcp_create(Conv, (void*)this);
		::ikcp_nodelay(m_Kcp, 1, 0, 2, 1);
		//设定窗口大小
		::ikcp_wndsize(m_Kcp, 1024, 2048);
		//m_Interval = 100;
		m_Kcp->output = &Session::output;
		g_pTimer->add_timer(100, std::bind(&Session::update, this));
		m_Kcp->interval = 10;
		m_Kcp->rx_minrto = 50;
		m_Kcp->stream = 1;
	}

	sockaddr* Session::reset()
	{
		sockaddr* pTemp = m_pSockaddr;
		m_ListenFd = 0;
		m_ListenFdNAT = 0;
		std::lock_guard<std::mutex> Lock(m_KcpMutex);
		m_pSockaddr = nullptr;
		if (nullptr != m_Kcp)
		{
			ikcp_release(m_Kcp);
		}
		m_Kcp = nullptr;
		return pTemp;
	}

	void Session::send(const char* pMessage, uint16_t Len)
	{
		//基础协议包不受窗口限制
		if (Len == BASE_HEADER_LEN)
		{
			m_CurUpLoad += BASE_HEADER_LEN;
			::sendto(m_ListenFd, pMessage, Len, 0, m_pSockaddr, ADDR_LEN_IPV6);
			return;
		}
		//在当前窗口超出上传速率限制的数据直接丢弃
		//if (m_CurUpLoad + Len < m_MaxDownLoad)
		{
			m_CurUpLoad += Len;
			::sendto(m_ListenFd, pMessage, Len, 0, m_pSockaddr, ADDR_LEN_IPV6);
		}
	}

	void Session::send_nat(const char* pMessage, uint16_t Len)
	{
		m_CurUpLoad += Len;
		::sendto(m_ListenFdNAT, pMessage, Len, 0, m_pSockaddr, ADDR_LEN_IPV6);
	}

	void Session::input(char* pMessage, uint16_t Len)
	{
		std::unique_lock<std::mutex> Lock(m_KcpMutex);
		//解析数据，去除KCP部分
		::ikcp_input(m_Kcp, pMessage, Len);
		::ikcp_update(m_Kcp, _clock());
		m_CondVrb.wait(Lock, [=]() 
			{
			//当前没有其他线程在读取时，允许写入
			return m_RecvBuffer.reader_count() == 0;
			});
		//尝试循环读取KCP缓存，直到返回-1
		for (;;)
		{
			//按序读取可用数据
			int32_t Res = ::ikcp_recv(m_Kcp, pMessage, MIN_MTU);
			if (Res < 0)
			{
				return;
			}
			//空间不够时append会扩容
			m_RecvBuffer.append(pMessage, Res);
		}
	}

	char* Session::read(uint16_t& Len)
	{
		std::lock_guard<std::mutex> Lock(m_KcpMutex);
		int32_t BufLen = m_RecvBuffer.readable_size();
		if (BufLen == 0)
		{
			return nullptr;
		}
		//读取当前数据的长度
		char* pMessage = m_RecvBuffer.begin_read();
		//uint16_t Temp;
		memcpy(&Len, &pMessage[0], 2);
		//当前数据已经完整
		if (Len + 2 > m_RecvBuffer.readable_size())
		{
			return nullptr;
		}
		//LOG_ERROR << "Len = " << Len << " Readable = " << BufLen;

		//告知Buffer现在有一个读者正在读取Buffer，此时禁止扩容
		m_RecvBuffer.retrieve(Len + 2);

		return pMessage + 2;
	}

	void Session::read_over()
	{
		//std::lock_guard<std::mutex> Lock(m_KcpMutex);
		m_RecvBuffer.read_over();
	}

	void Session::send_reliable(const char* pMessage, uint16_t Len)
	{
		std::lock_guard<std::mutex> Lock(m_KcpMutex);
		::ikcp_send(m_Kcp, (char*)&Len, 2);
		::ikcp_send(m_Kcp, pMessage, Len);
		::ikcp_update(m_Kcp, _clock());
	}

	void Session::send_reliable(std::vector<const char*>& VecMessage, std::vector <uint16_t>& VecLen)
	{
		std::lock_guard<std::mutex> Lock(m_KcpMutex);
		uint16_t Len = 0;
		for (auto& L : VecLen)
			Len += L;
		::ikcp_send(m_Kcp, (char*)&Len, 2);
		for (int32_t Cur = 0; Cur < VecMessage.size(); ++Cur)
		{
			::ikcp_send(m_Kcp, VecMessage[Cur], VecLen[Cur]);
		}
		::ikcp_update(m_Kcp, _clock());
	}

	bool Session::get_session_info(std::string& strIpAddr, uint16_t& Port)
	{
		if (AF_INET == m_pSockaddr->sa_family)//IPV4
		{
			char Buf[INET_ADDRSTRLEN];
			Port = ntohs(((sockaddr_in*)m_pSockaddr)->sin_port);
			if (NULL != inet_ntop(m_pSockaddr->sa_family, &((sockaddr_in*)m_pSockaddr)->sin_addr,
				Buf, INET_ADDRSTRLEN))
			{
				strIpAddr = Buf;
			}
			else
			{
				return false;
			}
		}
		else if (AF_INET6 == m_pSockaddr->sa_family)//IPV6
		{
			char Buf[INET6_ADDRSTRLEN];
			Port = ntohs(((sockaddr_in6*)m_pSockaddr)->sin6_port);
			if (NULL != inet_ntop(m_pSockaddr->sa_family, &((sockaddr_in6*)m_pSockaddr)->sin6_addr,
				Buf, INET_ADDRSTRLEN))
			{
				strIpAddr = Buf;
			}
			else
			{
				return false;
			}
		}
		return true;
	}

	uint32_t Session::_clock()
	{
		uint64_t Cur = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
		Cur &= 0xfffffffful;
		return Cur;
	}

	bool Session::update()
	{
		std::lock_guard<std::mutex> Lock(m_KcpMutex);
		if (nullptr == m_Kcp)
		{
			return true;
		}
		::ikcp_update(m_Kcp, _clock());
		return false;
	}

	int32_t Session::output(const char* pMessage, int32_t Len, ikcpcb* pKcp, void* pSession)
	{
		Session* pCurSession = (Session*)pSession;
		pCurSession->send(pMessage, Len);
		return 0;
	}

	peer::PeerInfo* Session::peer_info()
	{
		return m_pPeerInfo;
	}

	int32_t Session::peer_id()
	{
		return g_pPeerManager->peer_id(m_pSockaddr);
	}
}