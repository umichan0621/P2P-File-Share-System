#include "file.h"
#include <locale>
#include <io.h>
#include <direct.h>
#include <third/md5/md5.h>
#include <base/logger/logger.h>
#include <vector>
#pragma warning(disable:6297)
#pragma warning(disable:4244)

namespace file
{
	constexpr uint32_t PAGE_SIZE = 4096;//4KB

	File::File() :
		m_FileSize(0),
		m_pFileHandle(NULL),
		m_pMapFileHandle(NULL),
		m_pMapFile(nullptr),
		m_bIsWriteable(true) {}

	File::~File()
	{
		close();
	}

	bool File::open(const char* FilePath, bool bIsWriteable)
	{
		m_bIsWriteable = bIsWriteable;
		LPCWSTR wstrFilePath = to_wstring(FilePath);
		if (true == bIsWriteable)
		{
			m_pFileHandle = CreateFile(wstrFilePath,
				GENERIC_READ | GENERIC_WRITE, 
				FILE_SHARE_READ, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		}
		else
		{
			m_pFileHandle = CreateFile(wstrFilePath,
				GENERIC_READ, 
				FILE_SHARE_READ, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		}

		if (m_pFileHandle == INVALID_HANDLE_VALUE)
		{
			LOG_ERROR << "Error in CreateFile, ErrorNo = "<<GetLastError()<<" ,path: "<< FilePath;
			return false;
		}
		//读取文件大小
		m_FileSize = GetFileSize(m_pFileHandle, NULL);
		//创建一个文件映射内核对象;
		m_pMapFileHandle = CreateFileMapping(m_pFileHandle,
			NULL,
			PAGE_READWRITE,
			0,
			0,
			NULL);
		if (NULL == m_pMapFileHandle)
		{
			LOG_ERROR << "Error in CreateFileMapping";
			return false;
		}
		m_pMapFile = (char*)MapViewOfFile(m_pMapFileHandle,
			FILE_MAP_ALL_ACCESS,
			0,
			0,
			0);
		if (m_pMapFile == NULL)
		{
			LOG_ERROR << "Error in MapViewOfFile";
			return false;
		}
		return true;
	}

	bool File::open(std::string strFilePath, bool bIsWriteable)
	{
		return open(strFilePath.c_str(), bIsWriteable);
	}

	const char* File::read(uint64_t Start, uint64_t& Len)
	{
		//起始地址大于文件长度
		if (Start >= m_FileSize)
		{
			return nullptr;
		}
		Len = (Start + Len > m_FileSize) ? (m_FileSize - Start) : Len;
		return (m_pMapFile + Start);
	}

	bool File::read_binary(uint64_t Start)
	{
		char Pos = 1<< (Start & 0x7);
		return m_pMapFile[Start >> 3] & Pos;
	}

	bool File::write(const char* pBuf, uint64_t Start, uint64_t Len)
	{
		//文件不可写或起始地址大于文件长度
		if (Start >= m_FileSize)
		{
			return false;
		}
		Len = (Start + Len > m_FileSize) ? (m_FileSize - Start) : Len;
		memcpy(m_pMapFile + Start, pBuf, Len);
		return true;
	}

	bool File::write_binary(bool bValue, uint64_t Start)
	{
		if(Start >= (m_FileSize<<3))
		{
			return false;
		}		
		char& Cur=m_pMapFile[Start >> 3];
		char Pos = 1<<(Start & 0x7);
		if (bValue == 0)
		{
			std::lock_guard<std::mutex> Lock(FileMutex);
			Cur &= ~Pos;
		}
		else
		{
			std::lock_guard<std::mutex> Lock(FileMutex);
			Cur |= Pos;
		}
		return true;
	}

	uint64_t File::size()
	{
		return m_FileSize;
	}

	bool File::time(uint64_t& WriteTime)
	{
		FILETIME CreateFileTime, AccessFileTime, WriteFileTime;
		if (false == GetFileTime(m_pFileHandle, &CreateFileTime, &AccessFileTime, &WriteFileTime))
		{
			LOG_ERROR << "Fail to get file time";
			return false;
		}
		WriteTime = WriteFileTime.dwHighDateTime;
		WriteTime = (WriteTime << 32) | WriteFileTime.dwLowDateTime;
		return true;
	}

	bool File::time(uint64_t& CreateTime, uint64_t& AccessTime, uint64_t& WriteTime)
	{
		FILETIME CreateFileTime, AccessFileTime, WriteFileTime;
		if (false==GetFileTime(m_pFileHandle, &CreateFileTime, &AccessFileTime, &WriteFileTime))
		{
			LOG_ERROR << "Fail to get file time";
			return false;
		}
		CreateTime = CreateFileTime.dwHighDateTime;
		CreateTime = (CreateTime << 32) | CreateFileTime.dwLowDateTime;
		AccessTime = AccessFileTime.dwHighDateTime;
		AccessTime = (AccessTime << 32) | AccessFileTime.dwLowDateTime;
		WriteTime = WriteFileTime.dwHighDateTime;
		WriteTime = (WriteTime << 32) | WriteFileTime.dwLowDateTime;
		return true;
	}

	void File::close()
	{
		//关闭句柄;
		if (nullptr != m_pMapFile)
		{
			UnmapViewOfFile(m_pMapFile);
		}
		if (nullptr != m_pMapFileHandle)
		{
			CloseHandle(m_pMapFileHandle);
		}
		if (nullptr != m_pFileHandle)
		{
			CloseHandle(m_pFileHandle);
		}
	}

	bool File::sha1(base::SHA1& SHA1Struct)
	{
		return sha1(SHA1Struct, 0, m_FileSize);
	}

	bool File::sha1(base::SHA1& SHA1Struct, uint64_t Start, uint64_t Len)
	{
		if (Start >= m_FileSize)
		{
			return false;
		}
		Len = (Start + Len >= m_FileSize) ? (m_FileSize - Start) : Len;
	
		SHA1_CTX SHA1Context;
		SHA1DCInit(&SHA1Context);
		const char* pCurPos = m_pMapFile + Start;
		if (nullptr == pCurPos)
		{
			LOG_ERROR << "fsafs";
		}
		while (Len >= PAGE_SIZE)
		{
			SHA1DCUpdate(&SHA1Context, pCurPos, PAGE_SIZE);
			Len -= PAGE_SIZE;
			pCurPos += PAGE_SIZE;
		}
		if (0 != Len)
		{
			SHA1DCUpdate(&SHA1Context, pCurPos, Len);
		}
		return SHA1DCFinal(SHA1Struct.Hash, &SHA1Context);
	}


	bool File::md5(base::MD5& MD5Obj)
	{
		return md5(MD5Obj,0, m_FileSize);
	}

	bool File::md5(base::MD5& MD5Obj,uint64_t Start, uint64_t Len)
	{
		if (Start >= m_FileSize)
		{
			return false;
		}
		Len = (Start + Len >= m_FileSize) ? (m_FileSize - Start) : Len;
		MD5_CTX MD5Context;
		MD5Init(&MD5Context);
		uint8_t* pCurPos = (uint8_t*)m_pMapFile + Start;
		while (Len >= PAGE_SIZE)
		{
			MD5Update(&MD5Context, pCurPos, PAGE_SIZE);
			Len -= PAGE_SIZE;
			pCurPos += PAGE_SIZE;
		}
		if (0 != Len)
		{
			MD5Update(&MD5Context, pCurPos, Len);
		}

		uint8_t Digest[16];
		MD5Final(Digest, &MD5Context);
		MD5Obj.PartA = 0;
		MD5Obj.PartB = 0;
		for (uint8_t i = 0; i < 8; ++i)
		{
			uint64_t cur = Digest[i];
			cur <<= (i << 3);
			MD5Obj.PartA |= cur;
		}
		for (uint8_t i = 0; i < 8; ++i)
		{
			uint64_t cur = Digest[i+8];
			cur <<= (i << 3);
			MD5Obj.PartB |= cur;
		}
		return true;
	}

	LPCWSTR to_wstring(const std::string& strText)
	{
		//要打开的文件路径可能含中文，设置全局locale为本地环境
		std::locale Loc = std::locale::global(std::locale(""));
		size_t Len = strText.length() + 1;
		const size_t NewSize = 100;
		size_t ConvertedChars = 0;
		wchar_t* wstrText = (wchar_t*)malloc(sizeof(wchar_t) * (strText.length() - 1));
		mbstowcs_s(&ConvertedChars, wstrText, Len, strText.c_str(), _TRUNCATE);
		std::locale::global(Loc);//恢复全局locale
		return wstrText;
	}

	bool create_directory(const std::string& strFolderPath)
	{
		size_t Len = strFolderPath.size();
		char DirPath[256] = { 0 };
		for (size_t i = 0; i < Len; ++i)
		{
			DirPath[i] = strFolderPath[i];
			if (DirPath[i] == '\\' || DirPath[i] == '/')
			{
				if (-1 == _access(DirPath, 0))
				{
					if (-1 == _mkdir(DirPath))
					{
						return false;
					}
				}
			}
		}
		return true;
	}

	bool create_file(const std::string& strFilePath, uint64_t Len)
	{
		LPCWSTR wstrFilePath = to_wstring(strFilePath);
		HANDLE pFileHandle = CreateFile(wstrFilePath,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == pFileHandle)
		{
			return false;
		}
		uint32_t High, Low;
		High = (Len >> 32);
		Low = (Len & 0x00000000ffffffff);
		// 创建的文件总大小 = High * 4G + Low
		// 当总大小小于4G时，High可以设置为0，Low数字最大为4G - 1
		HANDLE pFileMapHandle = CreateFileMapping(pFileHandle, NULL, PAGE_READWRITE, High, Low, NULL);
		if (NULL == pFileMapHandle)
		{
			return false;
		}
		CloseHandle(pFileMapHandle);
		CloseHandle(pFileHandle);
		return true;
	}
	
	bool delete_file(const std::string& strFilePath)
	{
		return DeleteFile(to_wstring(strFilePath));
	}

	bool folder_exist(const std::string& strFolderPath)
	{
		if (-1==_access(strFolderPath.c_str(), 0))	//如果文件夹不存在
		{
			return false;
		}
		return true;
	}

	bool file_exist(const std::string& strFilePath)
	{
		struct stat Buffer;
		return (stat(strFilePath.c_str(), &Buffer) == 0);
	}

}