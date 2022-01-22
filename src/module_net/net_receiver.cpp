#include "net_receiver.h"
#include <base/timer.h>
#include <base/logger/logger.h>
#include <base/protocol/protocol_base.h>
#include <module_net/session_manager.h>

namespace net
{
	NetReceiver::NetReceiver():
		m_pNetHandler(nullptr){}

	NetReceiver::~NetReceiver() {}

	void NetReceiver::listen_fd(SOCKET& ListenFd, SOCKET& ListenFd6, SOCKET& ListenFdNAT)
	{
		ListenFd = m_ListenFd;
		ListenFd6 = m_ListenFd6;
		ListenFdNAT = m_ListenFdNAT;
	}

	void NetReceiver::set_handler(handler::HandlerManager* pHandler)
	{
		m_pNetHandler = pHandler;
	}

	void NetReceiver::on_accept(uint16_t SessionId, const PeerAddress& PeerAddr)
	{
		char Buf[BASE_HEADER_LEN];
		sockaddr* pSockaddr = (sockaddr*) & PeerAddr;
		SOCKET SendFd = (pSockaddr->sa_family == AF_INET ? m_ListenFd : m_ListenFd6);
		//未能分配可用Session
		if (ERROR_SESSION_ID == SessionId)
		{
			create_header(Buf, PROTOCOL_BASE_CONNECT_RFS);
			::sendto(SendFd, Buf, BASE_HEADER_LEN, 0, pSockaddr, ADDR_LEN_IPV6);
			return;
		}
		//建立映射失败
		if (false == g_pSessionManager->new_session(SessionId, PeerAddr))
		{
			create_header(Buf, PROTOCOL_BASE_CONNECT_RFS);
			::sendto(SendFd, Buf, BASE_HEADER_LEN, 0, pSockaddr, ADDR_LEN_IPV6);
			return;
		}
		if (false == m_pNetHandler->handle_on_accept(SessionId))
		{
			create_header(Buf, PROTOCOL_BASE_CONNECT_RFS);
			::sendto(SendFd, Buf, BASE_HEADER_LEN, 0, pSockaddr, ADDR_LEN_IPV6);
			return;
		}
		create_header(Buf, PROTOCOL_BASE_CONNECT_ACK);
		::sendto(SendFd, Buf, BASE_HEADER_LEN, 0, pSockaddr, ADDR_LEN_IPV6);
	}

	bool NetReceiver::on_gateway(const PeerAddress& PeerAddr, char* pMessage, uint16_t Len)
	{
		//长度小于协议头，或者大于等于KCP协议长度，是格式错误的消息
		//无连接协议长度只允许在KCP协议长度(24Byte)以内
		if (Len < BASE_HEADER_LEN||Len>=KCP_HEADER_LEN)
		{
			return false;
		}
		//返回值为true表示同意建立连接
		int8_t Res = m_pNetHandler->handle_on_gateway(pMessage, Len);
		//建立连接
		if (DO_CONNECT == Res)
		{
			return true;
		}
		sockaddr* pSockaddr = (sockaddr*)&PeerAddr;
		//正常回复
		if (DO_REPLY == Res)
		{
			SOCKET SendFd = (pSockaddr->sa_family == AF_INET ? m_ListenFd : m_ListenFd6);
			::sendto(SendFd, pMessage, Len, 0, pSockaddr, ADDR_LEN_IPV6);
		}
		//NAT回复
		else if (DO_REPLY_NAT == Res)
		{
			::sendto(m_ListenFdNAT, pMessage, Len, 0, pSockaddr, ADDR_LEN_IPV6);
		}
		return false;
	}

	void NetReceiver::on_recv(uint16_t SessionId, char* pMessage, uint16_t Len)
	{
		//长度小于协议头是格式错误的消息
		if (Len < BASE_HEADER_LEN)
		{
			return;
		}
		int8_t Res = m_pNetHandler->handle_on_recv(SessionId, pMessage, Len);
		//断开连接
		if (DO_DISCONNECT == Res)
		{
			g_pSessionManager->disconnect(SessionId);   
			on_disconnect(SessionId);
		}
	}

	void NetReceiver::on_disconnect(uint16_t SessionId)
	{
		m_pNetHandler->handle_on_disconnect(SessionId);
	}
}