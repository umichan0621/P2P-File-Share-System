/***********************************************************************/
/* 名称:对象池														   */
/* 说明:预先创建对象，不够时new，用完回收，减少开销					   */
/* 创建时间:2021/02/20												   */
/* Email:umichan0621@gmail.com									       */
/***********************************************************************/
#pragma once
#include <list>
#include <mutex>

namespace base
{
	template<typename _Obj>
	class ObjectPool
	{
	public:
		ObjectPool() {};
		~ObjectPool()
		{
			std::lock_guard<std::mutex> Lock(m_ObjectPoolMutex);
			auto it = m_ObjList.begin();
			for (; it != m_ObjList.end(); ++it)
			{
				if (nullptr != (*it))
				{
					delete (*it);
				}
			}
			m_ObjList.clear();
		};
	public:
		void init(uint16_t InitNum)
		{
			m_ObjList.clear();
			for (uint16_t i = 0; i < InitNum; ++i)
			{
				_Obj* pObj = new _Obj();
				m_ObjList.push_back(pObj);
			}
		}
		_Obj* allocate()
		{
			_Obj* pObj = nullptr;
			std::lock_guard<std::mutex> Lock(m_ObjectPoolMutex);
			if (m_ObjList.size() > 0)
			{
				pObj = m_ObjList.back();
				m_ObjList.pop_back();
			}
			else
			{
				pObj = new _Obj();
			}
			return pObj;
		}
		void release(_Obj* pObj)
		{
			std::lock_guard<std::mutex> Lock(m_ObjectPoolMutex);
			m_ObjList.push_front(pObj);
		}
	private:
		ObjectPool(const ObjectPool&) = delete;//禁止复制		
		ObjectPool& operator=(const ObjectPool) = delete;
	private:
		std::list<_Obj*>		m_ObjList;
		std::mutex				m_ObjectPoolMutex;
	};
}
