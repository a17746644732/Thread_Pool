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
		// ʵ�����������
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

		// �����뵽���ڴ��ʼ��Ϊ0
		memset(threadIDs_, 0, sizeof(pthread_t) * max);
		minNum_ = min;
		maxNum_ = max;
		busyNum_ = 0;
		liveNum_ = min;                     // ����߳���������С�߳��������
		exitNum_ = 0;

		if (pthread_mutex_init(&mutex_pool_, NULL) != 0 || 
			pthread_cond_init(&not_empty_, NULL) != 0) {
			cout << "mutex or condition init fail........" << endl;
			break;
		}

		shutdown_ = false;

		// �����������߳�
		pthread_create(&managerID_, NULL, Manager, this);
		// ���������߳�
		for (int i = 0; i < min; ++i) {
			pthread_create(&threadIDs_[i], NULL, Worker, this);
		}
		
		return; // ������ִ�У��ɹ������򷵻أ�ʧ��������ִ���ͷ��Ѿ��������Դ
	} while (0);

	// �ͷ���Դ
	if (threadIDs_) delete[] threadIDs_;
	if (taskQ_) delete taskQ_;

}

template <typename T>
ThreadPool<T>::~ThreadPool()
{
	// �ر��̳߳�
	shutdown_ = true;
	// �������չ������߳�
	pthread_join(managerID_, NULL);
	// �����������������̣߳���ʱ���������û�����ˣ����������߳������Ǵ����߳���
	for (int i = 0; i < liveNum_;++i) {
		// һ�λ���һ�����������������߳�
		pthread_cond_signal(&not_empty_);
	}

	// �ͷŶ��ڴ�
	if (taskQ_) {
		delete taskQ_;
	}
	if (threadIDs_) {
		delete[] threadIDs_;
	}

	// ���ٻ���������������
	pthread_mutex_destroy(&mutex_pool_);
	pthread_cond_destroy(&not_empty_);

}

template <typename T>
void ThreadPool<T>::Add_Task(Task<T> task)
{
	// ��Ϊadd_task���Ѿ������������������ﲻ��Ҫ������
	// pthread_mutex_lock(&mutex_pool_);
	

	// ����̳߳عرգ�ֱ�ӷ���
	if (shutdown_) {
		// pthread_mutex_unlock(&mutex_pool_);
		return;
	}

	// �������
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
		// ��ǰ��������Ƿ�Ϊ��
		while (pool->taskQ_->TaskNumber() == 0 && !pool->shutdown_) {
			// ���������߳�
			// ��ʱmutex_pool_�����������������̻߳�����������not_empty_ʱ�����̻߳����
			pthread_cond_wait(&pool->not_empty_, &pool->mutex_pool_);

			// �ж��Ƿ�Ҫ�����߳�
			if (pool->exitNum_ > 0) {
				pool->exitNum_--;
				if (pool->liveNum_ > pool->minNum_) {
					pool->liveNum_--;
					pthread_mutex_unlock(&pool->mutex_pool_);
					pool->Thread_Exit();
				}

			}

		}

		// �ж��̳߳��Ƿ񱻹ر���
		if (pool->shutdown_) {
			pthread_mutex_unlock(&pool->mutex_pool_);
			pool->Thread_Exit();
		}

		// �����������ȡ��һ������
		Task<T> task = pool->taskQ_->take_task();
		pool->busyNum_++;
		pthread_mutex_unlock(&pool->mutex_pool_);

		cout << "thread " << to_string(pthread_self()) << " start working........." << endl;
		// ����ִ�й����л�һֱ��������
		task.function_(task.arg_);
		// ����ִ������ͷ���Դ
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
		// ÿ��3s���һ���̳߳ؽ��й���
		sleep(3);

		// ȡ���̳߳����������������ǰ����̵߳������͵�ǰ��æ���̵߳�����
		pthread_mutex_lock(&pool->mutex_pool_);
		int size = pool->taskQ_->TaskNumber();
		int liveNum = pool->liveNum_;
		int busyNum = pool->busyNum_;
		pthread_mutex_unlock(&pool->mutex_pool_);

		// ����߳�
		// ����ĸ��� > ����̵߳����� && �����߳��� < ����߳�����
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

		// �����߳�
		// ��æ���߳��� * 2 < �����߳��� && �����߳��� > ��С�߳���
		if (busyNum * 2 < liveNum && liveNum > pool->minNum_) {
			pthread_mutex_lock(&pool->mutex_pool_);
			pool->exitNum_ = NUMBER;
			pthread_mutex_unlock(&pool->mutex_pool_);
			// ���������߳���ɱ
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
