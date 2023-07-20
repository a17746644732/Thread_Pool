#pragma once
#include <queue>
#include <pthread.h>

using callback = void (*)(void* arg);
// 任务结构体
template <typename T>
struct Task {

	Task<T>() :function_(nullptr), arg_(nullptr)
	{ }
	Task<T>(callback f, void* arg) {
		this->arg_ = static_cast<T*>(arg);
		function_ = f;
	}

	// void (*function)(void* arg);
	callback function_;
	T* arg_;
};

template <typename T>
class TaskQueue
{
public:
	TaskQueue();
	~TaskQueue();

	// 添加任务
	void add_task(Task<T> task);
	void add_task(callback f, void* arg);
	// 取出一个任务
	Task<T> take_task();
	// 获取当前任务的个数
	inline size_t TaskNumber() {
		return m_taskQ_.size();
	}

private:
	pthread_mutex_t m_mutex_;
	std::queue<Task<T>> m_taskQ_;
};


