/***********************************************************************/
/* 名称:													   */
/* 说明:										   */
/* 创建时间:2022/01/07												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <list>
#include <vector>
#include <mutex>
#include <unordered_set>
#include "node.h"
constexpr uint8_t KBUCKET_SIZE = 10;
constexpr uint8_t KALPHA_REQUESTS = 3;

namespace peer
{
	/** \brief K桶用于存放节点，对外没有删除的接口，
	 * 原则上倾向于保存更早之前加入的节点，除非之前的节点已经断开连接失去联系。
	 * K桶会优先保存Good的节点，即有连接的节点。
	 * 当达到上限时，会遍历然后淘汰状态更差的节点。
	 * 如果现在存放的都是Good节点，就不会记录新的节点。
	 * 这么做可以减少恶意节点的影响，在线更久的节点更容易被保留。
	 */
	class KBucket
	{
	public:
		KBucket();

		//向当前K桶加入节点
		void add_node(Node& N);

		//输入一个Key，返回当前K桶中的所有节点，并按照距离排序
		void get_node(const uint8_t Key[], std::unordered_set<int32_t>& PeerList);
	private:
		std::list<Node>		m_NodeList;
		std::mutex			m_KBucketMutex;
	};
}