#ifndef THREADPOOL_H_
#define THREADPOOL_H_
#include "thread.h"
#include "task.h"
#include <pthread.h>
#include <list>
#include <stdint.h>
#include <sys/time.h>

using namespace std;

class ThreadPool;

class WorkerThread  {
public:
    WorkerThread();
    ~WorkerThread();

    static void* StartRoutine(void* arg);

    void Start();
    void Execute();
    void PushTask(Task* pTask);

    void set_thread_idx(uint32_t idx) { m_thread_idx = idx; }
    uint32_t task_cnt() const;
    void set_parent_thread_pool(ThreadPool *parent_thread_pool);
    void Redistribution();

private:
    uint32_t		m_thread_idx;
    volatile uint32_t	m_task_cnt;
    pthread_t		m_thread_id;
    ThreadNotify	m_thread_notify;
    list<Task*>     m_task_list;
    ThreadPool      *m_parent_thread_pool;
};

class ThreadPool {
public:
    ThreadPool();
    virtual ~ThreadPool();

    /* 测试用接口，该类接口只用于性能测试，生产环境勿用该接口 */
    bool SetMaxTaskCount(uint32_t  task_count);
    /* 初始化线程池的工作线程的数量 */
    bool Init(uint32_t worker_size);
    /* 添加任务到线程池 */
    void AddTask(Task* pTask);
    void Destory();
    /* 根据需要设置，当存在空闲线程时，超过阀值时, 当前工作线程就会把超过阀值的任务重新分派出去 */
    void SetThresholdRedistribution(uint32_t threshold);

private:
    void AtomicSubTaskCount();
    void CostTime();
    uint32_t GetOne();

    uint32_t busy_thread_idx() const;
    void update_busy_thread_idx();
    void clean_busy_thread_idx();
    void setidle_thread_idx(const pthread_t &idle_thread_idx);

    /* 让WorkerThread的函数Redistribution能访问 ThreadPool 的私有成员 */
    friend void WorkerThread::Redistribution();
    friend void WorkerThread::Execute();
private:
    /* idle thread id 空闲的线程编号 */
    uint32_t    m_idle_thread_idx;  
    uint32_t    m_worker_size;
    WorkerThread* 	m_worker_list;

    /* 在线程池中，某个线程做完任务后，在该空闲线程中更新这个值，
      * 然后m_busy_thread_idx线程允许任务再分配，并将m_busy_thread_idx赋值为m_worker_size
      */
    uint32_t m_busy_thread_idx;
    /* 当存在空闲线程时，超过阀值时, 当前工作线程就会把超过阀值的任务重新分派出去   */
    uint32_t m_threshold_redistribution;

    /* 测试接口的私有变量 */
    bool m_is_set_max_task_cnt;
    volatile uint32_t m_max_task_cnt;
    uint32_t m_max_task_cnt_record; //
    struct timeval m_time_start;
    struct timeval m_time_stop;
};

#endif /* THREADPOOL_H_ */
