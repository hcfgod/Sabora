#pragma once

#include "Core/Result.h"
#include "Renderer/Commands/RenderCommands.h"
#include <memory>
#include <queue>
#include <mutex>
#include <vector>

namespace Sabora
{
    // Forward declaration
    class Renderer;

    /**
     * @brief Thread-safe command queue for render commands.
     * 
     * RenderCommandQueue allows rendering commands to be recorded from any thread
     * and executed later on the main thread. Commands are stored in a thread-safe
     * queue and executed in order.
     * 
     * Thread Safety:
     * - All methods are thread-safe
     * - Commands can be recorded from any thread
     * - Commands must be executed on the main thread
     * 
     * Usage:
     * @code
     *   // From any thread, record commands
     *   RenderCommandQueue queue;
     *   queue.RecordClear(ClearFlags::Color | ClearFlags::Depth, color, depthStencil);
     *   queue.RecordDraw(vertexCount);
     *   
     *   // On main thread, execute all commands
     *   auto result = queue.ExecuteAll(renderer);
     * @endcode
     */
    class RenderCommandQueue
    {
    public:
        RenderCommandQueue() = default;
        ~RenderCommandQueue() = default;

        // Non-copyable, movable
        RenderCommandQueue(const RenderCommandQueue&) = delete;
        RenderCommandQueue& operator=(const RenderCommandQueue&) = delete;
        RenderCommandQueue(RenderCommandQueue&&) noexcept = default;
        RenderCommandQueue& operator=(RenderCommandQueue&&) noexcept = default;

        //==========================================================================
        // Command Recording (Thread-Safe)
        //==========================================================================

        /**
         * @brief Record a clear command.
         * @param flags Clear flags.
         * @param color Clear color.
         * @param depthStencil Clear depth/stencil.
         */
        void RecordClear(
            ClearFlags flags,
            const ClearColor& color = ClearColor{},
            const ClearDepthStencil& depthStencil = ClearDepthStencil{}
        );

        /**
         * @brief Record a set viewport command.
         * @param viewport Viewport configuration.
         */
        void RecordSetViewport(const Viewport& viewport);

        /**
         * @brief Record a set scissor command.
         * @param scissor Scissor rectangle.
         */
        void RecordSetScissor(const ScissorRect& scissor);

        /**
         * @brief Record a set framebuffer command.
         * @param framebuffer Framebuffer to bind (nullptr for default).
         */
        void RecordSetFramebuffer(Framebuffer* framebuffer);

        /**
         * @brief Record a set pipeline state command.
         * @param pipelineState Pipeline state to bind.
         */
        void RecordSetPipelineState(PipelineState* pipelineState);

        /**
         * @brief Record a draw command.
         * @param vertexCount Number of vertices.
         * @param instanceCount Number of instances (1 for non-instanced).
         * @param firstVertex Index of first vertex.
         * @param firstInstance Index of first instance (0 for non-instanced).
         */
        void RecordDraw(
            uint32_t vertexCount,
            uint32_t instanceCount = 1,
            uint32_t firstVertex = 0,
            uint32_t firstInstance = 0
        );

        /**
         * @brief Record a draw indexed command.
         * @param indexCount Number of indices.
         * @param instanceCount Number of instances (1 for non-instanced).
         * @param firstIndex Index of first index.
         * @param vertexOffset Offset to add to vertex indices.
         * @param firstInstance Index of first instance (0 for non-instanced).
         */
        void RecordDrawIndexed(
            uint32_t indexCount,
            uint32_t instanceCount = 1,
            uint32_t firstIndex = 0,
            int32_t vertexOffset = 0,
            uint32_t firstInstance = 0
        );

        /**
         * @brief Record a custom command.
         * @param func Function to execute.
         */
        void RecordCustom(std::function<Result<void>(Renderer*)> func);

        //==========================================================================
        // Command Execution (Main Thread Only)
        //==========================================================================

        /**
         * @brief Execute all queued commands.
         * @param renderer The renderer to execute commands on.
         * @return Result indicating success or failure.
         * 
         * This executes all commands in the queue in order. After execution,
         * the queue is cleared.
         * 
         * @note This must be called from the main thread.
         */
        [[nodiscard]] Result<void> ExecuteAll(Renderer* renderer);

        /**
         * @brief Get the number of queued commands.
         * @return Number of commands in the queue.
         * 
         * Thread Safety: This method is thread-safe.
         */
        [[nodiscard]] size_t GetCommandCount() const;

        /**
         * @brief Clear all queued commands without executing them.
         * 
         * Thread Safety: This method is thread-safe.
         */
        void Clear();

    private:
        /**
         * @brief Add a command to the queue (thread-safe).
         * @param command Command to add.
         */
        void AddCommand(std::unique_ptr<RenderCommand> command);

        mutable std::mutex m_Mutex;
        std::queue<std::unique_ptr<RenderCommand>> m_Commands;
    };

} // namespace Sabora
