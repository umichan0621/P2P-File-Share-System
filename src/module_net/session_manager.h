/***********************************************************************/
/* 名称:Session管理器												   */
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
#include <base/hash_function/file_sha1.h>
#include <module_peer/peer_manager.h>

namespace net
{
	typedef std::shared_ptr<Session> ShareSession;

	class SeesionManager
	{
		friend class Session;
		typedef base::ObjectPool<Session> SessionPool;
	public:
		//初始化，输入三个SocketFd
		bool init(SOCKET ListenFd, SOCKET ListenFd6, SOCKET ListenFdNAT);
		
		//sender建立session
		bool new_session(uint16_t SessionId, sockaddr* pSockaddr);

		//根据SessionId获取Session对象
		Session* session(uint16_t SessionId);

		peer::PeerInfo* peer_info(uint16_t SessionId);

		//给定Tracker的IP:Port，尝试连接
		uint16_t connect_tracker(const char* pIPAddress, uint16_t Port);

		//给定Tracker的IP:Port，尝试Ping
		void ping_tracker(const char* pIPAddress, uint16_t Port);

		//给定Tracker的IP:Port，尝试获取自身的NAT状态
		void nat_probe_tracker(const char* pIPAddress, uint16_t Port);

		//尝试连接一个Peer
		//输入RelaySessionId为告诉Peer联系方式的Session
		//pSockaddr是主动去连接的Peer
		void connect_peer(base::SHA1 CID,uint16_t RelaySessionId, sockaddr* pSockaddr);

		//尝试连接一个不一定可以连上的Peer
		void connect_peer(base::SHA1 CID, sockaddr* pSockaddr);

		//尝试ping一个Peer帮助他连上自己
		void ping_peer(sockaddr* pSockaddr);

		//断开Session的连接
		void disconnect(uint16_t SessionId);

		//在定时器执行的任务中调用，断开Session的连接
		void disconnect_in_timer(uint16_t SessionId);
	private:
		//主动连接IP:端口，成功返回SessionId，失败返回-1
		uint16_t _connect(const char* pIPAddress, uint16_t Port);

		//IPv6接口，同connect()
		uint16_t _connect6(const char* pIPAddress, uint16_t Port);

		sockaddr* _delete_session(uint16_t SessionId);
	private:
		SOCKET							m_ListenFd = 0;
		SOCKET							m_ListenFd6 = 0;
		SOCKET							m_ListenFdNAT = 0;
	private://Session相关
		SessionPool						m_SessionPool;
		std::vector<Session*>			m_SessionArr;
		std::mutex						m_SessionMutex;		//添加或删除Session时要加锁
	};
}

#define g_pSessionManager base::Singleton<net::SeesionManager>::get_instance()