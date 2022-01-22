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

	void RoutingTable::get_node(const uint8_t Key[], std::unordered_set<int32_t>& PeerSet)
	{
		uint8_t Distance = Node::distance(m_Key, Key);
		for (int32_t Cur = Distance; Cur < KLEN_KEY; ++Cur)
		{
			m_VecKBucket[Cur].get_node(Key, PeerSet);
			if (PeerSet.size() >= KALPHA_REQUESTS)
			{
				return;
			}
		}
		for (int32_t Cur = Distance - 1; Cur >= 0; --Cur)
		{
			m_VecKBucket[Cur].get_node(Key, PeerSet);
			if (PeerSet.size() >= KALPHA_REQUESTS)
			{
				return;
			}
		}
	}
}