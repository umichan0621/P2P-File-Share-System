/***********************************************************************/
/* 名称:Peer管理器												       */
/* 说明:管理建立连接的Peer											   */
/* 创建时间:2021/12/04												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <vector>
#include <list>
#include <mutex>
#include "session/session.h"
#include <base/timer.h>
#include <base/singleton.hpp>
#include <base/object_pool.hpp>
#include <base/buffer_pool.hpp>
#include <module_peer/peer_manager.h>


namespace net
{
	typedef std::shared_ptr<Session> ShareSession;

	class SeesionManager
	{
		friend class Session;
		typedef base::ObjectPool<Session> SessionPool;
	public:
		bool init(SOCKET ListenFd, SOCKET ListenFd6, SOCKET ListenFdNAT);
		Session* session(uint16_t SessionId);
		//sender建立session
		bool new_session(uint16_t SessionId, sockaddr* pSockaddr);
		sockaddr* delete_session(uint16_t SessionId);
		peer::PeerInfo* peer_info(uint16_t SessionId);
	public:
		uint16_t connect(const char* pIpAddr, uint16_t Port);
		void ping(const char* pIpAddr, uint16_t Port);
		void nat_probe(const char* pIpAddr, uint16_t Port);
		void disconnect(uint16_t SessionId);
	private://主动连接IP:端口，成功返回SessionId，失败返回-1
		uint16_t _connect(const char* pIpAddr, uint16_t Port);
		uint16_t _connect6(const char* pIpAddr, uint16_t Port);//IPv6接口，同connect()
	private://定时器任务
		bool try_connect(uint16_t SessionId);
		bool try_nat_probe(uint16_t SessionId);
		bool try_ping_probe(uint16_t SessionId);
	private:
		SOCKET							m_ListenFd;
		SOCKET							m_ListenFd6;
		SOCKET							m_ListenFdNAT;
	private://Session相关
		SessionPool						m_SessionPool;
		std::vector<Session*>			m_SessionArr;
		std::mutex						m_SessionMutex;		//添加或删除Session时要加锁
	};
}

#define g_pSessionManager base::Singleton<net::SeesionManager>::get_instance()