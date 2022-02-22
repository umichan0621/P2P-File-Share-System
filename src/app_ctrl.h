/***********************************************************************/
/* 名称:APP Ctrl													   */
/* 说明:连接GUI界面和其他所有模块							           */
/* 创建时间:2021/12/19											       */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <base/timer.h>
#include <base/thread_pool.h>
#include <module_db/database.h>
#include <module_gui/ui_main_widget.h>
#include <module_net/net_receiver.h>
#include <module_handler/handler_manager.h>
#include <module_file/file_manager.h>


class AppCtrl:public QObject
{
	Q_OBJECT
private:
	typedef gui::MainWidget					MoudleGui;
	typedef net::NetReceiver				MoudleNet;
	typedef handler::HandlerManager			MoudleHandler;
public:
	AppCtrl();
	~AppCtrl();
public:
	bool init();
	void start();
private://初始化
	void init_slots();	//连接各个按钮和槽函数
	void init_file();	//读取数据库，加载所有记录的文件
	void init_config();	//读取配置表并设置

	//根据文件链接和保存路径，尝试添加一个下载文件
	bool try_add_download_file(std::string& strLink, std::string& strPath);

	//处理接收到的信号，打开文件序号为FileSeq的文件所在的文件夹
	void open_folder(int32_t FileSeq);

	//处理接收到的信号，复制文件序号为FileSeq的文件的链接
	void get_link(int32_t FileSeq);

	//使用一个工作线程尝试新增一个分享文件
	void thread_add_share_file(std::string& strRemark, std::string& strPath);

	//循环运行的线程，调用文件模块获取下载任务
	//然后发送给相关联的Peer请求下载
	void thread_loop_download();

	//循环运行的线程，调用路由模块请求搜索CID和PID
	//获取没有询问过的Peer，然后尝试搜索获取更多Peer
	void thread_loop_search();

	//计算文件的SHA1值，然后与实际的SHA1值比较
	//结果一致表示下载完成，否则可能有问题
	void thread_sha1_check(file::FileCtrl FileCtrl);
private://各个模块
	MoudleGui*					m_pMoudleGui;
	MoudleNet*					m_pMoudleNet;
	MoudleHandler*				m_pMoudleHandler;
private:
	base::ThreadPool*			m_pThreadPool;
};