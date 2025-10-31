#pragma once

#include <ctpl.h>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

class PriorityThreadPool
{
public:
    enum class Priority : int
    {
        High = 0,
        Normal = 1,
        Low = 2
    };

    explicit PriorityThreadPool(size_t workerThreads = std::thread::hardware_concurrency());

    ~PriorityThreadPool();

    PriorityThreadPool(const PriorityThreadPool&) = delete;
    PriorityThreadPool& operator=(const PriorityThreadPool&) = delete;

    void Submit(Priority priority, std::function<void()> task);

    void WaitAll()
    {
        pool.stop(true);
    }

    void Shutdown();

private:
    struct QueuedTask
    {
        int priority;
        std::function<void()> task;

        bool operator<(const QueuedTask& other) const
        {
            return priority > other.priority;
        }
    };

    std::priority_queue<QueuedTask> taskQueue;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::atomic<bool> shutdown{false};

    ctpl::thread_pool pool;
    std::thread dispatcherThread;

    void DispatchLoop();
};
