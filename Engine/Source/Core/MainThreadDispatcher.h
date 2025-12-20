#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <thread>

namespace Sabora
{
    /**
     * @brief Thread-safe dispatcher for executing work on the main thread.
     * 
     * The MainThreadDispatcher allows any thread to queue work that must be
     * executed on the main thread. This is essential for graphics APIs like
     * OpenGL, which require all operations to be performed on the thread that
     * created the context.
     * 
     * Usage:
     * @code
     *   // From any thread, queue work for the main thread
     *   MainThreadDispatcher::Get().Dispatch([]() {
     *       // This will run on the main thread
     *       glBindBuffer(GL_ARRAY_BUFFER, vbo);
     *   });
     *   
     *   // In the main loop, process queued work
     *   MainThreadDispatcher::Get().ProcessQueue();
     * @endcode
     * 
     * This is particularly useful for:
     * - OpenGL/DirectX operations (must be on main thread)
     * - Window operations (must be on main thread)
     * - SDL operations (must be on main thread)
     * - Any work that needs to run synchronously on the main thread
     */
    class MainThreadDispatcher
    {
    public:
        /**
         * @brief Get the singleton instance of MainThreadDispatcher.
         * @return Reference to the MainThreadDispatcher instance.
         */
        [[nodiscard]] static MainThreadDispatcher& Get()
        {
            static MainThreadDispatcher instance;
            return instance;
        }

        /**
         * @brief Queue a function to be executed on the main thread.
         * @param func The function to execute on the main thread.
         * 
         * This method is thread-safe and can be called from any thread.
         * The function will be executed the next time ProcessQueue() is called
         * on the main thread.
         * 
         * @note Functions are executed in the order they were queued (FIFO).
         * @note The function will be copied, so capture by value if needed.
         */
        void Dispatch(std::function<void()> func);

        /**
         * @brief Queue a function to be executed on the main thread and wait for completion.
         * @param func The function to execute on the main thread.
         * 
         * This method is thread-safe and can be called from any thread.
         * It will block until the function has been executed on the main thread.
         * 
         * @warning Use with caution - this will block the calling thread until
         *          the main thread processes the queue. Only use when you need
         *          synchronous execution.
         * 
         * @note This uses a condition variable to wait for completion.
         */
        void DispatchSync(std::function<void()> func);

        /**
         * @brief Process all queued work on the main thread.
         * 
         * This should be called once per frame on the main thread (typically
         * in the Application::Run() loop). It executes all queued functions
         * in the order they were added.
         * 
         * @note This method is NOT thread-safe and should only be called
         *       from the main thread.
         */
        void ProcessQueue();

        /**
         * @brief Get the number of queued functions waiting to be executed.
         * @return The number of queued functions.
         * 
         * This is useful for monitoring queue size and detecting potential
         * performance issues if the queue grows too large.
         */
        [[nodiscard]] size_t GetQueueSize() const noexcept;

        /**
         * @brief Clear all queued work without executing it.
         * 
         * This is useful for cleanup or when you want to discard
         * pending work. Only call this from the main thread.
         */
        void ClearQueue();

    private:
        MainThreadDispatcher();
        ~MainThreadDispatcher() = default;

        // Non-copyable, non-movable
        MainThreadDispatcher(const MainThreadDispatcher&) = delete;
        MainThreadDispatcher& operator=(const MainThreadDispatcher&) = delete;
        MainThreadDispatcher(MainThreadDispatcher&&) = delete;
        MainThreadDispatcher& operator=(MainThreadDispatcher&&) = delete;

        /**
         * @brief Check if the current thread is the main thread.
         * @return True if we're on the main thread.
         */
        [[nodiscard]] bool IsMainThread() const noexcept;

        // Thread-safe queue for work items
        mutable std::mutex m_QueueMutex;
        std::queue<std::function<void()>> m_WorkQueue;
        
        // Main thread ID for detection
        std::thread::id m_MainThreadId;

        // Synchronous dispatch support
        struct SyncWorkItem
        {
            std::function<void()> func;
            std::atomic<bool> completed{false};
        };

        mutable std::mutex m_SyncQueueMutex;
        std::queue<std::shared_ptr<SyncWorkItem>> m_SyncWorkQueue;
        mutable std::condition_variable m_SyncCondition;
    };

} // namespace Sabora
