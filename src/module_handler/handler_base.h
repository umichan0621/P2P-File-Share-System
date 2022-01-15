/***********************************************************************/
/* 名称:基础事件处理器												   */
/* 说明:用于处理基础的回调事件，降低耦合度						       */
/* 创建时间:2021/10/10												   */
/* Email:umichan0621@gmail.com  									   */
/***********************************************************************/
#pragma once
#include "handler_interface.h"
#include <base/protocol/protocol_base.h>

namespace handler
{
	class HandlerBase :public HandlerInterface
	{
	public:
		HandlerBase();
	public:
		int8_t handle_event(uint16_t& SessionId, char* pMessage, uint16_t& Len) override;
		void register_recv_event();
		void register_gateway_event();
	private:
		int8_t handle_connect_req(uint16_t& SessionId, char* pMessage, uint16_t& Len);
		int8_t handle_connect_ack(uint16_t& SessionId, char* pMessage, uint16_t& Len);
		int8_t handle_connect_rfs(uint16_t& SessionId, char* pMessage, uint16_t& Len);
		int8_t handle_connect_help_req(uint16_t& SessionId, char* pMessage, uint16_t& Len);
		int8_t handle_connect_help_ack(uint16_t& SessionId, char* pMessage, uint16_t& Len);
		int8_t handle_connect_help_rfs(uint16_t& SessionId, char* pMessage, uint16_t& Len);
		int8_t handle_ping_req(uint16_t& SessionId, char* pMessage, uint16_t& Len);
		int8_t handle_ping_ack(uint16_t& SessionId, char* pMessage, uint16_t& Len);
		int8_t handle_ping_help(uint16_t& SessionId, char* pMessage, uint16_t& Len);
		int8_t handle_heartbeat_req(uint16_t& SessionId, char* pMessage, uint16_t& Len);
		int8_t handle_disconnect(uint16_t& SessionId, char* pMessage, uint16_t& Len);
		int8_t handle_nat_probe_req(uint16_t& SessionId, char* pMessage, uint16_t& Len);
		int8_t handle_nat_probe_ack(uint16_t& SessionId, char* pMessage, uint16_t& Len);
	private:
		HandlerMap		m_RecvHandlerMap;
	};
}
