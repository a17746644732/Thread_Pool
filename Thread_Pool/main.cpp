#include <cstdio>
#include <unistd.h>
#include <iostream>
#include "ThreadPool.h"
#include "ThreadPool.cpp"

using namespace std;

void taskFunc(void* arg) {
    int num = *(int*)arg;
    cout << "thread " << to_string(pthread_self()) << " is working, number = " << num << endl;
    sleep(1);
}


int main(int argc, char* argv[])
{
    // 创建线程池对象
    ThreadPool<int> pool(10, 20);

    // 向线程池中添加新任务
    for (int i = 0; i < 1000; ++i) {
        int* num = new int(i + 1);
        pool.Add_Task(Task<int>(taskFunc, num));
    }
    
    getchar();

    return 0;
}