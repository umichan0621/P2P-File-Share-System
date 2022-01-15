﻿/***********************************************************************/
/* 名称:路由协议头													   */
/* 说明:通信时UDP包的路由协议头										   */
/* 创建时间:2022/01/11												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include "protocol_base.h"

//文件传输协议
enum PROTOCOL_ROUTING
{
	PROTOCOL_ROUTING_NULL = PROTOCOL_TYPE::PROTOCOL_TYPE_ROUTING << 10,
	PROTOCOL_ROUTING_SEARCH_REQ,			//向其他节点查询几个Key（主动发送）
	PROTOCOL_ROUTING_SEARCH_ACK,			//回复路由表中距离Key最近的节点
	PROTOCOL_ROUTING_REGISTER,				//向其他节点注册几个Key（不需要回复，没收到也无妨）
	PROTOCOL_ROUTING_PARTNER,				//查询命中，自身与查询Key有关联，返回更多相关节点
	PROTOCOL_ROUTING_END
};

