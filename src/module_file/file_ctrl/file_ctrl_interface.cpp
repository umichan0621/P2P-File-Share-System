#include "file_ctrl_interface.h"
#include <base/config.hpp>

namespace file
{
	
	FileCtrlInterface::FileCtrlInterface():
		m_pFile(nullptr),
		m_FileSeq(0),
		m_ActualSHA1({0}){}

	FileCtrlInterface::~FileCtrlInterface(){}

	bool FileCtrlInterface::time(uint64_t& WriteTime)
	{
		return m_pFile->time(WriteTime);
	}

	void FileCtrlInterface::set_sha1(const SHA1& SHA1Struct)
	{
		m_ActualSHA1= SHA1Struct;
	}

	void FileCtrlInterface::full_sha1(SHA1& SHA1Struct)
	{
		SHA1Struct = m_ActualSHA1;
	}
	
	bool FileCtrlInterface::sha1(SHA1& SHA1Struct)
	{
		return m_pFile->sha1(SHA1Struct);
	}
	
	void FileCtrlInterface::check_segment_md5(uint64_t SegmentStart, MD5& MD5Obj){}

	void FileCtrlInterface::complete_fragment(const char* pBuf, uint64_t FragmentStart, uint16_t Len) {}

	void FileCtrlInterface::complete_segment(uint64_t SegmentStart) {}

	void FileCtrlInterface::set_file_seq(int32_t FileSeq)
	{
		m_FileSeq = FileSeq;
	}

	uint32_t FileCtrlInterface::file_seq()
	{
		return m_FileSeq;
	}
	
	uint64_t FileCtrlInterface::full_size()
	{
		return m_pFile->size();
	}

	const std::string& FileCtrlInterface::file_name()
	{
		return m_strFileName;
	}

}