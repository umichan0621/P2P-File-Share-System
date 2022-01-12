/***********************************************************************/
/* ����:													   */
/* ˵��:										   */
/* ����ʱ��:2022/01/07												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <mutex>
#include <set>
#include <list>
#include <unordered_map>
#include <base/singleton.hpp>
#include <base/hash_function/file_sha1.h>

namespace peer
{
	typedef std::unordered_map<base::SHA1, 
		std::set<uint16_t>, 
		base::SHA1HashFunc, 
		base::SHA1EqualFunc>					PartnerMap;		//SHA1->Partner List
	typedef std::list<base::SHA1>				CIDList;		//�����ṩ�Լ�����Ȥ��CID
	typedef std::unordered_map <uint16_t,
		std::list<base::SHA1>>					PartnerContent;	//Partner->SHA1 List
	/** \brief PartnerTable���ڹ����������Լ���ֱ�ӹ����Ľڵ㡣
	 * ֻ�ж���ͬCID����Ȥ�ұ������ӵĽڵ�Żᱻ����PartnerTable��
	 * ֻҪ�ҵ�Partner���ܿ��ٷ��������ж�CID����Ȥ�Ĵ����ɿ��ڵ㡣
	 */
	class PartnerTable
	{
		//����ͬһ��CID����ͬʱ���ӵ����Session����
		void set_partner_max_num(uint16_t PartnerMaxNum);

		//Ϊ�ض�CID���һ��Partner
		void add_partner(const base::SHA1& CID, uint16_t SessionId);

		//ɾ��һ��Partner����ɾ��������CID�еļ�¼
		void delete_partner(uint16_t SessionId);

		//ɾ��һ��CID����ɾ��������ص�Partner
		void delete_cid(const base::SHA1& CID);

		//��ȡһ��CID����ȡʧ�ܷ���false
		//�����������ڵ���ɢ�Լ���CID�����
		bool get_cid(base::SHA1& CID);
	private:
		uint16_t						m_PartnerMaxNum;
		std::mutex						m_PartnerMutex;
		PartnerMap						m_PartnerMap;
		PartnerContent					m_PartnerContent;
		CIDList							m_CIDList;
	};
}
#define g_pPartnerTable base::Singleton<peer::PartnerTable>::get_instance()