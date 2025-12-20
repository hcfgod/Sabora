#pragma once

#include "Core/Result.h"
#include "Renderer/Core/RendererTypes.h"
#include <cstdint>
#include <memory>

namespace Sabora
{
    /**
     * @brief Memory access modes for buffer mapping.
     */
    enum class MemoryAccess : uint8_t
    {
        Read = 0,
        Write,
        ReadWrite,
    };

    /**
     * @brief Abstract GPU buffer interface.
     * 
     * Buffer represents a GPU buffer that can store vertex data, index data,
     * uniform data, or storage data. It provides a unified interface for
     * buffer operations across different graphics APIs.
     * 
     * Thread Safety:
     * - Buffer creation/destruction is thread-safe (uses MainThreadDispatcher)
     * - Buffer updates should be done from the main thread or via MainThreadDispatcher
     * - Read operations are thread-safe
     */
    class Buffer
    {
    public:
        virtual ~Buffer() = default;

        /**
         * @brief Get the buffer type.
         * @return The buffer type (vertex, index, uniform, etc.).
         */
        [[nodiscard]] virtual BufferType GetType() const = 0;

        /**
         * @brief Get the buffer size in bytes.
         * @return Buffer size in bytes.
         */
        [[nodiscard]] virtual size_t GetSize() const = 0;

        /**
         * @brief Get the buffer usage hint.
         * @return The buffer usage (static, dynamic, stream).
         */
        [[nodiscard]] virtual BufferUsage GetUsage() const = 0;

        /**
         * @brief Update buffer data.
         * @param data Pointer to the new data.
         * @param size Size of the data in bytes.
         * @param offset Offset into the buffer in bytes (0 for full update).
         * @return Result indicating success or failure.
         * 
         * This updates the buffer contents. For dynamic buffers, this can be
         * called multiple times. For static buffers, this should typically
         * only be called once after creation.
         */
        [[nodiscard]] virtual Result<void> UpdateData(
            const void* data,
            size_t size,
            size_t offset = 0
        ) = 0;

        /**
         * @brief Map buffer memory for CPU access.
         * @param access Access mode (read, write, read-write).
         * @return Result containing a pointer to mapped memory, or an error.
         * 
         * This maps the buffer memory to CPU-accessible memory. The buffer
         * must be unmapped before it can be used by the GPU again.
         * 
         * @note Not all APIs support mapping. Some may require UpdateData instead.
         */
        [[nodiscard]] virtual Result<void*> Map(MemoryAccess access) = 0;

        /**
         * @brief Unmap buffer memory.
         * @return Result indicating success or failure.
         * 
         * This unmaps the buffer memory, making it available for GPU use again.
         */
        [[nodiscard]] virtual Result<void> Unmap() = 0;

        /**
         * @brief Get the native API handle (for advanced use).
         * @return Opaque pointer to the native buffer handle.
         */
        [[nodiscard]] virtual void* GetNativeHandle() const = 0;

        /**
         * @brief Check if the buffer is valid.
         * @return True if the buffer is valid and can be used.
         */
        [[nodiscard]] virtual bool IsValid() const = 0;

    protected:
        Buffer() = default;

        // Non-copyable, movable
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&&) noexcept = default;
        Buffer& operator=(Buffer&&) noexcept = default;
    };

} // namespace Sabora
