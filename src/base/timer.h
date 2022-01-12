/***********************************************************************/
/* 名称:定时器组件													   */
/* 说明:有一定误差的定时器，用于不必非常严格的场景(e.g.心跳包)		   */
/* 创建时间:2021/02/03												   */
/* Email:umichan0621@gmail.com  									   */
/* Reference:https://cloud.tencent.com/developer/article/1361827	   */
/***********************************************************************/
#pragma once
#include <functional>
#include <list>
#include <unordered_set>
#include <mutex>
#include <chrono>
#include <base/singleton.hpp>

namespace base
{
	using namespace std::chrono;
	struct TimeTick
	{
		uint32_t Ms;						//毫秒
		uint32_t Sec;						//秒
		uint32_t Min;						//分钟
	};

	struct TimerEvent
	{
		uint32_t Interval;					//超时时间(为初始化的倍数)
		TimeTick EventTimeTick;				//时间
		std::function<bool()>  CallBack;	//回调函数
	};

	class Timer
	{
		typedef std::function<bool()> Task;
		typedef std::list<TimerEvent> WheelList;
	public:
		Timer();
		~Timer();
	public:
		//RefreshInterval，以毫秒为单位，表示定时器最小时间粒度，设定值应该被1000即1秒整除
		//MinInterval，表示定时器所能接受的分钟时间间隔,表示一轮时间的总分钟数
		//启动成功返回true，失败返回false，说明刷新间隔设定有问题
		bool init(uint32_t RefreshInterval, uint32_t MinInterval);
		//设置时间为TimeOut的事件时,根据TimeOut算出发生此事件时刻的指针位置{TriggerMin,TriggerS,TriggerMS};
		//增加计时器，Interval必须是m_RefreshInterval的倍数
		bool add_timer(uint32_t Interval, const Task& NewTask);

		bool add_timer_lockless(uint32_t Interval, const Task& NewTask);
		//手动刷新，可与其他需要定时刷新的组件一起使用，刷新间隔应该和初始化时一致
		void refresh_timer();
		//创建自动刷新的线程
		void create_thread_loop();
		//执行此函数当前线程会死循环，应该将此函数加入线程池
		void thread_loop();
		uint16_t get_timer_counter();
	private:
		Timer(const Timer&) = delete;
		Timer& operator=(const Timer) = delete;
		uint32_t _calculate_msec(TimeTick& EventTimeTick);
		void _insert_timer(uint32_t TimerInterval, TimerEvent& CurEvent);
		void _get_next_trigger_slot(uint32_t Interval, TimeTick& EventTimeTick);
		void _deal_time_wheel(WheelList& CurWheel);
	private:
		bool		m_bIsInited;				//是否已经初始化，避免重复初始化
		bool		m_bIsThreadCreate;			//是否已经创建线程，避免重复创建线程
		uint32_t	m_MsecTick;					//计数刻度
		uint32_t	m_SecTick;
		uint32_t	m_MinTick;
		uint32_t	m_RefreshInterval;			//刷新间隔
		uint16_t	m_TimerCount;				//计算定时器数
		WheelList*	m_pWheelList;				//此处不是链表，是链表数组的头指针，时间轮数组
		TimeTick	m_TimeTick;					//时间
		std::mutex	m_TimerMutex;
	};

	//计算毫秒数
	inline uint32_t Timer::_calculate_msec(TimeTick& EventTimeTick)
	{
		return m_RefreshInterval * EventTimeTick.Ms + EventTimeTick.Sec * 1000 + EventTimeTick.Min * 60 * 1000;
	}

	inline uint16_t Timer::get_timer_counter()
	{
		std::unique_lock<std::mutex> Lock(m_TimerMutex);
		return m_TimerCount;
	}

	//当前线程空转MircoSec微秒，CPU时间片不会被其他线程剥夺
	inline void delay(int64_t MircoSec)
	{
		int64_t Start = std::chrono::duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
		for (;;)
		{
			int64_t Delta = std::chrono::duration_cast<microseconds>(system_clock::now().time_since_epoch()).count() - Start;
			if (Delta >= MircoSec)
			{
				return;
			}
		}
	}

	inline uint64_t now_mill()
	{
		return std::chrono::duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	}
}
#define g_pTimer base::Singleton<base::Timer>::get_instance()