/***********************************************************************/
/* 名称:事件处理器接口											       */
/* 说明:事件处理器接口，利用多态处理不同类型事件					   */
/* 创建时间:2021/10/10												   */
/* Email:umichan0621@gmail.com  									   */
/***********************************************************************/
#pragma once
#include <stdint.h>
#include <unordered_map>
#include <functional>
#include <base/config.hpp>

enum HandleMethod
{
	DO_NOTHING=0,
	DO_CONNECT,
	DO_DISCONNECT,
	DO_REPLY,
	DO_REPLY_NAT
};

namespace handler
{
#ifndef __linux
	//启用文件模块
#define MOUDLE_FILE_ON
#endif
	typedef std::function<int8_t(uint16_t&, char*, uint16_t&)>		EventHandler;
	typedef std::unordered_map<uint16_t, EventHandler>				HandlerMap;

	class HandlerInterface
	{
	public:
		virtual void register_recv_event() = 0;
		virtual void register_gateway_event() = 0;
		virtual int8_t handle_event(uint16_t& SessionId, char* pMessage, uint16_t& Len) = 0;
	};
}
