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
	typedef database::DataBaseManager		MoudleDataBase;
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
signals:
	//新的下载任务
	void new_download(int32_t FileSeq, uint8_t Status, uint64_t FileSize, const QString& FileName);
	//新的分享文件
	void new_share(int32_t FileSeq, const QString& FileName,
		const QString& Remark, const QString& CreateTime, uint64_t UploadData);
	//更新下载进度
	void update_progress(int32_t FileSeq, uint64_t CurFileSize, uint64_t CurSpeed);
	void file_complete(int32_t FileSeq);
private:
	bool try_add_download_file(std::string& strLink, std::string& strPath);
	void thread_add_share_file(std::string& strRemark, std::string& strPath);
private:
	void open_folder(int32_t FileSeq);
	void get_link(int32_t FileSeq);
private://windows操作系统API封装
	bool set_clip_board(const std::string& strVal);
	bool set_clip_board(const char* pBuf,int32_t Len);
private:
	void download_thread();
	void sha1_check_thread(file::FileCtrl FileCtrl);
	//添加一个下载任务
private://各个模块
	MoudleGui*					m_pMoudleGui;
	MoudleNet*					m_pMoudleNet;
	MoudleDataBase*				m_pMoudleDataBase;
	MoudleHandler*				m_pMoudleHandler;
private:
	base::ThreadPool*			m_pThreadPool;

private:
	uint32_t					m_UnusedFileSeq;
};