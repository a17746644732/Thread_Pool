#pragma once
#pragma once
#include "TaskQueue.h"
#include "TaskQueue.cpp"

template <typename T>
class ThreadPool
{
public:
	// 创建线程池并初始化
	ThreadPool(int min, int max);

	// 销毁线程池
	~ThreadPool();

	// 给线程池添加任务
	void Add_Task(Task<T> task);

	// 获取线程池中工作的线程的数量
	int Get_BusyNum();

	// 获取线程池中存活的线程的数量
	int Get_AliveNum();

private:
	/**********************************************************************/
	// 工作的线程(消费者线程)任务函数
	static void* Worker(void* arg);
	// 管理者线程任务函数
	static void* Manager(void* arg);
	// 单个线程退出
	void Thread_Exit();

private:
	// 任务队列
	TaskQueue<T>* taskQ_;

	pthread_t managerID_;                       // 管理者线程ID
	pthread_t* threadIDs_;                      // 工作的线程ID

	int minNum_;                                // 最小线程数量
	int maxNum_;                                // 最大线程数量
	int busyNum_;                               // 忙的线程数量
	int liveNum_;                               // 存活的线程数量
	int exitNum_;                               // 要销毁的线程数量

	pthread_mutex_t mutex_pool_;                // 锁整个线程池
	pthread_cond_t not_empty_;                  // 任务队列是不是为空
	static const int NUMBER = 2;

	bool shutdown_;                             // 是否要销毁线程池
};


