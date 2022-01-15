/***********************************************************************/
/* 名称:Session ID映射组件											   */
/* 说明:将sockaddr*与Session ID绑定，简化接口						   */
/* 创建时间:2021/05/08												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <list>
#include <mutex>
#include <vector>
#include <unordered_map>

#ifndef __linux__
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif
#include <base/object_pool.hpp>
#include <base/singleton.hpp>
#pragma warning(disable : 6297)
#pragma warning(disable : 26812)

constexpr uint16_t	ERROR_SESSION_ID = 0xffff;
constexpr uint8_t	KSOCKADDR_LEN_V6 = 28;

namespace peer
{
	enum PeerStatus
	{
		GOOD = 0,
		QUESTIONALBE = 1,
		BAD = 2
	};

	//Node:SHA1值的Key寻找的目标，一个Node记录了对当前Key感兴趣的Peer，Node等价于list[Peer]
	//Peer:与路由表有关联的节点，可能彼此都没有感兴趣的文件，但是为了维护路由表而有关联
	//Session:与Peer一样，但是相互之间有可靠的UDP连接，更适合推荐给其他节点
	//当前类用于管理所有的Peer和Session
	class PeerManager
	{
	private:
		struct Peer
		{
			sockaddr*	PeerAddress;	//存储Peer的地址
			uint8_t		Count;			//记录同时使用当前Peer的数量
			uint8_t	    Status;			//Peer的状态
			Peer() :
				PeerAddress(nullptr),
				Count(0),
				Status(BAD) {}
			Peer(sockaddr* pSockaddr) :
				PeerAddress(pSockaddr),
				Count(1),
				Status(QUESTIONALBE) {}
		};
		struct HashFunc					//自定义sockaddr数据结构的哈希函数
		{
			size_t operator()(const sockaddr* pSockaddr) const
			{
				size_t Res = 0xdeadbeef;
				for (uint16_t i = 0; i < 7; ++i)
				{
					size_t Temp = pSockaddr->sa_data[i];
					Res ^= Temp << i;
				}
				return Res;
			}
		};
		struct EqualFunc				//自定义sockaddr数据结构的相等条件
		{
			bool operator()(const sockaddr* pSockaddr1, const sockaddr* pSockaddr2) const
			{
				for (uint16_t i = 0; i < 14; ++i)
				{
					if (pSockaddr1->sa_data[i] != pSockaddr2->sa_data[i])
						return false;
				}
				return pSockaddr1->sa_family == pSockaddr2->sa_family;
			}
		};
		typedef std::unordered_map<sockaddr*, uint16_t, HashFunc, EqualFunc>	SessionIdMap;
		typedef std::unordered_map<sockaddr*, int32_t, HashFunc, EqualFunc>		PeerIdMap;
		typedef std::unordered_map<int32_t, Peer>								PeerMap;
		//适配IPv4和IPv6
		typedef base::ObjectPool<sockaddr_in6>									SockaddrPool;
	public:
		PeerManager();
		~PeerManager();
	public:
		//初始化，设定允许的最大Session连接数和路由表最大Peer数
		void init(uint16_t SessionCapacity, int32_t PeerCapacity);

		//根据输入的sockaddr得到一个PeerId，如果之前没有存储会随机生成
		//返回值<0表示当前Peer已加入黑名单
		//返回值==0表示当前路由表可记录Peer已达上限
		//每次调用引用+1
		int32_t peer_id(sockaddr* pSockaddr);

		//返回Peer的引用
		sockaddr* peer(int32_t PeerId, uint8_t& Status);

		uint8_t peer_status(int32_t PeerId);

		//释放一个对Peer的引用
		//每次调用引用-1
		//如果引用为0会删除这个Peer的信息
		void free_peer(int32_t PeerId);

		//批量释放Peer的引用
		//在淘汰Node时使用
		void free_peer(const std::vector<int32_t>& ArrPeerId);

		//将一个Peer加入黑名单，路由表不再记录它的信息
		void ban_peer(sockaddr* pSockaddr);

		sockaddr* get_sockaddr(const char* pIPAddressess, uint16_t Port) const;

		sockaddr* get_sockaddr6(const char* pIPAddressess, uint16_t Port) const;

		//根据输入的Sockaddr得到一个已连接的SessionId
		//返回0表示当前sockaddr未登记
		//返回0xffff表示sockaddr有误
		uint16_t session_id(sockaddr* pSockaddr);

		//成功返回分配的SessionId，失败返回0xffff
		//连接成功后Peer状态为Good
		uint16_t connect_peer(sockaddr* pSockaddr);

		//断开连接，计数-1，状态变为Questionable
		void disconnect_peer(sockaddr* pSockaddr);

		//回收之前分配出去的SessionId
		//应该由定时器调用，延时回收
		void recycle_session(uint16_t SessionId);

		//使用内置的对象池获取一个sockaddr
		sockaddr* allocate_sockaddr();

		void release_sockaddr(sockaddr* pSockaddr);

		//获取一个还没有查询过的Peer
		uint16_t search_pop();

		//加入一个没有查询过的Peer
		void search_push(uint16_t SessionId);

		//获取一个等待连接的Peer
		int32_t connect_pop();

		//加入一个没有连接的Peer
		void connect_push(int32_t PeerId);

		static bool info(sockaddr* pSockaddr, std::string& strIP, uint16_t& Port);
	private:

		//获取一个没有使用的SessionId
		uint16_t _session_id();

		//随机生成一个没有使用的PeerId
		int32_t _peer_id();
	private:
		SockaddrPool				m_SockaddrPool;		//用于分配和回收Sockaddr*
		std::mutex					m_PeerManagerMutex;
	private://Peer相关
		int32_t						m_PeerCapacity;		//Peer上限
		PeerIdMap					m_PeerIdMap;		//Sockaddr*->PeerId的映射
		PeerMap						m_PeerMap;			//PeerId->Peer的映射
	private://Session相关
		uint16_t					m_SessionCapacity;	//Session上限
		uint16_t					m_SessionNum;		//当前Session数
		SessionIdMap				m_SessionIdMap;		//Sockaddr*->SessionId的映射
		std::list<uint16_t>			m_SessionIdQueue;	//用于分配未使用的SessionId
	private:
		std::list<uint16_t>			m_PrepareToSearch;	//等待发起搜索的Session
		std::list<int32_t>			m_PrepareToConnect;	//等待建立连接的Peer
		std::mutex					m_SearchMutex;
		std::mutex					m_ConnectMutex;
	};
}
#define g_pPeerManager base::Singleton<peer::PeerManager>::get_instance()