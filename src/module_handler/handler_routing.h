/***********************************************************************/
/* 名称:路由事件处理器												   */
/* 说明:用于处理路由的回调事件，降低耦合度						       */
/* 创建时间:2021/01/12												   */
/* Email:umichan0621@gmail.com  									   */
/***********************************************************************/
#pragma once
#include "handler_interface.h"
#include <base/protocol/protocol_routing.h>
#ifdef __linux__
#define TRACKER_MODE
#else
#include <module_peer/partner_table.h>
#endif
namespace handler
{
	class HandlerRouting :public HandlerInterface
	{
	public:
		HandlerRouting();
	public:
		int8_t handle_event(uint16_t& SessionId, char* pMessage, uint16_t& Len) override;
		void register_recv_event();
		void register_gateway_event();
	private:
		//接收到其他节点发送的查询请求
		//如果命中PartnerTable，那么就加入ParterTable并回复routing_register
		//如果没有命中PartnerTable，就加入RoutingTable
		//然后回复距离接近的节点信息routing_search_ack
		//最后会发送自己的PID和几个CID，routing_register
		int8_t handle_routing_search_req(uint16_t& SessionId, char* pMessage, uint16_t& Len);

		int8_t handle_routing_search_ack(uint16_t& SessionId, char* pMessage, uint16_t& Len);

		int8_t handle_routing_register(uint16_t& SessionId, char* pMessage, uint16_t& Len);

		int8_t handle_routing_partner(uint16_t& SessionId, char* pMessage, uint16_t& Len);
	private:
		HandlerMap		m_RecvHandlerMap;
	};
}
