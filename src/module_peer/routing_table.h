/***********************************************************************/
/* ����:													   */
/* ˵��:										   */
/* ����ʱ��:2022/01/07												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#include "routing/k_bucket.h"
#include <base/singleton.hpp>

constexpr uint8_t KALPHA_REQUESTS = 3;

namespace peer
{
	/** \brief RoutingTable���ڹ������������ڵ����Ϣ��������Ϊ��·�ɱ�
	 * PID��Peer ID�����ڱ�ʶDHT�����еĽڵ㣬������Ӧ����Ψһ�ģ�ʵ���ϳ�ͻҲ��Ӱ�졣
	 * ÿ���ڵ㶼���Լ�������ɵ�PID��·�ɱ�������ڼ�¼�����Լ�PID�����PID����CID��
	 * CID��Content ID�����ڱ�ʶ�����е�ĳһ����񣬱���ĳ���ļ�������ĳ��Ӧ�õ�ID��
	 * �������ǰ�������Ľڵ�ۼ���һ��һ���ҵ�һ���ڵ㣬�Ϳ��Կ��ٷ���������ĸ���ڵ㡣
	 */
	class RoutingTable
	{
	public:
		void init();

		//�����Լ�������ɵ�PID�������������ڵ�Ǽ���Ϣ
		const uint8_t* pid();

		//��DHT����ڵ�
		void add_node(Node& N);

		//��ѯKey������ALPHA���ڵ���Ϣ
		//���DHT�е��ܽڵ���С��ALPHA��ֻ�����ܵ�����
		void get_node(const uint8_t Key[], std::list<int32_t>& PeerList);
	private:
		uint8_t						m_Key[KLEN_KEY] = { 0 };
		KBucket						m_VecKBucket[160];
	};
}
#define g_pRoutingTable base::Singleton<peer::RoutingTable>::get_instance()