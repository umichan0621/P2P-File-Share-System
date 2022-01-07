#include "session_manager.h"
#include <base/logger/logger.h>
#include <base/timer.h>
#include <base/protocol/protocol_base.h>
#include <module_net/net/nat_type.hpp>
#include <base/config.hpp>

namespace net
{

	Session* SeesionManager::session(uint16_t SessionId)
	{
		return m_SessionArr[SessionId];
	}

	bool SeesionManager::init(SOCKET ListenFd, SOCKET ListenFd6, SOCKET ListenFdNAT)
	{
		m_ListenFd = ListenFd;
		m_ListenFd6 = ListenFd6;
		m_ListenFdNAT = ListenFdNAT;
		int32_t ConnectionNum = 1+g_pConfig->max_connection_num();
		m_SessionArr.resize(ConnectionNum,nullptr);
		//初始创建10个Session对象
		m_SessionPool.init(10);
		return true;
	}

	bool SeesionManager::new_session(uint16_t SessionId, sockaddr* pSockaddr)
	{
		//当前Session已经分配
		if (nullptr != m_SessionArr[SessionId])
		{
			return false;
		}
		std::lock_guard<std::mutex> Lock(m_SessionMutex);
		m_SessionArr[SessionId] = m_SessionPool.allocate();
		if (AF_INET == pSockaddr->sa_family)
		{
			m_SessionArr[SessionId]->init(pSockaddr,m_ListenFd,m_ListenFdNAT);
		}
		else
		{
			m_SessionArr[SessionId]->init(pSockaddr, m_ListenFd6, m_ListenFdNAT);
		}
		return true;
	}

	sockaddr* SeesionManager::delete_session(uint16_t SessionId)
	{
		if (nullptr == m_SessionArr[SessionId])
		{
			return nullptr;
		}
		std::lock_guard<std::mutex> Lock(m_SessionMutex);
		m_SessionPool.release(m_SessionArr[SessionId]);
		sockaddr* pTemp=m_SessionArr[SessionId]->reset();
		m_SessionArr[SessionId] = nullptr;
		return pTemp;
	}

	peer::PeerInfo* SeesionManager::peer_info(uint16_t SessionId)
	{
		Session* pCurSession = session(SessionId);
		if (nullptr == pCurSession)
		{
			return nullptr;
		}
		return pCurSession->peer_info();
	}

	uint16_t SeesionManager::_connect(const char* pIpAddr, uint16_t Port)
	{
		sockaddr_in* pSockaddrin = (sockaddr_in*)g_pPeerManager->allocate_sockaddr();
		pSockaddrin->sin_family = AF_INET;
		inet_pton(AF_INET, pIpAddr, &pSockaddrin->sin_addr);		//设定新连接的IP
		pSockaddrin->sin_port = htons(Port);						//设定新连接的端口
		sockaddr* pSockaddr = (sockaddr*)pSockaddrin;
		//获取SessionId
		uint16_t SessionId = g_pPeerManager->connect_peer(pSockaddr);
		//未能分配可用Session
		if (ERROR_SESSION_ID == SessionId)
		{
			g_pPeerManager->release_sockaddr(pSockaddr);
			return ERROR_SESSION_ID;
		}
		//建立映射失败
		new_session(SessionId, pSockaddr);
		return SessionId;
	}

	uint16_t SeesionManager::_connect6(const char* pIpAddr, uint16_t Port)
	{
		sockaddr_in6* pSockaddrin6 = (sockaddr_in6*)g_pPeerManager->allocate_sockaddr();
		pSockaddrin6->sin6_family = AF_INET6;
		inet_pton(AF_INET6, pIpAddr, &pSockaddrin6->sin6_addr);		//设定新连接的IP
		pSockaddrin6->sin6_port = htons(Port);						//设定新连接的端口
		sockaddr* pSockaddr = (sockaddr*)pSockaddrin6;
		//获取SessionId
		uint16_t SessionId = g_pPeerManager->connect_peer(pSockaddr);
		//未能分配可用Session
		if (ERROR_SESSION_ID == SessionId)
		{
			g_pPeerManager->release_sockaddr(pSockaddr);
			return ERROR_SESSION_ID;
		}
		//建立映射失败
		new_session(SessionId, pSockaddr);
		return SessionId;
	}

	uint16_t SeesionManager::connect(const char* pIpAddr, uint16_t Port)
	{
		//IPv6未完成
		uint16_t SessionId = _connect(pIpAddr, Port);
		//IPv6未完成
		//已达到最大连接数
		if (ERROR_SESSION_ID == SessionId)
		{
			return ERROR_SESSION_ID;
		}
		if (false == g_pTimer->add_timer(PING_RTT, std::bind(&SeesionManager::try_connect,this, SessionId)))
		{
			LOG_ERROR << "Connect peer timer create fail...";
		}
		return SessionId;
	}

	void SeesionManager::ping(const char* pIpAddr, uint16_t Port)
	{
		uint16_t SessionId = _connect(pIpAddr, Port);
		//已达到最大连接数
		if (ERROR_SESSION_ID == SessionId)
		{
			return;
		}
		if (false == g_pTimer->add_timer(PING_CLOCK, std::bind(&SeesionManager::try_ping_probe,this, SessionId)))
		{
			LOG_ERROR << "Ping peer timer create fail...";
		}
	}

	void SeesionManager::nat_probe(const char* pIpAddr, uint16_t Port)
	{
		uint16_t SessionId = _connect(pIpAddr, Port);
		//已达到最大连接数
		if (ERROR_SESSION_ID == SessionId)
		{
			return;
		}
		if (false == g_pTimer->add_timer(PING_RTT, std::bind(&SeesionManager::try_nat_probe,this, SessionId)))
		{
			LOG_ERROR << "Nat probe timer create fail...";
		}
	}

	void SeesionManager::disconnect(uint16_t SessionId)
	{
		//终止Sender内的session
		sockaddr* pSockaddr = delete_session(SessionId);
		//回收sockaddr
		if (nullptr != pSockaddr)
		{
			g_pPeerManager->disconnect_peer(pSockaddr);
			//on_disconnect(SessionId);
		}
	}
	
	//定时器任务
	bool SeesionManager::try_connect(uint16_t SessionId)
	{
		static uint16_t TriggerTimes = 0;
		//连接服务器超时
		if (++TriggerTimes >= CONNECT_TIMEOUT_COUNT)
		{
			LOG_DEBUG << "fail to connect";
			disconnect(SessionId);
			return true;
		}
		net::Session* pCurSession = session(SessionId);
		if (nullptr == pCurSession)
		{
			return true;
		}
		char pMessage[2];
		//如果之前建立了连接，对端可能还保持着连接信息，让它先断开
		if (1 == TriggerTimes)
		{
			create_header(pMessage, PROTOCOL_BASE_DISCONNECT);
			pCurSession->send(pMessage, BASE_HEADER_LEN);
		}
		SessionStatus CurStatus = pCurSession->status();
		//已连接或者连接失败，结束定时器
		if (SessionStatus::STATUS_CONNECT_COMPLETE == CurStatus ||
			SessionStatus::STATUS_NULL == CurStatus)
		{
			return true;
		}
		//当前未ping成功
		if (SessionStatus::STATUS_DISCONNECT == CurStatus)
		{
			create_header(pMessage, PROTOCOL_BASE_PING_REQ);
		}
		else
		{
			create_header(pMessage, PROTOCOL_BASE_CONNECT_REQ);
		}
		pCurSession->send(pMessage, BASE_HEADER_LEN);
		return false;
	}

	bool SeesionManager::try_nat_probe(uint16_t SessionId)
	{
		static uint16_t TriggerTimes = 0;
		net::Session* pCurSession = session(SessionId);
		if (nullptr == pCurSession)
		{
			return true;
		}
		//已经判明NAT类型
		if (g_pHostNatType->get_nat_type() != NAT_TYPE::NAT_TYPE_NULL)
		{
			//如果当前状态下连接未建立，则断开连接
			if (SessionStatus::STATUS_DISCONNECT == pCurSession->status())
			{
				disconnect(SessionId);
			}
			return true;
		}
		if (++TriggerTimes >= NAT_PROBE_TIMEOUT_COUNT)
		{
			//如果当前状态下连接未建立，则断开连接
			if (SessionStatus::STATUS_DISCONNECT == pCurSession->status())
			{
				disconnect(SessionId);
			}
			//超时，可能自身NAT类型为C-，也可能是对端节点的问题
			LOG_TRACE << "NAT = C-";
			g_pHostNatType->set_nat_type(NAT_TYPE::NAT_TYPE_C_MINUS);
			return true;
		}
		char pMessage[2];
		create_header(pMessage, PROTOCOL_BASE_NAT_TYPE_PROBE_REQ);
		pCurSession->send(pMessage, BASE_HEADER_LEN);
		return false;
	}

	bool SeesionManager::try_ping_probe(uint16_t SessionId)
	{
		static uint16_t TriggerTimes = 0;
		if (TriggerTimes == PING_TIMEOUT_COUNT)
		{
			return true;
		}
		net::Session* pCurSession = session(SessionId);
		if (nullptr == pCurSession)
		{
			return true;
		}
		SessionStatus CurStatus = pCurSession->status();
		if (CurStatus == SessionStatus::STATUS_PING_COMPLETE ||
			CurStatus == SessionStatus::STATUS_CONNECT_COMPLETE)
		{
			return true;
		}
		char pMessage[2];
		create_header(pMessage, PROTOCOL_BASE_PING_REQ);
		pCurSession->send(pMessage, BASE_HEADER_LEN);
		return false;
	}
	//定时器任务
}