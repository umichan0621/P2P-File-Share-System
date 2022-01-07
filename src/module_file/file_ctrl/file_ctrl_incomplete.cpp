#include "file_ctrl_incomplete.h"
#include <chrono>
#include <base/protocol/protocol_base.h>
#include <base/logger/logger.h>
using namespace std::chrono;
#pragma warning(disable:4244)
#pragma warning(disable:4267)

namespace file
{
	FileCtrlIncmpl::FileCtrlIncmpl() :
		m_FragmentNum(0),
		m_SegmentNum(0),
		m_pCtrlFile(nullptr),
		m_PriorityFragment(64),
		m_CurFragment(0),
		m_CurFragmentNum(0),
		m_PreFragmentNum(0),
		m_PreClock(0){}

	FileCtrlIncmpl::~FileCtrlIncmpl()
	{
		if (nullptr == m_pFile)
		{
			delete m_pFile;
		}
		if (nullptr == m_pCtrlFile)
		{
			delete m_pCtrlFile;
		}
	}

	bool FileCtrlIncmpl::get_task(uint64_t& FragmentStart)
	{
		//正确得到Fragment任务返回true
		//如果有优先下载的Segment
		if (false == m_PrioritySegmentQueue.empty())
		{
			while (false == m_PrioritySegmentQueue.empty())
			{
				uint64_t SegmentSeq = m_PrioritySegmentQueue.front();
				//当前Fragment还没有开始下载，初始化
				if (64 == m_PriorityFragment)
				{
					//计算PriorityFragment是最后一个Segment下的情况
					m_PriorityFragment = min((SegmentSeq << 6) + 63, m_FragmentNum - 1) & 0x3f;
				}
				while (1)
				{
					//计算要下载的Fragment序号
					uint64_t FragmentSeq = (SegmentSeq << 6) + m_PriorityFragment;
					if (0 == m_PriorityFragment)
					{
						m_PriorityFragment = 64;
						m_PrioritySegmentQueue.pop();
						//当前Fragment还没有下载
						bool bRes = m_pCtrlFile->read_binary(FragmentSeq);
						if (1 == bRes)
						{
							FragmentStart = FragmentSeq << 15;
							return true;
						}
						break;
					}
					else
					{
						--m_PriorityFragment;
						//当前Fragment还没有下载
						bool bRes = m_pCtrlFile->read_binary(FragmentSeq);
						if (1 == bRes)
						{
							FragmentStart = FragmentSeq << 15;
							return true;
						}
						//当前Fragment已下载，则循环检测下一个Fragment
					}

				}
			}
		}
		//没有优先下载的Segment，则按顺序下载
		else
		{
			while (1)
			{
				if (m_CurFragment >= m_FragmentNum)
				{
					return false;
				}
				//某个Segment的第一个Fragment
				if (0 == (m_CurFragment & 0x3f))
				{
					//计算得到SegmentSeq
					uint64_t SegmentSeq = m_CurFragment >> 6;

					//当前Segment是优先任务，不在这里处理
					if (0 != m_PrioritySegmentSet.count(SegmentSeq))
					{
						m_CurFragment += 64;
						continue;
					}
					bool bRes = m_pCtrlFile->read_binary(m_FragmentNum + SegmentSeq);
					//1表示未完成，0表示已完成，如果当前Segment已完成
					if (0 == bRes)
					{
						m_CurFragment += 64;
						continue;
					}
				}
				//计算得到下一个判断Segment的边界
				uint64_t Boundary = ((m_CurFragment >> 6) + 1) << 6;
				//边界应该小于等于最大Fragment数
				Boundary = min(Boundary, m_FragmentNum);
				while (m_CurFragment < Boundary)
				{
					bool bRes = m_pCtrlFile->read_binary(m_CurFragment);
					if (1 == bRes)
					{
						FragmentStart = m_CurFragment << 15;
						++m_CurFragment;
						return true;
					}
					//LOG_ERROR << m_CurFragment << " ok = "<< m_pCtrlFile->read_binary(m_CurFragment);
					++m_CurFragment;
				}
			}
		}
		return true;
	}
	
	bool FileCtrlIncmpl::check_task(uint64_t& FragmentStart)
	{
		m_CurFragment = 0;
		return get_task(FragmentStart);
	}
	
	void FileCtrlIncmpl::add_priority_task(uint64_t SegmentSeq)
	{
		if (0 == m_PrioritySegmentSet.count(SegmentSeq))
		{
			m_PrioritySegmentSet.insert(SegmentSeq);
			m_PrioritySegmentQueue.push(SegmentSeq);
		}
	}

	void FileCtrlIncmpl::complete_fragment(const char* pBuf, uint64_t FragmentStart, uint16_t Len)
	{
		m_pFile->write(pBuf, FragmentStart, Len);
		//获取FragmentSeq
		FragmentStart >>= 15;
		m_pCtrlFile->write_binary(0, FragmentStart);
		++m_CurFragmentNum;
	}

	void FileCtrlIncmpl::complete_segment(uint64_t SegmentStart)
	{
		//获取SegmentSeq
		SegmentStart >>= 21;
		if (SegmentStart <= m_SegmentNum)
		{
			m_pCtrlFile->write_binary(0, m_FragmentNum + SegmentStart);
		}
	}

	const char* FileCtrlIncmpl::read_fragment(uint64_t FragmentStart, uint64_t& Len)
	{
		uint64_t SegmentSeq = FragmentStart >> 21;
		if (SegmentSeq > m_SegmentNum)
		{
			return nullptr;
		}
		bool bRes = m_pCtrlFile->read_binary(m_FragmentNum + SegmentSeq);
		//当前Segment未通过校验，为避免错误数据在网络中分发，拒绝回复
		if (1 == bRes)
		{
			return nullptr;
		}
		return m_pFile->read(FragmentStart, Len);
	}

	bool FileCtrlIncmpl::init(const std::string& strFilePath, uint64_t Len)
	{
		int32_t Pos = (int32_t)strFilePath.rfind("/");
		Pos = max(Pos, (int32_t)strFilePath.rfind("\\"));
		if (Pos < 0)
		{
			LOG_ERROR << "Illgeal file path: " << strFilePath;
			return false;
		}
		//获取文件名字和文件所在路径
		m_strFolderPath = strFilePath.substr(0, (size_t)Pos + 1);
		m_strFileName = strFilePath.substr((size_t)Pos + 1, strFilePath.size() - Pos - 1);
		//下载文件是否存在
		bool bFileExist = file_exist(strFilePath);
		//控制文件是否存在
		bool bCtrlFileExist = file_exist(strFilePath + ".ctrl");
		//如果下载文件存在但是控制文件丢失，当前下载任务异常，返回false
		if (true == bFileExist && false == bCtrlFileExist)
		{
			LOG_ERROR << "Fail to start, ctrl file not exist, delete the file and retry...";
			return false;
		}
		//如果下载文件不存在，则创建
		if (false == bFileExist)
		{
			//创建失败return
			if (false == init_file(Len))
			{
				return false;
			}
		}
		//下载文件已存在，尝试打开
		else
		{
			//打开文件失败
			if (false == open_file(Len))
			{
				return false;
			}
		}
		//计算Fragment数量
		m_FragmentNum = Len >> 15;
		if (0 != Len % FULL_FRAGMENT_SIZE)
		{
			++m_FragmentNum;
		}
		//计算Segment数量
		m_SegmentNum = Len >> 21;
		if (0 != Len % FULL_SEGMENT_SIZE)
		{
			++m_SegmentNum;
		}
		//计算控制文件大小（等于Fragment数+Segment数）
		uint64_t Temp = m_FragmentNum + m_SegmentNum;
		uint64_t CtrlFileSize = Temp >> 3;
		if (0 != Temp % 8)
		{
			++CtrlFileSize;
		}
		//控制文件不存在
		if (false == bCtrlFileExist)
		{
			init_ctrl_file(CtrlFileSize);
		}
		else
		{
			open_ctrl_file(CtrlFileSize);
		}
		init_fragment_num();
		return true;
	}

	void FileCtrlIncmpl::close()
	{
		m_pFile->close();
		m_pCtrlFile->close();
	}

	bool FileCtrlIncmpl::open_file(uint64_t Len)
	{
		if (nullptr != m_pFile)
		{
			delete m_pFile;
		}
		m_pFile = new File();
		std::string FilePath = m_strFolderPath + m_strFileName;
		//打开当前文件
		if (false == m_pFile->open(FilePath, true))
		{
			LOG_ERROR << "Fail to open file: " << FilePath;
			return false;
		}
		//当前文件夹可能有其他同名文件，终止操作
		if (Len != m_pFile->size())
		{
			LOG_ERROR << "File name might be conflict: " << FilePath;
			return false;
		}
		return true;
	}

	bool FileCtrlIncmpl::open_ctrl_file(uint64_t Len)
	{
		//初始化控制文件
		if (nullptr != m_pCtrlFile)
		{
			delete m_pCtrlFile;
		}
		m_pCtrlFile = new File();
		std::string strControlFilePath = m_strFolderPath + m_strFileName + ".ctrl";
		//打开控制文件
		if (false == m_pCtrlFile->open(strControlFilePath, true))
		{
			delete_file(strControlFilePath);
			LOG_ERROR << "Fail to open control file: " << strControlFilePath;
			return false;
		}
		if (m_pCtrlFile->size() != Len)
		{
			LOG_ERROR << "Control file might be error";
			return false;
		}
		return true;
	}

	bool FileCtrlIncmpl::init_file(uint64_t Len)
	{
		//创建文件夹路径
		if (false == create_directory(m_strFolderPath))
		{
			LOG_ERROR << "Fail to create floder: " << m_strFolderPath;
			return false;
		}
		std::string FilePath = m_strFolderPath + m_strFileName;
		//创建文件
		if (false == create_file(FilePath, Len))
		{
			LOG_ERROR << "Fail to create file: " << FilePath;
			return false;
		}
		if (nullptr != m_pFile)
		{
			delete m_pFile;
		}
		//初始化文件
		m_pFile = new File();
		//打开当前文件
		if (false == m_pFile->open(FilePath, true))
		{
			LOG_ERROR << "Fail to open file: " << FilePath;
			return false;
		}
		return true;
	}

	bool FileCtrlIncmpl::init_ctrl_file(uint64_t Len)
	{
		std::string strControlFilePath = m_strFolderPath + m_strFileName + ".ctrl";
		//创建控制文件
		if (false == create_file(strControlFilePath, Len))
		{
			LOG_ERROR << "Fail to create control file: " << strControlFilePath;
			return false;
		}
		//设置为隐藏文件
		SetFileAttributes(to_wstring(strControlFilePath), FILE_ATTRIBUTE_HIDDEN);
		//初始化控制文件
		if (nullptr != m_pCtrlFile)
		{
			delete m_pCtrlFile;
		}
		m_pCtrlFile = new File();
		//打开控制文件
		if (false == m_pCtrlFile->open(strControlFilePath, true))
		{
			delete_file(strControlFilePath);
			LOG_ERROR << "Fail to open control file: " << strControlFilePath;
			return false;
		}
		//初始化全为1
		for (int32_t i = 0; i < m_SegmentNum + m_FragmentNum; ++i)
		{
			m_pCtrlFile->write_binary(1, i);
		}
		return true;
	}

	void FileCtrlIncmpl::init_fragment_num()
	{
		for (uint64_t CurSegment = 0; CurSegment < m_SegmentNum; ++CurSegment)
		{
			bool bRes = m_pCtrlFile->read_binary(m_FragmentNum + CurSegment);
			if (0 == bRes)
			{
				continue;
			}
			uint64_t Start = (CurSegment << 6);
			uint64_t End = min(Start + 64, m_FragmentNum);

			for (uint64_t CurFragment = Start; CurFragment < End; ++CurFragment)
			{
				bool bRes = m_pCtrlFile->read_binary(CurFragment);
				if (0 == bRes)
				{
					++m_CurFragmentNum;
				}
			}
		}
		m_PreClock = _clock();
	}

	void FileCtrlIncmpl::check_segment_md5(uint64_t FragmentStart, MD5& MD5Obj)
	{
		//std::string CurMD5 = md5(FragmentStart, FULL_SEGMENT_SIZE);
		//uint64_t SegmentSeq = FragmentStart >> 21;
		//int sum = 0;
		//for (int i = 0; i < 32; ++i)
		//	sum += (CurMD5[i] - MD5[i]);
		//if (sum == 0)
		//{
		//	set_segment(SegmentSeq);
		//	LOG_TRACE << "CHECK:"<< FragmentStart;
		//}
		//else
		//{
		//	LOG_ERROR << "CHECK FAIL:" << CurMD5<<","<< MD5<<",";
		//	m_TaskQueue.push(FragmentStart+1);
		//	//此处重组Fragment
		//}
	}

	bool FileCtrlIncmpl::md5_segment(MD5& MD5Obj, uint64_t SegmentStart)
	{
		uint64_t SegmentSeq = SegmentStart >> 21;
		bool bRes = m_pCtrlFile->read_binary(m_FragmentNum + SegmentSeq);
		if (0 == bRes)
		{
			return m_pFile->md5(MD5Obj, SegmentStart, FULL_SEGMENT_SIZE);
		}
		return false;
	}

	uint64_t FileCtrlIncmpl::rate()
	{
		uint64_t DeltaByte = (m_CurFragmentNum - m_PreFragmentNum)<<15;
		m_PreFragmentNum = m_CurFragmentNum;
		uint64_t CurClock = _clock();
		uint64_t DeltaClock = CurClock - m_PreClock;
		m_PreClock = CurClock;
		return (DeltaByte * 1000)/ DeltaClock;
	}

	uint64_t FileCtrlIncmpl::size() 
	{
		//LOG_ERROR << m_CurFragmentNum;
		if (m_CurFragmentNum < m_FragmentNum)
		{
			return m_CurFragmentNum << 15;
		}
		return m_pFile->size();
	}

	uint64_t FileCtrlIncmpl::_clock()
	{
		return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	}
}