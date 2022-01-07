/***********************************************************************/
/* 名称:Epoll UDP接口												   */
/* 说明:封装Linux环境下UDP接口，底层使用Epoll						   */
/* 创建时间:2021/02/03												   */
/* Email:umichan0621@gmail.com  									   */
/***********************************************************************/
#pragma once
#include <sys/socket.h>
#include <base/thread_pool.h>
#include <base/buffer_pool.hpp>
#include <base/object_pool.hpp>
#include <base/config.hpp>

#define SOCKET int32_t

namespace net
{
	class NetReceiverLinux
	{
		typedef base::ThreadPool				ThreadPool;
		typedef base::BufferPool				BufferPool;
		typedef base::ObjectPool<sockaddr>		SockaddrPool;
	public:
		NetReceiverLinux();
		~NetReceiverLinux();
	public:
		//初始化，设定最大连接数，监听端口
		bool init(uint16_t MaxConnectionNum, uint16_t Port, uint16_t Port6 = 0);
		//监听端口，同时循环接受消息，初始化资源
		void listen(ThreadPool* pThreadPool);
	private:
		virtual void on_accept(uint16_t SessionId, sockaddr* pSockaddr) = 0;
		virtual bool on_gateway(sockaddr* pSockaddr, char* pMessage, uint16_t Len) = 0;
		virtual void on_disconnect(uint16_t SessionId) = 0;
		virtual void on_recv(uint16_t SessionId, char* pMessage, uint16_t Len) = 0;
	private:
		bool _init_socket(uint16_t Port);
		bool _init_socket6(uint16_t Port6);
		bool _init_socket_nat(uint16_t PortNAT);
		bool _init_epoll(bool bIPv6 = false);
		void _reactor();
		void _gateway(sockaddr* pSockaddr, char* pMessage, uint16_t Len);
		void _recv(uint16_t SessionId, char* pMessage, uint16_t Len);
	protected:
		//Socket相关
		SOCKET				m_ListenFd;					//监听套接字描述符 IPv4版本
		SOCKET				m_ListenFd6;				//监听套接字描述符 IPv6版本	
		SOCKET				m_ListenFdNAT;				//监听套接字描述符
	private:
		socklen_t			m_AddrLen6;					//IPV6地址长度
		//Epoll相关
		int32_t				m_EpollFd;
		//资源分配相关
		ThreadPool*			m_pThreadPool;				//工作线程池
		BufferPool			m_BufPool;
		SockaddrPool		m_SockaddrPool;				//sockaddr对象池
	};
}