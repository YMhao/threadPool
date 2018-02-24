#include "threadPool.h"
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <stdio.h>

#define atomic_add(x,y) __sync_fetch_and_add((x),(y))  // 对变量进行原子增操作
#define atomic_sub(x,y) __sync_fetch_and_sub((x),(y))  // 对变量进行原子减操作

WorkerThread::WorkerThread()
{
    m_task_cnt = 0;
    m_parent_thread_pool = NULL;
}

WorkerThread::~WorkerThread()
{

}

void* WorkerThread::StartRoutine(void* arg)
{
    WorkerThread* pThread = (WorkerThread*)arg;

    pThread->Execute();

    return NULL;
}

void WorkerThread::Start()
{
    (void)pthread_create(&m_thread_id, NULL, StartRoutine, this);
}

void WorkerThread::Redistribution()
{
    // 资源再分配
    if(m_thread_idx == m_parent_thread_pool->busy_thread_idx()
            && m_task_cnt > m_parent_thread_pool->m_threshold_redistribution ) {
        uint32_t pop_task_cnt = m_task_cnt >> 1; // 分配一半出去
        Task* pTask = NULL;
        for(uint32_t i=0; i< pop_task_cnt; i++) {
            m_thread_notify.Lock();
            pTask = m_task_list.front();
            m_task_list.pop_front();
            atomic_sub(&m_task_cnt,1);
            m_thread_notify.Unlock();
            m_parent_thread_pool->AddTask(pTask);
        }
        m_parent_thread_pool->clean_busy_thread_idx();
    }
}

void WorkerThread::Execute()
{
    while (true) {
        m_thread_notify.Lock();
        // put wait in while cause there can be spurious wake up (due to signal/ENITR)
        while (m_task_list.empty()) {
            m_parent_thread_pool->setidle_thread_idx(m_thread_idx);
            m_parent_thread_pool->update_busy_thread_idx();
            m_thread_notify.Wait();
        }

        Task* pTask = m_task_list.front();
        m_task_list.pop_front();
        m_thread_notify.Unlock();

        pTask->run();
        m_parent_thread_pool->AtomicSubTaskCount();
        atomic_sub(&m_task_cnt,1);

        this->Redistribution();

        delete pTask;
    }
}

void WorkerThread::PushTask(Task* pTask)
{
    m_thread_notify.Lock();
    m_task_list.push_back(pTask);
    atomic_add(&m_task_cnt,1);
    m_thread_notify.Signal();
    m_thread_notify.Unlock();
}

uint32_t WorkerThread::task_cnt() const
{
    return m_task_cnt;
}

void WorkerThread::set_parent_thread_pool(ThreadPool *parent_thread_pool)
{
    m_parent_thread_pool = parent_thread_pool;
}

ThreadPool::ThreadPool()
{
    m_worker_size = 0;
    m_worker_list = NULL;
    m_busy_thread_idx = 0;
    m_is_set_max_task_cnt = false;
    m_max_task_cnt = 0;
    m_threshold_redistribution = 1;
}

ThreadPool::~ThreadPool()
{

}

bool ThreadPool::SetMaxTaskCount(uint32_t task_count)
{
    gettimeofday(&m_time_start, NULL);
    m_is_set_max_task_cnt = true;
    m_max_task_cnt = task_count;
    m_max_task_cnt_record = task_count;
}

void ThreadPool::AtomicSubTaskCount()
{
    if(m_is_set_max_task_cnt) {
        atomic_sub(&m_max_task_cnt, 1);
        if (m_max_task_cnt == 0) {
            gettimeofday(&m_time_stop, NULL);
            sleep(1);
            //任务完成了
            CostTime();
            _exit(0);
        }
    }
}

void ThreadPool::CostTime()
{
    int usec,sec;
    int borrow = 0;
    if (m_time_stop.tv_usec > m_time_start.tv_usec) {
        usec = m_time_stop.tv_usec - m_time_start.tv_usec;
    } else {
        borrow = 1;
        usec = m_time_stop.tv_usec+1000000 - m_time_start.tv_usec;
    }

    if (borrow == 1) {
        sec = m_time_stop.tv_sec-1 - m_time_start.tv_sec;
    }  else {
        sec = m_time_stop.tv_sec - m_time_start.tv_sec;
    }
    double tps =(1.0 * m_max_task_cnt_record ) / (1.0* sec + 1.0*usec/1000000 );
    printf("total: %d, elapsed time: %d.%06d usec, tps:=%f \n", m_max_task_cnt_record, sec, usec, tps);
}


bool ThreadPool::Init(uint32_t worker_size)
{
    if(worker_size == 0) {
        worker_size = get_nprocs();
        worker_size *=2;
        //LOG_DEBUG("worker_size %d",worker_size);
    }

    m_worker_size = worker_size;
    m_worker_list = new WorkerThread [m_worker_size];
    if (!m_worker_list) {
        return false;
    }

    for (uint32_t i = 0; i < m_worker_size; i++) {
        m_worker_list[i].set_thread_idx(i);
        m_worker_list[i].set_parent_thread_pool(this);
        m_worker_list[i].Start();
    }

    m_idle_thread_idx = 0;
    m_busy_thread_idx = m_worker_size;
    return true;
}

void ThreadPool::Destory()
{
    if(m_worker_list)
        delete [] m_worker_list;
}

void ThreadPool::SetThresholdRedistribution(uint32_t threshold)
{
    if (threshold == 0) {
        m_threshold_redistribution = 1;
    } else {
        m_threshold_redistribution = threshold;
    }
}

void ThreadPool::setidle_thread_idx(const pthread_t &idle_thread_idx)
{
    m_idle_thread_idx = idle_thread_idx;
}

uint32_t ThreadPool::busy_thread_idx() const
{
    return m_busy_thread_idx;
}

void ThreadPool::update_busy_thread_idx()
{
    uint32_t _busy_thread_idx=m_worker_size;
    uint32_t max_task_cnt=0;
    uint32_t tmp_cnt;
    for(uint32_t i=0; i<m_worker_size; i++) {
        tmp_cnt = m_worker_list[i].task_cnt();
        if(max_task_cnt < tmp_cnt) {
            max_task_cnt = tmp_cnt;
            _busy_thread_idx = i;
        }
    }
    m_busy_thread_idx = _busy_thread_idx;
}

void ThreadPool::clean_busy_thread_idx()
{
    m_busy_thread_idx = m_worker_size;
}

uint32_t ThreadPool::GetOne()
{
    // uint32_t thread_idx = random() % m_worker_size;
    // m_worker_list[thread_idx].PushTask(pTask);
    uint32_t idle_thread_idx = m_idle_thread_idx;
    uint32_t task_cnt = m_worker_list[m_idle_thread_idx].task_cnt();
    if(task_cnt > 0) {
        uint32_t tmp_cnt=0;
        // select a idle thread 选择一个空闲的线程
        for(uint32_t i=0; i<m_worker_size; i++) {
            tmp_cnt = m_worker_list[i].task_cnt();
            if(tmp_cnt == 0) {
                idle_thread_idx = i;
                break;
            }
            else {
                if(tmp_cnt < task_cnt) {
                    idle_thread_idx = i;
                    task_cnt = tmp_cnt;
                }
            }
        }
    }
    return idle_thread_idx;
}

void ThreadPool::AddTask(Task* pTask)
{
    m_worker_list[this->GetOne()].PushTask(pTask);
}

