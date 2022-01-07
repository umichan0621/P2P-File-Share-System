#include "file_md5.h"
#include <base/logger/logger.h>
#pragma warning(disable:6297)
#pragma warning(disable:4244)

namespace file
{
	static const char* HEX_TABLE = "0123456789abcdef";

	void md5_value(const MD5& MD5Struct, std::string& strMD5)
	{
		strMD5 = "";
		uint64_t TempA = MD5Struct.PartA, TempB = MD5Struct.PartB;
		for (int8_t i = 0; i < 8; ++i)
		{
			uint8_t cur = TempA & 0xff;
			TempA >>= 8;
			strMD5 += HEX_TABLE[cur >> 4];
			strMD5 += HEX_TABLE[cur & 0xf];
		}
		for (int8_t i = 0; i < 8; ++i)
		{
			uint8_t cur = TempB & 0xff;
			TempB >>= 8;
			strMD5 += HEX_TABLE[cur >> 4];
			strMD5 += HEX_TABLE[cur & 0xf];
		}
	}

	static uint8_t cal_hex(uint8_t c)
	{
		if ('0' <= c && '9' >= c)
		{
			return c - '0';
		}
		else if ('a' <= c && 'f' >= c)
		{
			return c - 'a' + 10;
		}
		else
		{
			return 16;
		}
	}

	bool md5_parse(const std::string& strMD5, MD5& MD5Struct)
	{
		if (strMD5.size() != 32)
		{
			return false;
		}
		MD5Struct.PartA = 0;
		MD5Struct.PartB = 0;
		for (uint8_t i = 0; i < 8; ++i)
		{
			uint8_t Pos = i << 1;
			uint64_t Cur0 = cal_hex(strMD5[Pos]);
			if (16 == Cur0)
			{
				return false;
			}
			++Pos;
			uint64_t Cur1 = cal_hex(strMD5[Pos]);
			if (16 == Cur1)
			{
				return false;
			}
			Cur0 = (Cur0 << 4) | Cur1;
			MD5Struct.PartA |= Cur0 <<(i<<3);
		}

		for (uint8_t i = 0; i < 8; ++i)
		{
			uint8_t Pos = (i + 8) << 1;
			uint64_t Cur0 = cal_hex(strMD5[Pos]);
			if (16 == Cur0)
			{
				return false;
			}
			++Pos;
			uint64_t Cur1 = cal_hex(strMD5[Pos]);

			if (16 == Cur1)
			{
				return false;
			}
			Cur0 = (Cur0 << 4) | Cur1;
			MD5Struct.PartB |= Cur0 << (i << 3);
		}
		return true;
	}

}