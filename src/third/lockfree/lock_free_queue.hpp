/***********************************************************************/
/* 名称:无锁队列													   */
/* 说明:基于循环数组的有界无锁队列									   */
/* 创建时间:2021/10/27												   */
/* Email:umichan0621@gmail.com									       */
/* Reference:https://blog.csdn.net/xin_hen/article/details/108142403   */
/***********************************************************************/
#pragma once
#include <vector>
#include <atomic>
#include <thread>
#include <assert.h>

namespace Base
{
	//push or pop 避让策略
	enum class Strategy 
	{
		ABANDON, //放弃
		FORCE,  //必须得到
		YIELD,  //必须得到，但会调用yield
	};
	//front or rear为-1时，表示其他线程已加锁，正在操作数据
	constexpr int32_t EXCLUDE = -1;
	//保证T应当是trival
	template<typename T>
	class LockFreeQueue
	{
	public:
		LockFreeQueue() {}
		//保证初始化在单线程下完成
		LockFreeQueue(uint32_t Capacity) :
			m_Capacity(Capacity),
			m_Size({ 0 }),
			m_Front({ 0 }),
			m_Rear({ 0 }),
			m_Data(Capacity) {}
		~LockFreeQueue() {}
	public:
		bool full() { return m_Size.load() == m_Capacity; }
		bool empty() { return m_Size.load() == 0; }
		bool push(T val, Strategy strategy = Strategy::FORCE);
		bool pop(T& val, Strategy strategy = Strategy::FORCE);
	private:
		const uint32_t					m_Capacity;			//数组最大容量
		std::atomic<int32_t>			m_Size;				//当前size
		std::atomic<int32_t>			m_Front;			//头指针
		std::atomic<int32_t>			m_Rear;				//尾指针
		std::vector<std::atomic<T>>		m_Data;
	};

	template<typename T>
	bool LockFreeQueue<T>::push(T Val, Strategy CurStrategy) {
		int Rear = m_Rear.load();
		while (true) {
			if (Rear == EXCLUDE || full()) {
				switch (CurStrategy) {
				case Strategy::YIELD:
					std::this_thread::yield();
				case Strategy::FORCE:
					Rear = m_Rear.load();
					continue;
				}
				return false;
			}
			//加rear锁
			if (m_Rear.compare_exchange_weak(Rear, EXCLUDE)) {
				//已满，失败解锁回退
				if (full()) {
					int excepted = EXCLUDE;
					bool flag = m_Rear.compare_exchange_weak(excepted, Rear);
					assert(flag);
					continue;
				}
				break;
			}
		}
		m_Data[Rear].store(Val);
		++m_Size; //必须在解锁前面
		int excepted = EXCLUDE;
		//释放rear锁
		bool flag = m_Rear.compare_exchange_weak(excepted, (Rear + 1) % m_Capacity);
		assert(flag);
		return true;
	}

	template<typename T>
	bool LockFreeQueue<T>::pop(T& val, Strategy strategy) {
		int front = m_Front.load();
		while (true) {
			if (front == EXCLUDE || empty()) {
				switch (strategy) {
				case Strategy::YIELD:
					std::this_thread::yield();
				case Strategy::FORCE:
					front = m_Front.load();
					continue;
				}
				return false;
			}
			//加锁
			if (m_Front.compare_exchange_weak(front, EXCLUDE)) {
				//空，失败解锁回退
				if (empty()) {
					int excepted = EXCLUDE;
					bool flag = m_Front.compare_exchange_weak(excepted, front);
					assert(flag);
					continue;
				}
				break;
			}
		}
		val = m_Data[front].load();
		//_data[front].store(Empty);
		--m_Size; //必须在解锁前面
		int excepted = EXCLUDE;
		bool flag = m_Front.compare_exchange_weak(excepted, (front + 1) % m_Capacity);
		assert(flag);
		return true;
	}
}