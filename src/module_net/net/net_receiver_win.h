/***********************************************************************/
/* 名称:IOCP UDP接口											       */
/* 说明:封装Windows环境下UDP接口，底层使用IOCP						   */
/* 创建时间:2021/02/19												   */
/* Email:umichan0621@gmail.com									       */
/* Reference:https://github.com/TTGuoying/IOCPServer				   */
/***********************************************************************/
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>
#include <stdint.h>
#include <windows.h>
#include <base/thread_pool.h>
#include <base/object_pool.hpp>
#include <module_peer/peer_manager.h>
#pragma comment(lib, "ws2_32.lib")

namespace net
{
	enum class IO_OPERATION_TYPE
	{
		NULL_POSTED,		// 用于初始化，无意义
		SEND_POSTED,		// 投递Send操作
		RECV_POSTED,		// 投递Recv操作
	};

	class IoData
	{
	public:
		IoData();
		~IoData();
	public:
		OVERLAPPED				OverLapped;		//每个socket的每一个IO操作都需要一个重叠结构
		WSABUF					WsaBuf;			//数据缓冲
		IO_OPERATION_TYPE		IoType;			//IO操作类型
		PeerAddress				PeerAddr;		//接受到信息时会填充进去
	};

	class NetReceiverWin
	{
		typedef base::ThreadPool			ThreadPool;
		typedef base::ObjectPool<IoData>	IoDataPool;
	public:
		NetReceiverWin();
		virtual ~NetReceiverWin();
	public:
		//初始化，设定最大连接数，监听端口
		bool init(uint16_t MaxConnectionNum, uint16_t Port, uint16_t Port6 = 0);
		//监听端口，同时循环接受消息，初始化资源
		void listen(ThreadPool* pThreadPool);
	private:
		virtual void on_accept(uint16_t SessionId, const PeerAddress& PeerAddr) = 0;
		virtual bool on_gateway(const PeerAddress& PeerAddr, char* pMessage, uint16_t Len) = 0;
		virtual void on_recv(uint16_t SessionId, char* pMessage, uint16_t Len) = 0;
		virtual void on_disconnect(uint16_t SessionId) = 0;
	private:
		bool _init_socket(uint16_t Port);
		bool _init_socket6(uint16_t Port6);
		bool _init_socket_nat(uint16_t PortNAT);
		bool _init_completionport(bool bIPv6 = false);
		void _post_recv(IoData* pIoData);
		void _post_send();
		void _gateway(const PeerAddress PeerAddr, char* pMessage, uint16_t Len);
		void _recv(uint16_t SessionId, char* pMessage, uint16_t Len);
		void _worker_thread();
		void _stop();
	protected://Socket相关
		SOCKET				m_ListenFd;					//监听套接字描述符 IPv4版本
		SOCKET				m_ListenFd6;				//监听套接字描述符 IPv6版本	
		SOCKET				m_ListenFdNAT;				//监听套接字描述符
	private:
		int32_t				m_AddrLen6;					//IPV6地址长度
	private://IOCP相关
		uint16_t			m_ThreadNum;				//工作线程数
		HANDLE				m_Iocp;						//IOCP句柄
		HANDLE				m_StopEv;					//通知线程停止的句柄
	private://资源分配相关
		ThreadPool*			m_pThreadPool;				//工作线程池
		IoDataPool			m_IoDataPool;				//IO对象池
	};
}