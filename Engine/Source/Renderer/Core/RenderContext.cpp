#include "pch.h"
#include "Renderer/Core/RenderContext.h"
#include "Renderer/RendererManager.h"
#include "Renderer/OpenGL/OpenGLContext.h"
#include "Core/Window.h"
#include "Core/Log.h"

namespace Sabora
{
    Result<std::unique_ptr<RenderContext>> RenderContext::Create(
        Window* window,
        RenderContext* shareContext)
    {
        if (!window || !window->IsValid())
        {
            return Result<std::unique_ptr<RenderContext>>::Failure(
                ErrorCode::CoreNullPointer,
                "Window is null or invalid"
            );
        }

        // Check if OpenGL is available and delegate to OpenGLContext
        // This provides a unified interface while maintaining the abstraction
        if (RendererManager::IsAPIAvailable(RendererAPI::OpenGL))
        {
            auto contextResult = OpenGLContext::Create(window, shareContext);
            if (contextResult.IsFailure())
            {
                return Result<std::unique_ptr<RenderContext>>::Failure(contextResult.GetError());
            }

            // Convert OpenGLContext to RenderContext (base class pointer)
            std::unique_ptr<RenderContext> context = std::move(contextResult).Value();
            return Result<std::unique_ptr<RenderContext>>::Success(std::move(context));
        }

        // No supported API available
        return Result<std::unique_ptr<RenderContext>>::Failure(
            ErrorCode::CoreNotImplemented,
            "No supported graphics API is available. OpenGL is not available on this system."
        );
    }

} // namespace Sabora
