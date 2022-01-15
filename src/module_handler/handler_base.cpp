#include "handler_base.h"
#include <base/timer.h>
#include <base/config.hpp>
#include <base/logger/logger.h>
#include <module_net/net/nat_type.hpp>
#include <module_net/session_manager.h>
#include <module_peer/routing_table.h>
#ifndef TRACKER_MODE
#include <module_peer/partner_table.h>
#endif
#define BASE_REGISTER(_FUNC) std::bind(&HandlerBase::_FUNC,this, \
std::placeholders::_1, std::placeholders::_2, std::placeholders::_3) 

//定时器事件
static bool heartbeat_probe(uint16_t SessionId)
{
	net::Session* pCurSession = g_pSessionManager->session(SessionId);
	if (nullptr == pCurSession)
	{
		return true;
	}
	//当前连接断开，终止定时器
	if (SessionStatus::STATUS_CONNECT_COMPLETE != pCurSession->status())
	{
		return true;
	}
	//所有数据包都会重置超时次数，如果当前超时次数为0，这时得到1
	uint8_t TimeoutCount = pCurSession->timeout();
	//LOG_TRACE << "TIME OUT " << TimeoutCount;

	//=2表示当前时间段内没有数据传输，发送一个心跳包检测
	if (2 == TimeoutCount)
	{
		//对端收到之后会发送KCP的ACK报文，收到后会重置超时次数
		//发送一个长度为0的心跳包即可
		pCurSession->send_reliable(nullptr, 0);
	}
	else if (3 == TimeoutCount)
	{
		//断开连接
		pCurSession->set_status(SessionStatus::STATUS_DISCONNECT);
		LOG_TRACE << "Session ID = " << SessionId << " disconnect, Reason:Timeout.";
		g_pSessionManager->disconnect_in_timer(SessionId);
		return true;
	}
	return false;
}

static bool second_try_connect_peer(base::SHA1 CID, uint16_t TargetSessionId)
{
	static uint16_t TriggerTimes = 0;
	net::Session* pTargetSession = g_pSessionManager->session(TargetSessionId);
	if (nullptr == pTargetSession)
	{
		return true;
	}
	//连接服务器超时
	if (++TriggerTimes >= CONNECT_TIMEOUT_COUNT)
	{
		//Test
		{
			std::string strIP;
			uint16_t Port;
			bool res2 = peer::PeerManager::info(pTargetSession->get_sockaddr(), strIP, Port);
			if (res2)
			{
				LOG_TRACE << "Can't connect Peer "<<strIP << ":" << Port<<" maybe because NAT or offline.";
			}
		}
		//Test
		g_pSessionManager->disconnect_in_timer(TargetSessionId);
		return true;
	}
	char pMessage[2];
	//如果之前建立了连接，对端可能还保持着连接信息，让它先断开
	if (1 == TriggerTimes)
	{
		create_header(pMessage, PROTOCOL_BASE_DISCONNECT);
		pTargetSession->send(pMessage, BASE_HEADER_LEN);
	}
	SessionStatus CurStatus = pTargetSession->status();
	//连接失败，结束定时器
	if (SessionStatus::STATUS_NULL == CurStatus)
	{
		return true;
	}

	//已连接，结束定时器,CID加入RoutingTable或者PartnerTable
	if (SessionStatus::STATUS_CONNECT_COMPLETE == CurStatus)
	{
#ifndef TRACKER_MODE
		//如果命中PartnerTable
		if (true == g_pPartnerTable->search_cid(CID))
		{
			g_pPartnerTable->add_partner(CID, TargetSessionId);
			return true;
		}
#endif
		//加入路由表
		int32_t PeerId = g_pPeerManager->peer_id(pTargetSession->get_sockaddr());
		if (PeerId >= 0)
		{
			peer::Node CurNode(CID.Hash, PeerId);
			g_pRoutingTable->add_node(CurNode);
		}
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
	pTargetSession->send(pMessage, BASE_HEADER_LEN);
	return false;
}

static bool try_ping_peer(uint16_t TargetSessionId)
{

}
//定时器事件

namespace handler
{
	HandlerBase::HandlerBase() {}

	void HandlerBase::register_recv_event()
	{
		//注册Recv事件的处理方式
		//m_RecvHandlerMap[PROTOCOL_BASE_HEARTBEAT_ACK] = BASE_REGISTER(handle_heartbeat_ack);
		m_RecvHandlerMap[PROTOCOL_BASE_HEARTBEAT_REQ] = BASE_REGISTER(handle_heartbeat_req);
		m_RecvHandlerMap[PROTOCOL_BASE_PING_REQ] = BASE_REGISTER(handle_ping_req);
		m_RecvHandlerMap[PROTOCOL_BASE_PING_ACK] = BASE_REGISTER(handle_ping_ack);
		m_RecvHandlerMap[PROTOCOL_BASE_PING_HELP] = BASE_REGISTER(handle_ping_help);
		m_RecvHandlerMap[PROTOCOL_BASE_CONNECT_REQ] = BASE_REGISTER(handle_connect_req);
		m_RecvHandlerMap[PROTOCOL_BASE_CONNECT_ACK] = BASE_REGISTER(handle_connect_ack);
		m_RecvHandlerMap[PROTOCOL_BASE_CONNECT_RFS] = BASE_REGISTER(handle_connect_rfs);
		m_RecvHandlerMap[PROTOCOL_BASE_CONNECT_HELP_REQ] = BASE_REGISTER(handle_connect_help_req);
		m_RecvHandlerMap[PROTOCOL_BASE_CONNECT_HELP_ACK] = BASE_REGISTER(handle_connect_help_ack);
		m_RecvHandlerMap[PROTOCOL_BASE_CONNECT_HELP_RFS] = BASE_REGISTER(handle_connect_help_rfs);
		m_RecvHandlerMap[PROTOCOL_BASE_DISCONNECT] = BASE_REGISTER(handle_disconnect);
		m_RecvHandlerMap[PROTOCOL_BASE_NAT_TYPE_PROBE_REQ] = BASE_REGISTER(handle_nat_probe_req);
		m_RecvHandlerMap[PROTOCOL_BASE_NAT_TYPE_PROBE_ACK] = BASE_REGISTER(handle_nat_probe_ack);

	}

	void HandlerBase::register_gateway_event()
	{
		//注册Gateway事件的处理方式
		m_RecvHandlerMap[PROTOCOL_BASE_PING_REQ] = BASE_REGISTER(handle_ping_req);
		m_RecvHandlerMap[PROTOCOL_BASE_PING_ACK] = BASE_REGISTER(handle_ping_ack);
		m_RecvHandlerMap[PROTOCOL_BASE_CONNECT_REQ] = BASE_REGISTER(handle_connect_req);
		m_RecvHandlerMap[PROTOCOL_BASE_NAT_TYPE_PROBE_REQ] = BASE_REGISTER(handle_nat_probe_req);
		m_RecvHandlerMap[PROTOCOL_BASE_NAT_TYPE_PROBE_ACK] = BASE_REGISTER(handle_nat_probe_ack);
	}

	int8_t HandlerBase::handle_event(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		//解析基础协议头
		uint16_t ProtocolId;
		parse_header(pMessage, ProtocolId);
		//如果协议注册过
		if (0 != m_RecvHandlerMap.count(ProtocolId))
		{
			return m_RecvHandlerMap[ProtocolId](SessionId, pMessage, Len);
		}
		//未注册协议直接无视
		return DO_NOTHING;
	}

	int8_t HandlerBase::handle_connect_req(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		LOG_DEBUG << "Connected Req";
		//对端节点申请建立连接，如果连接建立需要加入定时器检测心跳包
		//还未建立连接
		if (ERROR_SESSION_ID == SessionId)
		{
			//回复接受
			return DO_CONNECT;
		}
		//已建立连接，回复CONNECT_ACK
		create_header(pMessage, PROTOCOL_BASE_CONNECT_ACK);
		return DO_REPLY;
	}

	int8_t HandlerBase::handle_connect_ack(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		LOG_DEBUG << "Connected";
		net::Session* pCurSession = g_pSessionManager->session(SessionId);
		if (nullptr == pCurSession)
		{
			return DO_NOTHING;
		}
		//第一次收到ACK包，创建定时器
		if (SessionStatus::STATUS_CONNECT_COMPLETE != pCurSession->status())
		{
			pCurSession->set_status(SessionStatus::STATUS_CONNECT_COMPLETE);
			//定时检测超时状态，然后发送心跳包
			g_pTimer->add_timer(HEARTBEAT_CLOCK, std::bind(heartbeat_probe, SessionId));
			//连接成功，接下来需要查询PID和CID
			g_pPeerManager->search_push(SessionId);
		}
		//无需回复
		return DO_NOTHING;
	}

	int8_t HandlerBase::handle_connect_rfs(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		LOG_DEBUG << "Connect Fail";
		//对端节点发送CONNECT_RFS，表示对端节点拒绝建立连接，但是可以发送询问消息
		if (ERROR_SESSION_ID == SessionId)
		{
			return DO_NOTHING;
		}
		return DO_DISCONNECT;
	}

	int8_t HandlerBase::handle_connect_help_req(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		//其他Peer想要连接某个Peer，但是因为NAT的原因无法连接
		//想通过我帮他建立连接
		//Req发起连接的请求方
		//Target被连接的接收方
		uint16_t ReqSessionId = SessionId;
		net::Session* pReqSession = g_pSessionManager->session(ReqSessionId);
		if (nullptr == pReqSession)
		{
			return DO_NOTHING;
		}
		sockaddr_in6 TargetSockaddr6 = { 0 };
		memcpy(&TargetSockaddr6, &pMessage[2], KSOCKADDR_LEN_V6);
		sockaddr* pTargetSockaddr = (sockaddr*)&TargetSockaddr6;
		uint16_t TargetSessionId = g_pPeerManager->session_id(pTargetSockaddr);

		{//Test
			std::string strIP, strIP1;
			uint16_t Port, Port1;

			{
				bool res1 = peer::PeerManager::info(pReqSession->get_sockaddr(), strIP, Port);
				bool res2 = peer::PeerManager::info(pTargetSockaddr, strIP1, Port1);
				if (res1 && res2)
				{
					LOG_TRACE << strIP << ":" << Port << " want to connect " << strIP1 << ":" << Port1;
				}
			}

		}//Test
		
		//目标Peer不可达，拒绝协助
		if (0 == TargetSessionId || ERROR_SESSION_ID == TargetSessionId)
		{
			create_header(pMessage, PROTOCOL_BASE_CONNECT_HELP_RFS);
			pReqSession->send_reliable(pMessage, 2 + KSOCKADDR_LEN_V6);
			return DO_NOTHING;
		}
		net::Session* pTargetSession = g_pSessionManager->session(TargetSessionId);
		if (nullptr == pTargetSession)
		{
			create_header(pMessage, PROTOCOL_BASE_CONNECT_HELP_RFS);
			pReqSession->send_reliable(pMessage, 2 + KSOCKADDR_LEN_V6);
			return DO_NOTHING;
		}

		//可以协助当前Peer
		create_header(pMessage, PROTOCOL_BASE_CONNECT_HELP_ACK);
		//告知请求节点尝试继续连接，已通知目标节点协助UDP打洞
		pReqSession->send_reliable(pMessage, Len);

		//请求方的sockaddr
		sockaddr_in6* pReqSockaddr = (sockaddr_in6*)pReqSession->get_sockaddr();
		create_header(pMessage, PROTOCOL_BASE_PING_HELP);
		memcpy(&pMessage[2], pReqSockaddr, KSOCKADDR_LEN_V6);
		//通知目标节点协助UDP打洞
		pTargetSession->send_reliable(pMessage, 2 + KSOCKADDR_LEN_V6);
		//TEST
		{
			std::string strIP, strIP1;
			uint16_t Port, Port1;
			bool res1 = peer::PeerManager::info(pReqSession->get_sockaddr(), strIP, Port);
			bool res2 = peer::PeerManager::info(pTargetSession->get_sockaddr(), strIP1, Port1);
			if (res1 && res2)
			{
				LOG_TRACE << "Try help " << strIP << ":" << Port << " connect " << strIP1 << ":" << Port1;
			}
		}
		//TEST
		return DO_NOTHING;
	}

	int8_t HandlerBase::handle_connect_help_ack(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		//对方已接受请求，已经通知另一个节点尝试ping自己
		//如果NAT没问题可以实现UDP打洞
		//继续尝试连接
		sockaddr_in6 TargetSockaddr6 = { 0 };
		memcpy(&TargetSockaddr6, &pMessage[2], KSOCKADDR_LEN_V6);
		sockaddr* pTargetSockaddr = (sockaddr*)&TargetSockaddr6;
		uint16_t TargetSessionId = g_pPeerManager->session_id(pTargetSockaddr);
		if (0 != TargetSessionId && ERROR_SESSION_ID != TargetSessionId)
		{
			const uint8_t* pKey = (uint8_t*)&pMessage[30];
			base::SHA1 CID = { 0 };
			memcpy(&CID, pKey, 20);
			if (false == g_pTimer->add_timer(3 * PING_RTT, std::bind(second_try_connect_peer, CID, TargetSessionId)))
			{
				LOG_ERROR << "Fail to create connect tracker timer...";
			}
			//TEST
			{
				std::string strIP, strIP1;
				uint16_t Port, Port1;
				net::Session* pAckSession = g_pSessionManager->session(SessionId);
				if (nullptr != pAckSession)
				{
					bool res1 = peer::PeerManager::info(pAckSession->get_sockaddr(), strIP, Port);
					bool res2 = peer::PeerManager::info(pTargetSockaddr, strIP1, Port1);
					if (res1 && res2)
					{
						LOG_TRACE << strIP << ":" << Port << " will help me to connect " << strIP1 << ":" << Port1;
					}
				}
			}
			//TEST
		}
		return DO_NOTHING;
	}

	int8_t HandlerBase::handle_connect_help_rfs(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		LOG_ERROR << "Conenct Help Rfs";
		//拒绝协助，就直接断开连接
		sockaddr_in6 TargetSockaddr6 = { 0 };
		memcpy(&TargetSockaddr6, &pMessage[2], KSOCKADDR_LEN_V6);
		sockaddr* pTargetSockaddr = (sockaddr*)&TargetSockaddr6;
		uint16_t TargetSessionId = g_pPeerManager->session_id(pTargetSockaddr);
		if (0 != TargetSessionId && ERROR_SESSION_ID != TargetSessionId)
		{
			g_pSessionManager->disconnect(TargetSessionId);
			//Test
			{
				std::string strIP;
				uint16_t Port;
				bool res = peer::PeerManager::info(pTargetSockaddr, strIP, Port);
				if (true == res)
				{
					LOG_TRACE << "Fail to connect" << strIP << ":" << Port << ", Relay Node can't help.";
				}
			}
			//Test
		}
		return DO_NOTHING;
	}

	int8_t HandlerBase::handle_ping_ack(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		//对端节点发送PING_ACK，表示本机与对端节点可以建立直接连接并通信
		net::Session* pCurSession = g_pSessionManager->session(SessionId);
		if (nullptr != pCurSession)
		{
			if (SessionStatus::STATUS_DISCONNECT == pCurSession->status())
			{
				pCurSession->set_status(SessionStatus::STATUS_PING_COMPLETE);
				LOG_DEBUG << "Ping Success";
			}
		}
		//无需回复
		return DO_NOTHING;
	}

	int8_t HandlerBase::handle_ping_req(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		LOG_DEBUG << "Recv Ping";
		//对端节点发送PING_REQ，尝试ping本机，直接回复ACK
		create_header(pMessage, PROTOCOL_BASE_PING_ACK);
		//当前状态未连接
		return DO_REPLY;
	}

	int8_t HandlerBase::handle_ping_help(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		sockaddr_in6 TargetSockaddr6 = { 0 };
		memcpy(&TargetSockaddr6, &pMessage[2], KSOCKADDR_LEN_V6);
		sockaddr* pTargetSockaddr = (sockaddr*)&TargetSockaddr6;
		g_pSessionManager->ping_peer(pTargetSockaddr);
		//TEST
		{
			std::string strIP;
			uint16_t Port;


				bool res1 = peer::PeerManager::info(pTargetSockaddr, strIP, Port);
				if (res1)
				{
					LOG_TRACE << "Ping "<<strIP << ":" << Port << " to help it to connect me.";
				}
		}
		//TEST
		return DO_NOTHING;
	}

	int8_t HandlerBase::handle_heartbeat_req(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		LOG_DEBUG << "Recv Heartbeat";
		//对端节点发送HEARTBEAT_REQ
		//重置心跳包定时器超时计数
		net::Session* pCurSession = g_pSessionManager->session(SessionId);
		if (nullptr != pCurSession)
		{
			pCurSession->reset_timeout();
			//回复HEARTBEAT_ACK表示连接无异常
			create_header(pMessage, PROTOCOL_BASE_HEARTBEAT_ACK);
			return DO_REPLY;
		}
		return DO_NOTHING;
	}

	int8_t HandlerBase::handle_disconnect(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		return DO_DISCONNECT;
	}

	int8_t HandlerBase::handle_nat_probe_req(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		//直接用其他端口返回NAT确认
		LOG_TRACE << "NAT probe";
		create_header(pMessage, PROTOCOL_BASE_NAT_TYPE_PROBE_ACK);
		return DO_REPLY_NAT;
	}

	int8_t HandlerBase::handle_nat_probe_ack(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		//能够收到表面自身NAT类型为B+
		LOG_TRACE << "NAT = B+";
		g_pHostNatType->set_nat_type(NAT_TYPE::NAT_TYPE_B_PLUS);
		//无需回复
		return DO_NOTHING;
	}

}