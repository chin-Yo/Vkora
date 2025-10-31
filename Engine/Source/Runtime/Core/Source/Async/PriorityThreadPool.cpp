#include "Async/PriorityThreadPool.hpp"

#include <cassert>

PriorityThreadPool::PriorityThreadPool(size_t workerThreads)
    : shutdown(false), pool(workerThreads)
{
    if (workerThreads == 0) workerThreads = 1;
    dispatcherThread = std::thread(&PriorityThreadPool::DispatchLoop, this);
}

PriorityThreadPool::~PriorityThreadPool()
{
    Shutdown();
}

void PriorityThreadPool::Submit(Priority priority, std::function<void()> task)
{
    assert(task);
    QueuedTask qt{static_cast<int>(priority), std::move(task)};

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.push(std::move(qt));
    }
    cv.notify_one();
}

void PriorityThreadPool::Shutdown()
{
    if (shutdown.exchange(true)) return;

    cv.notify_all();
    if (dispatcherThread.joinable())
    {
        dispatcherThread.join();
    }
    pool.stop(true);
}

void PriorityThreadPool::DispatchLoop()
{
    while (!shutdown.load())
    {
        QueuedTask task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this]
            {
                return shutdown.load() || !taskQueue.empty();
            });

            if (shutdown.load() && taskQueue.empty()) break;

            if (!taskQueue.empty())
            {
                task = std::move(const_cast<QueuedTask&>(taskQueue.top()));
                taskQueue.pop();
            }
        }

        if (task.task)
        {
            pool.push([t = std::move(task.task)](int /*id*/)
            {
                t();
            });
        }
    }
}
