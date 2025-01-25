#include "pch.h"
#include "ThreadPool.h"

ThreadPool::ThreadPool(int min_num_threads, int max_num_threads) :
    _min_num_threads(min_num_threads),
    _max_num_threads(max_num_threads)
{
    InitializeThreadpoolEnvironment(&_callback_environment);
    _pool = CreateThreadpool(nullptr);
    if (!_pool)
    {
        throw std::runtime_error("Failed to create thread pool.");
    }
    SetThreadpoolThreadMaximum(_pool,
        static_cast<DWORD>(max_num_threads)); // Set maximum number of threads

    SetThreadpoolThreadMinimum(_pool,
        static_cast<DWORD>(min_num_threads)); // Set minimum number of threads

    _cleanup_group = CreateThreadpoolCleanupGroup();
    if (!_cleanup_group)
    {
        CloseThreadpool(_pool);
        throw std::runtime_error("Failed to create cleanup group.");
    }
    SetThreadpoolCallbackPool(&_callback_environment, _pool);
    SetThreadpoolCallbackCleanupGroup(&_callback_environment, _cleanup_group, nullptr);
}

ThreadPool::~ThreadPool()
{
    CloseThreadpoolCleanupGroupMembers(_cleanup_group, FALSE, nullptr);
    CloseThreadpoolCleanupGroup(_cleanup_group);
    CloseThreadpool(_pool);
    DestroyThreadpoolEnvironment(&_callback_environment);
}

void ThreadPool::add_task(std::function<void()> task)
{
    PTP_WORK work = CreateThreadpoolWork(work_callback,
        new std::function<void()>(task), &_callback_environment);
    if (!work)
    {
        throw std::runtime_error("Failed to create thread pool work object.");
    }
    _work_items.push_back(work);
    SubmitThreadpoolWork(work);
}

void ThreadPool::wait_for_tasks_to_complete()
{
    for (PTP_WORK work : _work_items)
    {
        WaitForThreadpoolWorkCallbacks(work, FALSE);
        CloseThreadpoolWork(work);
    }
    _work_items.clear();
}

void CALLBACK ThreadPool::work_callback(PTP_CALLBACK_INSTANCE instance, PVOID parameter, PTP_WORK work)
{
    std::function<void()>* task = static_cast<std::function<void()>*>(parameter);
    (*task)();
    delete task;
}

int ThreadPool::get_min_num_threads() const
{
    return this->_min_num_threads;
}

int ThreadPool::get_max_num_threads() const
{
    return this->_max_num_threads;
}