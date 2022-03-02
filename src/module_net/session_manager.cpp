#include "session_manager.h"
#include <base/timer.h>
#include <base/config.hpp>
#include <base/logger/logger.h>
#include <base/protocol/protocol_base.h>
#include <base/protocol/protocol_routing.h>
#include <module_net/net/nat_type.hpp>
#include <module_peer/routing_table.h>
#include <module_peer/partner_table.h>

//定时器任务
static bool try_connect_tracker(uint16_t SessionId)
{
	static char SendBuf[BASE_HEADER_LEN] = { 0 };
	static uint16_t TriggerTimes = 0;
	//连接服务器超时
	if (++TriggerTimes >= CONNECT_TIMEOUT_COUNT)
	{
		LOG_TRACE << "Fail to connect tracker...";
		g_pSessionManager->disconnect_in_timer(SessionId);
		return true;
	}
	net::Session* pCurSession = g_pSessionManager->session(SessionId);
	if (nullptr == pCurSession)
	{
		return true;
	}
	//如果之前建立了连接，对端可能还保持着连接信息，让它先断开
	if (1 == TriggerTimes)
	{
		create_header(SendBuf, PROTOCOL_BASE_DISCONNECT);
		pCurSession->send(SendBuf, BASE_HEADER_LEN);
	}
	SessionStatus CurStatus = pCurSession->status();

	if (SessionStatus::STATUS_CONNECT_COMPLETE == CurStatus ||//已连接，结束定时器
		SessionStatus::STATUS_NULL == CurStatus)//连接失败，结束定时器
	{
		return true;
	}

	//当前未ping成功
	if (SessionStatus::STATUS_DISCONNECT == CurStatus)
	{
		create_header(SendBuf, PROTOCOL_BASE_PING_REQ);
	}
	else
	{
		create_header(SendBuf, PROTOCOL_BASE_CONNECT_REQ);
	}
	pCurSession->send(SendBuf, BASE_HEADER_LEN);
	return false;
}

static bool try_nat_probe_tracker(uint16_t SessionId)
{
	static uint16_t TriggerTimes = 0;
	net::Session* pCurSession = g_pSessionManager->session(SessionId);
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
			g_pSessionManager->disconnect(SessionId);
		}
		return true;
	}
	if (++TriggerTimes >= NAT_PROBE_TIMEOUT_COUNT)
	{
		//如果当前状态下连接未建立，则断开连接
		if (SessionStatus::STATUS_DISCONNECT == pCurSession->status())
		{
			g_pSessionManager->disconnect_in_timer(SessionId);
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

static bool try_ping_probe_tracker(uint16_t SessionId)
{
	static uint16_t TriggerTimes = 0;
	if (TriggerTimes == PING_TIMEOUT_COUNT)
	{
		return true;
	}

	net::Session* pCurSession = g_pSessionManager->session(SessionId);
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

static bool recycle_session(uint16_t SessionId)
{
	LOG_DEBUG << "Session ID:" << SessionId << " recycle.";
	g_pPeerManager->recycle_session(SessionId);
	return true;
}

static bool check_connect_peer_status(uint16_t SessionId)
{
	//检查当前Peer的连接状态，如果找中间节点协助都连接不上就放弃连接
	//当前任务交给定时器延时执行
	net::Session* pCurSession = g_pSessionManager->session(SessionId);
	if (nullptr == pCurSession)
	{
		return true;
	}
	SessionStatus CurStatus = pCurSession->status();
	//给定时间内没有连接成功
	if (SessionStatus::STATUS_CONNECT_COMPLETE != CurStatus)
	{
		g_pSessionManager->disconnect_in_timer(SessionId);
	}
	return true;

}

static bool try_connect_peer(base::SHA1 CID, uint16_t SessionId)
{
	//创建发送区缓存
	static char pSendBuf[BASE_HEADER_LEN + KLEN_KEY] = { 0 };
	//尝试连接一个Peer，如果失败就不再尝试
	static uint16_t TriggerTimes = 0;
	net::Session* pCurSession = g_pSessionManager->session(SessionId);
	if (nullptr == pCurSession)
	{
		return true;
	}
	//连接服务器超时
	if (++TriggerTimes >= CONNECT_TIMEOUT_COUNT*2)
	{
		g_pSessionManager->disconnect_in_timer(SessionId);
		{//Test
			LOG_DEBUG << "Fail to connect session:" << SessionId << ", reason:offline.";
		}
		return true;
	}
	
	//如果之前建立了连接，对端可能还保持着连接信息，让它先断开
	if (1 == TriggerTimes)
	{
		create_header(pSendBuf, PROTOCOL_BASE_DISCONNECT);
		pCurSession->send(pSendBuf, BASE_HEADER_LEN);
	}
	SessionStatus CurStatus = pCurSession->status();
	//连接失败，结束定时器
	if (SessionStatus::STATUS_NULL == CurStatus)
	{
		return true;
	}
	//已连接，结束定时器，发送路由搜索协议
	if (SessionStatus::STATUS_CONNECT_COMPLETE == CurStatus)
	{
		create_header(pSendBuf, PROTOCOL_ROUTING_SEARCH_REQ);
		memcpy(&pSendBuf[BASE_HEADER_LEN], &CID, KLEN_KEY);
		pCurSession->send(pSendBuf, BASE_HEADER_LEN+ KLEN_KEY);
		return true;
	}
	//当前未ping成功
	if (SessionStatus::STATUS_DISCONNECT == CurStatus)
	{
		create_header(pSendBuf, PROTOCOL_BASE_PING_REQ);
		pCurSession->send(pSendBuf, BASE_HEADER_LEN);
	}
	//ping成功之后发送连接协议
	else
	{
		create_header(pSendBuf, PROTOCOL_BASE_CONNECT_REQ);
		pCurSession->send(pSendBuf, BASE_HEADER_LEN);
	}
	return false;
}

static bool try_connect_peer_relay(base::SHA1 CID, uint16_t TargetSessionId, uint16_t RealySessionId)
{
	static char SendBuf[BASE_HEADER_LEN + KSOCKADDR_LEN_V6+ KLEN_KEY] = { 0 };
	//先尝试直接连接，如果连不上通过中间Peer再尝试连接
	static uint16_t TriggerTimes = 0;
	net::Session* pTargetSession = g_pSessionManager->session(TargetSessionId);
	if (nullptr == pTargetSession)
	{
		return true;
	}
	//连接服务器超时，尝试借助中间Peer连接
	if (++TriggerTimes >= CONNECT_TIMEOUT_COUNT)
	{
		LOG_DEBUG << "Fail to connect, will try connect by relay node.";
		//尝试通过中间节点连接，然后终止当前任务
		net::Session* pRelaySession = g_pSessionManager->session(RealySessionId);
		if (nullptr == pRelaySession)
		{
			return true;
		}
		PeerAddress TargetAddr = { 0 };
		pTargetSession->get_peer_addr(TargetAddr);
		create_header(SendBuf, PROTOCOL_BASE_CONNECT_HELP_REQ);
		memcpy(&SendBuf[BASE_HEADER_LEN], &TargetAddr, KSOCKADDR_LEN_V6);
		memcpy(&SendBuf[BASE_HEADER_LEN + KSOCKADDR_LEN_V6], &CID, KLEN_KEY);
		//向中间节点发送请求
		pRelaySession->send_reliable(SendBuf, 50);

		//添加定时器，10s后如果没有连接成功直接释放连接
		if (false == g_pTimer->add_timer_lockless(10 * 1000,
			std::bind(check_connect_peer_status, TargetSessionId)))
		{
			LOG_ERROR << "Fail to create check connect peer status timer...";
		}
		return true;
	}
	//如果之前建立了连接，对端可能还保持着连接信息，让它先断开
	if (1 == TriggerTimes)
	{
		create_header(SendBuf, PROTOCOL_BASE_DISCONNECT);
		pTargetSession->send(SendBuf, BASE_HEADER_LEN);
	}
	SessionStatus CurStatus = pTargetSession->status();
	//连接失败，结束定时器
	if (SessionStatus::STATUS_NULL == CurStatus)
	{
		return true;
	}
	//已连接，结束定时器，发送路由搜索协议
	if (SessionStatus::STATUS_CONNECT_COMPLETE == CurStatus)
	{
		create_header(SendBuf, PROTOCOL_ROUTING_SEARCH_REQ);
		memcpy(&SendBuf[BASE_HEADER_LEN], &CID, KLEN_KEY);
		pTargetSession->send(SendBuf, BASE_HEADER_LEN+ KLEN_KEY);
		return true;
	}

	//当前未ping成功
	if (SessionStatus::STATUS_DISCONNECT == CurStatus)
	{
		create_header(SendBuf, PROTOCOL_BASE_PING_REQ);
		pTargetSession->send(SendBuf, BASE_HEADER_LEN);
	}
	//ping成功之后发送连接协议
	else
	{
		create_header(SendBuf, PROTOCOL_BASE_CONNECT_REQ);
		pTargetSession->send(SendBuf, BASE_HEADER_LEN);
	}
	return false;
}

static bool try_ping_peer(uint16_t TargetSessionId)
{
	static char SendBuf[BASE_HEADER_LEN] = { 0 };
	static uint16_t TriggerTimes = 0;
	if (++TriggerTimes == PING_TIMEOUT_COUNT)
	{
		return true;
	}
	net::Session* pCurSession = g_pSessionManager->session(TargetSessionId);
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
	create_header(SendBuf, PROTOCOL_BASE_PING_REQ);
	pCurSession->send(SendBuf, BASE_HEADER_LEN);
	return false;
}
//定时器任务

namespace net
{
	SeesionManager::SeesionManager() :
		m_ListenFd(-1),
		m_ListenFd6(-1),
		m_ListenFdNAT(-1) {}

	Session* SeesionManager::session(uint16_t SessionId)
	{
		return m_SessionArr[SessionId];
	}

	bool SeesionManager::init(SOCKET ListenFd, SOCKET ListenFd6, SOCKET ListenFdNAT)
	{
		m_ListenFd = ListenFd;
		m_ListenFd6 = ListenFd6;
		m_ListenFdNAT = ListenFdNAT;
		int32_t ConnectionNum = 1 + g_pConfig->max_connection_num();
		m_SessionArr.resize(ConnectionNum, nullptr);
		//初始创建10个Session对象
		m_SessionPool.init(10);
		return true;
	}

	bool SeesionManager::new_session(uint16_t SessionId, const PeerAddress& PeerAddr)
	{
		//当前Session已经分配
		if (nullptr != m_SessionArr[SessionId])
		{
			return false;
		}
		std::lock_guard<std::mutex> Lock(m_SessionMutex);
		m_SessionArr[SessionId] = m_SessionPool.allocate();
		sockaddr* pSockaddr = (sockaddr*)&PeerAddr;
		if (AF_INET == pSockaddr->sa_family)
		{
			m_SessionArr[SessionId]->init(PeerAddr, m_ListenFd, m_ListenFdNAT);
		}
		else
		{
			m_SessionArr[SessionId]->init(PeerAddr, m_ListenFd6, m_ListenFdNAT);
		}
		return true;
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

	uint16_t SeesionManager::connect_tracker(const char* pIPAddress, uint16_t Port)
	{
		//IPv6未完成
		uint16_t SessionId = _connect(pIPAddress, Port);
		//IPv6未完成
		//已达到最大连接数
		if (ERROR_SESSION_ID == SessionId)
		{
			return ERROR_SESSION_ID;
		}
		if (false == g_pTimer->add_timer(PING_RTT, std::bind(try_connect_tracker, SessionId)))
		{
			LOG_ERROR << "Fail to create connect tracker timer...";
		}
		return SessionId;
	}

	void SeesionManager::ping_tracker(const char* pIPAddress, uint16_t Port)
	{
		uint16_t SessionId = _connect(pIPAddress, Port);
		//已达到最大连接数
		if (ERROR_SESSION_ID == SessionId)
		{
			return;
		}
		if (false == g_pTimer->add_timer(PING_CLOCK, std::bind(try_ping_probe_tracker, SessionId)))
		{
			LOG_ERROR << "Ping peer timer create fail...";
		}
	}

	void SeesionManager::nat_probe_tracker(const char* pIPAddress, uint16_t Port)
	{
		uint16_t SessionId = _connect(pIPAddress, Port);
		//已达到最大连接数
		if (ERROR_SESSION_ID == SessionId)
		{
			return;
		}
		if (false == g_pTimer->add_timer(PING_RTT, std::bind(try_nat_probe_tracker, SessionId)))
		{
			LOG_ERROR << "Nat probe timer create fail...";
		}
	}

	void SeesionManager::connect_peer(base::SHA1 CID, uint16_t RelaySessionId, const PeerAddress& PeerAddr)
	{
		uint16_t TargetSessionId = g_pPeerManager->session_id(PeerAddr);
		//当前Session已经连接或者输入的PeerAddress有问题
		if (0 != TargetSessionId)
		{
			return;
		}
		TargetSessionId = g_pPeerManager->connect_peer(PeerAddr);
		//未能分配可用Session
		if (ERROR_SESSION_ID == TargetSessionId)
		{
			return;
		}
		//建立映射失败
		if (false == new_session(TargetSessionId, PeerAddr))
		{
			return;
		}
		if (false == g_pTimer->add_timer(PING_RTT, std::bind(try_connect_peer_relay, CID, TargetSessionId, RelaySessionId)))
		{
			LOG_ERROR << "Fail to create connect peer timer...";
		}
	}

	void SeesionManager::connect_peer(base::SHA1 CID, const PeerAddress& PeerAddr)
	{
		uint16_t SessionId = g_pPeerManager->session_id(PeerAddr);
		//输入的sockaddr有问题
		if (ERROR_SESSION_ID== SessionId)
		{
			return;
		}
		//当前Session已经连接
		if (0 == SessionId)
		{
			SessionId = g_pPeerManager->connect_peer(PeerAddr);
			//未能分配可用Session
			if (ERROR_SESSION_ID == SessionId)
			{
				return;
			}
			//建立映射失败
			if (false == new_session(SessionId, PeerAddr))
			{
				return;
			}
		}
		//相比可以直接连接的Peer，发送connect请求的频率降低
		if (false == g_pTimer->add_timer(PING_RTT*3, std::bind(try_connect_peer, CID, SessionId)))
		{
			LOG_ERROR << "Fail to create connect tracker timer...";
		}
	}

	void SeesionManager::ping_peer(const PeerAddress& PeerAddr)
	{
		uint16_t SessionId = g_pPeerManager->session_id(PeerAddr);
		//当前Session已经连接或者输入的PeerAddress有问题
		if (0 != SessionId)
		{
			return;
		}
		SessionId = g_pPeerManager->connect_peer(PeerAddr);
		//未能分配可用Session
		if (ERROR_SESSION_ID == SessionId)
		{
			return;
		}
		//建立映射失败
		if (false == new_session(SessionId, PeerAddr))
		{
			return;
		}
		if (false == g_pTimer->add_timer(3 * PING_RTT, std::bind(try_ping_peer, SessionId)))
		{
			LOG_ERROR << "Fail to create connect tracker timer...";
		}
	}

	void SeesionManager::disconnect(uint16_t SessionId)
	{
		net::Session* pCurSession = session(SessionId);
		if (nullptr == pCurSession)
		{
			return;
		}
		PeerAddress CurPeerAddr = { 0 };
		pCurSession->get_peer_addr(CurPeerAddr);
		//终止Sender内的session
		_delete_session(CurPeerAddr, SessionId);
		//直接断开PeerAddress->SessionId的映射
		g_pPeerManager->disconnect_peer(CurPeerAddr);

		//当前在定时器内部被调用
		if (false == g_pTimer->add_timer(20 * 1000, std::bind(recycle_session, SessionId)))
		{
			LOG_ERROR << "Recycle Session ID = " << SessionId << " timer create fail...";
		}
		//on_disconnect(SessionId);
	}

	void SeesionManager::disconnect_in_timer(uint16_t SessionId)
	{
		net::Session* pCurSession = session(SessionId);
		if (nullptr == pCurSession)
		{
			return;
		}
		PeerAddress CurPeerAddr = { 0 };
		pCurSession->get_peer_addr(CurPeerAddr);
		//终止Sender内的session
		_delete_session(CurPeerAddr, SessionId);
		//直接断开PeerAddress->SessionId的映射
		g_pPeerManager->disconnect_peer(CurPeerAddr);

		//当前在定时器内部被调用
		if (false == g_pTimer->add_timer_lockless(20 * 1000, std::bind(recycle_session, SessionId)))
		{
			LOG_ERROR << "Recycle Session ID = " << SessionId << " timer create fail...";
		}
		//on_disconnect(SessionId);
	}

	uint16_t SeesionManager::_connect(const char* pIPAddress, uint16_t Port)
	{
		PeerAddress CurPeerAddr = { 0 };
		g_pPeerManager->get_sockaddr(CurPeerAddr, pIPAddress, Port);
		//获取SessionId
		uint16_t SessionId = g_pPeerManager->connect_peer(CurPeerAddr);
		//未能分配可用Session
		if (ERROR_SESSION_ID == SessionId)
		{
			return ERROR_SESSION_ID;
		}
		//建立映射失败
		if (false == new_session(SessionId, CurPeerAddr))
		{
			return ERROR_SESSION_ID;
		}
		return SessionId;
	}

	uint16_t SeesionManager::_connect6(const char* pIPAddress, uint16_t Port)
	{
		PeerAddress CurPeerAddr = { 0 };
		g_pPeerManager->get_sockaddr6(CurPeerAddr, pIPAddress, Port);
		//获取SessionId
		uint16_t SessionId = g_pPeerManager->connect_peer(CurPeerAddr);
		//未能分配可用Session
		if (ERROR_SESSION_ID == SessionId)
		{
			return ERROR_SESSION_ID;
		}
		//建立映射失败
		if (false == new_session(SessionId, CurPeerAddr))
		{
			return ERROR_SESSION_ID;
		}
		return SessionId;
	}

	void SeesionManager::_delete_session(PeerAddress& PeerAddr, uint16_t SessionId)
	{
		if (nullptr == m_SessionArr[SessionId])
		{
			return;
		}
		std::lock_guard<std::mutex> Lock(m_SessionMutex);
		m_SessionPool.release(m_SessionArr[SessionId]);
		m_SessionArr[SessionId] = nullptr;
	}

}