#pragma once

#include "Renderer/Resources/Buffer.h"
#include <cstdint>

namespace Sabora
{
    /**
     * @brief OpenGL implementation of the Buffer interface.
     * 
     * OpenGLBuffer provides OpenGL-specific buffer operations including
     * vertex buffers, index buffers, uniform buffers, storage buffers,
     * and indirect command buffers.
     * 
     * Thread Safety:
     * - Creation/destruction should use MainThreadDispatcher
     * - Operations should be called from main thread or queued
     */
    class OpenGLBuffer : public Buffer
    {
    public:
        /**
         * @brief Create a new OpenGL buffer.
         * @param type Buffer type (vertex, index, uniform, etc.).
         * @param size Buffer size in bytes.
         * @param usage Buffer usage hint (static, dynamic, stream).
         * @param data Optional initial data (nullptr for uninitialized).
         * @return Result containing the created buffer, or an error.
         * 
         * This factory method creates and initializes the buffer.
         * The actual OpenGL buffer creation happens on the main thread.
         */
        [[nodiscard]] static Result<std::unique_ptr<OpenGLBuffer>> Create(
            BufferType type,
            size_t size,
            BufferUsage usage,
            const void* data = nullptr
        );

        /**
         * @brief Destructor - destroys the OpenGL buffer.
         */
        ~OpenGLBuffer() override;

        // Non-copyable, movable
        OpenGLBuffer(const OpenGLBuffer&) = delete;
        OpenGLBuffer& operator=(const OpenGLBuffer&) = delete;
        OpenGLBuffer(OpenGLBuffer&& other) noexcept;
        OpenGLBuffer& operator=(OpenGLBuffer&& other) noexcept;

        //==========================================================================
        // Buffer Interface Implementation
        //==========================================================================

        [[nodiscard]] BufferType GetType() const override { return m_Type; }
        [[nodiscard]] size_t GetSize() const override { return m_Size; }
        [[nodiscard]] BufferUsage GetUsage() const override { return m_Usage; }

        [[nodiscard]] Result<void> UpdateData(
            const void* data,
            size_t size,
            size_t offset = 0
        ) override;

        [[nodiscard]] Result<void*> Map(MemoryAccess access) override;
        [[nodiscard]] Result<void> Unmap() override;

        [[nodiscard]] void* GetNativeHandle() const override;
        [[nodiscard]] bool IsValid() const override;

    private:
        /**
         * @brief Private constructor - use Create() factory method.
         * @param bufferId OpenGL buffer ID.
         * @param type Buffer type.
         * @param size Buffer size in bytes.
         * @param usage Buffer usage hint.
         */
        OpenGLBuffer(
            uint32_t bufferId,
            BufferType type,
            size_t size,
            BufferUsage usage
        ) noexcept;

        /**
         * @brief Get OpenGL buffer target for this buffer type.
         * @return OpenGL buffer target enum.
         */
        [[nodiscard]] uint32_t GetGLTarget() const;

        /**
         * @brief Get OpenGL usage hint for this buffer usage.
         * @return OpenGL usage enum.
         */
        [[nodiscard]] uint32_t GetGLUsage() const;

        /**
         * @brief Get OpenGL access mode for memory mapping.
         * @param access Memory access mode.
         * @return OpenGL access enum.
         */
        [[nodiscard]] uint32_t GetGLAccess(MemoryAccess access) const;

        uint32_t m_BufferId = 0;      ///< OpenGL buffer ID
        BufferType m_Type;            ///< Buffer type
        size_t m_Size = 0;            ///< Buffer size in bytes
        BufferUsage m_Usage;          ///< Buffer usage hint
        bool m_Mapped = false;         ///< Whether buffer is currently mapped
    };

} // namespace Sabora
