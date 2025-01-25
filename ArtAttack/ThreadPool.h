#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <windows.h>
#include <functional>
#include <vector>
#include <iostream>

class ThreadPool
{
public:
    ThreadPool(int min_num_threads, int max_num_threads);
    ~ThreadPool();

    void add_task(std::function<void()> task);
    void wait_for_tasks_to_complete();

    int get_min_num_threads() const;
    int get_max_num_threads() const;

private:
    static void CALLBACK work_callback(PTP_CALLBACK_INSTANCE instance,
        PVOID parameter, PTP_WORK work);

    int _min_num_threads = -1;
    int _max_num_threads = -1;
    std::vector<PTP_WORK> _work_items;
    TP_CALLBACK_ENVIRON _callback_environment = {};
    PTP_POOL _pool = nullptr;
    PTP_CLEANUP_GROUP _cleanup_group = nullptr;
};

#endif // !THREAD_POOL_H
