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

3、ThreadPool::AddTask(Task* pTask);
    添加任务到线程池

4、ThreadPool::SetMaxTaskCount(uint32_t  task_count)
    设置最大任务数，任务完成后，打印总耗时和tps，并强行退出进程， 该类接口只用于线程池性能测试，生产环境勿用该接口

# 如何衡量一个线程池的性能

测试大量消耗时长不同的 Task。

# 测试demo

请查看 demo.cpp(比较粗略的估算在一定计算量的task的线程池性能)