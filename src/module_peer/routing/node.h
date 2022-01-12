/***********************************************************************/
/* ����:													   */
/* ˵��:										   */
/* ����ʱ��:2022/01/07												   */
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

constexpr uint8_t KLEN_KEY = 20;//=SHA1����
namespace peer
{
	class Node
	{
	public:
		Node() = delete;
		Node(const uint8_t Key[], int32_t PeerId);
	public:
		//���ص�ǰ�ڵ��PeerId
		int32_t peer_id() const;

		PeerStatus node_status() const;

		//��������Key֮��ľ���
		//���붨��Ϊ�����ڵ�Keyֵ���֮��ǰ��0�ĸ���
		//��λӰ��󣬵�λӰ��С
		//ֵԽС����Խ����ֵԽ�����ԽԶ
		static uint8_t distance(const uint8_t Key1[], const uint8_t Key2[]);

		//��������Node֮��ľ���
		static uint8_t distance(const Node& N1, const Node& N2);

		//�����Լ���һ��Key�ľ���
		uint8_t distance(const uint8_t Key[]) const;

		//�����Լ��������ڵ�ľ���
		uint8_t distance(const Node& N) const;

		//��̭��ǰ�ڵ�
		//ΪPeer�ļ���-1
		void free();
	private:
		uint8_t					m_Key[KLEN_KEY];
		int32_t					m_PeerId;			//��¼PeerId
	};
}
