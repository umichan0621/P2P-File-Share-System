#include "handler_routing.h"
#include <base/config.hpp>
#include <base/buffer_pool.hpp>
#include <base/logger/logger.h>
#include <base/hash_function/file_sha1.h>
#include <module_net/session_manager.h>
#include <module_peer/routing_table.h>
#include <module_peer/partner_table.h>
#pragma warning(disable:4789)

#define ROUTING_REGISTER(_FUNC) std::bind(&HandlerRouting::_FUNC,this, \
std::placeholders::_1, std::placeholders::_2, std::placeholders::_3) 

namespace handler
{
	HandlerRouting::HandlerRouting() {}

	void HandlerRouting::register_recv_event()
	{
		//注册Recv事件的处理方式
		m_RecvHandlerMap[PROTOCOL_ROUTING_SEARCH_REQ] = ROUTING_REGISTER(handle_routing_search_req);
		m_RecvHandlerMap[PROTOCOL_ROUTING_SEARCH_ACK] = ROUTING_REGISTER(handle_routing_search_ack);
		m_RecvHandlerMap[PROTOCOL_ROUTING_PARTNER] = ROUTING_REGISTER(handle_routing_partner);
		m_RecvHandlerMap[PROTOCOL_ROUTING_REGISTER_REQ] = ROUTING_REGISTER(handle_routing_register_req);
		m_RecvHandlerMap[PROTOCOL_ROUTING_REGISTER_ACK] = ROUTING_REGISTER(handle_routing_register_ack);
	}

	void HandlerRouting::register_gateway_event()
	{
	}

	int8_t HandlerRouting::handle_event(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		//解析基础协议头
		uint16_t ProtocolId;
		parse_header(pMessage, ProtocolId);
		//如果协议注册过
		if (m_RecvHandlerMap.count(ProtocolId) != 0)
		{
			return m_RecvHandlerMap[ProtocolId](SessionId, pMessage, Len);
		}
		return false;
	}

	int8_t HandlerRouting::handle_routing_search_req(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		uint16_t ReqSessionId = SessionId;
		//检测当前Session是否合法
		net::Session* pReqSession = g_pSessionManager->session(ReqSessionId);
		if (nullptr == pReqSession)
		{
			return DO_NOTHING;
		}
		//获取PeerId
		int32_t ReqPeerId = pReqSession->peer_id();
		if (0 >= ReqPeerId)
		{
			return DO_NOTHING;
		}
		//分配发送用的缓存区
		char* pSendBuf = g_pBufferPoolMgr->allocate();
		//循环解析Key，每个Key都有一个数据包
		//对每个Key执行两步操作
		//如果当前模式不是Tracker先查询PartnerTable
		//如果PartnerTable命中就不继续执行
		//然后再查询RoutingTable，给出距离最近的几个节点
		//Tracker模式下没有PartnerTable所以只执行第二步
		for (int Pos = BASE_HEADER_LEN; ; Pos += KLEN_KEY)
		{
			if (Len - Pos < KLEN_KEY)
			{
				break;
			}
			//获取Key
			const uint8_t* pKey = (uint8_t*)&pMessage[Pos];

			//如果不是Tracker，优先查询PartnerTable，命中就不执行RoutingTable操作
#ifndef TRACKER_MODE
			base::SHA1 CID = { 0 };
			memcpy(&CID, pKey, KLEN_KEY);
			std::vector<uint16_t> PartnerList;
			PeerAddress PartnerAddr = { 0 };
			//PartnerTable命中
			if (true == g_pPartnerTable->search_cid(CID, PartnerList))
			{
				//加入自己的PartnerTable
				g_pPartnerTable->add_partner(CID, ReqSessionId);
				{//TEST
					std::string strIP, strCID;
					uint16_t Port;
					base::sha1_value(pKey, strCID);
					PeerAddress ReqAddr = { 0 };
					pReqSession->get_peer_addr(ReqAddr);
					bool res = peer::PeerManager::info(ReqAddr, strIP, Port);
					if (false != res)
					{
						LOG_DEBUG << "[Partner] <" << strCID << ", " << strIP << ":" << Port << ">";
					}
				}

				//构造Partner协议
				//协议格式:
				//[固定头部(2B)] +[PID/CID(20B)]+(0-N个)[sockaddr(28B)]
				//将所有Partner的信息交给对方
				//限制于Buffer消息的长度，按10个一组发送
				create_header(pSendBuf, PROTOCOL_ROUTING_PARTNER);
				memcpy(&pSendBuf[BASE_HEADER_LEN], pKey, KLEN_KEY);
				//没有记录的Partner
				if (0 == PartnerList.size())
				{
					pReqSession->send_reliable(pSendBuf, BASE_HEADER_LEN+ KLEN_KEY);
				}
				else
				{
					int32_t PartnerPos = BASE_HEADER_LEN + KLEN_KEY;
					int32_t Count = 0;
					//单次允许发送的Partner信息数量
					int32_t PartnerNum = (MIN_MTU - 50) / KSOCKADDR_LEN_V6;
					for (auto& PartnerId : PartnerList)
					{
						if (0 == (Count % PartnerNum))
						{
							PartnerPos = BASE_HEADER_LEN + KLEN_KEY;
						}
						net::Session* pPartnerSession = g_pSessionManager->session(PartnerId);
						//当前Partner断连或者是请求者自己
						if (nullptr == pPartnerSession || PartnerId == ReqSessionId)
						{
							continue;
						}
						pPartnerSession->get_peer_addr(PartnerAddr);
						memcpy(&pSendBuf[PartnerPos], &PartnerAddr, KSOCKADDR_LEN_V6);
						PartnerPos += KSOCKADDR_LEN_V6;
						++Count;
						//PartnerNum个一组，发送Partner命中消息
						if (0 == (Count % PartnerNum))
						{

							pReqSession->send_reliable(pSendBuf, PartnerPos);
							Count = 0;
						}
					}
					//不满PartnerNum个的一组
					if (0 != (Count % PartnerNum))
					{
						pReqSession->send_reliable(pSendBuf, PartnerPos);
					}
				}
				continue;
			}
#endif
			//构造节点
			peer::Node CurNode(pKey, ReqPeerId);
			//登记当前节点到路由表中
			g_pRoutingTable->add_node(CurNode);
			{//TEST
				std::string strIP, strCID;
				uint16_t Port;
				base::sha1_value(pKey, strCID);
				PeerAddress ReqAddr = { 0 };
				pReqSession->get_peer_addr(ReqAddr);
				bool res = peer::PeerManager::info(ReqAddr, strIP, Port);
				if (false != res)
				{
					LOG_DEBUG << "[Routing] <" << strCID << ", " << strIP << ":" << Port << ">";
				}
			}

			//查询自己的路由表，如果命中会返回信息
			PeerAddress SearchAddr = { 0 };
			create_header(pSendBuf, PROTOCOL_ROUTING_SEARCH_ACK);
			//读取几个路由表中距离最近的节点
			std::unordered_set<int32_t> PeerSet;
			g_pRoutingTable->get_node(pKey, ReqPeerId, PeerSet);
			//当前节点能够找到距离较近的其他节点，应该返回数据
			//协议格式:[固定头部(2B)] +[PID/CID(20B)]+(0-α个)[[sockaddr(28B)]+[Status[1B]]
			if (false == PeerSet.empty())
			{
				memcpy(&pSendBuf[BASE_HEADER_LEN], pKey, KLEN_KEY);
				uint16_t RoutingPos = BASE_HEADER_LEN + KLEN_KEY;
				for (int32_t SearchPeerID : PeerSet)
				{
					if (ReqPeerId == SearchPeerID)
					{
						continue;
					}
					uint8_t PeerStatus;
					bool bRes = g_pPeerManager->peer(SearchPeerID, SearchAddr, PeerStatus);
					if (true == bRes)
					{
						memcpy(&pSendBuf[RoutingPos], &SearchAddr, KSOCKADDR_LEN_V6);
						RoutingPos += KSOCKADDR_LEN_V6;
						memcpy(&pSendBuf[RoutingPos], &PeerStatus, 1);
						++RoutingPos;
					}
				}
				pReqSession->send_reliable(pSendBuf, RoutingPos);
			}
		}
		//发送Register消息
		//向当前通信的扩散自己的CID
		//协议格式:(和search_req内容一致，但是对方收到之后除非命中Partner表否则不会回复信息)
		//[固定头部(2B)] + (0-3个)[CID(20B)]
#ifndef TRACKER_MODE
		create_header(pSendBuf, PROTOCOL_ROUTING_REGISTER_ACK);
		uint16_t Pos = BASE_HEADER_LEN;
		std::unordered_set<base::SHA1, base::SHA1HashFunc, base::SHA1EqualFunc> CIDSet;
		base::SHA1 CID = { 0 };
		for (int8_t i = 0; i < 3; ++i)
		{
			if (false == g_pPartnerTable->get_cid(CID))
			{
				break;
			}
			CIDSet.insert(CID);
		}
		for (auto& CurCID : CIDSet)
		{
			memcpy(&pSendBuf[Pos], &CurCID, KLEN_KEY);
			Pos += KLEN_KEY;
		}
		pReqSession->send_reliable(pSendBuf, Pos);
#endif
		g_pBufferPoolMgr->release(pSendBuf);
		return DO_NOTHING;
	}

	int8_t HandlerRouting::handle_routing_search_ack(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		uint16_t AckSessionId = SessionId;
		//当前Handler只做[解析->尝试建立连接]这件事
		//协议格式:
		//[固定头部(2B)] +[PID/CID(20B)]+(0-α个)[[sockaddr(28B)]+[Status[1B]]
		//获取Key
		const uint8_t* pKey = (uint8_t*)&pMessage[BASE_HEADER_LEN];
		base::SHA1 CID = { 0 };
		memcpy(&CID, pKey, KLEN_KEY);

		
		PeerAddress TargetPeerAddr = { 0 };
		uint8_t PeerStatus;

		for (uint16_t Pos = BASE_HEADER_LEN + KLEN_KEY; Pos < Len;)
		{
			//读取Peer信息
			memcpy(&TargetPeerAddr, &pMessage[Pos], KSOCKADDR_LEN_V6);
			Pos += KSOCKADDR_LEN_V6;
			memcpy(&PeerStatus, &pMessage[Pos], 1);
			++Pos;
			uint16_t TargetSessionId = g_pPeerManager->session_id(TargetPeerAddr);
			//如果不是Tracker，优先查询PartnerTable，命中就不执行RoutingTable操作
#ifndef TRACKER_MODE
			//如果命中PartnerTable
			if (true == g_pPartnerTable->search_cid(CID))
			{
				//当前获取的节点已经建立连接，不用再建立连接
				if (0 != TargetSessionId)
				{
					g_pPartnerTable->add_partner(CID, TargetSessionId);
				}
				//尝试建立连接
				else
				{
					//状态为GOOD，如果因为NAT连不上可以借助中间节点连接
					if (peer::GOOD == PeerStatus)
					{
						g_pSessionManager->connect_peer(CID, AckSessionId, TargetPeerAddr);
						{//TEST
							std::string strIP;
							uint16_t Port;
							bool res1 = peer::PeerManager::info(TargetPeerAddr, strIP, Port);
							if (false != res1)
							{
								LOG_DEBUG << "Try to connect " << strIP << ":" << Port << " by relay SessionID = " << AckSessionId;
							}
						}
					}
					//其他状态下，中间节点无法协助连接，只尝试一次
					else
					{
						g_pSessionManager->connect_peer(CID, TargetPeerAddr);
						{//TEST
							std::string strIP;
							uint16_t Port;
							bool res = peer::PeerManager::info(TargetPeerAddr, strIP, Port);
							if (false != res)
							{
								LOG_DEBUG << "Try to connect " << strIP << ":" << Port << " once";
							}
						}
					}
				}
				continue;
			}
#endif
			//加入路由表
			int32_t PeerId = g_pPeerManager->peer_id(TargetPeerAddr);
			if (PeerId >= 0)
			{
				peer::Node CurNode(pKey, PeerId);
				g_pRoutingTable->add_node(CurNode);
			}
			//当前获取的节点没有建立连接
			if (0 == TargetSessionId)
			{
				//状态为GOOD，如果因为NAT连不上可以借助中间节点连接
				if (peer::GOOD == PeerStatus)
				{
					g_pSessionManager->connect_peer(CID, AckSessionId, TargetPeerAddr);
					{//TEST
						std::string strIP;
						uint16_t Port;
						bool res1 = peer::PeerManager::info(TargetPeerAddr, strIP, Port);
						if (false != res1)
						{
							LOG_DEBUG << "Try to connect " << strIP << ":" << Port << " by relay SessionID = " << AckSessionId;
						}
					}

				}
				//其他状态下，中间节点无法协助连接，只尝试一次
				else
				{
					g_pSessionManager->connect_peer(CID, TargetPeerAddr);
					{//TEST
						std::string strIP;
						uint16_t Port;
						bool res = peer::PeerManager::info(TargetPeerAddr, strIP, Port);
						if (false != res)
						{
							LOG_DEBUG << "Try to connect " << strIP << ":" << Port << " once";
						}
					}
				}
			}
		}
		return DO_NOTHING;
	}

	int8_t HandlerRouting::handle_routing_register_req(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		//其他节点发送自己的PID，应该回复本机的PID
		//应该把PID加入路由表
		uint16_t AckSessionId = SessionId;
		//检测当前Session是否合法
		net::Session* pAckSession = g_pSessionManager->session(AckSessionId);
		if (nullptr == pAckSession)
		{
			return DO_NOTHING;
		}

		PeerAddress AckPeerAddr = { 0 };
		pAckSession->get_peer_addr(AckPeerAddr);
		//加入路由表
		int32_t PeerId = g_pPeerManager->peer_id(AckPeerAddr);
		if (PeerId >= 0)
		{
			const uint8_t* pKey = (uint8_t*)&pMessage[BASE_HEADER_LEN];
			peer::Node CurNode(pKey, PeerId);
			g_pRoutingTable->add_node(CurNode);
			{//TEST
				std::string strIP, strCID;
				base::sha1_value(pKey, strCID);
				uint16_t Port;
				bool res = peer::PeerManager::info(AckPeerAddr, strIP, Port);
				if (false != res)
				{
					LOG_DEBUG << "[Routing] <" << strCID << ", " << strIP << ":" << Port << ">";
				}
			}		
		}
		//回复自己的PID
		create_header(pMessage, PROTOCOL_ROUTING_REGISTER_ACK);
		memcpy(&pMessage[BASE_HEADER_LEN], g_pRoutingTable->pid(), KLEN_KEY);
		pAckSession->send_reliable(pMessage, BASE_HEADER_LEN + KLEN_KEY);
		return DO_NOTHING;
	}

	int8_t HandlerRouting::handle_routing_register_ack(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		//其他节点回复自己的PID
		uint16_t AckSessionId = SessionId;
		//检测当前Session是否合法
		net::Session* pAckSession = g_pSessionManager->session(AckSessionId);
		if (nullptr == pAckSession)
		{
			return DO_NOTHING;
		}
		PeerAddress AckPeerAddr = { 0 };
		pAckSession->get_peer_addr(AckPeerAddr);

		//加入路由表
		int32_t PeerId = g_pPeerManager->peer_id(AckPeerAddr);
		if (PeerId >= 0)
		{
			const uint8_t* pKey = (uint8_t*)&pMessage[BASE_HEADER_LEN];
			peer::Node CurNode(pKey, PeerId);
			g_pRoutingTable->add_node(CurNode);	
			{//TEST
				std::string strIP, strCID;
				base::sha1_value(pKey, strCID);
				uint16_t Port;
				bool res = peer::PeerManager::info(AckPeerAddr, strIP, Port);
				if (false != res)
				{
					LOG_DEBUG << "[Routing] <" << strCID << ", " << strIP << ":" << Port << ">";
				}
			}
		}
		return DO_NOTHING;
	}

	int8_t HandlerRouting::handle_routing_partner(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		//协议结构:[Header(2B)]+[CID(20B)]+(0-N个)[PeerAddress(28B)]
		//获取Key
		const uint8_t* pKey = (uint8_t*)&pMessage[BASE_HEADER_LEN];
		base::SHA1 CID = { 0 };
		memcpy(&CID, pKey, KLEN_KEY);

		//如果当前已经不需要这个CID相关的Partner
		if (true != g_pPartnerTable->search_cid(CID))
		{
			return DO_NOTHING;
		}
		//当前消息的发送方也是Partner
		g_pPartnerTable->add_partner(CID, SessionId);
		//遍历返回的Partner列表
		uint16_t Pos = BASE_HEADER_LEN + KLEN_KEY;
		PeerAddress TargetPeerAddr = { 0 };
		for (; Pos < Len;)
		{
			//读取Peer信息
			memcpy(&TargetPeerAddr, &pMessage[Pos], KSOCKADDR_LEN_V6);
			Pos += KSOCKADDR_LEN_V6;
			uint16_t TargetSessionId = g_pPeerManager->session_id(TargetPeerAddr);
			//当前Peer已经建立连接
			if (0 != TargetSessionId)
			{
				//直接加入Partner表
				g_pPartnerTable->add_partner(CID, TargetSessionId);
				{//TEST
					std::string strIP, strCID;
					base::sha1_value(pKey, strCID);
					uint16_t Port;
					bool res = peer::PeerManager::info(TargetPeerAddr, strIP, Port);
					if (false != res)
					{
						LOG_DEBUG << "[Partner] <" << strCID << ", " << strIP << ":" << Port << ">";
					}
				}
			}
			//尝试与当前Peer连接
			else
			{
				//状态一定为GOOD，如果因为NAT连不上可以借助中间节点连接
				g_pSessionManager->connect_peer(CID, SessionId, TargetPeerAddr);
				{//TEST
					std::string strIP;
					uint16_t Port;
					bool res = peer::PeerManager::info(TargetPeerAddr, strIP, Port);
					if (false != res)
					{
						LOG_DEBUG << "Try to connect " << strIP << ":" << Port << " By relay node:" << SessionId;
					}
				}
			}
		}
		return DO_NOTHING;
	}
}