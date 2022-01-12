/***********************************************************************/
/* ����:													   */
/* ˵��:										   */
/* ����ʱ��:2022/01/07												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <list>
#include <vector>
#include <mutex>
#include "node.h"

constexpr uint8_t KBUCKET_SIZE = 10;

namespace peer
{
	/** \brief KͰ���ڴ�Žڵ㣬����û��ɾ���Ľӿڣ�
	 * ԭ���������ڱ������֮ǰ����Ľڵ㣬����֮ǰ�Ľڵ��Ѿ��Ͽ�����ʧȥ��ϵ��
	 * KͰ�����ȱ���Good�Ľڵ㣬�������ӵĽڵ㡣
	 * ���ﵽ����ʱ�������Ȼ����̭״̬����Ľڵ㡣
	 * ������ڴ�ŵĶ���Good�ڵ㣬�Ͳ����¼�µĽڵ㡣
	 * ��ô�����Լ��ٶ���ڵ��Ӱ�죬���߸��õĽڵ�����ױ�������
	 */
	class KBucket
	{
	public:
		KBucket();

		//��ǰKͰ����ڵ�
		void add_node(Node& N);

		//����һ��Key�����ص�ǰKͰ�е����нڵ㣬�����վ�������
		void get_node(const uint8_t Key[], std::list<int32_t>& PeerList);
	private:
		std::list<Node>		m_NodeList;
		std::mutex			m_KBucketMutex;
	};
}