/***********************************************************************/
/* 名称:下载中的文件控制器											   */
/* 说明:下载中文件的完整性控制										   */
/* 创建时间:2021/10/24												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <queue>
#include <atomic>
#include <unordered_set>
#include <base/config.hpp>
#include "file_ctrl_interface.h"

namespace file
{
	//不完整的文件，下载的同时提供已下载且校验通用的Fragment
	//当前模块作为上传请求使用，只有单线程处理不加锁
	class FileCtrlIncmpl :public FileCtrlInterface
	{
		typedef std::unordered_set<uint64_t> SegmentSet;
		typedef std::queue<uint64_t> SegmentQueue;
	public:
		FileCtrlIncmpl();
		~FileCtrlIncmpl() override;
	public://初始化
		//初始化不完整的文件或者空的文件，如果文件不存在则创建
		bool init(const std::string& strFilePath,uint64_t Len) override;
		void close() override;
		//获取一个没有完成的Fragment任务
		bool get_task(uint64_t& FragmentStart);
		//检查当前文件是否所有任务都已完成，返回false表示确实全部完成
		bool check_task(uint64_t& FragmentStart);
		const char* read_fragment(uint64_t FragmentStart, uint64_t& Len) override;
		void add_priority_task(uint64_t SegmentSeq);
	public://MD5验证相关
		//校验2M的Segment完整性
		void check_segment_md5(uint64_t SegmentStart, MD5& MD5Obj) override;
		//计算文件部分内容的MD5
		bool md5_segment(MD5& MD5Obj, uint64_t SegmentStart) override;
	public://接受消息之后下载任务完成
		void complete_fragment	(const char* pBuf, uint64_t FragmentStart, uint16_t Len) override;
		void complete_segment	(uint64_t SegmentStart) override;
	public:
		//获取传输速率
		uint64_t rate() override;
		uint64_t size() override;
	private://初始化相关
		bool open_file		(uint64_t Len);		//尝试打开下载文件
		bool init_file		(uint64_t Len);		//尝试创建下载文件
		bool open_ctrl_file	(uint64_t Len);		//尝试打开控制文件
		bool init_ctrl_file	(uint64_t Len);		//尝试创建控制文件
		void init_fragment_num();
	private:
		uint64_t _clock();
	private://控制文件（记录下载文件的进度）相关
		uint64_t						m_FragmentNum;			//Fragment	= 32KB
		uint64_t						m_SegmentNum;			//Segment	= 2MB
		File*							m_pCtrlFile;
	private://文件下载任务相关	
		SegmentSet						m_PrioritySegmentSet;	//存放优先下载的Segment
		SegmentQueue					m_PrioritySegmentQueue;	//用于优先分配某些Segment
		uint64_t						m_PriorityFragment;		//优先下载的Fragment
		uint64_t						m_CurFragment;			//当前应该下载的Fragment
	private://传输完成进度相关
		std::atomic<uint64_t>			m_CurFragmentNum;
		uint64_t						m_PreFragmentNum;
		uint64_t						m_PreClock;
	};
}