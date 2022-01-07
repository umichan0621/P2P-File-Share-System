/***********************************************************************/
/* 名称:文件事件处理器												   */
/* 说明:用于处理文件的回调事件，降低耦合度						       */
/* 创建时间:2021/11/02												   */
/* Email:umichan0621@gmail.com  									   */
/***********************************************************************/
#pragma once
#include "handler_interface.h"
#include <base/protocol/protocol_file.h>

namespace handler
{
	class HandlerFile :public HandlerInterface
	{
	public:
		HandlerFile();
	public:
		int8_t handle_event(uint16_t& SessionId, char* pMessage, uint16_t& Len) override;
		void register_recv_event();
		void register_gateway_event();
	private:
		int8_t handle_file_fragment_req(uint16_t& SessionId, char* pMessage, uint16_t& Len);
		int8_t handle_file_fragment_ack(uint16_t& SessionId, char* pMessage, uint16_t& Len);
		int8_t handle_file_md5_check_req(uint16_t& SessionId, char* pMessage, uint16_t& Len);
		int8_t handle_file_md5_check_ack(uint16_t& SessionId, char* pMessage, uint16_t& Len);
	private:
		HandlerMap		m_RecvHandlerMap;
	};
}
