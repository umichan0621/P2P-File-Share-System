/***********************************************************************/
/* 名称:Tracker Ctrl												   */
/* 说明:Linux服务器运行的Tracker节点程序							   */
/* 创建时间:2021/12/19											       */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#include <base/thread_pool.h>
#include <module_net/net_receiver.h>
#include <module_handler/handler_manager.h>
#include <module_peer/peer_manager.h>

class TrackerCtrl
{
	typedef net::NetReceiver				MoudleNet;
	typedef handler::HandlerManager			MoudleHandler;
public:
	TrackerCtrl();
	~TrackerCtrl();
public:
	bool init();
	void init_config();
private://各个模块
	MoudleNet*			m_pMoudleNet;
	//MoudlePeer*			m_pMoudlePeer;
	MoudleHandler*		m_pMoudleHandler;
	base::ThreadPool*	m_pThreadPool;
};