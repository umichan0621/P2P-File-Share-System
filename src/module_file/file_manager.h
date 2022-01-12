/***********************************************************************/
/* 名称:文件管理器												       */
/* 说明:管理所有已完成文件的共享和未完成文件的下载					   */
/* 创建时间:2021/11/29												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <unordered_map>
#include <vector>
#include <deque>
#include <list>
#include <mutex>
#include <base/singleton.hpp>
#include <module_file/file_ctrl/file_ctrl_interface.h>

namespace file
{
	typedef std::shared_ptr<FileCtrlInterface> FileCtrl;		//使用智能指针避免错误释放

	struct DownloadStatus
	{
		int32_t FileSeq;
		uint64_t FileSize;
		uint64_t DownloadRate;
	};

	class FileManager
	{
		typedef std::unordered_map<base::SHA1, int32_t, 
			base::SHA1HashFunc, base::SHA1EqualFunc>							SeqMap;			//SHA1->文件序号
		typedef std::unordered_map<int32_t, FileCtrl>							FileMap;		//文件序号->文件控制器
		typedef std::deque<FileCtrl>											DownloadList;
		typedef std::list<FileCtrl>												WaitList;
	public:
		FileManager();
		~FileManager();
	public://添加和删除文件
		//分享中的文件相关
		bool add_share_file(FileCtrlInterface* pCmplFile);
		bool kill_share_file(int32_t FileSeq);
		//下载中的文件相关
		bool add_download_file(FileCtrlInterface* pIncmplFile);
		bool kill_download_file(int32_t FileSeq);
		bool check_download_file(int32_t FileSeq);
	public:
		void refresh_download_list();
		//获取一个下载任务
		bool download_task(FileCtrl& FileCtrl);
		bool file_ctrl(const base::SHA1& SHA1Struct, FileCtrl& FileCtrl);
		void download_status(std::vector<DownloadStatus>& VecStatus);
		void set_max_download(uint16_t MaxDownLoadNum);
	private://文件控制器的相关容器
		SeqMap				m_SeqMap;			//SHA1->文件序号的映射
		FileMap				m_FileMap;			//文件序号->文件控制器的映射
		DownloadList		m_DownloadList;		//正在下载的文件列表
		WaitList			m_WaitList;			//等待下载的文件列表
	private://下载相关参数
		uint16_t			m_MaxDownloadNum;
		uint16_t			m_CurDownload;		//当前选中的文件序号
	private:
		std::mutex			m_FileMgrMutex;
	};
}

#define g_pFileManager base::Singleton<file::FileManager>::get_instance()