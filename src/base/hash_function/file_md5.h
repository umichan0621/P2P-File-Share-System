/***********************************************************************/
/* 名称:MD5														       */
/* 说明:自定义MD5数据结构以及MD5相关操作							   */
/* 创建时间:2021/11/24											       */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <stdint.h>
#include <string>
#include <third/md5/md5.h>

namespace base
{
	//MD5长度为128位，可以用两个64位int表示
	struct MD5
	{
		uint64_t PartA;
		uint64_t PartB;
		MD5():PartA(0), PartB(0) {}
		MD5(uint64_t _PartA,uint64_t _PartB):PartA(_PartA), PartB(_PartB){}

	};

	//void md5_value(const MD5& MD5Struct, std::string& strMD5);
	//bool md5_parse(const std::string& strMD5, MD5& MD5Struct);

	//自定义MD5数据结构的哈希函数
	//struct MD5HashFunc
	//{
	//	size_t operator()(const MD5& MD5Obj) const
	//	{
	//		return (size_t)MD5Obj.PartA;
	//	}
	//};
	//自定义MD5数据结构的相等条件
	//struct MD5EqualFunc
	//{
	//	bool operator()(const MD5& MD5Obj1, const MD5& MD5Obj2) const
	//	{
	//		return MD5Obj1.PartA== MD5Obj2.PartA&& MD5Obj1.PartB == MD5Obj2.PartB;
	//	}
	//};
}

