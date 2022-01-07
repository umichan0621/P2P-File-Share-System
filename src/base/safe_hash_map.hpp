#pragma once
#include <unordered_map>
#include <mutex>

namespace Base
{
	template<typename _Key, typename _Val>
	class HashMap
	{
	public:
		HashMap() = default;
		HashMap(const HashMap&) = delete;
		HashMap& operator=(const HashMap&) = delete;
	public:
		bool exist(_Key Key)
		{
			std::lock_guard<std::mutex> Lock(m_HashMapMutex);
			return m_HashMap.find(Key) != m_HashMap.end();
		}
		void insert(_Key Key, _Val Val)
		{
			std::lock_guard<std::mutex> Lock(m_HashMapMutex);
			m_HashMap.insert({ Key ,Val });
		}
		void erase(_Key Key)
		{
			std::lock_guard<std::mutex> Lock(m_HashMapMutex);
			m_HashMap.erase(Key);
		}

		uint32_t size()
		{
			std::lock_guard<std::mutex> Lock(m_HashMapMutex);
			return m_HashMap.size();
		}

		_Val& operator[](_Key Key)
		{
			std::lock_guard<std::mutex> Lock(m_HashMapMutex);
			return m_HashMap[Key];
		}
	private:
		std::unordered_map<_Key, _Val>	m_HashMap;
		std::mutex						m_HashMapMutex;
	};
}