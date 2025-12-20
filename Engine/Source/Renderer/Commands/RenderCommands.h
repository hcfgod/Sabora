#pragma once

#include "Renderer/Core/RendererTypes.h"
#include "Core/Result.h"
#include "Renderer/Resources/PipelineState.h"
#include "Renderer/Resources/Framebuffer.h"
#include "Renderer/Core/Renderer.h"
#include <cstdint>
#include <memory>
#include <functional>

namespace Sabora
{
    // Forward declarations
    class Buffer;
    class Texture;

    /**
     * @brief Base class for all render commands.
     * 
     * Render commands are recorded into a command queue and executed later
     * on the main thread. This allows rendering operations to be recorded
     * from any thread safely.
     */
    class RenderCommand
    {
    public:
        virtual ~RenderCommand() = default;

        /**
         * @brief Execute this command.
         * @param renderer The renderer to execute the command on.
         * @return Result indicating success or failure.
         */
        [[nodiscard]] virtual Result<void> Execute(Renderer* renderer) = 0;

    protected:
        RenderCommand() = default;
    };

    /**
     * @brief Command to clear render targets.
     */
    class ClearCommand : public RenderCommand
    {
    public:
        ClearCommand(ClearFlags flags, const ClearColor& color, const ClearDepthStencil& depthStencil)
            : m_Flags(flags)
            , m_Color(color)
            , m_DepthStencil(depthStencil)
        {
        }

        [[nodiscard]] Result<void> Execute(Renderer* renderer) override
        {
            return renderer->Clear(m_Flags, m_Color, m_DepthStencil);
        }

    private:
        ClearFlags m_Flags;
        ClearColor m_Color;
        ClearDepthStencil m_DepthStencil;
    };

    /**
     * @brief Command to set the viewport.
     */
    class SetViewportCommand : public RenderCommand
    {
    public:
        explicit SetViewportCommand(const Viewport& viewport)
            : m_Viewport(viewport)
        {
        }

        [[nodiscard]] Result<void> Execute(Renderer* renderer) override
        {
            return renderer->SetViewport(m_Viewport);
        }

    private:
        Viewport m_Viewport;
    };

    /**
     * @brief Command to set the scissor rectangle.
     */
    class SetScissorCommand : public RenderCommand
    {
    public:
        explicit SetScissorCommand(const ScissorRect& scissor)
            : m_Scissor(scissor)
        {
        }

        [[nodiscard]] Result<void> Execute(Renderer* renderer) override
        {
            return renderer->SetScissor(m_Scissor);
        }

    private:
        ScissorRect m_Scissor;
    };

    /**
     * @brief Command to set the active framebuffer.
     */
    class SetFramebufferCommand : public RenderCommand
    {
    public:
        explicit SetFramebufferCommand(Framebuffer* framebuffer)
            : m_Framebuffer(framebuffer)
        {
        }

        [[nodiscard]] Result<void> Execute(Renderer* renderer) override
        {
            return renderer->SetFramebuffer(m_Framebuffer);
        }

    private:
        Framebuffer* m_Framebuffer;
    };

    /**
     * @brief Command to set the active pipeline state.
     */
    class SetPipelineStateCommand : public RenderCommand
    {
    public:
        explicit SetPipelineStateCommand(PipelineState* pipelineState)
            : m_PipelineState(pipelineState)
        {
        }

        [[nodiscard]] Result<void> Execute(Renderer* renderer) override
        {
            return renderer->SetPipelineState(m_PipelineState);
        }

    private:
        PipelineState* m_PipelineState;
    };

    /**
     * @brief Command to draw primitives.
     */
    class DrawCommand : public RenderCommand
    {
    public:
        DrawCommand(
            uint32_t vertexCount,
            uint32_t instanceCount = 1,
            uint32_t firstVertex = 0,
            uint32_t firstInstance = 0
        )
            : m_VertexCount(vertexCount)
            , m_InstanceCount(instanceCount)
            , m_FirstVertex(firstVertex)
            , m_FirstInstance(firstInstance)
        {
        }

        [[nodiscard]] Result<void> Execute(Renderer* renderer) override
        {
            return renderer->Draw(m_VertexCount, m_InstanceCount, m_FirstVertex, m_FirstInstance);
        }

    private:
        uint32_t m_VertexCount;
        uint32_t m_InstanceCount;
        uint32_t m_FirstVertex;
        uint32_t m_FirstInstance;
    };

    /**
     * @brief Command to draw indexed primitives.
     */
    class DrawIndexedCommand : public RenderCommand
    {
    public:
        DrawIndexedCommand(
            uint32_t indexCount,
            uint32_t instanceCount = 1,
            uint32_t firstIndex = 0,
            int32_t vertexOffset = 0,
            uint32_t firstInstance = 0
        )
            : m_IndexCount(indexCount)
            , m_InstanceCount(instanceCount)
            , m_FirstIndex(firstIndex)
            , m_VertexOffset(vertexOffset)
            , m_FirstInstance(firstInstance)
        {
        }

        [[nodiscard]] Result<void> Execute(Renderer* renderer) override
        {
            return renderer->DrawIndexed(m_IndexCount, m_InstanceCount, m_FirstIndex, m_VertexOffset, m_FirstInstance);
        }

    private:
        uint32_t m_IndexCount;
        uint32_t m_InstanceCount;
        uint32_t m_FirstIndex;
        int32_t m_VertexOffset;
        uint32_t m_FirstInstance;
    };

    /**
     * @brief Command to execute a custom function.
     * 
     * This allows arbitrary rendering code to be executed as a command.
     * Useful for API-specific operations that don't fit into the standard
     * command types.
     */
    class CustomCommand : public RenderCommand
    {
    public:
        explicit CustomCommand(std::function<Result<void>(Renderer*)> func)
            : m_Func(std::move(func))
        {
        }

        [[nodiscard]] Result<void> Execute(Renderer* renderer) override
        {
            if (m_Func)
            {
                return m_Func(renderer);
            }
            return Result<void>::Success();
        }

    private:
        std::function<Result<void>(Renderer*)> m_Func;
    };

} // namespace Sabora
