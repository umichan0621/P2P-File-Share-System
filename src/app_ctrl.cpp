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

using namespace std::chrono;
#ifdef _DEBUG
static const char* DATA_BASE_PATH = "../test_debug.db";
#else
static const char* DATA_BASE_PATH = "../test_release.db";
#endif
//Tracker列表
static std::vector<std::pair<const char*, uint16_t>> TRACKER_LIST = {
	//#ifdef _DEBUG
	{"121.5.179.213",2248}
	//#else
	//{"127.0.0.1",2345}
	//#endif
	//{"127.0.0.1",2345}
};

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
	m_pMoudleHandler(new MoudleHandler()),
	m_pThreadPool(new base::ThreadPool())
{}

AppCtrl::~AppCtrl() {}

bool AppCtrl::init()
{
	//根据时间生成随机种子
	srand(base::now_milli());
	//初始化数据库
	if (false == g_pDataBaseManager->open(DATA_BASE_PATH))
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
	//初始化Buffer池
	g_pBufferPoolMgr->init(32, MIN_MTU);
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
	g_pSessionManager->init(ListenFd, ListenFd6, ListenFdNAT);
	//初始化信号和槽
	init_slots();
	//设置同时最大下载数
	g_pFileManager->set_max_download(g_pConfig->max_download());
	//Net模块开始监听端口
	m_pMoudleNet->listen(m_pThreadPool);
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
	//依次连接Tracker
	for (auto& Tracker : TRACKER_LIST)
	{
		LOG_ERROR <<"Tracker<"<< Tracker.first << ":" << Tracker.second<<">";
		uint16_t Session = g_pSessionManager->connect_tracker(Tracker.first, Tracker.second);

	}
	m_pThreadPool->add_task(std::bind(&AppCtrl::thread_loop_search, this));
	////启动循环下载线程
	//m_pThreadPool->add_task(std::bind(&AppCtrl::thread_loop_download, this));
	m_pMoudleGui->show();

#else//客户端
	
	//依次连接Tracker
	for (auto& Tracker : TRACKER_LIST)
	{
		uint16_t Session = g_pSessionManager->connect_tracker(Tracker.first, Tracker.second);

	}


	//启动循环下载线程
	m_pThreadPool->add_task(std::bind(&AppCtrl::thread_loop_download, this));

	m_pThreadPool->add_task(std::bind(&AppCtrl::thread_loop_search, this));
	m_pMoudleGui->show();

#endif
}

//初始化
void AppCtrl::init_slots()
{
	qRegisterMetaType<int32_t>("int32_t");
	qRegisterMetaType<uint64_t>("uint64_t");
	{
		//来自下载界面删除下载文件的信号
		connect(m_pMoudleGui->download_list(), &gui::DownloadList::output_delete_file, this,
			[&](int32_t FileSeq)
			{
				bool bRes = g_pDataBaseManager->delete_file_info(FileSeq);
				if (true == bRes)
				{
					m_pMoudleGui->my_file()->clear_file(FileSeq);
					g_pFileManager->kill_download_file(FileSeq);
				}
			});
		//来自分享界面删除分享文件的信号
		connect(m_pMoudleGui->share_tree(), &gui::ShareTree::delete_file, this,
			[&](int32_t FileSeq)
			{
				bool bRes = g_pDataBaseManager->delete_file_info(FileSeq);
				if (true == bRes)
				{
					m_pMoudleGui->my_file()->clear_file(FileSeq);
					g_pFileManager->kill_share_file(FileSeq);
				}
			});
		//来自My File删除文件/文件夹的信号
		connect(m_pMoudleGui->my_file(), &gui::MyFile::output_delete_file, this,
			[&](int32_t FileSeq, uint8_t FileType)
			{
				bool bRes = g_pDataBaseManager->delete_file_info(FileSeq);
				if (true == bRes)
				{
					//是分享文件
					if (STATUS_SHARE == FileType)
					{
						m_pMoudleGui->share_tree()->clear_file(FileSeq);
					}
					//是下载文件
					else if (STATUS_SHARE > FileType)
					{
						m_pMoudleGui->download_list()->clear_file(FileSeq);
					}
					LOG_ERROR << "DELETE " << FileSeq<<" "<< FileType;
				}
			});
	}
	
	//获取下载文件分享链接的信号
	connect(m_pMoudleGui->download_list(), &gui::DownloadList::create_link, this,
		[&](int32_t FileSeq)//下载页面
		{
			get_link(FileSeq);
		});
	connect(m_pMoudleGui->share_tree(), &gui::ShareTree::create_link, this,
		[&](int32_t FileSeq)//分享页面
		{
			get_link(FileSeq);
		});
	//打开下载文件路径的信号
	connect(m_pMoudleGui->download_list(), &gui::DownloadList::open_folder, this,
		[&](int32_t FileSeq)
		{
			open_folder(FileSeq);
		});
	connect(m_pMoudleGui->share_tree(), &gui::ShareTree::open_folder, this,
		[&](int32_t FileSeq)
		{
			open_folder(FileSeq);
		});
	//添加下载任务的信号
	connect(m_pMoudleGui->add_dialog(), &gui::AddDialog::add_download_file, this,
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
	connect(m_pMoudleGui->add_dialog(), &gui::AddDialog::add_share_file, this,
		[&](const QString& strRemark, const QString& strPath)
		{
			//将任务交给其他工作线程处理，避免出现卡死的现象（当前任务耗时较长）
			m_pThreadPool->add_task(std::bind(
				&AppCtrl::thread_add_share_file,
				this,
				strRemark.toStdString(),
				strPath.toStdString()));
		});

	//收到确认移动文件的信号，实施文件移动
	connect(m_pMoudleGui->folder_choose_dialog(), 
		&gui::FolderChooseDialog::file_move_to, this, [&]
	(int32_t FileSeq, int32_t FileParent)
		{
			bool bRes = g_pDataBaseManager->update_file_parent(FileSeq, FileParent);
			if (true == bRes)
			{

				emit(m_pMoudleGui->my_file()->file_move_to(FileSeq, FileParent));
			}
		});
}

void AppCtrl::init_file()
{
	uint64_t FileSize = 0;
	base::SHA1 SHA1Struct = { 0 };
	std::string strSHA1;
	std::string strFilePath, strFileName;
	std::vector<int32_t> VecFileSeq;

	{//加载下载中文件
		VecFileSeq.clear();
		g_pDataBaseManager->select_file_info(VecFileSeq, STATUS_DOWNLOAD);
		for (auto& FileSeq : VecFileSeq)
		{
			g_pDataBaseManager->select_file_info(FileSeq, strSHA1);
			base::sha1_parse(strSHA1, SHA1Struct);
			g_pDataBaseManager->select_file_info(FileSeq, strFilePath, strFileName, FileSize);
			//载入文件
			file::FileCtrlIncmpl* pFileCtrl = new file::FileCtrlIncmpl();
			bool bRes = pFileCtrl->init(base::utf8_to_string(strFilePath), FileSize);
			if (false == bRes)
			{
				//delete pFileCtrl;
				//删除.ctrl文件
				//提示客户端当前的错误，让客户端决定是否删除文件
				//
			}
			else
			{
				pFileCtrl->set_file_seq(FileSeq);
				pFileCtrl->set_sha1(SHA1Struct);
				g_pFileManager->add_download_file(pFileCtrl);
				g_pFileManager->refresh_download_list();
				//加入PartnerTable
				g_pPartnerTable->add_cid(SHA1Struct);
				{//GUI显示
					emit(m_pMoudleGui->my_file()->show_my_file(FileSeq));
					emit(m_pMoudleGui->download_list()->show_download(FileSeq));
				}//GUI显示
			}
		}
	}//加载下载中文件

	{//加载暂停下载的文件
		VecFileSeq.clear();
		g_pDataBaseManager->select_file_info(VecFileSeq, STATUS_PAUSE);
		for (auto& FileSeq : VecFileSeq)
		{
			g_pDataBaseManager->select_file_info(FileSeq, strSHA1);
			base::sha1_parse(strSHA1, SHA1Struct);
			//GUI显示
			g_pDataBaseManager->select_file_info(FileSeq, strFilePath, strFileName, FileSize);
			emit(m_pMoudleGui->download_list()->show_download(FileSeq));
		}
	}//加载暂停下载的文件


	{//加载下载完成的文件
		VecFileSeq.clear();
		g_pDataBaseManager->select_file_info(VecFileSeq, STATUS_COMPLETE);
		for (auto& FileSeq : VecFileSeq)
		{
			g_pDataBaseManager->select_file_info(FileSeq, strSHA1);
			base::sha1_parse(strSHA1, SHA1Struct);

			//GUI显示
			g_pDataBaseManager->select_file_info(FileSeq, strFilePath, strFileName, FileSize);
			emit(m_pMoudleGui->download_list()->show_download(FileSeq));
		}
	}//加载下载完成的文件


	{//加载分享的文件
		VecFileSeq.clear();
		g_pDataBaseManager->select_file_info(VecFileSeq, STATUS_SHARE);
		for (auto& FileSeq : VecFileSeq)
		{
			g_pDataBaseManager->select_file_info(FileSeq, strSHA1);
			base::sha1_parse(strSHA1, SHA1Struct);
			g_pDataBaseManager->select_file_info(FileSeq, strFilePath, strFileName, FileSize);
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

				{//GUI显示
					//发送向文件管理系统添加文件/文件夹的信号
					emit(m_pMoudleGui->my_file()->show_my_file(FileSeq));
					//发送新建分享文件的信号
					emit(m_pMoudleGui->share_tree()->show_share(FileSeq));
				}//GUI显示
			}
		}
	}//加载分享的文件

	{//加载文件夹
		VecFileSeq.clear();
		g_pDataBaseManager->select_file_info(VecFileSeq, STATUS_FOLDER);
		for (auto& FileSeq : VecFileSeq)
		{
			//GUI显示
			emit(m_pMoudleGui->my_file()->show_my_file(FileSeq));
		}
	}//加载文件夹
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
	g_pDataBaseManager->select_file_info(FileSeq, strSHA1);
	g_pDataBaseManager->select_file_size(FileSeq, FileSize);
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
	g_pDataBaseManager->select_file_name(FileSeq, FileName);
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
	g_pDataBaseManager->select_file_path(FileSeq, strPath);
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
	file::FileCtrlIncmpl* pDownloadFile;
	uint64_t FragmentStart;
	base::SHA1 FileSHA1;
	std::vector<uint16_t> PartnerList;
	uint64_t LastRefreshTime = base::now_milli() - PROGRESS_REFRESH_FREQUENCY;
	uint32_t BaseSleepTime = (1000 * 1000) >> 5;
	//构造发送的数据包头部
	char DLReqBuf[30] = { 0 };
	create_header(DLReqBuf, PROTOCOL_FILE_FRAGMENT_REQ);
	high_resolution_clock::time_point WakeUpTime = high_resolution_clock::now();
	for (;;)
	{
		{//定时刷新各个任务的下载进度
			if (base::now_milli() - LastRefreshTime >= PROGRESS_REFRESH_FREQUENCY)
			{
				LastRefreshTime = base::now_milli();
				std::vector<file::DownloadStatus> VecStatus;
				g_pFileManager->download_status(VecStatus);
				for (auto& Status : VecStatus)
				{
					emit(m_pMoudleGui->download_list()->update_progress(
						Status.FileSeq, Status.FileSize, Status.DownloadRate));
				}
			}
		}//定时刷新各个任务的下载进度

		{//获取当前下载的文件和下载的Fragment
			bool bRes = g_pFileManager->download_task(FileCtrl);
			//下载列表为空
			if (false == bRes)
			{
				FileCtrl.reset();
				continue;
			}
			else
			{
				//当前的下载任务
				pDownloadFile = (file::FileCtrlIncmpl*)FileCtrl.get();
				bRes = pDownloadFile->get_task(FragmentStart);
				//返回false表示当前文件已无需要下载的Fragment，可能已经全部下载完成
				if (false == bRes)
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
							m_pThreadPool->add_task(std::bind(
								&AppCtrl::thread_sha1_check, this, FileCtrl));
							FileCtrl.reset();
							continue;
						}
					}
				}
			}
		}//获取当前下载的文件和下载的Fragment

		{//获取下载任务并向Peer请求
			pDownloadFile->full_sha1(FileSHA1);
			//写入文件SHA1值和FragmentStart
			memcpy(&DLReqBuf[2], &FileSHA1, 20);
			memcpy(&DLReqBuf[22], &FragmentStart, 8);
			PartnerList.clear();

			g_pPartnerTable->search_cid(FileSHA1, PartnerList);
			if (true == PartnerList.empty())
			{
				FileCtrl.reset();
				continue;
			}
			//随机一个Peer
			int32_t RanPos = rand() % PartnerList.size();
			for (int32_t i = 0; i < PartnerList.size(); ++i)
			{
				int32_t Pos = (RanPos + i) % PartnerList.size();
				uint16_t SessionId = PartnerList[Pos];
				net::Session* pCurSession = g_pSessionManager->session(SessionId);
				if (nullptr != pCurSession)
				{
					pCurSession->send_reliable(DLReqBuf, 30);
					FileCtrl.reset();
					break;
				}
			}
		}//获取下载任务并向Peer请求

		WakeUpTime += microseconds(BaseSleepTime / DOWNLOAD_RATE);
		std::this_thread::sleep_until(WakeUpTime);
	}
}

void AppCtrl::thread_loop_search()
{
	high_resolution_clock::time_point WakeUpTime = high_resolution_clock::now();
	//构造路由搜索协议头
	//协议格式:
	//[固定头部(2B)] + (0-3个)[CID(20B)]
	char SearchBuf[100] = { 0 };
	create_header(SearchBuf, PROTOCOL_ROUTING_SEARCH_REQ);
	//构造路由注册协议头，加入固定的PID
	//协议格式:
	//[固定头部(2B)] + [PID(20B)]
	char RegisterBuf[30] = { 0 };
	create_header(RegisterBuf, PROTOCOL_ROUTING_REGISTER_REQ);
	memcpy(&RegisterBuf[2], g_pRoutingTable->pid(), KLEN_KEY);

	{//TEST
		std::string str;
		base::sha1_value(g_pRoutingTable->pid(), str);
		LOG_ERROR << "Current PID:" << str;
	}//TEST
	
	for (;;)
	{
		uint16_t SessionId = g_pPeerManager->register_pop();

		if (ERROR_SESSION_ID != SessionId)
		{
			net::Session* pCurSession = g_pSessionManager->session(SessionId);
			if (nullptr == pCurSession)
			{
				continue;
			}
			int32_t Pos = 2;
			//获取3个CID，如果重复说明总的CID不足3个
			std::unordered_set<base::SHA1, base::SHA1HashFunc, base::SHA1EqualFunc> CIDSet;
			base::SHA1 CID = { 0 };
			for (int32_t i = 0; i < 3; ++i)
			{
				if (false == g_pPartnerTable->get_cid(CID))
				{
					break;
				}
				CIDSet.insert(CID);
			}
			for (auto& CurCID : CIDSet)
			{
				memcpy(&SearchBuf[Pos], &CurCID, KLEN_KEY);
				Pos += KLEN_KEY;
			}
			{//Test
				LOG_DEBUG << "Search Session:" << SessionId;
			}//Test
				
			pCurSession->send_reliable(RegisterBuf, 2 + KLEN_KEY);
			pCurSession->send_reliable(SearchBuf, Pos);
		}
		WakeUpTime += microseconds(1 * 1000 * 1000);
		std::this_thread::sleep_until(WakeUpTime);
	}
}

void AppCtrl::thread_sha1_check(file::FileCtrl FileCtrl)
{
	base::SHA1 ActrualSHA1Struct, SHA1Struct;
	FileCtrl->sha1(SHA1Struct);
	FileCtrl->full_sha1(ActrualSHA1Struct);
	//校验SHA1无问题
	int32_t FileSeq = FileCtrl->file_seq();
	if (true == sha1_equal(ActrualSHA1Struct, SHA1Struct))
	{
		LOG_ERROR << "File["<<FileSeq <<"] Complete";
		g_pDataBaseManager->update_file_type(FileSeq, STATUS_COMPLETE);
		emit(m_pMoudleGui->download_list()->file_complete(FileSeq));
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
	int32_t FileSeq = g_pDataBaseManager->select_file_seq(strSHA1);

	//当前MD5的文件已在数据库中
	if (-1 != FileSeq)
	{
		LOG_TRACE << "[Download Task]" << strSHA1 << " has been recorded, FileSeq = " << FileSeq;
		return false;
	}

	FileSeq = g_pDataBaseManager->file_seq();

	std::string FileName;
	if (strLink.size() > 40)
	{
		FileName = strLink.substr(40, strLink.size() - 40);
	}
	else
	{
		FileName = strSHA1;
	}
	//创建本地文件
	file::FileCtrlIncmpl* pFileCtrl = new file::FileCtrlIncmpl();
	
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
		pFileCtrl->set_file_seq(FileSeq);
		pFileCtrl->set_sha1(SHA1Struct);
		//数据库持久化
		g_pDataBaseManager->insert_file_info(FileSeq, strSHA1, STATUS_DOWNLOAD, strFilePath, FileSize);
		g_pDataBaseManager->update_file_info(FileSeq, FileName, "");
		g_pFileManager->add_download_file(pFileCtrl);
		g_pFileManager->refresh_download_list();
		//加入PartnerTable
		g_pPartnerTable->add_cid(SHA1Struct);
		{//借助路由表查询当前CID
		//构造路由搜索协议
			char SearchBuf[22] = { 0 };
			create_header(SearchBuf, PROTOCOL_ROUTING_SEARCH_REQ);
			memcpy(&SearchBuf[2], &SHA1Struct, KLEN_KEY);
			std::unordered_set<int32_t> PeerSet;
			g_pRoutingTable->get_node(SHA1Struct.Hash, PeerSet);
			LOG_ERROR << PeerSet.size();
			for (auto& PeerId : PeerSet)
			{
				uint16_t SessionId = g_pPeerManager->session_id(PeerId);
				LOG_ERROR << SessionId;

				if (0 != SessionId)
				{
					net::Session* pCurSession = g_pSessionManager->session(SessionId);
					if (nullptr != pCurSession)
					{
						pCurSession->send_reliable(SearchBuf, 22);
					}
				}
			}
		}//借助路由表查询当前CID
		{//GUI显示
			emit(m_pMoudleGui->my_file()->show_my_file(FileSeq));
			emit(m_pMoudleGui->download_list()->show_download(FileSeq));
		}//GUI显示

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
	base::sha1_value(SHA1Struct, strSHA1);
	LOG_ERROR << strSHA1;
	int32_t FileSeq = g_pDataBaseManager->select_file_seq(strSHA1);
	//当前MD5的文件已在数据库中
	if (-1 != FileSeq)
	{
		LOG_TRACE << "The file:" + strPathLocal + " has been recorded, FileSeq = " << FileSeq;
		return;
	}
	//获取可用的FileSeq
	FileSeq = g_pDataBaseManager->file_seq();

	bool bRes = g_pDataBaseManager->insert_file_info(FileSeq, strSHA1, STATUS_SHARE, strPath, FileSize);
	if (false == bRes)
	{
		return;
	}

	std::string FileName = base::string_to_utf8(pFileCtrl->file_name());
	bRes = g_pDataBaseManager->update_file_info(FileSeq, FileName, strRemark);
	pFileCtrl->set_file_seq(FileSeq);
	pFileCtrl->set_sha1(SHA1Struct);

	g_pFileManager->add_share_file(pFileCtrl);
	//加入PartnerTable
	g_pPartnerTable->add_cid(SHA1Struct);
	{//借助路由表查询当前CID
		//构造路由搜索协议
		char SearchBuf[22] = { 0 };
		create_header(SearchBuf, PROTOCOL_ROUTING_SEARCH_REQ);
		memcpy(&SearchBuf[2], &SHA1Struct, KLEN_KEY);
		std::unordered_set<int32_t> PeerSet;
		g_pRoutingTable->get_node(SHA1Struct.Hash, PeerSet);
		for (auto& PeerId : PeerSet)
		{
			uint16_t SessionId = g_pPeerManager->session_id(PeerId);
			if (0 != SessionId)
			{
				net::Session* pCurSession = g_pSessionManager->session(SessionId);
				if (nullptr != pCurSession)
				{
					pCurSession->send_reliable(SearchBuf, 22);
				}
			}
		}
	}//借助路由表查询当前CID

	{//GUI显示
		emit(m_pMoudleGui->download_list()->show_download(FileSeq));
		emit(m_pMoudleGui->my_file()->show_my_file(FileSeq));
		emit(m_pMoudleGui->share_tree()->show_share(FileSeq));
	}//GUI显示
}