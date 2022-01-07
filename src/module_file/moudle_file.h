/***********************************************************************/
/* 名称:File模块													   */
/* 说明:提供File模块最顶层API										   */
/* 创建时间:2021/11/02												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <stdint.h>
#include <string>
#include <iostream>
#include <unordered_map>
#include <moudle_file/file/file_md5.h>

class MoudleFile
{
public:
	//请求特定MD5值文件的信息
	static void request_file_info(uint16_t SessionId, const File::MD5& MD5Obj);
	//请求与其他节点建立当前文件的连接
	static void request_set_file(uint16_t SessionId, const File::MD5& MD5Obj);
	//向当前已连接的Peer请求某个文件的某个Fragment
	static void request_fragment(uint16_t SessionId,uint64_t FragmentStart);
	static void request_md5_check(uint16_t SessionId,uint64_t SegmentStart);
private:
	//static FileMap			m_FileMap;	//MD5到文件控制对象的映射
};
