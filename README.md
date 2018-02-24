# 概述

这是线程池的一种实现

# 工作线程

为了更好的描述，，我把线程池里的线程称为工作线程。
每个工作线程都拥有自己的任务队列和锁。

# 线程池任务分配原则

1、优先分配：新任务优先分配给空闲的工作线程，如果全部工作线程都忙，优先分配任务队列比较短的工作线程。
2、任务再分配：当存在空闲线程时，超过阀值时, 当前工作线程就会把超过阀值的任务重新分配出去。

# 使用说明

如果在windows用这个库，要安装pthread库, gcc库（原子操作）， linux可以直接用

线程池类：class ThreadPool
任务基类：class Task

# 接口说明

1、 ThreadPool::Init(uint32_t worker_size) 
    初始化线程池的工作线程的数量

2、ThreadPool::SetThresholdRedistribution(uint32_t threshold)
    设置任务重新分配的阈值，根据需要设置，当存在空闲线程时，超过阀值时, 当前工作线程就会把超过阀值的任务重新分派出去

3、ThreadPool::SetMaxTaskCount(uint32_t  task_count)
    设置最大任务数，任务完成后，打印总耗时和tps，并强行退出进程， 该类接口只用于线程池性能测试，生产环境勿用该接口
```

# 示例代码

```
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include "threadPool.h"
#include "thread.h"
#include "task.h"

using namespace std;

class  TestTask : public Task
{
public:
    TestTask(int i){
        m_i = i;
    }

    void run() {
        //printf("hello %d\n", m_i);
    }
private:
    int m_i;
};

int main(int argc, char *argv[])
{

    int max_task_cnt = 10 * 10000;
    ThreadPool thread_pool; // 建立线程池
    thread_pool.Init(10); // 线程池的数量
    thread_pool.SetThresholdRedistribution(1); // 设置重新分配的阀值，当存在空闲线程时，超过阀值时, 当前工作线程就会把超过阀值的任务重新分派出去
     
    thread_pool.SetMaxTaskCount(max_task_cnt); // 设置最大任务数，任务完成后，打印总耗时和tps，并强行退出进程，只用于线程池性能测试,如果不测，则注释掉这一句

    TestTask *t = NULL;
    for (int i=0 ; i< max_task_cnt; i++) {
        t = new TestTask(i);
        thread_pool.AddTask(t);
    }
    pause();
    return 0;
}
```
