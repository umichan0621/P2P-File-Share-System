#include "peer_manager.h"
#include <base/logger/logger.h>

namespace peer
{
	PeerManager::PeerManager() :
		m_PeerCapacity(0),
		m_SessionCapacity(0),
		m_SessionNum(0) {}

	PeerManager::~PeerManager() {}

	void PeerManager::init(uint16_t SessionCapacity, int32_t PeerCapacity)
	{
		//最大允许连接数
		m_SessionCapacity = SessionCapacity < 65534 ? SessionCapacity : 65534;
		//初始桶的大小
		int32_t BucketSize = m_SessionCapacity * 15 / 10;
		m_SessionIdMap.rehash(BucketSize);

		m_PeerCapacity = PeerCapacity;
		BucketSize = PeerCapacity * 15 / 10;

		m_PeerIdMap.rehash(BucketSize);
		m_PeerMap.rehash(BucketSize);
	}

	int32_t PeerManager::peer_id(const PeerAddress& PeerAddr)
	{
		std::lock_guard<std::mutex> Lock(m_PeerManagerMutex);
		//没有记录，则创建
		if (0 == m_PeerIdMap.count(PeerAddr))
		{
			//已达到允许上限
			if (m_PeerCapacity == 0)
			{
				return 0;
			}
			--m_PeerCapacity;
			int32_t CurId = _peer_id();
			m_PeerIdMap[PeerAddr] = CurId;

			m_PeerMap[CurId] = Peer(PeerAddr);
			return CurId;
		}
		int32_t CurId = m_PeerIdMap[PeerAddr];
		if (CurId > 0)
		{
			++m_PeerMap[CurId].Count;
		}
		return CurId;
	}

	bool PeerManager::peer(int32_t PeerId, PeerAddress& PeerAddr, uint8_t& Status)
	{
		std::lock_guard<std::mutex> Lock(m_PeerManagerMutex);
		if (0 == m_PeerMap.count(PeerId))
		{
			return false;
		}
		Status = m_PeerMap[PeerId].Status;
		PeerAddr= m_PeerMap[PeerId].PeerAddr;
		return true;
	}

	uint8_t PeerManager::peer_status(int32_t PeerId)
	{
		std::lock_guard<std::mutex> Lock(m_PeerManagerMutex);
		if (0 == m_PeerMap.count(PeerId))
		{
			return BAD;
		}
		return m_PeerMap[PeerId].Status;
	}

	void PeerManager::free_peer(int32_t PeerId)
	{
		std::lock_guard<std::mutex> Lock(m_PeerManagerMutex);
		if (0 != m_PeerMap.count(PeerId))
		{
			//引用计数-1
			--m_PeerMap[PeerId].Count;
			if (0 == m_PeerMap[PeerId].Count)
			{
				const PeerAddress& CurPeerAddr= m_PeerMap[PeerId].PeerAddr;
				m_PeerIdMap.erase(CurPeerAddr);
				m_PeerMap.erase(PeerId);
				++m_PeerCapacity;
			}
		}
	}

	void PeerManager::free_peer(const std::vector<int32_t>& ArrPeerId)
	{
		std::lock_guard<std::mutex> Lock(m_PeerManagerMutex);
		for (auto& PeerId : ArrPeerId)
		{
			if (0 != m_PeerMap.count(PeerId))
			{
				//引用计数-1
				--m_PeerMap[PeerId].Count;
				if (0 == m_PeerMap[PeerId].Count)
				{
					const PeerAddress& CurPeerAddr = m_PeerMap[PeerId].PeerAddr;
					m_PeerIdMap.erase(CurPeerAddr);
					m_PeerMap.erase(PeerId);
					++m_PeerCapacity;
				}
			}
		}
	}

	void PeerManager::ban_peer(const PeerAddress& PeerAddr)
	{
		std::lock_guard<std::mutex> Lock(m_PeerManagerMutex);
		if (0 != m_PeerIdMap.count(PeerAddr))
		{
			int32_t CurId = m_PeerIdMap[PeerAddr];
			m_PeerIdMap[PeerAddr] = -1;
			if (0 != m_PeerMap.count(CurId))
			{
				m_PeerIdMap.erase(PeerAddr);
			}
		}
	}

	void PeerManager::get_sockaddr(PeerAddress& PeerAddr, const char* pIPAddressess, uint16_t Port) const
	{
		sockaddr_in* pSockaddr = (sockaddr_in*)&PeerAddr;
		pSockaddr->sin_family = AF_INET;
		inet_pton(AF_INET, pIPAddressess, &pSockaddr->sin_addr);		//设定新连接的IP
		pSockaddr->sin_port = htons(Port);							//设定新连接的端口
	}

	void PeerManager::get_sockaddr6(PeerAddress& PeerAddr, const char* pIPAddressess, uint16_t Port) const
	{
		sockaddr_in6* pSockaddr = &PeerAddr;
		pSockaddr->sin6_family = AF_INET6;
		inet_pton(AF_INET6, pIPAddressess, &pSockaddr->sin6_addr);		//设定新连接的IP
		pSockaddr->sin6_port = htons(Port);							//设定新连接的端口
	}

	uint16_t PeerManager::session_id(const PeerAddress& PeerAddr)
	{
		std::lock_guard<std::mutex> Lock(m_PeerManagerMutex);
		//当前sockaddr未登记
		if (0 == m_SessionIdMap.count(PeerAddr))
		{
			return 0;
		}
		return 	m_SessionIdMap[PeerAddr];
	}

	uint16_t PeerManager::connect_peer(const PeerAddress& PeerAddr)
	{
		std::unique_lock<std::mutex> Lock(m_PeerManagerMutex);
		//已经存在当前sockaddr
		if (0 != m_SessionIdMap.count(PeerAddr))
		{
			return 	m_SessionIdMap[PeerAddr];
		}
		//获取未分配过的SessionId
		uint16_t SessionId = _session_id();
		//获取失败
		if (SessionId == ERROR_SESSION_ID)
			return SessionId;
		//创建Peer相关信息
		{
			int32_t CurId;
			//没有记录，则创建
			if (0 == m_PeerIdMap.count(PeerAddr))
			{
				//已达到允许上限
				if (0==m_PeerCapacity)
				{
					return ERROR_SESSION_ID;
				}
				--m_PeerCapacity;
				CurId = _peer_id();
				m_PeerIdMap[PeerAddr] = CurId;
				m_PeerMap[CurId] = Peer(PeerAddr);
			}
			else
			{
				CurId = m_PeerIdMap[PeerAddr];
				if (CurId > 0)
				{
					++m_PeerMap[CurId].Count;
				}
			}
			//当前连接建立，状态为Good
			m_PeerMap[CurId].Status = GOOD;
		}
		//建立映射
		m_SessionIdMap[PeerAddr] = SessionId;
		return SessionId;
	}

	void PeerManager::disconnect_peer(const PeerAddress& PeerAddr)
	{
		std::lock_guard<std::mutex> Lock(m_PeerManagerMutex);
		if (0 == m_SessionIdMap.count(PeerAddr))
		{
			return;
		}
		//删除sockaddr*->SessionId的映射
		m_SessionIdMap.erase(PeerAddr);
		//改变节点状态
		int32_t CurId = m_PeerIdMap[PeerAddr];
		if (0 != m_PeerMap.count(CurId))
		{
			--m_PeerMap[CurId].Count;

			if (0 == m_PeerMap[CurId].Count)
			{
				m_PeerIdMap.erase(PeerAddr);
				m_PeerMap.erase(CurId);
				++m_PeerCapacity;
			}
			else
			{
				m_PeerMap[CurId].Status = QUESTIONALBE;
			}
		}
	}

	void PeerManager::recycle_session(uint16_t SessionId)
	{
		std::lock_guard<std::mutex> Lock(m_PeerManagerMutex);
		m_SessionIdQueue.push_back(SessionId);
	}

	uint16_t PeerManager::_session_id()
	{
		//有已回收的，分配已回收的
		if (!m_SessionIdQueue.empty())
		{
			uint16_t SessionId = m_SessionIdQueue.front();
			m_SessionIdQueue.pop_front();
			return SessionId;
		}
		//容量不足
		if (m_SessionNum == m_SessionCapacity)
		{
			return ERROR_SESSION_ID;
		}
		++m_SessionNum;
		return m_SessionNum;
	}

	int32_t PeerManager::_peer_id()
	{
		int32_t RandId = rand();
		for (;;)
		{
			//当前Id没用过
			if (0 == m_PeerMap.count(RandId))
			{
				return RandId;
			}
			RandId = rand();
		}
		return -1;
	}

	uint16_t PeerManager::register_pop()
	{
		std::lock_guard<std::mutex> Lock(m_SearchMutex);
		if (true == m_PrepareToRegister.empty())
		{
			return ERROR_SESSION_ID;
		}
		uint16_t SessionId = m_PrepareToRegister.front();
		m_PrepareToRegister.pop_front();
		return SessionId;
	}

	void PeerManager::register_push(uint16_t SessionId)
	{
		std::lock_guard<std::mutex> Lock(m_SearchMutex);
		m_PrepareToRegister.push_back(SessionId);
	}

	int32_t PeerManager::search_pop()
	{
		std::lock_guard<std::mutex> Lock(m_ConnectMutex);
		if (true == m_PrepareToConnect.empty())
		{
			return -1;
		}
		int32_t PeerId = m_PrepareToConnect.front();
		m_PrepareToConnect.pop_front();
		return PeerId;
	}

	void PeerManager::search_push(int32_t PeerId)
	{
		std::lock_guard<std::mutex> Lock(m_ConnectMutex);
		m_PrepareToConnect.push_back(PeerId);
	}

	bool PeerManager::info(const PeerAddress& PeerAddr, std::string& strIP, uint16_t& Port)
	{
		sockaddr* pSockaddr = (sockaddr*)&PeerAddr;
		if (AF_INET == pSockaddr->sa_family)//IPV4
		{
			char Buf[INET_ADDRSTRLEN];
			Port = ntohs(((sockaddr_in*)pSockaddr)->sin_port);
			if (NULL != inet_ntop(pSockaddr->sa_family, &((sockaddr_in*)pSockaddr)->sin_addr,
				Buf, INET_ADDRSTRLEN))
			{
				strIP = Buf;
			}
			else
			{
				return false;
			}
		}
		else if (AF_INET6 == pSockaddr->sa_family)//IPV6
		{
			char Buf[INET6_ADDRSTRLEN];
			Port = ntohs(((sockaddr_in6*)pSockaddr)->sin6_port);
			if (NULL != inet_ntop(pSockaddr->sa_family, &((sockaddr_in6*)pSockaddr)->sin6_addr,
				Buf, INET_ADDRSTRLEN))
			{
				strIP = Buf;
			}
			else
			{
				return false;
			}
		}
		return true;
	}

	uint16_t PeerManager::connection_num()
	{
		return m_SessionNum;
	}
}