#include "handler_manager.h"
#include <base/timer.h>
#include <base/logger/logger.h>
#include <base/protocol/protocol_base.h>
#include <module_net/session_manager.h>

namespace handler
{
	static bool heartbeat_examine(uint16_t SessionId)
	{
		net::Session* pCurSession = g_pSessionManager->session(SessionId);
		if (nullptr == pCurSession)
		{
			return true;
		}
		//读取超时次数
		uint8_t TimeoutCount = pCurSession->timeout();
		//当前连接断开，终止定时器
		if (SessionStatus::STATUS_CONNECT_COMPLETE != pCurSession->status())
		{
			return true;
		}

		if (3 == TimeoutCount)
		{
			//断开连接
			pCurSession->set_status(SessionStatus::STATUS_DISCONNECT);
			LOG_TRACE << "Session ID = " << SessionId << " disconnect, Reason:Timeout.";
			g_pSessionManager->disconnect_in_timer(SessionId);
			return true;
		}
		return false;
	}

	HandlerManager::HandlerManager()
	{
		//注册协议处理方法
		m_GatewayEventMap.resize(64, nullptr);
		m_RecvEventMap.resize(64, nullptr);
	}

	HandlerManager::~HandlerManager() {}

	void HandlerManager::register_event(uint16_t ProtocolType, HandlerInterface* pHandler)
	{
		//注册Gateway事件
		pHandler->register_gateway_event();
		m_GatewayEventMap[ProtocolType] = pHandler;
		//注册Recv事件
		pHandler->register_recv_event();
		m_RecvEventMap[ProtocolType] = pHandler;
	}

	bool HandlerManager::handle_on_accept(uint16_t& SessionId)
	{
		net::Session* pCurSession = g_pSessionManager->session(SessionId);
		if (nullptr == pCurSession)
		{
			return false;
		}
		pCurSession->set_status(SessionStatus::STATUS_CONNECT_COMPLETE);
		pCurSession->reset_timeout();
		//启动定时器定时检测心跳包判断连接状态
		g_pTimer->add_timer(HEARTBEAT_CLOCK, std::bind(heartbeat_examine, SessionId));
		std::string strIpAddr;
		uint16_t Port;
		pCurSession->info(strIpAddr, Port);
		LOG_TRACE << "New Connection, Session ID = " << SessionId << ", IP:Port = " << strIpAddr << ":" << Port;
		return true;
	}

	bool HandlerManager::handle_on_disconnect(uint16_t& SessionId)
	{
		LOG_TRACE << "Session ID = " << SessionId << " disconnect";
		return true;
	}

	int8_t HandlerManager::handle_on_gateway(char* pMessage, uint16_t& Len)
	{
		//读取协议头类型
		uint16_t ProtocolType;
		parse_type(pMessage, ProtocolType);
		//利用多态处理不同类型的协议
		HandlerInterface* pEvent = m_GatewayEventMap[ProtocolType];
		uint16_t ErrorSessionId = ERROR_SESSION_ID;
		if (nullptr != pEvent)
		{
			//未建立连接时没有SessionId，传入非法SessionId
			return pEvent->handle_event(ErrorSessionId, pMessage, Len);
		}
		//接受到未注册类型信息直接无视
		return DO_NOTHING;
	}

	int8_t HandlerManager::handle_on_recv(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		net::Session* pCurSession = g_pSessionManager->session(SessionId);
		if (nullptr == pCurSession)
		{
			return DO_NOTHING;
		}
		pCurSession->reset_timeout();
		uint16_t ProtocolType;
		//可靠协议
		if (Len >= KCP_HEADER_LEN)
		{
			uint16_t BufLen;
			//收到数据，交给KCP处理，然后写入Session的Recv缓存
			pCurSession->input(pMessage, Len);
			//读取Session的Recv缓存，返回nullptr表示当前没有完整的数据
			//如果不为nullptr，当前Buffer会在内部计数器+1
			//处理完数据后需要使用read_over释放计数器
			for (;;)
			{
				char* pBuf = pCurSession->read(BufLen);

				if (nullptr == pBuf)
				{
					return DO_NOTHING;
				}
				//没有基础头协议，可能是心跳包
				if (BufLen < BASE_HEADER_LEN)
				{
					pCurSession->read_over();
					continue;
				}
				parse_type(pBuf, ProtocolType);
				//利用多态处理不同类型的协议
				HandlerInterface* pEvent = m_RecvEventMap[ProtocolType];
				if (nullptr != pEvent)
				{
					int8_t Res = pEvent->handle_event(SessionId, pBuf, BufLen);
					//处理之后判断应该断开连接
					if (DO_DISCONNECT == Res)
					{
						pCurSession->read_over();
						return DO_DISCONNECT;
					}
				}
				//释放Buffer计数器，避免扩容时写入读取中的Buffer
				pCurSession->read_over();
			}
		}
		//不可靠协议
		else
		{
			parse_type(pMessage, ProtocolType);
			//利用多态处理不同类型的协议
			HandlerInterface* pEvent = m_RecvEventMap[ProtocolType];
			if (nullptr != pEvent)
			{
				int8_t Res = pEvent->handle_event(SessionId, pMessage, Len);
				if (DO_DISCONNECT == Res)
				{
					return DO_DISCONNECT;
				}
				if (DO_REPLY == Res || DO_CONNECT == Res)
				{
					pCurSession->send(pMessage, Len);
				}
				else if (DO_REPLY_NAT == Res)
				{
					pCurSession->send_nat(pMessage, Len);
				}
			}
		}
		//接受到无关信息直接无视
		return DO_NOTHING;
	}

}
