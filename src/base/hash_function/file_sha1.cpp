#include "file_sha1.h"
#pragma warning(disable:6297)
#pragma warning(disable:4244)

namespace base
{
	static const char* HEX_TABLE = "0123456789abcdef";
	
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

	void sha1_value(const SHA1& SHA1Struct, std::string& strSHA1)
	{
		strSHA1 = "";
		for (int32_t i = 0; i < 20; ++i)
		{
			uint8_t Cur = SHA1Struct.Hash[i];

			strSHA1 += HEX_TABLE[Cur >> 4];
			strSHA1 += HEX_TABLE[Cur & 0xf];
		}
	}

	bool sha1_parse(const std::string& strSHA1, SHA1& SHA1Struct)
	{
		if (strSHA1.size() != 40)
		{
			return false;
		}
		for (int32_t i = 0; i < 20; ++i)
		{
			uint32_t Temp = cal_hex(strSHA1[2*i]);
			if (16 == Temp)
			{
				return false;
			}
			SHA1Struct.Hash[i] = Temp << 4;
			Temp= cal_hex(strSHA1[2*i+1]);
			if (16 == Temp)
			{
				return false;
			}
			SHA1Struct.Hash[i] |= Temp;
			
		}
		return true;
	}

	bool sha1_equal(const SHA1& SHA1Struct1, const SHA1& SHA1Struct2)
	{
		for (int i = 0; i < 20; ++i)
		{
			if (SHA1Struct1.Hash[i] != SHA1Struct2.Hash[i])
			{
				return false;
			}
		}
		return true;
	}

}