/***********************************************************************/
/* 名称:协议头														   */
/* 说明:通信时UDP包的协议头										       */
/* 创建时间:2021/02/22												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <stdint.h>

constexpr uint16_t		BASE_HEADER_LEN = 2;
constexpr uint16_t		KCP_HEADER_LEN	= 24;

enum PROTOCOL_TYPE
{
	PROTOCOL_TYPE_NULL=-1,
	PROTOCOL_TYPE_BASE,
	PROTOCOL_TYPE_DHT,
	PROTOCOL_TYPE_FILE,
};

//通信协议
enum PROTOCOL_BASE
{
	PROTOCOL_BASE_NULL = PROTOCOL_TYPE::PROTOCOL_TYPE_BASE << 10,
	PROTOCOL_BASE_HEARTBEAT_REQ,				//与其他节点的心跳包
	PROTOCOL_BASE_HEARTBEAT_ACK,
	PROTOCOL_BASE_CONNECT_REQ,					//连接其他节点
	PROTOCOL_BASE_CONNECT_ACK,
	PROTOCOL_BASE_CONNECT_RFS,
	PROTOCOL_BASE_NAT_TYPE_PROBE_REQ,			//NAT类型探测，可探测类型是B+/C-
	PROTOCOL_BASE_NAT_TYPE_PROBE_ACK,
	PROTOCOL_BASE_PING_REQ,
	PROTOCOL_BASE_PING_ACK,
	PROTOCOL_BASE_DISCONNECT,					//主动断连，如果发送失败会通过心跳包断连
	PROTOCOL_BASE_END
};

//基础的协议头
inline void create_header(char* pDes, uint16_t ProtocolId)
{
	pDes[0] = (char)(ProtocolId >> 8);
	pDes[1] = (char)(ProtocolId & (0xff));
}
//解析协议头
inline void parse_header(const char* pSrc, uint16_t& ProtocolId)
{
	//memcpy(&ProtocolId, pSrc, 2);
	ProtocolId = pSrc[0];
	ProtocolId <<= 8;
	ProtocolId |= pSrc[1];
}
//解析协议类型
inline void parse_type(const char* pSrc, uint16_t& ProtocolType)
{
	ProtocolType = pSrc[0]>>2;
	ProtocolType &= 0x3f;
}