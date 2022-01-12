/***********************************************************************/
/* 名称:SHA1													       */
/* 说明:自定义SHA1数据结构以及SHA1相关操作							   */
/* 创建时间:2021/11/24											       */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <stdint.h>
#include <string>
#include <cstring>
#include <third/sha1/sha1.h>

namespace base
{
	struct SHA1
	{
		uint8_t Hash[20];

		bool operator==(const SHA1& Other)
		{
			for (int32_t i = 0; i < 20; ++i)
			{
				if (Hash[i] != Other.Hash[i])
				{
					return false;
				}
			}
			return true;
		}
	};

	void sha1_value(const SHA1& SHA1Struct, std::string& strSHA1);
	bool sha1_parse(const std::string& strSHA1, SHA1& SHA1Struct);
	bool sha1_equal(const SHA1& SHA1Struct1, const SHA1& SHA1Struct2);
	//自定义MD5数据结构的哈希函数
	struct SHA1HashFunc
	{
		size_t operator()(const SHA1& SHA1Struct) const
		{
			uint32_t Temp;
			memcpy(&Temp, &SHA1Struct, 4);
			return Temp;
		}
	};
	//自定义MD5数据结构的相等条件
	struct SHA1EqualFunc
	{
		bool operator()(const SHA1& SHA1Struct1, const SHA1& SHA1Struct2) const
		{
			for (int32_t i = 0; i < 20; ++i)
			{
				if (SHA1Struct1.Hash[i] != SHA1Struct2.Hash[i])
				{
					return false;
				}
			}
			return true;
		}
	};
}

