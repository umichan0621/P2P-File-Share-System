#include "tracker_ctrl.h"
#include <base/timer.h>
#include <module_handler/handler_base.h>
#include <module_handler/handler_routing.h>
#include <module_net/session_manager.h>
#include <base/logger/logger.h>
#include <module_peer/routing_table.h>
#include <module_peer/peer_manager.h>

TrackerCtrl::TrackerCtrl():
	m_pMoudleNet(new MoudleNet()),
	m_pMoudleHandler(new MoudleHandler()),
	m_pThreadPool(new base::ThreadPool()){}

TrackerCtrl::~TrackerCtrl(){}

bool TrackerCtrl::init(uint16_t Port, uint16_t Port6)
{
	init_config();
	uint16_t ThreadNum = 4;
	if (false == m_pThreadPool->start(ThreadNum))
	{
		return false;
	}
	//初始化定时器，刷新间隔50ms，最大间隔10分钟
	if (false == g_pTimer->init(50, 10))
	{
		return false;
	}
	//设置Session连接数和路由表Peer记录数
	g_pPeerManager->init(g_pConfig->max_connection_num(), 1024);
	//初始化自己的PID
	g_pRoutingTable->init();
	//初始化被动接收的Net模块
	if (false == m_pMoudleNet->init(
		g_pConfig->max_connection_num(), Port, Port6))
	{
		return false;
	}
	//初始化Buffer池
	g_pBufferPoolMgr->init(32, MIN_MTU);
	//注册基本事件
	handler::HandlerBase* pHandlerBase = new handler::HandlerBase();
	m_pMoudleHandler->register_event(PROTOCOL_TYPE_BASE, pHandlerBase);
	//注册路由事件
	handler::HandlerRouting* pHandlerRouting = new handler::HandlerRouting();
	m_pMoudleHandler->register_event(PROTOCOL_TYPE_ROUTING, pHandlerRouting);
	//注册Net模块的Handler方式
	m_pMoudleNet->set_handler(m_pMoudleHandler);
	SOCKET ListenFd, ListenFd6, ListenFdNAT;
	//获取Socket描述符
	m_pMoudleNet->listen_fd(ListenFd, ListenFd6, ListenFdNAT);
	//初始化主动发送的Peer模块
	g_pSessionManager->init( ListenFd, ListenFd6, ListenFdNAT);

	//Net模块开始监听端口
	m_pMoudleNet->listen(m_pThreadPool);
	//启动定时器线程
	m_pThreadPool->add_task(std::bind(&base::Timer::thread_loop, g_pTimer));
}

void TrackerCtrl::init_config()
{
	g_pConfig->set_max_connection_num(1024);
	g_pConfig->set_port(TEST_PORT, TEST_PORT6);
}

void TrackerCtrl::output(const std::string& strCommand)
{
	if (strCommand == "/count")
	{
		LOG_TRACE << "Current connection num:"<<g_pPeerManager->connection_num();
	}
	else
	{

	}
}
