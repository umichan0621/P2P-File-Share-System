/***********************************************************************/
/* 名称:线程池														   */
/* 说明:预先创建工作线程，当有任务时交给创建好的工作线程处理		   */
/* 创建时间:2021/02/05												   */
/* Email:umichan0621@gmail.com									       */
/* Reference:https://www.cnblogs.com/ailumiyana/p/10016965.html        */
/***********************************************************************/
#pragma once
#include <vector>
#include <deque>
#include <thread>
#include <functional>
#include <condition_variable>

namespace base
{
	class ThreadPool
	{
		typedef std::function<void()> Task;
		typedef std::deque<Task> TaskList;
		typedef std::vector<std::thread*> WorkThreadQueue;
	public:
		ThreadPool();
		~ThreadPool();
	public:
		bool start(uint16_t ThreadNum=1);
		void stop();
		void add_task(const Task&);
	private:
		void _thread_loop();
		Task _accept_task();
	private:
		uint16_t					m_ThreadNum;
		bool						m_bIsStarted;
		WorkThreadQueue				m_WorkThreadList;
		TaskList					m_TaskList;
		std::mutex					m_ThreadPoolMutex;
		std::condition_variable		m_ConditionVariable;
	};
};