/***********************************************************************/
/* 名称:事件处理器													   */
/* 说明:用于处理Net UDP的回调事件  									   */
/* 创建时间:2021/10/06												   */
/* Email:umichan0621@gmail.com  									   */
/***********************************************************************/
#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include "handler_interface.h"

namespace handler
{
	class HandlerManager
	{
	public:
		HandlerManager();
		~HandlerManager();
	public:
		void register_event(uint16_t ProtocolType, HandlerInterface* pHandler);
		bool handle_on_accept(uint16_t& SessionId);
		bool handle_on_disconnect(uint16_t& SessionId);
		//返回true允许建立连接
		int8_t handle_on_gateway(char* pMessage, uint16_t& Len);
		//返回false断开连接
		int8_t handle_on_recv(uint16_t& SessionId, char* pMessage, uint16_t& Len);
	private:
		std::vector<HandlerInterface*> m_RecvEventMap;
		std::vector<HandlerInterface*> m_GatewayEventMap;
	};
}
