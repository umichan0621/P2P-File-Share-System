#include "file_manager.h"
#include <base/logger/logger.h>
#pragma warning(disable:4267)

namespace file
{
	FileManager::FileManager():
		m_MaxDownloadNum(8),
		m_CurDownload(0){}

	FileManager::~FileManager() {}

	//添加和删除文件
	bool FileManager::add_share_file(FileCtrlInterface* pCmplFile)
	{
		int32_t FileSeq = pCmplFile->file_seq();
		//添加一个分享的文件，同时建立映射关系
		std::lock_guard<std::mutex> Lock(m_FileMgrMutex);
		//添加文件失败
		if (0 != m_FileMap.count(FileSeq))
		{
			return false;
		}
		SHA1 SHA1Struct;
		pCmplFile->full_sha1(SHA1Struct);
		if (0 != m_SeqMap.count(SHA1Struct))
		{
			return false;
		}
		m_SeqMap[SHA1Struct] = FileSeq;
		m_FileMap[FileSeq] = FileCtrl(pCmplFile);
		return true;
	}

	bool FileManager::kill_share_file(int32_t FileSeq)
	{
		//删除维护的文件控制器
		std::lock_guard<std::mutex> Lock(m_FileMgrMutex);
		if (0!=m_FileMap.count(FileSeq))
		{
			//只删除自己维护的
			FileCtrl pFileCtrl = m_FileMap[FileSeq];
			SHA1 SHA1Struct;
			pFileCtrl->full_sha1(SHA1Struct);

			if (0 != m_SeqMap.count(SHA1Struct))
			{
				m_SeqMap.erase(SHA1Struct);
			}
			pFileCtrl.reset();
			m_FileMap.erase(FileSeq);
		}
		return false;
	}

	bool FileManager::add_download_file(FileCtrlInterface* pIncmplFile)
	{
		int32_t FileSeq = pIncmplFile->file_seq();
		//添加一个下载文件到等待队列中
		std::lock_guard<std::mutex> Lock(m_FileMgrMutex);
		//添加文件失败
		if (0 != m_FileMap.count(FileSeq))
		{
			return false;
		}
		SHA1 SHA1Struct;
		pIncmplFile->full_sha1(SHA1Struct);
		if (0 != m_SeqMap.count(SHA1Struct))
		{
			return false;
		}
		m_SeqMap[SHA1Struct] = FileSeq;
		FileCtrl CurDownload = FileCtrl(pIncmplFile);
		//建立映射
		m_FileMap[FileSeq] = CurDownload;
		//添加到等待下载队列
		m_WaitList.emplace_back(CurDownload);
		//刷新
		return true;
	}

	bool FileManager::kill_download_file(int32_t FileSeq)
	{
		std::lock_guard<std::mutex> Lock(m_FileMgrMutex);
		SHA1 SHA1Struct;
		m_FileMap[FileSeq]->full_sha1(SHA1Struct);
		m_SeqMap.erase(SHA1Struct);
		m_FileMap.erase(FileSeq);
		//检查下载队列
		for (auto It = m_DownloadList.begin(); It != m_DownloadList.end(); ++It)
		{
			if (It->get()->file_seq() == FileSeq)
			{
				m_DownloadList.erase(It);
				return true;
			}
		}
		//检查等待队列
		for (auto It = m_WaitList.begin(); It != m_WaitList.end(); ++It)
		{
			if (It->get()->file_seq() == FileSeq)
			{
				m_WaitList.erase(It);
				return true;
			}
		}
		return false;
	}
	//添加和删除文件

	bool FileManager::check_download_file(int32_t FileSeq)
	{
		LOG_ERROR << "CHECK";
		//校验当前下载文件的SHA1值
		std::unique_lock<std::mutex> Lock(m_FileMgrMutex);
		//从下载队列移除
		for (auto It = m_DownloadList.begin(); It != m_DownloadList.end(); ++It)
		{
			if (It->get()->file_seq() == FileSeq)
			{
				m_DownloadList.erase(It);
				return true;;
			}
		}
		return false;
	
	}

	void FileManager::refresh_download_list()
	{
		std::lock_guard<std::mutex> Lock(m_FileMgrMutex);
		while (m_DownloadList.size() < m_MaxDownloadNum)
		{
			if (true == m_WaitList.empty())
			{
				return;
			}
			FileCtrl& Temp= m_WaitList.front();
			m_DownloadList.emplace_back(Temp);
			m_WaitList.pop_front();
		}
	}

	bool FileManager::download_task(FileCtrl& FileCtrl)
	{
		std::lock_guard<std::mutex> Lock(m_FileMgrMutex);
		if (true == m_DownloadList.empty())
		{
			return false;
		}
		FileCtrl = m_DownloadList[m_CurDownload];
		++m_CurDownload;
		m_CurDownload %= m_DownloadList.size();	
		return true;
	}

	bool FileManager::file_ctrl(const SHA1& SHA1Struct, FileCtrl& FileCtrl)
	{
		std::lock_guard<std::mutex> Lock(m_FileMgrMutex);
		if (0 == m_SeqMap.count(SHA1Struct))
		{
			return false;
		}
		int32_t FileSeq = m_SeqMap[SHA1Struct];
		if (0 == m_FileMap.count(FileSeq))
		{
			return false;
		}
		FileCtrl = m_FileMap[FileSeq];
		return true;
	}

	void FileManager::download_status(std::vector<DownloadStatus>& VecStatus)
	{
		std::lock_guard<std::mutex> Lock(m_FileMgrMutex);
		for (auto& Task : m_DownloadList)
		{
			DownloadStatus Status = {0};
			Status.FileSeq=Task->file_seq();
			Status.FileSize = Task->size();
			Status.DownloadRate = Task->rate();
			VecStatus.emplace_back(std::move(Status));
		}
	}

	void FileManager::set_max_download(uint16_t MaxDownLoadNum)
	{
		m_MaxDownloadNum = MaxDownLoadNum;
	}
}