/***********************************************************************/
/* 名称:													   */
/* 说明:										   */
/* 创建时间:2022/01/07												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <mutex>
#include <set>
#include <list>
#include <vector>
#include <unordered_map>
#include <base/singleton.hpp>
#include <base/hash_function/file_sha1.h>

namespace peer
{
	typedef std::unordered_map<base::SHA1, 
		std::set<uint16_t>, 
		base::SHA1HashFunc, 
		base::SHA1EqualFunc>					PartnerMap;		//SHA1->Partner List
	typedef std::list<base::SHA1>				CIDList;		//对外提供自己感兴趣的CID
	typedef std::unordered_map <uint16_t,
		std::list<base::SHA1>>					PartnerContent;	//Partner->SHA1 List
	/** \brief PartnerTable用于管理所有与自己有直接关联的节点。
	 * 只有对相同CID感兴趣且保持连接的节点才会被加入PartnerTable。
	 * 只要找到Partner就能快速发现网络中对CID感兴趣的大量可靠节点。
	 */
	class PartnerTable
	{
	public:
		//设置同一个CID允许同时连接的最大Session数量
		void set_partner_max_num(uint16_t PartnerMaxNum);

		//添加一个感兴趣的CID
		void add_cid(const base::SHA1& CID);

		//为特定CID添加一个Partner
		void add_partner(const base::SHA1& CID, uint16_t SessionId);

		//删除一个Partner，并删除其所有CID中的记录
		void delete_partner(uint16_t SessionId);

		//查询一个Partner是否已经记录，存在返回true
		bool search_partner(const base::SHA1& CID, uint16_t SessionId);

		//删除一个CID，并删除与其相关的Partner
		void delete_cid(const base::SHA1& CID);

		//查询一个CID，如果存在返回true
		bool search_cid(const base::SHA1& CID);

		//查询一个CID，查询成功返回true，并将所有的Partnet加入PartnerList
		bool search_cid(const base::SHA1& CID, std::vector<uint16_t>& PartnerList);

		//获取一个CID，获取失败返回false
		//用于向其他节点扩散自己有CID这件事
		bool get_cid(base::SHA1& CID);
	private:
		uint16_t						m_PartnerMaxNum = 0;
		std::mutex						m_PartnerMutex;
		PartnerMap						m_PartnerMap;
		PartnerContent					m_PartnerContent;
		CIDList							m_CIDList;
	};
}
#define g_pPartnerTable base::Singleton<peer::PartnerTable>::get_instance()