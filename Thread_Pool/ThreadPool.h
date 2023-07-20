#pragma once
#pragma once
#include "TaskQueue.h"
#include "TaskQueue.cpp"

template <typename T>
class ThreadPool
{
public:
	// �����̳߳ز���ʼ��
	ThreadPool(int min, int max);

	// �����̳߳�
	~ThreadPool();

	// ���̳߳��������
	void Add_Task(Task<T> task);

	// ��ȡ�̳߳��й������̵߳�����
	int Get_BusyNum();

	// ��ȡ�̳߳��д����̵߳�����
	int Get_AliveNum();

private:
	/**********************************************************************/
	// �������߳�(�������߳�)������
	static void* Worker(void* arg);
	// �������߳�������
	static void* Manager(void* arg);
	// �����߳��˳�
	void Thread_Exit();

private:
	// �������
	TaskQueue<T>* taskQ_;

	pthread_t managerID_;                       // �������߳�ID
	pthread_t* threadIDs_;                      // �������߳�ID

	int minNum_;                                // ��С�߳�����
	int maxNum_;                                // ����߳�����
	int busyNum_;                               // æ���߳�����
	int liveNum_;                               // �����߳�����
	int exitNum_;                               // Ҫ���ٵ��߳�����

	pthread_mutex_t mutex_pool_;                // �������̳߳�
	pthread_cond_t not_empty_;                  // ��������ǲ���Ϊ��
	static const int NUMBER = 2;

	bool shutdown_;                             // �Ƿ�Ҫ�����̳߳�
};


