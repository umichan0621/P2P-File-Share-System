#include "node.h"
#include <cstring>

namespace peer
{
	Node::Node(const uint8_t Key[], int32_t PeerId)
		:m_PeerId(PeerId)
	{
		memcpy(m_Key, Key, KLEN_KEY);
	}

	int32_t Node::peer_id() const
	{
		return m_PeerId;
	}

	uint8_t Node::node_status() const
	{
		return g_pPeerManager->peer_status(m_PeerId);
	}

	uint8_t Node::distance(const uint8_t Key1[], const uint8_t Key2[])
	{
		int8_t i = 0, j = 0;
		for (; i < 20; ++i)
		{
			if (Key1[i] != Key2[i])
			{
				break;
			}
		}
		if (20 == i)
		{
			return 0;
		}
		uint8_t XOR = Key1[i] ^ Key2[i];
		while ((XOR & 0x80) == 0)
		{
			XOR <<= 1;
			j++;
		}
		return 159 - j - (i << 3);
	}

	uint8_t Node::distance(const Node& N1, const Node& N2)
	{
		return distance(N1.m_Key, N2.m_Key);
	}

	uint8_t Node::distance(const uint8_t Key[]) const
	{
		return distance(m_Key, Key);
	}

	uint8_t Node::distance(const Node& N) const
	{
		return distance(m_Key, N.m_Key);
	}

	void Node::free()
	{
		g_pPeerManager->free_peer(m_PeerId);
	}
}
