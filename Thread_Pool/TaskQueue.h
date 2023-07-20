#pragma once
#include <queue>
#include <pthread.h>

using callback = void (*)(void* arg);
// ����ṹ��
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

	// �������
	void add_task(Task<T> task);
	void add_task(callback f, void* arg);
	// ȡ��һ������
	Task<T> take_task();
	// ��ȡ��ǰ����ĸ���
	inline size_t TaskNumber() {
		return m_taskQ_.size();
	}

private:
	pthread_mutex_t m_mutex_;
	std::queue<Task<T>> m_taskQ_;
};


