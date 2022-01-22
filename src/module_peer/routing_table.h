/***********************************************************************/
/* 名称:													   */
/* 说明:										   */
/* 创建时间:2022/01/07												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#include "routing/k_bucket.h"
#include <base/singleton.hpp>



namespace peer
{
	/** \brief RoutingTable用于管理所有其他节点的信息，可以认为是路由表。
	 * PID：Peer ID，用于标识DHT网络中的节点，理论上应该是唯一的，实际上冲突也不影响。
	 * 每个节点都有自己随机生成的PID，路由表会倾向于记录距离自己PID最近的PID或者CID。
	 * CID：Content ID，用于标识网络中的某一项服务，比如某个文件，或者某个应用的ID。
	 * 其作用是把相关联的节点聚集在一起，一旦找到一个节点，就可以快速发现相关联的更多节点。
	 */
	class RoutingTable
	{
	public:
		void init();

		//返回自己随机生成的PID，用于向其他节点登记信息
		const uint8_t* pid();

		//向DHT加入节点
		void add_node(Node& N);

		//查询Key，返回ALPHA个节点信息
		//如果DHT中的总节点数小于ALPHA，只返回总的数量
		void get_node(const uint8_t Key[], std::unordered_set<int32_t>& PeerSet);
	private:
		uint8_t						m_Key[KLEN_KEY] = { 0 };
		KBucket						m_VecKBucket[160];
	};
}
#define g_pRoutingTable base::Singleton<peer::RoutingTable>::get_instance()