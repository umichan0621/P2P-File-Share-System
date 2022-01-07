/***********************************************************************/
/* 名称:文件类														   */
/* 说明:文件创建和读写										           */
/* 创建时间:2021/10/21												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string>
#include <mutex>
#include "file_md5.h"
#include "file_sha1.h"

namespace file
{
	bool create_directory(const std::string& strFolderPath);		//按指定路径创建文件夹
	bool create_file(const std::string& strFilePath, uint64_t Len);	//按指定路径和大小创建文件，创建成功返回true
	bool delete_file(const std::string& strFilePath);				//删除文件
	bool file_exist(const std::string& strFilePath);				//判断文件是否存在
	bool folder_exist(const std::string& strFolderPath);			//判断文件夹是否存在
	LPCWSTR to_wstring(const std::string& strText);					//字符串转换为宽字符串

	class File
	{
	public:
		File();
		~File();
	public://打开和关闭
		//打开指定路径文件，false表示文件只读
		bool open(const char* FilePath, bool bIsWriteable);
		bool open(std::string strFilePath, bool bIsWriteable);
		void close();
	public://读写
		/*从文件的Start位置读取长度为Len的内容
		如果Start超过文件长度返回nullptr
		如果Start+Len超过文件长度修改Len为可读最大长度*/
		const char* read(uint64_t Start, uint64_t& Len);
		bool read_binary(uint64_t Start);
		//将缓存写入文件，成功返回true，失败返回false
		bool write(const char* pBuf, uint64_t Start, uint64_t Len);
		bool write_binary(bool bValue, uint64_t Start);
	public://其他
		//读取文件大小
		uint64_t size();
		//读取文件的md5值
		bool sha1(SHA1& SHA1Struct);
		bool sha1(SHA1& SHA1Struct, uint64_t Start, uint64_t Len);
		bool md5(MD5& MD5Obj);
		bool md5(MD5& MD5Obj,uint64_t Start, uint64_t Len);
		bool time(uint64_t& WriteTime);
		bool time(uint64_t& CreateTime, uint64_t& AccessTime, uint64_t& WriteTime);
	private:
		uint64_t			m_FileSize;			//文件大小
		HANDLE				m_pFileHandle;		//文件句柄
		HANDLE				m_pMapFileHandle;	//文件映射内核对象
		char*				m_pMapFile;			//文件数据映射到进程的地址空间
		bool				m_bIsWriteable;		//文件是否可写
		std::mutex			FileMutex;
	};
}