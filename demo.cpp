#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include "threadPool.h"
#include "thread.h"
#include "task.h"

using namespace std;

#define PER_TASK_CNT 250 * 10000
#define PRODUCER_CNT 4
#define POOL_SIZE 4

class  TestTask : public Task
{
public:
    TestTask(int i){
        m_i = i;
    }

    void run() {
        for(int i=0; i< 500; i++) {
        }
    }
private:
    int m_i;
};

void *test_func(void *arg)
{
    ThreadPool *thread_pool = (ThreadPool *)arg;
    TestTask *t = NULL;
    int i = 0;
    for(i = 0; i < PER_TASK_CNT; i++)
    {
        t = new TestTask(i);
        thread_pool->AddTask(t);
    }
    return NULL;
}


int main(int argc, char *argv[])
{
    ThreadPool thread_pool; // 建立线程池
    thread_pool.Init(POOL_SIZE); // 线程池的数量
    thread_pool.SetThresholdRedistribution(10); //设置重新分配的阀值，当存在空闲线程时，超过阀值时, 当前工作线程就会把超过阀值的任务重新分派出去
    thread_pool.SetMaxTaskCount(PER_TASK_CNT * PRODUCER_CNT); // 设置任务数，该调用出发后完成任务数后进程退出，所以该调用只用于测试环境，生产环境不要调用该方法

    int i = 0;
    pthread_t thread_ids[PRODUCER_CNT];
    for(i = 0; i < PRODUCER_CNT; i++)
    {
        pthread_create(&thread_ids[i], NULL, test_func, &thread_pool);
    }

    for(i = 0; i < PRODUCER_CNT; i++)
    {
        pthread_join(thread_ids[i], NULL);
    }

    pause();
    return 0;
}
