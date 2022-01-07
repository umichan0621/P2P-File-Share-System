#include "file_ctrl_complete.h"
#include <base/config.hpp>
#include <base/logger/logger.h>
#pragma warning(disable:4267)

namespace file
{
	FileCtrlCmpl::FileCtrlCmpl(){}

	FileCtrlCmpl::~FileCtrlCmpl() {}

	bool FileCtrlCmpl::open(std::string strFilePath)
	{
		int32_t Pos = (int32_t)strFilePath.rfind("/");
		Pos = max(Pos, (int32_t)strFilePath.rfind("\\"));
		if (Pos < 0)
		{
			LOG_ERROR << "Illgeal file path: " << strFilePath;
			return false;
		}
		//文件是否存在
		bool bFileExist = file_exist(strFilePath);
		if (false == bFileExist)
		{
			LOG_ERROR << "File not exist: " << strFilePath;
			return false;
		}
		//获取文件名字和文件所在路径
		m_strFolderPath = strFilePath.substr(0, (size_t)Pos + 1);
		m_strFileName = strFilePath.substr((size_t)Pos + 1, strFilePath.size() - Pos - 1);
		if (nullptr != m_pFile)
		{
			delete m_pFile;
		}
		m_pFile = new File();
		//打开当前文件
		if (false == m_pFile->open(strFilePath, true))
		{
			LOG_ERROR << "Fail to open file: " << strFilePath;
			return false;
		}
		return true;
	}

	bool FileCtrlCmpl::init(const std::string& strFilePath, uint64_t Len)
	{
		int32_t Pos = (int32_t)strFilePath.rfind("/");
		Pos = max(Pos, (int32_t)strFilePath.rfind("\\"));
		if (Pos < 0)
		{
			LOG_ERROR << "Illgeal file path: " << strFilePath;
			return false;
		}
		//文件是否存在
		bool bFileExist = file_exist(strFilePath);
		if (false == bFileExist)
		{
			return false;
		}
		//获取文件名字和文件所在路径
		m_strFolderPath = strFilePath.substr(0, (size_t)Pos + 1);
		m_strFileName = strFilePath.substr((size_t)Pos + 1, strFilePath.size() - Pos - 1);


		if (nullptr != m_pFile)
		{
			delete m_pFile;
		}
		m_pFile = new File();
		//打开当前文件
		if (false == m_pFile->open(strFilePath, true))
		{
			LOG_ERROR << "Fail to open file: " << strFilePath;
			return false;
		}
		//当前文件夹可能有其他同名文件，终止操作
		if (Len != m_pFile->size())
		{
			LOG_ERROR << "File name might be conflict: " << strFilePath;
			return false;
		}
		return true;
	}

	void FileCtrlCmpl::close()
	{
		m_pFile->close();
	}

	const char* FileCtrlCmpl::read_fragment(uint64_t FragmentStart, uint64_t& Len)
	{
		return m_pFile->read(FragmentStart, Len);
	}

	bool FileCtrlCmpl::md5_segment(MD5& MD5Obj, uint64_t Start)
	{
		return m_pFile->md5(MD5Obj, Start, FULL_SEGMENT_SIZE);
	}

	uint64_t FileCtrlCmpl::rate() 
	{ 
		return 0; 
	}

	uint64_t FileCtrlCmpl::size() 
	{ 
		return m_pFile->size();
	}

	void FileCtrlCmpl::check_segment_md5(uint64_t SegmentStart, MD5& MD5Obj){}

	void FileCtrlCmpl::complete_fragment(const char* pBuf, uint64_t FragmentStart, uint16_t Len) {}

	void FileCtrlCmpl::complete_segment(uint64_t SegmentStart) {}
}