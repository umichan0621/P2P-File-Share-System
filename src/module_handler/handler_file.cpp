#include "handler_file.h"
#include <base/config.hpp>
#include <base/logger/logger.h>
#include <base/protocol/protocol_file.h>
#include <module_net/session_manager.h>
#include <module_file/file_manager.h>
#pragma warning(disable:6297)
#pragma warning(disable:4267)

namespace handler
{
#define FILE_REGISTER(_FUNC) std::bind(&HandlerFile::_FUNC,this, \
std::placeholders::_1, std::placeholders::_2, std::placeholders::_3) 

	HandlerFile::HandlerFile(){}

	void HandlerFile::register_recv_event()
	{
		//注册Recv事件的处理方式
		m_RecvHandlerMap[PROTOCOL_FILE_FRAGMENT_REQ] =		FILE_REGISTER(handle_file_fragment_req);
		m_RecvHandlerMap[PROTOCOL_FILE_FRAGMENT_ACK] =		FILE_REGISTER(handle_file_fragment_ack);
		m_RecvHandlerMap[PROTOCOL_FILE_MD5_CHECK_REQ] =		FILE_REGISTER(handle_file_md5_check_req);
		m_RecvHandlerMap[PROTOCOL_FILE_MD5_CHECK_ACK] =		FILE_REGISTER(handle_file_md5_check_ack);
	}

	void HandlerFile::register_gateway_event()
	{

	}

	int8_t HandlerFile::handle_event(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		//解析基础协议头
		uint16_t ProtocolId;
		parse_header(pMessage, ProtocolId);
		//如果协议注册过
		if (m_RecvHandlerMap.count(ProtocolId) != 0)
		{
			return m_RecvHandlerMap[ProtocolId](SessionId, pMessage, Len);
		}
		return false;
	}

	int8_t HandlerFile::handle_file_fragment_req(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		net::Session* pCurSession =g_pSessionManager->session(SessionId);
		if (nullptr == pCurSession)
		{
			return DO_NOTHING;
		}
		//处理其他节点发送的文件Fragment
		//LOG_ERROR << "FILE REQ";
		base::SHA1 SHA1Struct;
		memcpy(&SHA1Struct, &pMessage[2], 20);
		std::string strSHA1;
		base::sha1_value(SHA1Struct, strSHA1);
		uint64_t FragmentStart;
		memcpy(&FragmentStart, &pMessage[22], 8);
		//LOG_ERROR << strSHA1;
		//LOG_ERROR << FragmentStart;
		file::FileCtrl FileCtrl;
		//没有保存当前SHA1的文件
		bool bRes=g_pFileManager->file_ctrl(SHA1Struct, FileCtrl);
		if (false == bRes)
		{
			return DO_NOTHING;
		}
		uint64_t FragmentLen = FULL_FRAGMENT_SIZE;
		const char* pBuf=FileCtrl->read_fragment(FragmentStart, FragmentLen);
		//无法读取当前请求的Fragment
		if (nullptr == pBuf)
		{
			return DO_NOTHING;
		}
		//LOG_ERROR << "READ " << FragmentLen;
		create_header(pMessage, PROTOCOL_FILE_FRAGMENT_ACK);

		std::vector<const char*> VecMessage = { pMessage ,pBuf };
		std::vector<uint16_t> VecMessageLen = { 30,(uint16_t)FragmentLen };

		pCurSession->send_reliable(VecMessage, VecMessageLen);
		////处理Fragment请求

		return DO_NOTHING;
	}
	
	int8_t HandlerFile::handle_file_fragment_ack(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		//LOG_ERROR << "ACK Len = "<< Len;
		//获取当前Fragment数据的信息
		base::SHA1 SHA1Struct;
		memcpy(&SHA1Struct, &pMessage[2], 20);
		std::string strSHA1;
		base::sha1_value(SHA1Struct, strSHA1);
		uint64_t FragmentStart;
		memcpy(&FragmentStart, &pMessage[22], 8);
		file::FileCtrl FileCtrl;
		bool bRes = g_pFileManager->file_ctrl(SHA1Struct, FileCtrl);
		if (false == bRes)
		{
			return DO_NOTHING;
		}
		const char* pFragment = pMessage + 30;
		uint16_t FragmentLen = Len - 30;
		//LOG_ERROR << Len - 30;
		FileCtrl->complete_fragment(pFragment, FragmentStart, Len - 30);
		return DO_NOTHING;
	}

	int8_t HandlerFile::handle_file_md5_check_req(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		//uint64_t SegmentStart;//读取Segment开始位置
		//parse_fragment_start(pMessage, SegmentStart);
		//SegmentStart = (SegmentStart >> 21) << 21;
		////获取当前Fragment所在文件
		//peer::PeerInfo* pCurPeer = g_pPeerManager->peer_info(SessionId);
		//if (nullptr == pCurPeer)
		//{
		//	return DO_NOTHING;
		//}
		//file::FileCtrlInterface* pFileCtrl;// = pCurPeer->get_file();
		//if (nullptr == pFileCtrl)
		//{
		//	return DO_NOTHING;
		//}
		//file::MD5 MD5Obj;
		//if (false == pFileCtrl->md5_segment(MD5Obj, SegmentStart))
		//{
		//	return DO_NOTHING;
		//}
		//create_header(pMessage, PROTOCOL_FILE_MD5_CHECK_ACK);
		//memcpy(&pMessage[BASE_FILE_HEADER_LEN], &MD5Obj, 16);
		//Len += 16;
		return DO_REPLY;
	}

	int8_t HandlerFile::handle_file_md5_check_ack(uint16_t& SessionId, char* pMessage, uint16_t& Len)
	{
		//uint64_t SegmentStart;//读取Segment开始位置
		//parse_fragment_start(pMessage, SegmentStart);
		////当前位置未完成
		//file::MD5 MD5Obj;
		//memcpy(&MD5Obj, &pMessage[BASE_FILE_HEADER_LEN], 16);
		return false;
	}
}