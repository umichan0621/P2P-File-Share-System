/***********************************************************************/
/* 名称:文件协议头													   */
/* 说明:通信时UDP包的文件协议头										   */
/* 创建时间:2021/02/22												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include "protocol_base.h"
constexpr uint16_t  BASE_FILE_HEADER_LEN = 10;

//文件传输协议
enum PROTOCOL_FILE
{
	PROTOCOL_FILE_NULL = PROTOCOL_TYPE::PROTOCOL_TYPE_FILE << 10,
	//PROTOCOL_FILE_INFO_REQ,						//向其他节点请求指定MD5文件的信息
	//PROTOCOL_FILE_INFO_ACK,						//回复
	//PROTOCOL_FILE_INFO_RFS,						//拒绝回复
	//PROTOCOL_FILE_SET_REQ,						//向其他节点请求下载指定文件
	//PROTOCOL_FILE_SET_ACK,						//同意向其他节点上传指定文件
	//PROTOCOL_FILE_SET_RFS,						//拒绝向其他节点上传指定文件
	//PROTOCOL_FILE_REQ,							//请求下载整个文件，文件大小应小于2MB
	//PROTOCOL_FILE_ACK,							//回复上传整个文件，文件按512B一块块上传
	//PROTOCOL_FILE_LAST_ACK,						//表示当前文件传输结束
	PROTOCOL_FILE_FRAGMENT_REQ,					//请求指定文件的指定Fragment
	PROTOCOL_FILE_FRAGMENT_ACK,
	//PROTOCOL_FILE_FRAGMENT_LAST_ACK,			//表示当前Fragment已全部发送完毕
	PROTOCOL_FILE_MD5_CHECK_REQ,				//请求指定文件指定Segment的MD5值
	PROTOCOL_FILE_MD5_CHECK_ACK,
	PROTOCOL_FILE_END
};

inline void create_file_size(char* pDes, uint64_t FileSize)
{
	for (uint8_t i = 0; i < 8; ++i)
	{
		uint32_t Move = (7 - i) << 3;
		pDes[18 + i] = (char)(FileSize >> Move);
	}
}

inline void parse_file_size(char* pSrc, uint64_t& FileSize)
{
	FileSize = 0;
	for (uint8_t i = 0; i < 8; ++i)
	{
		uint32_t Move = (7 - i) << 3;
		uint64_t Tp = (uint8_t)pSrc[18 + i];
		FileSize |= (Tp << Move);
	}
}

//文件Fragment请求的Fragment开始位置
inline void create_fragment_start(char* pDes, uint64_t FragmentStart)
{
	for (uint8_t i = 0; i < 7; ++i)
	{
		uint32_t Move = (7 - i) << 3;
		pDes[2 + i] = (char)(FragmentStart >> Move);
	}
}
//解析文件Fragment请求的Fragment开始位置
inline void parse_fragment_start(const char* pSrc, uint64_t& FragmentStart)
{
	FragmentStart = 0;
	for (uint8_t i = 0; i < 7; ++i)
	{
		uint32_t Move = (7 - i) << 3;
		uint64_t Tp = (uint8_t)pSrc[2 + i];
		FragmentStart |= (Tp << Move);
	}
	FragmentStart = (FragmentStart >> 15) << 15;
}
//文件Block应答的Block开始位置
inline void create_block_start(char* pDes, uint64_t BlockStart)
{
	for (uint8_t i = 0; i < 7; ++i)
	{
		uint32_t Move = (7 - i) << 3;
		pDes[2 + i] = (char)(BlockStart >> Move);
	}
}
//解析文件Block应答的Block开始位置
inline void parse_block_start(const char* pSrc, uint64_t& BlockStart)
{
	BlockStart = 0;
	for (uint8_t i = 0; i < 7; ++i)
	{
		uint32_t Move = (7 - i) << 3;
		uint64_t Tp = (uint8_t)pSrc[2 + i];
		BlockStart |= (Tp << Move);
	}
	BlockStart = (BlockStart >> 9) << 9;
}