/***********************************************************************/
/* 名称:Session												           */
/* 说明:用于维护与其他节点的连接									   */
/* 创建时间:2021/02/19												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#ifdef __linux
#include <arpa/inet.h>
#else
#include <ws2tcpip.h>
#endif
#include <stdint.h>
#include <string>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <third/kcp/ikcp.h>
#include <module_peer/peer/peer_info.h>
#include <base/buffer.h>

#ifdef __linux
typedef int32_t SOCKET;
#endif

enum class SessionStatus
{
	STATUS_NULL = 0,
	STATUS_DISCONNECT,
	STATUS_PING_COMPLETE,
	STATUS_CONNECT_COMPLETE
};

namespace net
{
	class Session
	{
	public:
		Session();
		~Session();
	public://Session状态相关
		void reset_timeout();
		uint8_t timeout();
		SessionStatus status();
		void set_status(SessionStatus Status);
		void get_peer_addr(sockaddr_in6& PeerAddr) const;

		//初始化session
		bool init(const sockaddr_in6 PeerAddr,SOCKET ListenFd,SOCKET ListenFdNAT);	
		
		void init_kcp(uint32_t Conv);

		//给Session发送不可靠消息
		void send(const char* pMessage, uint16_t len);	

		//给Session发送不可靠NAT消息
		void send_nat(const char* pMessage, uint16_t Len);	

		//解析KCP数据并写入Recv缓存
		void input(char* pMessage, uint16_t Len);	

		char* read(uint16_t& Len);

		void read_over();

		//发送可靠消息，连接不断的情况下一定能够收到
		void send_reliable(const char* pMessage, uint16_t Len);

		//发送几段拼接起来的数据，用于发送长数据
		void send_reliable(std::vector<const char*>& VecMessage, std::vector <uint16_t>& VecLen);

		bool info(std::string& strIpAddr, uint16_t& Port);//获取IP地址和端口

		bool update();
	public:
		int32_t peer_id();
		peer::PeerInfo* peer_info();
	private:
		static uint32_t _clock();
		static int32_t output(const char* pMessage, int32_t Len, ikcpcb* pKcp, void* pSession);
	private://Session数据
		SOCKET							m_ListenFd;	
		SOCKET							m_ListenFdNAT;	
		sockaddr*						m_pSockaddr;
		sockaddr_in6					m_PeerAddr;
	private:
		base::Buffer					m_RecvBuffer;
	private://KCP相关
		ikcpcb*							m_Kcp;
		std::mutex						m_KcpMutex;
		std::condition_variable			m_CondVrb;
	private://Session状态
		std::atomic<uint8_t>			m_TimeoutCount;		//Heartbeat超时次数
		std::atomic<SessionStatus>		m_SessionStatus;
	private://流量控制
		//std::atomic<uint64_t>			m_MaxUpLoad;		//允许最大上行速率
		//std::atomic<uint64_t>			m_MaxDownLoad;		//允许最大下行速率
		std::atomic<uint64_t>			m_CurUpLoad;		//当前时间段上行速率
		std::atomic<uint64_t>			m_CurDownLoad;		//当前时间段下行速率
	private:
		peer::PeerInfo*					m_pPeerInfo;
	};
}