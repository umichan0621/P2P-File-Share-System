/***********************************************************************/
/* 名称:完整文件的控制器											   */
/* 说明:完整文件的控制										           */
/* 创建时间:2021/10/24												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include "file_ctrl_interface.h"

namespace file
{
	//完整的文件，只提供上传
	class FileCtrlCmpl :public FileCtrlInterface
	{
	public:
		FileCtrlCmpl();
		~FileCtrlCmpl() override;
	public://初始化
		bool open(std::string strFilePath);//文件作为分享文件第一次打开的接口
		bool init(const std::string& strFilePath,uint64_t Len) override;
		void close() override;
		const char* read_fragment(uint64_t FragmentStart, uint64_t& Len) override;
		bool md5_segment(MD5& MD5Obj, uint64_t Start) override;
		void check_segment_md5(uint64_t SegmentStart, MD5& MD5Obj)override;

		void complete_fragment(const char* pBuf, uint64_t FragmentStart, uint16_t Len)override;
		void complete_segment(uint64_t SegmentStart)override;

		uint64_t rate() override;
		uint64_t size() override;
	};
}