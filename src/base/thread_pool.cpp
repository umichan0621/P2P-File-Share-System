#include "thread_pool.h"

namespace base
{
	ThreadPool::ThreadPool()
	{
		m_ThreadNum = 1;
		m_bIsStarted = false;
	}

	ThreadPool::~ThreadPool()
	{
		if (true == m_bIsStarted)
		{
			stop();
		}
	}

	bool ThreadPool::start(uint16_t ThreadNum)
	{
		m_ThreadNum = ThreadNum;
		if (false == m_WorkThreadList.empty())
		{
			return false;
		}
		m_bIsStarted = true;
		//预先创建线程
		m_WorkThreadList.reserve(m_ThreadNum);
		for (uint16_t i = 0; i < m_ThreadNum; ++i)
		{
			m_WorkThreadList.push_back(new std::thread(std::bind(&ThreadPool::_thread_loop, this)));
		}
		return true;
	}

	void ThreadPool::stop()
	{
		std::lock_guard<std::mutex> Lock(m_ThreadPoolMutex);
		m_bIsStarted = false;
		m_ConditionVariable.notify_all();
		for (WorkThreadQueue::iterator it = m_WorkThreadList.begin(); it != m_WorkThreadList.end(); ++it)
		{
			(*it)->join();
			delete* it;
		}
		m_WorkThreadList.clear();
	}

	void ThreadPool::_thread_loop()
	{
		while (true == m_bIsStarted)
		{
			Task NewTask = _accept_task();
			if (NewTask)
			{
				NewTask();
			}
		}
	}

	void ThreadPool::add_task(const Task& NewTask)
	{
		std::lock_guard<std::mutex> Lock(m_ThreadPoolMutex);
		m_TaskList.push_back(NewTask);
		m_ConditionVariable.notify_one();
	}

	ThreadPool::Task ThreadPool::_accept_task()
	{
		std::unique_lock<std::mutex> Lock(m_ThreadPoolMutex);
		//always use a while-loop, due to spurious wakeup
		while (m_TaskList.empty() && m_bIsStarted)
		{
			m_ConditionVariable.wait(Lock);
		}
		Task NewTask;
		TaskList::size_type size = m_TaskList.size();
		if (!m_TaskList.empty() && m_bIsStarted)
		{
			NewTask = m_TaskList.front();
			m_TaskList.pop_front();
		}
		return NewTask;
	}
}