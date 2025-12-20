#include "pch.h"
#include "Renderer/Commands/RenderCommandQueue.h"
#include "Renderer/Commands/RenderCommands.h"
#include "Renderer/Core/Renderer.h"
#include "Core/Log.h"

namespace Sabora
{
    void RenderCommandQueue::RecordClear(
        ClearFlags flags,
        const ClearColor& color,
        const ClearDepthStencil& depthStencil)
    {
        AddCommand(std::make_unique<ClearCommand>(flags, color, depthStencil));
    }

    void RenderCommandQueue::RecordSetViewport(const Viewport& viewport)
    {
        AddCommand(std::make_unique<SetViewportCommand>(viewport));
    }

    void RenderCommandQueue::RecordSetScissor(const ScissorRect& scissor)
    {
        AddCommand(std::make_unique<SetScissorCommand>(scissor));
    }

    void RenderCommandQueue::RecordSetFramebuffer(Framebuffer* framebuffer)
    {
        AddCommand(std::make_unique<SetFramebufferCommand>(framebuffer));
    }

    void RenderCommandQueue::RecordSetPipelineState(PipelineState* pipelineState)
    {
        AddCommand(std::make_unique<SetPipelineStateCommand>(pipelineState));
    }

    void RenderCommandQueue::RecordDraw(
        uint32_t vertexCount,
        uint32_t instanceCount,
        uint32_t firstVertex,
        uint32_t firstInstance)
    {
        AddCommand(std::make_unique<DrawCommand>(vertexCount, instanceCount, firstVertex, firstInstance));
    }

    void RenderCommandQueue::RecordDrawIndexed(
        uint32_t indexCount,
        uint32_t instanceCount,
        uint32_t firstIndex,
        int32_t vertexOffset,
        uint32_t firstInstance)
    {
        AddCommand(std::make_unique<DrawIndexedCommand>(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance));
    }

    void RenderCommandQueue::RecordCustom(std::function<Result<void>(Renderer*)> func)
    {
        AddCommand(std::make_unique<CustomCommand>(std::move(func)));
    }

    Result<void> RenderCommandQueue::ExecuteAll(Renderer* renderer)
    {
        if (!renderer)
        {
            return Result<void>::Failure(
                ErrorCode::CoreNullPointer,
                "Renderer is null"
            );
        }

        // Collect all commands (minimize lock time)
        std::vector<std::unique_ptr<RenderCommand>> commands;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            commands.reserve(m_Commands.size());
            while (!m_Commands.empty())
            {
                commands.push_back(std::move(m_Commands.front()));
                m_Commands.pop();
            }
        }

        // Execute commands outside the lock
        for (auto& command : commands)
        {
            auto result = command->Execute(renderer);
            if (result.IsFailure())
            {
                SB_RENDERER_ERROR("Render command execution failed: {}", result.GetError().ToString());
                return result;
            }
        }

        return Result<void>::Success();
    }

    size_t RenderCommandQueue::GetCommandCount() const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Commands.size();
    }

    void RenderCommandQueue::Clear()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        while (!m_Commands.empty())
        {
            m_Commands.pop();
        }
    }

    void RenderCommandQueue::AddCommand(std::unique_ptr<RenderCommand> command)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Commands.push(std::move(command));
    }

} // namespace Sabora
