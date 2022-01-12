/***********************************************************************/
/* 名称:													   */
/* 说明:										   */
/* 创建时间:2022/01/07												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <vector>
#include <stdint.h>
//#ifndef __linux__
//#include <ws2tcpip.h>
//#else

//#endif
#include <module_peer/peer_manager.h>

constexpr uint8_t KLEN_KEY = 20;//=SHA1长度
namespace peer
{
	class Node
	{
	public:
		Node() = delete;
		Node(const uint8_t Key[], int32_t PeerId);
	public:
		//返回当前节点的PeerId
		int32_t peer_id() const;

		PeerStatus node_status() const;

		//计算两个Key之间的距离
		//距离定义为两个节点Key值异或之后前面0的个数
		//高位影响大，低位影响小
		//值越小距离越近，值越大距离越远
		static uint8_t distance(const uint8_t Key1[], const uint8_t Key2[]);

		//计算两个Node之间的距离
		static uint8_t distance(const Node& N1, const Node& N2);

		//计算自己和一个Key的距离
		uint8_t distance(const uint8_t Key[]) const;

		//计算自己与其他节点的距离
		uint8_t distance(const Node& N) const;

		//淘汰当前节点
		//为Peer的计数-1
		void free();
	private:
		uint8_t					m_Key[KLEN_KEY];
		int32_t					m_PeerId;			//记录PeerId
	};
}
