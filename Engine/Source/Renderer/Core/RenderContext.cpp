#include "pch.h"
#include "Renderer/Core/RenderContext.h"
#include "Core/Window.h"
#include "Core/Log.h"

namespace Sabora
{
    Result<std::unique_ptr<RenderContext>> RenderContext::Create(
        Window* window,
        [[maybe_unused]] RenderContext* shareContext)
    {
        if (!window || !window->IsValid())
        {
            return Result<std::unique_ptr<RenderContext>>::Failure(
                ErrorCode::CoreNullPointer,
                "Window is null or invalid"
            );
        }

        // This is a factory method that will be implemented by specific API contexts
        // For now, it's a placeholder - actual implementation will be in OpenGLContext, etc.
        SB_CORE_ERROR("RenderContext::Create() called but no API implementation available. Use RendererManager to create a renderer.");
        
        return Result<std::unique_ptr<RenderContext>>::Failure(
            ErrorCode::CoreNotImplemented,
            "RenderContext::Create() must be called through RendererManager"
        );
    }

} // namespace Sabora
