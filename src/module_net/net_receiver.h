/***********************************************************************/
/* 名称:UDP Net基础组件											       */
/* 说明:封装IOCP和EPOLL，实现Windows和Linux通用						   */
/* 创建时间:2021/06/06												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <atomic>
#ifndef __linux
#include <module_net/net/net_receiver_win.h>
#else
#include <module_net/net/net_receiver_linux.h>
#endif
#include <module_handler/handler_manager.h>

namespace net
{
#ifndef __linux
	typedef NetReceiverWin NetReceiverInterface;
#else
	typedef NetReceiverLinux NetReceiverInterface;
#endif
	class NetReceiver :public NetReceiverInterface
	{
	public:
		NetReceiver();
		virtual ~NetReceiver();
	public:
		void listen_fd(SOCKET& ListenFd, SOCKET& ListenFd6, SOCKET& ListenFdNAT);
		void set_handler(handler::HandlerManager* pHandler);
	private:
		//接收来自其他节点的连接请求
		void on_accept(uint16_t SessionId, sockaddr* pSockaddr) override;
		//用于处理所有
		//内部判断是否接受连接，如果是询问请求或者非法连接则返回false
		bool on_gateway(sockaddr* pSockaddr, char* pMessage, uint16_t Len) override;
		void on_recv(uint16_t SessionId, char* pMessage, uint16_t Len) override;
		void on_disconnect(uint16_t SessionId) override;
	private:
		handler::HandlerManager*			m_pNetHandler;
	};
}