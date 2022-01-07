#include "encoder.h"
#include <windows.h>
#include <wchar.h>

namespace base
{
	static uint8_t AlphabetMap[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	static uint8_t ReverseMap[] =
	{
		 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62, 255, 255, 255, 63,
			52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 255, 255, 255, 255, 255, 255,
			255, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
			15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 255, 255, 255, 255, 255,
			255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
			 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 255, 255, 255, 255, 255
	};

	//转换成base64编码
	int32_t base64_encode(const uint8_t* pSrc, int32_t SrcLen, uint8_t* pDes)
	{
		int32_t i, j;
		for (i = 0, j = 0; i + 3 <= SrcLen; i += 3)
		{
			//取出第一个字符的前6位并找出对应的结果字符
			pDes[j++] = AlphabetMap[pSrc[i] >> 2];
			//将第一个字符的后2位与第二个字符的前4位进行组合并找到对应的结果字符
			pDes[j++] = AlphabetMap[((pSrc[i] << 4) & 0x30) | (pSrc[i + 1] >> 4)];
			//将第二个字符的后4位与第三个字符的前2位组合并找出对应的结果字符
			pDes[j++] = AlphabetMap[((pSrc[i + 1] << 2) & 0x3c) | (pSrc[i + 2] >> 6)];
			//取出第三个字符的后6位并找出结果字符
			pDes[j++] = AlphabetMap[pSrc[i + 2] & 0x3f];
		}

		if (i < SrcLen)
		{
			int32_t Tail = SrcLen - i;
			if (Tail == 1)
			{
				pDes[j++] = AlphabetMap[pSrc[i] >> 2];
				pDes[j++] = AlphabetMap[(pSrc[i] << 4) & 0x30];
				pDes[j++] = '=';
				pDes[j++] = '=';
			}
			else //tail==2
			{
				pDes[j++] = AlphabetMap[pSrc[i] >> 2];
				pDes[j++] = AlphabetMap[((pSrc[i] << 4) & 0x30) | (pSrc[i + 1] >> 4)];
				pDes[j++] = AlphabetMap[(pSrc[i + 1] << 2) & 0x3c];
				pDes[j++] = '=';
			}
		}
		return j;
	}

	int32_t base64_encode(const char* pSrc, int32_t SrcLen, char* pDes)
	{
		return base64_encode((const uint8_t*)pSrc, SrcLen, (uint8_t*)pDes);
	}

	int32_t base64_encode(const std::string& strSrc, char* pDes)
	{
		return base64_encode(strSrc.c_str(), strSrc.size(), pDes);
	}
	//转换成base64编码

	//解析base64编码
	int32_t base64_decode(const uint8_t* pSrc, int32_t SrcLen, uint8_t* pDes)
	{
		//如果它的条件返回错误，则终止程序执行。4的倍数。
		if ((SrcLen & 0x03) != 0)
		{
			return -1;
		}

		int32_t i, j = 0;
		int8_t Temp[4];
		for (i = 0; i < SrcLen; i += 4)
		{
			for (int32_t k = 0; k < 4; ++k)
			{
				//分组，每组四个分别依次转换为base64表内的十进制数
				Temp[k] = ReverseMap[pSrc[i + k]];
			}
			if (Temp[0] >= 64 || Temp[1] >= 64)
			{
				return -1;
			}
			//assert(Temp[0] < 64 && Temp[1] < 64);
			//取出第一个字符对应base64表的十进制数的前6位与第二个字符对应base64表的十进制数的前2位进行组合
			pDes[j++] = (Temp[0] << 2) | (Temp[1] >> 4);

			if (Temp[2] >= 64)
				break;
			else if (Temp[3] >= 64)
			{
				//取出第二个字符对应base64表的十进制数的后4位与第三个字符对应base64表的十进制数的前4位进行组合
				pDes[j++] = (Temp[1] << 4) | (Temp[2] >> 2);
				break;
			}
			else
			{
				pDes[j++] = (Temp[1] << 4) | (Temp[2] >> 2);
				//取出第三个字符对应base64表的十进制数的后2位与第4个字符进行组合
				pDes[j++] = (Temp[2] << 6) | Temp[3];
			}
		}
		return j;
	}

	int32_t base64_decode(const char* pSrc, int32_t SrcLen, char* pDes)
	{
		return base64_decode((const uint8_t*)pSrc, SrcLen, (uint8_t*)pDes);
	}

	int32_t base64_decode(const std::string& strSrc, char* pDes)
	{
		return base64_decode(strSrc.c_str(), strSrc.size(), pDes);
	}
	//解析base64编码

	//utf8编码转换
	std::string string_to_utf8(const std::string& strSrc)
	{
		int32_t Len1 = ::MultiByteToWideChar(CP_ACP, 0, strSrc.c_str(), -1, NULL, 0);

		wchar_t* pwBuf = new wchar_t[Len1 + 1];//一定要加1，不然会出现尾巴
		ZeroMemory(pwBuf, Len1 * 2 + 2);

		::MultiByteToWideChar(CP_ACP, 0, strSrc.c_str(), strSrc.length(), pwBuf, Len1);

		int Len2 = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);

		char* pBuf = new char[Len2 + 1];
		ZeroMemory(pBuf, Len2 + 1);

		::WideCharToMultiByte(CP_UTF8, 0, pwBuf, Len1, pBuf, Len2, NULL, NULL);

		std::string strDes(pBuf);

		delete[]pwBuf;
		delete[]pBuf;

		pwBuf = NULL;
		pBuf = NULL;

		return strDes;
	}

	std::string utf8_to_string(const std::string& strSrc)
	{
		int32_t Len1 = MultiByteToWideChar(CP_UTF8, 0, strSrc.c_str(), -1, NULL, 0);

		wchar_t* pwBuf = new wchar_t[Len1 + 1];//一定要加1，不然会出现尾巴
		memset(pwBuf, 0, Len1 * 2 + 2);

		MultiByteToWideChar(CP_UTF8, 0, strSrc.c_str(), strSrc.length(), pwBuf, Len1);

		int32_t Len2 = WideCharToMultiByte(CP_ACP, 0, pwBuf, -1, NULL, NULL, NULL, NULL);

		char* pBuf = new char[Len2 + 1];
		memset(pBuf, 0, Len2 + 1);

		WideCharToMultiByte(CP_ACP, 0, pwBuf, Len1, pBuf, Len2, NULL, NULL);

		std::string strDes = pBuf;

		delete[]pBuf;
		delete[]pwBuf;

		pBuf = NULL;
		pwBuf = NULL;

		return strDes;
	}
	//utf8编码转换
}
