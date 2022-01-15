#include "routing_table.h"
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
		LOG_ERROR << "Distance = " << Distance;
		m_VecKBucket[Distance].add_node(N);
	}

	void RoutingTable::get_node(const uint8_t Key[], std::list<int32_t>& PeerList)
	{
		uint8_t Distance = Node::distance(m_Key, Key);

		m_VecKBucket[Distance].get_node(Key, PeerList);
		if (PeerList.size() > KALPHA_REQUESTS)
		{
			PeerList.resize(KALPHA_REQUESTS);
		}
	}
}