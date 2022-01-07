/***********************************************************************/
/* 名称:NAT Type										               */
/* 说明:存放NAT数据结构和本机NAT类型						           */
/* 创建时间:2021/06/06												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <atomic>
#include <base/singleton.hpp>
/***********************************************************************/
/*	NAT B+包括：	公网IP(无NAT)							(NAT S)    */
/*					完全锥型NAT(FULL CONE NAT)				(NAT A)	   */
/*					限制锥型NAT(RESTRICT CONE NAT)			(NAT B)	   */
/*		NAT B+可以和任意节点通信									   */
/*	NAT C-包括：	端口限制锥型NAT(PORT RESTRICT CONE NAT)	(NAT C)    */
/*					对称型NAT(SYMMETRIC NAT)				(NAT D)	   */
/*		NAT C可以和C和B+节点通信									   */
/*		NAT D只能和B+节点通信										   */
/***********************************************************************/
enum class NAT_TYPE
{
	NAT_TYPE_NULL,
	NAT_TYPE_S,
	NAT_TYPE_A,
	NAT_TYPE_B,
	NAT_TYPE_B_PLUS,
	NAT_TYPE_C_MINUS,
	NAT_TYPE_C,
	NAT_TYPE_D
};

namespace net
{
	class NatType
	{
	public:
		NatType() { m_NatType = NAT_TYPE::NAT_TYPE_NULL; }
		~NatType() {}
	public:
		NAT_TYPE get_nat_type() { return m_NatType; }
		void set_nat_type(NAT_TYPE NatType) { m_NatType = NatType; }
	private:
		std::atomic<NAT_TYPE>		m_NatType;
	};
}

//本机的NAT类型
#define g_pHostNatType base::Singleton<net::NatType>::get_instance()