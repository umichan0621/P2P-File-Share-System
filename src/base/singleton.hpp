/***********************************************************************/
/* 名称:单例模版												   	   */
/* 说明:可把任何类包装成线程安全的全局单例类，出口默认智能指针	   	   */
/* 创建时间:2021/01/23											   	   */
/* Email:umichan0621@gmail.com								   	       */
/* Reference:https://blog.csdn.net/godmaycry/article/details/78458329  */
/***********************************************************************/
#pragma once
#include <mutex>
#include <memory>

namespace base
{
	template<typename _Type>
	class Singleton
	{
	public:
		static std::shared_ptr<_Type> get_instance()
		{
			if (nullptr == m_pSingleton)
			{
				std::lock_guard<std::mutex> Lock(m_SingletonMutex);
				if (nullptr == m_pSingleton)
				{
					m_pSingleton = std::make_shared<_Type>();
				}
			}
			return m_pSingleton;
		}

		static void release_instance()
		{
			if (nullptr != m_pSingleton)
			{
				m_pSingleton.reset();
				m_pSingleton = nullptr;
			}
		}
	private:
		explicit Singleton();
		Singleton(const Singleton&) = delete;
		Singleton& operator=(const Singleton) = delete;
		~Singleton() {};
	private:
		static std::shared_ptr<_Type>	m_pSingleton;
		static std::mutex				m_SingletonMutex;
	};

	template<typename _Type>
	std::shared_ptr<_Type> Singleton<_Type>::m_pSingleton = nullptr;

	template<typename _Type>
	std::mutex Singleton<_Type>::m_SingletonMutex;
}

