/***********************************************************************/
/* 名称:缓存池														   */
/* 说明:预先创建固定大小缓存，不够时new，用完回收，减少开销			   */
/* 创建时间:2021/02/20												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <list>
#include <mutex>

#include <base/singleton.hpp>

namespace base
{
	class BufferPool
	{
	public:
		BufferPool() :m_BufSize(4000) { }
		~BufferPool()
		{
			std::lock_guard<std::mutex> Lock(m_BufferPoolMutex);
			auto it = m_BufferList.begin();
			for (; it != m_BufferList.end(); ++it)
			{
				delete[](*it);
			}
			m_BufferList.clear();
		}
	public:
		void init(uint16_t InitBufNum, uint16_t BufSize)
		{
			if (!m_BufferList.empty())
			{
				return;
			}
			m_BufSize = BufSize;
			for (uint16_t i = 0; i < InitBufNum; ++i)
			{
				char* pBuf = new char[BufSize];
				m_BufferList.push_back(pBuf);
			}
		}
		void init(uint16_t InitBufNum = 1024)
		{
			init(InitBufNum, m_BufSize);
		}
		char* allocate()
		{
			char* pBuf = nullptr;
			std::lock_guard<std::mutex> Lock(m_BufferPoolMutex);
			if (m_BufferList.size() > 0)
			{
				pBuf = m_BufferList.back();
				m_BufferList.pop_back();
			}
			else
			{
				pBuf = new char[m_BufSize];
			}
			return pBuf;
		}
		void release(char* pBuf)
		{
			std::lock_guard<std::mutex> Lock(m_BufferPoolMutex);
			m_BufferList.push_front(pBuf);
		}
	private:
		BufferPool(const BufferPool&) = delete;
		BufferPool& operator=(const BufferPool) = delete;
	private:
		uint16_t			m_BufSize;
		std::list<char*>	m_BufferList;
		std::mutex			m_BufferPoolMutex;
	};
}

#define g_pBufferPoolMgr base::Singleton<base::BufferPool>::get_instance()