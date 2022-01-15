#include "k_bucket.h"
#include <map>

namespace peer
{
	KBucket::KBucket() {}

	void KBucket::add_node(Node& N)
	{
		std::lock_guard<std::mutex> Lock(m_KBucketMutex);
		//KͰ��û��
		if (m_NodeList.size() < KBUCKET_SIZE)
		{
			m_NodeList.push_back(std::move(N));
		}
		else
		{
			uint8_t CurStatus = N.node_status();
			for (int32_t i = 0; i < m_NodeList.size(); ++i)
			{
				//��ǰ�ڵ���½ڵ�״̬��
				if (m_NodeList.front().node_status() > CurStatus)
				{
					//��̭�ɽڵ㣬����-1
					m_NodeList.front().free();
					m_NodeList.pop_front();
					//�½ڵ�����β
					m_NodeList.push_back(std::move(N));
					return;
				}
				else
				{
					m_NodeList.push_back(std::move(m_NodeList.front()));
					m_NodeList.pop_front();
				}
			}
		}
		//�ߵ������ʾ��������½ڵ㣬Ҳ������̭�ɽڵ�
	}

	void KBucket::get_node(const uint8_t Key[], std::list<int32_t>& PeerList)
	{
		std::lock_guard<std::mutex> Lock(m_KBucketMutex);
		std::map<uint8_t, int32_t> NearNodeMap;
		for (auto& Node : m_NodeList)
		{
			NearNodeMap.insert({ Node.distance(Key) ,Node.peer_id() });
		}
		for (auto& It : NearNodeMap)
		{
			PeerList.push_back(It.second);
		}
	}
}