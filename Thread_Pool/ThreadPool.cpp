#include "ThreadPool.h"
#include <iostream>
#include <string.h>
#include <string>
#include <unistd.h>

using namespace std;

template <typename T>
ThreadPool<T>::ThreadPool(int min, int max)
{

	do {
		// 实例化任务队列
		taskQ_ = new TaskQueue<int>;
		if (taskQ_ == nullptr) {
			cout << "Malloc taskQ_ fail........." << endl;
			break;
		}

		threadIDs_ = new pthread_t[max];
		if (threadIDs_ == nullptr) {
			cout << "Malloc threadIDs_ fail........" << endl;
			break;
		}

		// 对申请到的内存初始化为0
		memset(threadIDs_, 0, sizeof(pthread_t) * max);
		minNum_ = min;
		maxNum_ = max;
		busyNum_ = 0;
		liveNum_ = min;                     // 存活线程数量和最小线程数量相等
		exitNum_ = 0;

		if (pthread_mutex_init(&mutex_pool_, NULL) != 0 || 
			pthread_cond_init(&not_empty_, NULL) != 0) {
			cout << "mutex or condition init fail........" << endl;
			break;
		}

		shutdown_ = false;

		// 创建管理者线程
		pthread_create(&managerID_, NULL, Manager, this);
		// 创建工作线程
		for (int i = 0; i < min; ++i) {
			pthread_create(&threadIDs_[i], NULL, Worker, this);
		}
		
		return; // 不向下执行，成功构造则返回，失败则向下执行释放已经申请的资源
	} while (0);

	// 释放资源
	if (threadIDs_) delete[] threadIDs_;
	if (taskQ_) delete taskQ_;

}

template <typename T>
ThreadPool<T>::~ThreadPool()
{
	// 关闭线程池
	shutdown_ = true;
	// 阻塞回收管理者线程
	pthread_join(managerID_, NULL);
	// 唤醒阻塞的消费者线程，此时任务队列中没任务了，被阻塞的线程数就是存活的线程数
	for (int i = 0; i < liveNum_;++i) {
		// 一次唤醒一个被阻塞的消费者线程
		pthread_cond_signal(&not_empty_);
	}

	// 释放堆内存
	if (taskQ_) {
		delete taskQ_;
	}
	if (threadIDs_) {
		delete[] threadIDs_;
	}

	// 销毁互斥锁和条件变量
	pthread_mutex_destroy(&mutex_pool_);
	pthread_cond_destroy(&not_empty_);

}

template <typename T>
void ThreadPool<T>::Add_Task(Task<T> task)
{
	// 因为add_task中已经加了锁，所以在这里不需要加锁了
	// pthread_mutex_lock(&mutex_pool_);
	

	// 如果线程池关闭，直接返回
	if (shutdown_) {
		// pthread_mutex_unlock(&mutex_pool_);
		return;
	}

	// 添加任务
	taskQ_->add_task(task);
	pthread_cond_signal(&not_empty_);

	//pthread_mutex_unlock(&mutex_pool_);
}

template <typename T>
int ThreadPool<T>::Get_BusyNum()
{	
	pthread_mutex_lock(&mutex_pool_);
	int busyNum = busyNum_;
	pthread_mutex_unlock(&mutex_pool_);

	return busyNum;
}

template <typename T>
int ThreadPool<T>::Get_AliveNum()
{
	pthread_mutex_lock(&mutex_pool_);
	int liveNum = liveNum_;
	pthread_mutex_unlock(&mutex_pool_);

	return liveNum;
}

template <typename T>
void* ThreadPool<T>::Worker(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*> (arg);

	while (1) {
		pthread_mutex_lock(&pool->mutex_pool_);
		// 当前任务队列是否为空
		while (pool->taskQ_->TaskNumber() == 0 && !pool->shutdown_) {
			// 阻塞工作线程
			// 此时mutex_pool_被解锁，当生产者线程唤醒条件变量not_empty_时，该线程获得锁
			pthread_cond_wait(&pool->not_empty_, &pool->mutex_pool_);

			// 判断是否要销毁线程
			if (pool->exitNum_ > 0) {
				pool->exitNum_--;
				if (pool->liveNum_ > pool->minNum_) {
					pool->liveNum_--;
					pthread_mutex_unlock(&pool->mutex_pool_);
					pool->Thread_Exit();
				}

			}

		}

		// 判断线程池是否被关闭了
		if (pool->shutdown_) {
			pthread_mutex_unlock(&pool->mutex_pool_);
			pool->Thread_Exit();
		}

		// 从任务队列中取出一个任务
		Task<T> task = pool->taskQ_->take_task();
		pool->busyNum_++;
		pthread_mutex_unlock(&pool->mutex_pool_);

		cout << "thread " << to_string(pthread_self()) << " start working........." << endl;
		// 任务执行过程中会一直阻塞在这
		task.function_(task.arg_);
		// 任务执行完后释放资源
		delete task.arg_;
		task.arg_ = nullptr;
		cout << "thread " << to_string(pthread_self()) << " end working........." << endl;
		pthread_mutex_lock(&pool->mutex_pool_);
		pool->busyNum_--;
		pthread_mutex_unlock(&pool->mutex_pool_);

	}

	return nullptr;
}

template <typename T>
void* ThreadPool<T>::Manager(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*>(arg);
	while (!pool->shutdown_) {
		// 每隔3s检测一次线程池进行管理
		sleep(3);

		// 取出线程池中任务的数量，当前存活线程的数量和当前正忙的线程的数量
		pthread_mutex_lock(&pool->mutex_pool_);
		int size = pool->taskQ_->TaskNumber();
		int liveNum = pool->liveNum_;
		int busyNum = pool->busyNum_;
		pthread_mutex_unlock(&pool->mutex_pool_);

		// 添加线程
		// 任务的个数 > 存活线程的数量 && 存活的线程数 < 最大线程数量
		if (size > liveNum && liveNum < pool->maxNum_) {
			pthread_mutex_lock(&pool->mutex_pool_);
			int count = 0;
			for (int i = 0; i < pool->maxNum_ && count < NUMBER && 
				pool->liveNum_ < pool->maxNum_; ++i) {
				if (pool->threadIDs_[i] == 0) {
					pthread_create(&pool->threadIDs_[i], NULL, Worker, pool);
					count++;
					pool->liveNum_++;
				}
			}
			pthread_mutex_unlock(&pool->mutex_pool_);
		}

		// 销毁线程
		// 正忙的线程数 * 2 < 存活的线程数 && 存活的线程数 > 最小线程数
		if (busyNum * 2 < liveNum && liveNum > pool->minNum_) {
			pthread_mutex_lock(&pool->mutex_pool_);
			pool->exitNum_ = NUMBER;
			pthread_mutex_unlock(&pool->mutex_pool_);
			// 让阻塞的线程自杀
			for (int i = 0; i < NUMBER; ++i) {
				pthread_cond_signal(&pool->not_empty_);
			}
		}

	}

	return nullptr;
}

template <typename T>
void ThreadPool<T>::Thread_Exit()
{
	pthread_t tid = pthread_self();
	for (int i = 0; i < maxNum_; ++i) {
		if (threadIDs_[i] == tid) {
			threadIDs_[i] = 0;
			cout << "threadExit() called " << to_string(tid) << " exiting......." << endl;
			break;
		}
	}
	
	pthread_exit(NULL);
}
