#include "moudle_file.h"
#include <moudle_peer/peer_manager.h>
#include <base/protocol/protocol_file.h>

void MoudleFile::request_file_info(uint16_t SessionId, const File::MD5& MD5Obj)
{
	Peer::Session* pCurSession = g_pPeerManager->session(SessionId);
	if (nullptr != pCurSession)
	{
		char pMessage[BASE_HEADER_LEN + 16];
		create_header(pMessage, PROTOCOL_FILE_INFO_REQ);
		memcpy(&pMessage[2], &MD5Obj, 16);
		pCurSession->send_reliable(pMessage, BASE_HEADER_LEN + 16);
	}
}

void MoudleFile::request_set_file(uint16_t SessionId, const File::MD5& MD5Obj)
{
	Peer::Session* pCurSession = g_pPeerManager->session(SessionId);
	if (nullptr != pCurSession)
	{
		char pMessage[BASE_HEADER_LEN + 16];
		create_header(pMessage, PROTOCOL_FILE_SET_REQ);
		memcpy(&pMessage[2], &MD5Obj, 16);
		pCurSession->send_reliable(pMessage, BASE_HEADER_LEN + 16);
	}
}


void MoudleFile::request_fragment(uint16_t SessionId,uint64_t FragmentStart)
{
	Peer::Session* pCurSession = g_pPeerManager->session(SessionId);
	if (nullptr != pCurSession)
	{
		char pMessage[BASE_FILE_HEADER_LEN];
		create_header(pMessage, PROTOCOL_FILE_FRAGMENT_REQ);
		create_fragment_start(pMessage, FragmentStart);
		pCurSession->send_reliable(pMessage, BASE_FILE_HEADER_LEN);
	}
}

void MoudleFile::request_md5_check(uint16_t SessionId, uint64_t SegmentStart)
{
	Peer::Session* pCurSession = g_pPeerManager->session(SessionId);
	if (nullptr != pCurSession)
	{
		char pMessage[BASE_FILE_HEADER_LEN];
		create_header(pMessage, PROTOCOL_FILE_MD5_CHECK_REQ);
		create_fragment_start(pMessage, SegmentStart);
		pCurSession->send_reliable(pMessage, BASE_FILE_HEADER_LEN);
	}
}