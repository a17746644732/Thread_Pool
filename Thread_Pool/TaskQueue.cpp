#include "TaskQueue.h"

template <typename T>
TaskQueue<T>::TaskQueue()
{
	pthread_mutex_init(&m_mutex_, NULL);
}

template <typename T>
TaskQueue<T>::~TaskQueue()
{
	pthread_mutex_destroy(&m_mutex_);
}

template <typename T>
void TaskQueue<T>::add_task(Task<T> task)
{
	pthread_mutex_lock(&m_mutex_);
	m_taskQ_.push(task);
	pthread_mutex_unlock(&m_mutex_);
}

template <typename T>
void TaskQueue<T>::add_task(callback f, void* arg)
{
	pthread_mutex_lock(&m_mutex_);
	m_taskQ_.push(Task<T>(f, arg));
	pthread_mutex_unlock(&m_mutex_);
}

template <typename T>
Task<T> TaskQueue<T>::take_task()
{
	pthread_mutex_lock(&m_mutex_);
	Task<T> t;
	if (!m_taskQ_.empty()) {
		t = m_taskQ_.front();
		m_taskQ_.pop();
	}
	pthread_mutex_unlock(&m_mutex_);
	return t;
}
