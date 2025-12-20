#include "pch.h"
#include "MainThreadDispatcher.h"
#include <thread>

namespace Sabora
{
    MainThreadDispatcher::MainThreadDispatcher() : m_MainThreadId(std::this_thread::get_id())
    {
    }

    bool MainThreadDispatcher::IsMainThread() const noexcept
    {
        return std::this_thread::get_id() == m_MainThreadId;
    }

    void MainThreadDispatcher::Dispatch(std::function<void()> func)
    {
        if (!func)
        {
            return;
        }

        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_WorkQueue.push(std::move(func));
    }

    void MainThreadDispatcher::DispatchSync(std::function<void()> func)
    {
        if (!func)
        {
            return;
        }

        // If we're already on the main thread, execute directly to avoid deadlock
        if (IsMainThread())
        {
            func();
            return;
        }

        // Create a shared work item that we can wait on
        auto workItem = std::make_shared<SyncWorkItem>();
        workItem->func = std::move(func);

        // Add to sync queue
        {
            std::lock_guard<std::mutex> lock(m_SyncQueueMutex);
            m_SyncWorkQueue.push(workItem);
        }

        // Notify main thread that work is available
        m_SyncCondition.notify_one();

        // Wait for completion using condition variable for better performance
        std::unique_lock<std::mutex> lock(m_SyncQueueMutex);
        m_SyncCondition.wait(lock, [workItem]() {
            return workItem->completed.load(std::memory_order_acquire);
        });
    }

    void MainThreadDispatcher::ProcessQueue()
    {
        // Collect all async work items first (minimize lock time)
        std::vector<std::function<void()>> asyncWork;
        {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            asyncWork.reserve(m_WorkQueue.size());
            while (!m_WorkQueue.empty())
            {
                asyncWork.push_back(std::move(m_WorkQueue.front()));
                m_WorkQueue.pop();
            }
        }

        // Execute async work outside the lock
        for (auto& func : asyncWork)
        {
            func();
        }

        // Collect all sync work items
        std::vector<std::shared_ptr<SyncWorkItem>> syncWork;
        {
            std::lock_guard<std::mutex> lock(m_SyncQueueMutex);
            syncWork.reserve(m_SyncWorkQueue.size());
            while (!m_SyncWorkQueue.empty())
            {
                syncWork.push_back(m_SyncWorkQueue.front());
                m_SyncWorkQueue.pop();
            }
        }

        // Execute sync work outside the lock
        for (auto& workItem : syncWork)
        {
            // Execute the function
            workItem->func();

            // Mark as completed
            workItem->completed.store(true, std::memory_order_release);
        }

        // Notify all waiting threads after all work is done
        if (!syncWork.empty())
        {
            std::lock_guard<std::mutex> lock(m_SyncQueueMutex);
            m_SyncCondition.notify_all();
        }
    }

    size_t MainThreadDispatcher::GetQueueSize() const noexcept
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        return m_WorkQueue.size();
    }

    void MainThreadDispatcher::ClearQueue()
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        
        // Clear the queue by swapping with an empty queue
        std::queue<std::function<void()>> empty;
        m_WorkQueue.swap(empty);
    }

} // namespace Sabora
