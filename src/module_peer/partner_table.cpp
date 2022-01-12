#include "partner_table.h"

namespace peer
{
	void PartnerTable::set_partner_max_num(uint16_t PartnerMaxNum)
	{
		m_PartnerMaxNum = PartnerMaxNum;
	}


	void PartnerTable::add_partner(const base::SHA1& CID, uint16_t SessionId)
	{
		std::unique_lock<std::mutex> Lock(m_PartnerMutex);
		//已经记录
		if (0 != m_PartnerMap.count(CID))
		{
			if (0 == m_PartnerMap[CID].count(SessionId))
			{
				m_PartnerMap[CID].insert(SessionId);
				m_PartnerContent[SessionId].push_back(CID);
			}
		}
		else
		{
			m_PartnerMap[CID].insert(SessionId);
			m_PartnerContent[SessionId].push_back(CID);
			m_CIDList.push_front(CID);
		}
	}

	void PartnerTable::delete_partner(uint16_t SessionId)
	{
		std::unique_lock<std::mutex> Lock(m_PartnerMutex);
		if (0 != m_PartnerContent.count(SessionId))
		{
			//删除与SessionId有关的CID中的SessionId
			for (auto& CID : m_PartnerContent[SessionId])
			{
				m_PartnerMap[CID].erase(SessionId);
				//如果为空
				if (true == m_PartnerMap[CID].empty())
				{
					m_PartnerMap.erase(CID);
				}
			}
			m_PartnerContent.erase(SessionId);
		}
	}

	void PartnerTable::delete_cid(const base::SHA1& CID)
	{
		std::unique_lock<std::mutex> Lock(m_PartnerMutex);
		if (0 != m_PartnerMap.count(CID))
		{
			//与CID有关的所有SessionId
			for (auto& SessionId : m_PartnerMap[CID])
			{
				for (auto It = m_PartnerContent[SessionId].begin(); It != m_PartnerContent[SessionId].end(); ++It)
				{
					//删除SessionId中的CID记录
					if ((*It) == CID)
					{
						m_PartnerContent[SessionId].erase(It);
						//如果为空
						if (true == m_PartnerContent[SessionId].empty())
						{
							m_PartnerContent.erase(SessionId);
						}
						break;
					}
				}
			}
		}
	}

	bool PartnerTable::get_cid(base::SHA1& CID)
	{
		if (true == m_CIDList.empty())
		{
			return false;
		}
		while (true != m_CIDList.empty())
		{
			//当前CID有效
			if (0 != m_PartnerMap.count(m_CIDList.front()))
			{
				CID = m_CIDList.front();
				m_CIDList.pop_front();
				m_CIDList.push_back(CID);
				return true;
			}
			else
			{
				m_CIDList.pop_front();
			}
		}
		return false;
	}
}