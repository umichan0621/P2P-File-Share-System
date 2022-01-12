/***********************************************************************/
/* 名称:文件控制器接口												   */
/* 说明:文件完整性控制										           */
/* 创建时间:2021/10/24												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <module_file/file/file.h>

namespace file
{
	class FileCtrlInterface
	{
	public:
		FileCtrlInterface();
		virtual ~FileCtrlInterface();
	public://初始化
		virtual bool init(const std::string& strFilePath, uint64_t Len) = 0;//初始化文件信息
		virtual void close() = 0;//关闭文件控制
		virtual const char* read_fragment(uint64_t FragmentStart, uint64_t& Len) = 0;
	public://MD5验证相关
		virtual void check_segment_md5(uint64_t SegmentStart, base::MD5& MD5Obj) = 0;//校验2M的Segment完整性
		virtual bool md5_segment(base::MD5& MD5Obj,uint64_t SegmentStart) = 0;//计算文件部分内容的MD5
	public://接受消息之后下载任务完成
		virtual void complete_fragment(const char* pBuf, uint64_t FragmentStart, uint16_t Len) = 0;//某个分片下载完毕
		virtual void complete_segment(uint64_t SegmentStart) = 0;//某个段下载完毕且通过校验
	public://文件信息
		void set_file_seq(int32_t FileSeq);
		void set_sha1(const base::SHA1& SHA1Struct);
		uint32_t file_seq();
		uint64_t full_size();
		void full_sha1(base::SHA1& SHA1Struct);		//获取当前文件设定的正确MD5值
		bool sha1(base::SHA1& SHA1Struct);			//计算文件当前的SHA1
		bool time(uint64_t& WriteTime);
		const std::string& file_name();
	public://传输进度相关
		virtual uint64_t rate()=0;//传输速率
		virtual uint64_t size()=0;//完成的大小
	protected://文件相关
		std::string						m_strFolderPath;		//文件所在文件夹路径 
		std::string						m_strFileName;			//文件名
		File*							m_pFile;
		base::SHA1						m_ActualSHA1;
	private:
		uint32_t						m_FileSeq;
	};
}