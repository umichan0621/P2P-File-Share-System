#include "app_ctrl.h"
#include <QMetaType>
#include <QProcess>
#include <base/timer.h>
#include <base/encoder.h>
#include <base/config.hpp>
#include <base/logger/logger.h>
#include <base/protocol/protocol_file.h>
#include <module_peer/peer_manager.h>
#include <module_peer/routing_table.h>
#include <module_peer/partner_table.h>
#include <module_net/session_manager.h>
#include <module_handler/handler_base.h>
#include <module_handler/handler_file.h>
#include <module_handler/handler_routing.h>
#include <module_file/file_ctrl/file_ctrl_complete.h>
#include <module_file/file_ctrl/file_ctrl_incomplete.h>
#include <iostream>
#define CONNECT_SIGNAL(_OBJ,_SIGNAL,_FUNC)  connect(_OBJ, &_SIGNAL, this, _FUNC)

#ifdef _DEBUG
static const char* DATA_BASE_PATH = "../test_debug.db";
#else
static const char* DATA_BASE_PATH = "../test_release.db";
#endif

//windows操作系统API封装
//设置当前粘贴板的内容
static bool set_clip_board(const char* pBuf, int32_t Len)
{
	HWND pHwnd = NULL;
	OpenClipboard(pHwnd);//打开剪切板
	EmptyClipboard();//清空剪切板
	HANDLE pHandle = GlobalAlloc(GMEM_FIXED, Len + 1);//分配内存
	if (0 == pHandle)
	{
		return false;
	}
	char* pData = (char*)GlobalLock(pHandle);//锁定内存，返回申请内存的首地址
	if (nullptr == pData)
	{
		return false;
	}
	memcpy(pData, pBuf, Len);
	//pData[Len] = '\0';
	SetClipboardData(CF_TEXT, pHandle);//设置剪切板数据
	GlobalUnlock(pHandle);//解除锁定
	CloseClipboard();//关闭剪切板
	return true;
}

static bool set_clip_board(const std::string& strVal)
{
	return set_clip_board(strVal.c_str(), strVal.size());
}
//windows操作系统API封装

AppCtrl::AppCtrl() :
	m_pMoudleGui(new MoudleGui()),
	m_pMoudleNet(new MoudleNet()),
	m_pMoudleDataBase(new MoudleDataBase()),
	m_pMoudleHandler(new MoudleHandler()),
	m_pThreadPool(new base::ThreadPool()),
	m_UnusedFileSeq(0){}

AppCtrl::~AppCtrl(){}

bool AppCtrl::init()
{
	//根据时间生成随机种子
	srand(base::now_milli());
	//初始化数据库
	if (false == m_pMoudleDataBase->open(DATA_BASE_PATH))
	{
		return false;
	}
	//载入配置表
	init_config();
	//初始化线程池
	SYSTEM_INFO SysInfo;
	GetSystemInfo(&SysInfo);//CPU核心数
	uint16_t ThreadNum = 4 * (uint16_t)SysInfo.dwNumberOfProcessors;
	if (false == m_pThreadPool->start(ThreadNum))
	{
		return false;
	}
	//初始化定时器，刷新间隔50ms，最大间隔10分钟
	if (false == g_pTimer->init(50, 10))
	{
		return false;
	}
	//初始化被动接收的Net模块
	uint16_t Port, Port6;
	g_pConfig->port(Port, Port6);
	if (false == m_pMoudleNet->init(
		g_pConfig->max_connection_num(), Port, Port6))
	{
		return false;
	}
	//设置Session连接数和路由表Peer记录数
	g_pPeerManager->init(g_pConfig->max_connection_num(), 1024);
	//初始化自己的PID
	g_pRoutingTable->init();
	//注册基本事件
	handler::HandlerBase* pHandlerBase = new handler::HandlerBase();
	m_pMoudleHandler->register_event(PROTOCOL_TYPE_BASE, pHandlerBase);
	//注册文件事件
	handler::HandlerFile* pHandlerFile = new handler::HandlerFile();
	m_pMoudleHandler->register_event(PROTOCOL_TYPE_FILE, pHandlerFile);
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
	//获取还没使用过的最小FileSeq
	m_UnusedFileSeq = m_pMoudleDataBase->select_unused_file_seq();
	//初始化信号和槽
	init_slots();
	//设置同时最大下载数
	g_pFileManager->set_max_download(g_pConfig->max_download());
	//Net模块开始监听端口
	m_pMoudleNet->listen(m_pThreadPool);
	//初始化Gui
	m_pMoudleGui->init();
	//加载数据库中记录的文件
	init_file();
	//启动定时器线程
	m_pThreadPool->add_task(std::bind(&base::Timer::thread_loop, g_pTimer));
	return true;
}


void AppCtrl::start()
{
	int x;
#ifndef _DEBUG//服务器
	//const char* p = "192.168.50.13";
	const char* p = "127.0.0.1";
	//const char* p = "121.5.179.213";
	uint16_t Session = g_pSessionManager->connect_tracker(p, 42543);
	std::cin >> x;
	m_pThreadPool->add_task(std::bind(&AppCtrl::thread_loop_search, this));
	m_pMoudleGui->show();

#else//客户端
	//const char* p = "192.168.50.13";

	//const char* p = "127.0.0.1";
	const char* p = "121.5.179.213";
	uint16_t Session = g_pSessionManager->connect_tracker(p, 2342);
	//启动循环下载线程
	//m_pThreadPool->add_task(std::bind(&AppCtrl::download_thread, this));
	std::cin >> x;
	m_pThreadPool->add_task(std::bind(&AppCtrl::thread_loop_search, this));
	m_pMoudleGui->show();

#endif
}

//初始化
void AppCtrl::init_slots()
{
	qRegisterMetaType<int32_t>("int32_t");
	qRegisterMetaType<uint64_t>("uint64_t");
	//开始下载文件的信号
	CONNECT_SIGNAL(m_pMoudleGui->download_list(), gui::DownloadList::start_file,
		[&](int32_t FileSeq)
		{
			int8_t Status = STATUS_DOWNLOAD;
			m_pMoudleDataBase->update_file_info(FileSeq, Status);
		});
	//暂停下载文件的信号
	CONNECT_SIGNAL(m_pMoudleGui->download_list(), gui::DownloadList::pause_file,
		[&](int32_t FileSeq)
		{
			int8_t Status = STATUS_PAUSE;
			m_pMoudleDataBase->update_file_info(FileSeq, Status);

		});
	//删除下载文件的信号
	CONNECT_SIGNAL(m_pMoudleGui->download_list(), gui::DownloadList::delete_file,
		[&](int32_t FileSeq)
		{
			m_pMoudleDataBase->delete_file_info(FileSeq);
			g_pFileManager->kill_download_file(FileSeq);
		});
	//删除分享文件的信号
	CONNECT_SIGNAL(m_pMoudleGui->share_tree(), gui::ShareTree::delete_file,
		[&](int32_t FileSeq)
		{
			m_pMoudleDataBase->delete_file_info(FileSeq);
			g_pFileManager->kill_share_file(FileSeq);
		});
	//获取下载文件分享链接的信号
	CONNECT_SIGNAL(m_pMoudleGui->download_list(), gui::DownloadList::create_link,
		[&](int32_t FileSeq)//下载页面
		{
			get_link(FileSeq);
		});
	CONNECT_SIGNAL(m_pMoudleGui->share_tree(), gui::ShareTree::create_link,
		[&](int32_t FileSeq)//分享页面
		{
			get_link(FileSeq);
		});
	//打开下载文件路径的信号
	CONNECT_SIGNAL(m_pMoudleGui->download_list(), gui::DownloadList::open_folder,
		[&](int32_t FileSeq)
		{
			open_folder(FileSeq);
		});
	CONNECT_SIGNAL(m_pMoudleGui->share_tree(), gui::ShareTree::open_folder,
		[&](int32_t FileSeq)
		{
			open_folder(FileSeq);
		});
	//添加下载任务的信号
	CONNECT_SIGNAL(m_pMoudleGui->add_dialog(), gui::AddDialog::add_download_file,
		[&](const QString& strLink, const QString& strPath)
		{
			std::string Link = strLink.toStdString();
			std::string Path = strPath.toStdString();
			bool bRes = try_add_download_file(Link, Path);
			if (true == bRes)
			{

			}
			else
			{

			}
		});
	//添加分享文件的信号
	CONNECT_SIGNAL(m_pMoudleGui->add_dialog(), gui::AddDialog::add_share_file,
		[&](const QString& strRemark, const QString& strPath)
		{
			//将任务交给其他工作线程处理，避免出现卡死的现象（当前任务耗时较长）
			m_pThreadPool->add_task(std::bind(
				&AppCtrl::thread_add_share_file,
				this,
				strRemark.toStdString(),
				strPath.toStdString()));
		});

	connect(this, &AppCtrl::new_share, m_pMoudleGui->share_tree(), &gui::ShareTree::new_share);
	//发送新建下载任务的信号
	connect(this, &AppCtrl::new_download, m_pMoudleGui->download_list(), &gui::DownloadList::new_download);
	//发送更新下载进度的信号
	connect(this, &AppCtrl::update_progress, m_pMoudleGui->download_list(), &gui::DownloadList::update_progress);
	//发送文件下载完成的信号
	connect(this, &AppCtrl::file_complete, m_pMoudleGui->download_list(), &gui::DownloadList::file_complete);

}

void AppCtrl::init_file()
{
	uint64_t FileSize;
	base::SHA1 SHA1Struct;
	std::string strSHA1;
	std::string strFilePath, strFileName;
	std::vector<int32_t> VecFileSeq;

	//加载下载中文件
	{
		VecFileSeq.clear();
		m_pMoudleDataBase->select_file_info(VecFileSeq, STATUS_DOWNLOAD);
		for (auto& FileSeq : VecFileSeq)
		{
			m_pMoudleDataBase->select_file_info(FileSeq, strSHA1);
			base::sha1_parse(strSHA1, SHA1Struct);
			m_pMoudleDataBase->select_file_info(FileSeq, strFilePath, strFileName, FileSize);
			//载入文件
			file::FileCtrlIncmpl* pFileCtrl = new file::FileCtrlIncmpl();
			bool bRes = pFileCtrl->init(base::utf8_to_string(strFilePath), FileSize);
			pFileCtrl->set_file_seq(FileSeq);
			pFileCtrl->set_sha1(SHA1Struct);
			if (false == bRes)
			{
				delete pFileCtrl;
				//删除.ctrl文件
				//提示客户端当前的错误，让客户端决定是否删除文件
				//

			}
			else
			{
				g_pFileManager->add_download_file(pFileCtrl);
				g_pFileManager->refresh_download_list();
				//加入PartnerTable
				g_pPartnerTable->add_cid(SHA1Struct);
				//GUI显示
				emit(new_download(FileSeq, STATUS_DOWNLOAD, FileSize, QString::fromStdString(strFileName)));
			}
		}
	}
	//加载暂停下载的文件
	{
		VecFileSeq.clear();
		m_pMoudleDataBase->select_file_info(VecFileSeq, STATUS_PAUSE);
		for (auto& FileSeq : VecFileSeq)
		{
			m_pMoudleDataBase->select_file_info(FileSeq, strSHA1);
			base::sha1_parse(strSHA1, SHA1Struct);
			//GUI显示
			m_pMoudleDataBase->select_file_info(FileSeq, strFilePath, strFileName, FileSize);
			emit(new_download(FileSeq, STATUS_PAUSE, FileSize, QString::fromStdString(strFileName)));
		}
	}
	//加载下载完成的文件
	{
		VecFileSeq.clear();
		m_pMoudleDataBase->select_file_info(VecFileSeq, STATUS_COMPLETE);
		for (auto& FileSeq : VecFileSeq)
		{
			m_pMoudleDataBase->select_file_info(FileSeq, strSHA1);
			base::sha1_parse(strSHA1, SHA1Struct);

			//GUI显示
			m_pMoudleDataBase->select_file_info(FileSeq, strFilePath, strFileName, FileSize);
			emit(new_download(FileSeq, STATUS_COMPLETE, FileSize, QString::fromStdString(strFileName)));
		}
	}
	//加载分享的文件
	{
		VecFileSeq.clear();
		m_pMoudleDataBase->select_file_info(VecFileSeq, STATUS_SHARE);
		for (auto& FileSeq : VecFileSeq)
		{
			m_pMoudleDataBase->select_file_info(FileSeq, strSHA1);
			base::sha1_parse(strSHA1, SHA1Struct);
			m_pMoudleDataBase->select_file_info(FileSeq, strFilePath, strFileName, FileSize);
			//载入文件
			file::FileCtrlCmpl* pFileCtrl = new file::FileCtrlCmpl();
			bool bRes = pFileCtrl->init(base::utf8_to_string(strFilePath), FileSize);
			pFileCtrl->set_file_seq(FileSeq);
			pFileCtrl->set_sha1(SHA1Struct);
			if (false == bRes)
			{
				delete pFileCtrl;
				//删除.ctrl文件
				//提示客户端当前的错误，让客户端决定是否删除文件
				//
			}
			else
			{
				//加入内存中的分享列表
				g_pFileManager->add_share_file(pFileCtrl);
				//加入PartnerTable
				g_pPartnerTable->add_cid(SHA1Struct);
				//GUI显示
				std::string CreateTime, FileName, FileRemark;
				uint64_t UploadData;
				bool bRes = m_pMoudleDataBase->select_create_time(FileSeq, CreateTime);
				bRes = m_pMoudleDataBase->select_file_info(FileSeq, FileName, UploadData, FileRemark);
				emit(new_share(FileSeq, QString::fromStdString(FileName),
					QString::fromStdString(FileRemark), QString::fromStdString(CreateTime), UploadData));
			}
		}
	}
}

void AppCtrl::init_config()
{
	g_pConfig->set_port(TEST_PORT, TEST_PORT6);
	g_pConfig->set_max_connection_num(1024);
	g_pConfig->set_max_download(8);
	g_pConfig->set_path_download("E:/0");
	g_pConfig->set_path_share("D:/OneDrive - anbaia/Computer Science/Computer Network/lambda");
}
//初始化

void AppCtrl::get_link(int32_t FileSeq)
{
	base::SHA1 SHA1Struct;
	std::string strSHA1;
	uint64_t FileSize;
	m_pMoudleDataBase->select_file_info(FileSeq, strSHA1);
	m_pMoudleDataBase->select_file_size(FileSeq, FileSize);
	base::sha1_parse(strSHA1, SHA1Struct);
	char Buf[30];
	memcpy(Buf, &SHA1Struct, 20);
	memcpy(&Buf[20], &FileSize, 8);
	char Des[41];
	memset(Des, 0, 40);
	base::base64_encode(Buf, 30, Des);
	if (false == set_clip_board(Des, 40))
	{
		LOG_ERROR << "Fail to copy to the clipboard";
	}
	Des[40] = '\0';
	std::string strLink = Des;
	LOG_ERROR << strLink;
	std::string FileName;
	m_pMoudleDataBase->select_file_name(FileSeq, FileName);
	strLink += base::utf8_to_string(FileName);
	LOG_ERROR << strLink;
	if (false == set_clip_board(strLink))
	{
		LOG_ERROR << "Fail to copy to the clipboard";
	}
}

void AppCtrl::open_folder(int32_t FileSeq)
{
	LOG_ERROR << "open_folder";
	std::string strPath;
	m_pMoudleDataBase->select_file_path(FileSeq, strPath);
	QString qstrPath = QString::fromStdString(strPath);

	strPath = base::utf8_to_string(strPath);
	LOG_ERROR << strPath;
	if (false == file::folder_exist(strPath))
	{
		LOG_ERROR << "Folder path not exist:" << strPath;
	}
	else
	{
		QProcess Processor(this);
		Processor.start("explorer /select," + qstrPath.replace("/", "\\"));
		Processor.waitForFinished();
	}

}

void AppCtrl::thread_loop_download()
{
	file::FileCtrl FileCtrl;
	uint64_t LastRefreshTime = base::now_milli()-500;
	char DLReqBuf[30] = { 0 };
	create_header(DLReqBuf, PROTOCOL_FILE_FRAGMENT_REQ);
	for (;;)
	{
		if (base::now_milli() - LastRefreshTime >= 500)
		{
			LastRefreshTime = base::now_milli();
			//更新进度
			std::vector<file::DownloadStatus> VecStatus;
			g_pFileManager->download_status(VecStatus);
			for (auto& Status : VecStatus)
			{
				emit(update_progress(Status.FileSeq, Status.FileSize, Status.DownloadRate));
			}
		}
		bool bRes = g_pFileManager->download_task(FileCtrl);
		//下载列表为空
		if (false == bRes)
		{
			continue;
			//LOG_ERROR << "empty";
		}
		else
		{
			//当前的下载任务
			file::FileCtrlIncmpl* pDownloadFile = (file::FileCtrlIncmpl*)FileCtrl.get();
			uint64_t FragmentStart;
			bRes = pDownloadFile->get_task(FragmentStart);
			//返回false表示当前文件已无需要下载的Fragment，可能已经全部下载完成
			if (false== bRes)
			{
				//检查是否真的全部下载完成
				bRes = pDownloadFile->check_task(FragmentStart);
				//下载完成，校验SHA1
				if (false == bRes)
				{
					int32_t FileSeq = pDownloadFile->file_seq();
					//从下载队列移除
					bool bRes = g_pFileManager->check_download_file(FileSeq);
					if (true == bRes)
					{
						//校验任务耗时长，交给其他线程处理
						m_pThreadPool->add_task(
							std::bind(&AppCtrl::sha1_check_thread, this, FileCtrl));
						continue;
					}
				}
			}
			//获取下载任务并向Peer请求
			{
				//获取当前下载文件的必要信息
				base::SHA1 SHA1Struct;
				pDownloadFile->full_sha1(SHA1Struct);
				std::string strSHA1;
				base::sha1_value(SHA1Struct, strSHA1);
				//LOG_ERROR << strSHA1;
				//写入文件SHA1值和FragmentStart
				memcpy(&DLReqBuf[2], &SHA1Struct, 20);
				memcpy(&DLReqBuf[22], &FragmentStart, 8);
				net::Session* pCurSession=g_pSessionManager->session(1);
				pCurSession->send_reliable(DLReqBuf, 30);
			}
		}

		base::delay_micro(10 * 1000);
		FileCtrl.reset();
	}
}

void AppCtrl::thread_loop_search()
{
	//构造路由搜索协议头，加入固定的PID
	//协议格式:
	//[固定头部(2B)] +[PID(20B)]+(0-3个)[CID(20B)]
	char SendBuf[100] = { 0 };
	create_header(SendBuf, PROTOCOL_ROUTING_SEARCH_REQ);
	memcpy(&SendBuf[2], g_pRoutingTable->pid(), KLEN_KEY);

	std::string str;
	base::sha1_value(g_pRoutingTable->pid(), str);
	for (;;)
	{
		uint16_t SessionId = g_pPeerManager->search_pop();

		if (ERROR_SESSION_ID != SessionId)
		{
			net::Session* pCurSession = g_pSessionManager->session(SessionId);
			if (nullptr == pCurSession)
			{
				continue;
			}
			int32_t Pos = 2 + KLEN_KEY;
			//获取3个CID，如果重复说明总的CID不足3个
			std::unordered_set<base::SHA1, base::SHA1HashFunc,base::SHA1EqualFunc> CIDSet;
			base::SHA1 CID = { 0 };
			for (int32_t i = 0; i < 3; ++i)
			{
				if (false == g_pPartnerTable->get_cid(CID))
				{
					break;
				}
				CIDSet.insert(CID);
				//base::sha1_value(CID, str);
				//LOG_ERROR << str;
			}
			for (auto& CurCID : CIDSet)
			{
				memcpy(&SendBuf[Pos], &CurCID, KLEN_KEY);
				Pos += KLEN_KEY;
			}

			pCurSession->send_reliable(SendBuf, Pos);
		}
		base::delay_micro(1 * 1000 * 1000);
	}
}

void AppCtrl::sha1_check_thread(file::FileCtrl FileCtrl)
{
	base::SHA1 ActrualSHA1Struct,SHA1Struct;
	FileCtrl->sha1(SHA1Struct);
	FileCtrl->full_sha1(ActrualSHA1Struct);
	//校验SHA1无问题
	int32_t FileSeq = FileCtrl->file_seq();
	if (true == sha1_equal(ActrualSHA1Struct, SHA1Struct))
	{
		LOG_ERROR << "SHA1 CHECK OK";
		emit(file_complete(FileSeq));
	}
	else
	{
		LOG_ERROR << "FILE =  ERROR";
	}

}

bool AppCtrl::try_add_download_file(std::string& strLink, std::string& strPath)
{
	//解析Link，Link分成多种
	//基础Link=MD5+FileSize(Len=32)
	if (strLink.size() < 32)
	{
		return false;
	}
	//解析MD5和FileSize
	base::SHA1 SHA1Struct;
	uint64_t FileSize;
	char Buf[30];
	base::base64_decode(strLink.c_str(), 40, Buf);
	memcpy(&SHA1Struct, &Buf[0], 20);
	memcpy(&FileSize, &Buf[20], 8);
	std::string strSHA1;
	base::sha1_value(SHA1Struct, strSHA1);

	LOG_ERROR << strSHA1;
	LOG_ERROR << FileSize;
	int32_t FileSeq = m_pMoudleDataBase->select_file_seq(strSHA1);

	//当前MD5的文件已在数据库中
	if (-1 != FileSeq)
	{
		LOG_TRACE << "[Download Task]" << strSHA1 << " has been recorded, FileSeq = " << FileSeq;
		return false;
	}
	FileSeq = m_UnusedFileSeq;

	++m_UnusedFileSeq;
	std::string FileName;
	if (strLink.size() > 40)
	{
		FileName = strLink.substr(40, strLink.size() - 40);
	}
	else
	{
		FileName= strSHA1;
	}
	//创建本地文件
	file::FileCtrlIncmpl* pFileCtrl = new file::FileCtrlIncmpl();
	pFileCtrl->set_file_seq(FileSeq);
	LOG_ERROR << strPath;
	std::string strFilePath = strPath;
	if (strFilePath.back() != '/')
	{
		strFilePath.push_back('/');
	}
	strFilePath += FileName;
	bool bRes = pFileCtrl->init(base::utf8_to_string(strFilePath), FileSize);
	if (false == bRes)
	{
		delete pFileCtrl;
		//删除.ctrl文件
		//提示客户端当前的错误，让客户端决定是否删除文件
		//
		return false;
	}
	else
	{
		pFileCtrl->set_sha1(SHA1Struct);
		//数据库持久化
		m_pMoudleDataBase->insert_file_info(FileSeq, strSHA1, STATUS_DOWNLOAD, strFilePath, FileSize);
		m_pMoudleDataBase->update_file_info(FileSeq, FileName,"");
		g_pFileManager->add_download_file(pFileCtrl);
		g_pFileManager->refresh_download_list();
		//GUI显示
		emit(new_download(FileSeq, STATUS_DOWNLOAD, FileSize, QString::fromStdString(FileName)));
	}
	return true;
}

void AppCtrl::thread_add_share_file(std::string& strRemark, std::string& strPath)
{
	std::string strPathLocal = base::utf8_to_string(strPath);
	//打开文件，然后将信息写入数据库
	file::FileCtrlCmpl* pFileCtrl = new file::FileCtrlCmpl();
	if (false == pFileCtrl->open(strPathLocal))
	{
		return;
	}
	//获取当前文件的大小和SHA1值
	uint64_t FileSize = pFileCtrl->size();
	base::SHA1 SHA1Struct;
	std::string strSHA1;
	pFileCtrl->sha1(SHA1Struct);
	base::sha1_value(SHA1Struct,strSHA1);
	LOG_ERROR << strSHA1;
	int32_t FileSeq = m_pMoudleDataBase->select_file_seq(strSHA1);
	//当前MD5的文件已在数据库中
	if (-1 != FileSeq)
	{
		LOG_TRACE << "The file:" + strPathLocal + " has been recorded, FileSeq = " << FileSeq;
		return;
	}
	//获取可用的FileSeq
	FileSeq = m_UnusedFileSeq;
	++m_UnusedFileSeq;
	bool bRes = m_pMoudleDataBase->insert_file_info(FileSeq, strSHA1, STATUS_SHARE, strPath, FileSize);
	if (false == bRes)
	{
		//LOG_TRACE <<
		return;
	}

	std::string FileName = base::string_to_utf8(pFileCtrl->file_name());
	bRes = m_pMoudleDataBase->update_file_info(FileSeq, FileName, strRemark);
	std::string CreateTime;
	bRes = m_pMoudleDataBase->select_create_time(FileSeq, CreateTime);
	//GUI显示
	QString Temp = QString::fromStdString(FileName);
	emit(new_share(FileSeq, QString::fromStdString(FileName),
		QString::fromStdString(strRemark), QString::fromStdString(CreateTime), 0));
	//加入内存
}