/***********************************************************************/
/* 名称:base64														   */
/* 说明:字符串和base64编码之间的转换						           */
/* 创建时间:2021/12/22												   */
/* Email:umichan0621@gmail.com									       */
/* Reference:https://www.cnblogs.com/Severus-Cavendish/p/11623240.html */
/***********************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <assert.h>

namespace base
{
	//转换成base64编码
	int32_t base64_encode(const uint8_t* pSrc, int32_t SrcLen, uint8_t* pDes);
	int32_t base64_encode(const char* pSrc, int32_t SrcLen, char* pDes);
	int32_t base64_encode(const std::string& strSrc, char* pDes);
	//解析base64编码
	int32_t base64_decode(const uint8_t* pSrc, int32_t SrcLen, uint8_t* pDes);
	int32_t base64_decode(const char* pSrc, int32_t SrcLen, char* pDes);
	int32_t base64_decode(const std::string& strSrc, char* pDes);
	//utf8编码转换
	std::string string_to_utf8(const std::string& strSrc);
	std::string utf8_to_string(const std::string& strSrc);
}