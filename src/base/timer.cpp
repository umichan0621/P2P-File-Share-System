#include "timer.h"
#include <thread>
#include <string.h>

namespace base
{
	using namespace std::chrono;
	constexpr uint16_t MAX_TIMER_NUM = 0xffff;

	Timer::Timer() :
		m_bIsInited(false),
		m_bIsThreadCreate(false),
		m_MsecTick(0),
		m_SecTick(60),
		m_MinTick(0),
		m_RefreshInterval(0),
		m_TimerCount(0),
		m_pWheelList(nullptr)
	{
		memset(&m_TimeTick, 0, sizeof(m_TimeTick));
	}

	Timer::~Timer(){}

	bool Timer::init(uint32_t RefreshInterval, uint32_t MinInterval)
	{
		std::unique_lock<std::mutex> Lock(m_TimerMutex);
		if (true == m_bIsInited)
		{
			return false;
		}
		if (0 != 1000 % RefreshInterval)			//设定刷新间隔不合适
		{
			return false;
		}
		m_RefreshInterval = RefreshInterval;		//刷新间隔
		m_MsecTick = 1000 / RefreshInterval;		//毫秒槽
		m_MinTick = MinInterval;					//分钟槽
		uint32_t WheelListLen = m_MsecTick + m_SecTick + m_MinTick;
		//初始化链表的数组，数组里的元素为链表,时间轮数组
		m_pWheelList = new WheelList[WheelListLen];
		m_bIsInited = true;
		return true;
	}

	bool Timer::add_timer(uint32_t Interval, const Task& NewTask)
	{
		if (Interval < m_RefreshInterval || Interval % m_RefreshInterval != 0
			|| Interval >= m_RefreshInterval * m_MsecTick * m_SecTick * m_MinTick)
		{
			return false;
		}
		TimerEvent NewEvent = { 0 };
		NewEvent.Interval = Interval;
		//初始化回调函数
		NewEvent.CallBack = NewTask;
		//初始化事件的时间
		NewEvent.EventTimeTick.Ms = m_TimeTick.Ms;
		NewEvent.EventTimeTick.Sec = m_TimeTick.Sec;
		NewEvent.EventTimeTick.Min = m_TimeTick.Min;
		//对内部数据结构操作，加锁
		std::unique_lock<std::mutex> Lock(m_TimerMutex);
		if (m_TimerCount == MAX_TIMER_NUM)
		{
			return false;
		}
		//插入定时器
		_insert_timer(Interval, NewEvent);
		m_TimerCount++;
		return true;
	}

	bool Timer::add_timer_lockless(uint32_t Interval, const Task& NewTask)
	{
		if (Interval < m_RefreshInterval || Interval % m_RefreshInterval != 0
			|| Interval >= m_RefreshInterval * m_MsecTick * m_SecTick * m_MinTick)
		{
			return false;
		}
		TimerEvent NewEvent = { 0 };
		NewEvent.Interval = Interval;
		//初始化回调函数
		NewEvent.CallBack = NewTask;
		//初始化事件的时间
		NewEvent.EventTimeTick.Ms = m_TimeTick.Ms;
		NewEvent.EventTimeTick.Sec = m_TimeTick.Sec;
		NewEvent.EventTimeTick.Min = m_TimeTick.Min;

		if (m_TimerCount == MAX_TIMER_NUM)
		{
			return false;
		}
		//插入定时器
		_insert_timer(Interval, NewEvent);
		m_TimerCount++;
		return true;
	}

	void Timer::refresh_timer()
	{
		TimeTick CurTime = { 0 };
		_get_next_trigger_slot(m_RefreshInterval, CurTime);
		TimeTick PreTime = m_TimeTick;
		m_TimeTick = CurTime;

		if (CurTime.Min != PreTime.Min)
		{
			WheelList& CurWheel = m_pWheelList[m_TimeTick.Min + m_SecTick + m_MsecTick];
			_deal_time_wheel(CurWheel);
			CurWheel.clear();
		}
		else if (CurTime.Sec != PreTime.Sec)
		{
			WheelList& CurWheel = m_pWheelList[m_TimeTick.Sec + m_MsecTick];
			_deal_time_wheel(CurWheel);
			CurWheel.clear();
		}
		else if (CurTime.Ms != PreTime.Ms)
		{
			WheelList& CurWheel = m_pWheelList[m_TimeTick.Ms];
			_deal_time_wheel(CurWheel);
			CurWheel.clear();
		}
	}

	void Timer::create_thread_loop()
	{
		std::unique_lock<std::mutex> Lock(m_TimerMutex);
		if (true == m_bIsThreadCreate)
		{
			return;
		}
		m_bIsThreadCreate = true;
		std::thread th([&]
			{
				std::unique_lock<std::mutex> Lock(m_TimerMutex);
				if (true == m_bIsThreadCreate)
				{
					return;
				}
				m_bIsThreadCreate = true;
				Lock.unlock();

				high_resolution_clock::time_point BeignTime = high_resolution_clock::now();
				//使用sleep_until可以自动补正误差时间
				for (;;)
				{
					BeignTime += milliseconds(m_RefreshInterval);
					refresh_timer();
					std::this_thread::sleep_until(BeignTime);
				}
			});
		th.detach();
	}

	void Timer::thread_loop()
	{
		std::unique_lock<std::mutex> Lock(m_TimerMutex);
		if (true == m_bIsThreadCreate)
		{
			return;
		}
		m_bIsThreadCreate = true;
		Lock.unlock();

		high_resolution_clock::time_point BeignTime = high_resolution_clock::now();
		//使用sleep_until可以自动补正误差时间
		for (;;)
		{
			BeignTime += milliseconds(m_RefreshInterval);
			refresh_timer();
			std::this_thread::sleep_until(BeignTime);
		}
	}

	void Timer::_insert_timer(uint32_t TimerInterval, TimerEvent& CurEvent)
	{
		TimeTick EventTimeTick = { 0 };
		_get_next_trigger_slot(TimerInterval, EventTimeTick);

		//数组的每一个元素都是一个list链表
		if (EventTimeTick.Min != m_TimeTick.Min)
			m_pWheelList[m_MsecTick + m_SecTick + EventTimeTick.Min].push_back(CurEvent);
		else if (EventTimeTick.Sec != m_TimeTick.Sec)
			m_pWheelList[m_MsecTick + EventTimeTick.Sec].push_back(CurEvent);
		else if (EventTimeTick.Ms != m_TimeTick.Ms)
			m_pWheelList[EventTimeTick.Ms].push_back(CurEvent);
	}

	void Timer::_get_next_trigger_slot(uint32_t Interval, TimeTick& EventTimeTick)
	{
		uint32_t FutureMsec = _calculate_msec(m_TimeTick) + Interval;
		EventTimeTick.Min = (FutureMsec / 60000) % m_MinTick;
		EventTimeTick.Sec = (FutureMsec % 60000) / 1000;
		EventTimeTick.Ms = (FutureMsec % 1000) / m_RefreshInterval;
	}

	void Timer::_deal_time_wheel(WheelList& CurWheel)
	{
		//对内部数据结构操作，加锁
		std::unique_lock<std::mutex> Lock(m_TimerMutex);
		for (auto CurEvent = CurWheel.begin(); CurEvent != CurWheel.end(); )
		{
			uint32_t CurMsec = _calculate_msec(m_TimeTick);
			uint32_t PreMsec = _calculate_msec(CurEvent->EventTimeTick);
			uint32_t DiffMsec = (CurMsec - PreMsec + (m_MinTick + 1) * 60 * 1000) % ((m_MinTick + 1) * 60 * 1000);

			//当前定时器触发
			if (DiffMsec == CurEvent->Interval)
			{
				//调用回调函数
				bool bRes = CurEvent->CallBack();
				//返回false表示定时器继续运行
				if (false == bRes)
				{
					CurEvent->EventTimeTick = m_TimeTick;
					//重新插入定时器
					_insert_timer(CurEvent->Interval, *CurEvent);
					++CurEvent;
				}
				//返回true表示定时器运行结束，内部删除该定时器
				else
				{
					m_TimerCount--;
					CurEvent = CurWheel.erase(CurEvent);
				}
			}
			//未触发当前定时器，重新插入定时器
			else
			{
				_insert_timer(CurEvent->Interval - DiffMsec, *CurEvent);
				++CurEvent;
			}
		}
	}
}