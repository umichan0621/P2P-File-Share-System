#include "routing_table.h"
#include <unordered_set>
#include <base/logger/logger.h>

namespace peer
{
	void RoutingTable::init()
	{
		for (int32_t i = 0; i < 20; ++i)
		{
			m_Key[i] = rand() % 128;
		}
	}

	const uint8_t* RoutingTable::pid()
	{
		return m_Key;
	}

	void RoutingTable::add_node(Node& N)
	{
		uint8_t Distance = N.distance(m_Key);
		m_VecKBucket[Distance].add_node(N);
	}

	void RoutingTable::get_node(const uint8_t Key[], int32_t ReqPeerId, std::unordered_set<int32_t>& PeerSet)
	{
		//计算当前Key与PID的距离
		uint8_t Distance = Node::distance(m_Key, Key);

		//如果有，先从路由表中得到一个在线的PeerId
		{
			//K桶从左到右
			for (int32_t Cur = Distance; Cur < KLEN_KEY; ++Cur)
			{
				int32_t PeerId = m_VecKBucket[Cur].get_online_node(ReqPeerId);
				if (-1 != PeerId)
				{
					PeerSet.insert(PeerId);
					break;
				}
			}
			if (true == PeerSet.empty())
			{
				//K桶从右到左
				for (int32_t Cur = Distance - 1; Cur >= 0; --Cur)
				{
					int32_t PeerId = m_VecKBucket[Cur].get_online_node(ReqPeerId);
					if (-1 != PeerId)
					{
						PeerSet.insert(PeerId);
						break;
					}
				}
			}
		}
		//从路由表中得到PeerId，直到达到ALPHA个
		{
			for (int32_t Cur = Distance; Cur < KLEN_KEY; ++Cur)
			{
				m_VecKBucket[Cur].get_node(Key, ReqPeerId, PeerSet);
				if (PeerSet.size() >= KALPHA_REQUESTS)
				{
					return;
				}
			}
			for (int32_t Cur = Distance - 1; Cur >= 0; --Cur)
			{
				m_VecKBucket[Cur].get_node(Key, ReqPeerId, PeerSet);
				if (PeerSet.size() >= KALPHA_REQUESTS)
				{
					return;
				}
			}
		}
	}
}