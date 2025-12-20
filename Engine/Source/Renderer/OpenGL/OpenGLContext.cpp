#include "pch.h"
#include "Renderer/OpenGL/OpenGLContext.h"
#include "Core/Window.h"
#include "Core/Log.h"
#include <SDL3/SDL.h>
#include <glad/gl.h>

namespace Sabora
{
    // Thread-local storage for current context
    thread_local OpenGLContext* OpenGLContext::s_CurrentContext = nullptr;

    Result<std::unique_ptr<OpenGLContext>> OpenGLContext::Create(
        Window* window,
        RenderContext* shareContext)
    {
        if (!window || !window->IsValid())
        {
            return Result<std::unique_ptr<OpenGLContext>>::Failure(
                ErrorCode::CoreNullPointer,
                "Window is null or invalid"
            );
        }

        SDL_Window* sdlWindow = window->GetSDLWindow();
        if (!sdlWindow)
        {
            return Result<std::unique_ptr<OpenGLContext>>::Failure(
                ErrorCode::CoreNullPointer,
                "SDL window handle is null"
            );
        }

        // Set OpenGL attributes before creating context
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        // Enable debug context in debug builds
        #ifdef SABORA_DEBUG
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
        #endif

        // Get share context if provided
        SDL_GLContext shareGLContext = nullptr;
        if (shareContext)
        {
            OpenGLContext* glShareContext = dynamic_cast<OpenGLContext*>(shareContext);
            if (glShareContext)
            {
                shareGLContext = glShareContext->m_GLContext;
            }
        }

        // Create OpenGL context
        SDL_GLContext glContext = SDL_GL_CreateContext(sdlWindow);
        if (!glContext)
        {
            return Result<std::unique_ptr<OpenGLContext>>::Failure(
                ErrorCode::GraphicsContextCreationFailed,
                fmt::format("Failed to create OpenGL context: {}", SDL_GetError())
            );
        }

        // Make context current temporarily to load GLAD
        if (!SDL_GL_MakeCurrent(sdlWindow, glContext))
        {
            SDL_GL_DestroyContext(glContext);
            return Result<std::unique_ptr<OpenGLContext>>::Failure(
                ErrorCode::GraphicsContextCreationFailed,
                fmt::format("Failed to make OpenGL context current: {}", SDL_GetError())
            );
        }

        // Get OpenGL version from SDL attributes (before creating context object)
        int32_t majorVersion = 0, minorVersion = 0;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &majorVersion);
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minorVersion);

        // Create context object
        auto context = std::unique_ptr<OpenGLContext>(new OpenGLContext(sdlWindow, glContext));
        context->m_MajorVersion = majorVersion;
        context->m_MinorVersion = minorVersion;

        // Load GLAD
        auto gladResult = context->LoadGLAD();
        if (gladResult.IsFailure())
        {
            SDL_GL_DestroyContext(glContext);
            return Result<std::unique_ptr<OpenGLContext>>::Failure(gladResult.GetError());
        }

        SB_CORE_INFO("OpenGL context created successfully: {}.{}", majorVersion, minorVersion);

        // Release current context (will be made current when needed)
        SDL_GL_MakeCurrent(sdlWindow, nullptr);

        return Result<std::unique_ptr<OpenGLContext>>::Success(std::move(context));
    }

    OpenGLContext::OpenGLContext(SDL_Window* window, SDL_GLContext glContext)
        : m_Window(window)
        , m_GLContext(glContext)
    {
    }

    OpenGLContext::~OpenGLContext()
    {
        if (m_GLContext)
        {
            // Make sure we're not current on any thread before deleting
            if (s_CurrentContext == this)
            {
                SDL_GL_MakeCurrent(m_Window, nullptr);
                s_CurrentContext = nullptr;
            }

            SDL_GL_DestroyContext(m_GLContext);
            m_GLContext = nullptr;
        }
    }

    OpenGLContext::OpenGLContext(OpenGLContext&& other) noexcept
        : m_Window(other.m_Window)
        , m_GLContext(other.m_GLContext)
        , m_MajorVersion(other.m_MajorVersion)
        , m_MinorVersion(other.m_MinorVersion)
        , m_GLADLoaded(other.m_GLADLoaded)
    {
        // Update thread-local if this was the current context
        if (s_CurrentContext == &other)
        {
            s_CurrentContext = this;
        }

        other.m_Window = nullptr;
        other.m_GLContext = nullptr;
        other.m_GLADLoaded = false;
    }

    OpenGLContext& OpenGLContext::operator=(OpenGLContext&& other) noexcept
    {
        if (this != &other)
        {
            // Cleanup current context
            if (m_GLContext)
            {
                if (s_CurrentContext == this)
                {
                    SDL_GL_MakeCurrent(m_Window, nullptr);
                    s_CurrentContext = nullptr;
                }
                SDL_GL_DestroyContext(m_GLContext);
            }

            // Move data
            m_Window = other.m_Window;
            m_GLContext = other.m_GLContext;
            m_MajorVersion = other.m_MajorVersion;
            m_MinorVersion = other.m_MinorVersion;
            m_GLADLoaded = other.m_GLADLoaded;

            // Update thread-local if other was current
            if (s_CurrentContext == &other)
            {
                s_CurrentContext = this;
            }

            // Clear other
            other.m_Window = nullptr;
            other.m_GLContext = nullptr;
            other.m_GLADLoaded = false;
        }

        return *this;
    }

    Result<void> OpenGLContext::MakeCurrent()
    {
        if (!m_GLContext || !m_Window)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "OpenGL context is invalid"
            );
        }

        if (!SDL_GL_MakeCurrent(m_Window, m_GLContext))
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                fmt::format("Failed to make OpenGL context current: {}", SDL_GetError())
            );
        }

        s_CurrentContext = this;
        return Result<void>::Success();
    }

    Result<void> OpenGLContext::ReleaseCurrent()
    {
        if (s_CurrentContext == this)
        {
            if (!SDL_GL_MakeCurrent(m_Window, nullptr))
            {
                return Result<void>::Failure(
                    ErrorCode::GraphicsInvalidOperation,
                    fmt::format("Failed to release OpenGL context: {}", SDL_GetError())
                );
            }
            s_CurrentContext = nullptr;
        }

        return Result<void>::Success();
    }

    bool OpenGLContext::IsCurrent() const
    {
        return s_CurrentContext == this;
    }

    Result<void> OpenGLContext::SwapBuffers()
    {
        if (!m_GLContext || !m_Window)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "OpenGL context is invalid"
            );
        }

        if (!IsCurrent())
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "OpenGL context is not current on this thread"
            );
        }

        SDL_GL_SwapWindow(m_Window);
        return Result<void>::Success();
    }

    void* OpenGLContext::GetNativeHandle() const
    {
        return static_cast<void*>(m_GLContext);
    }

    bool OpenGLContext::IsValid() const
    {
        return m_GLContext != nullptr && m_Window != nullptr;
    }

    Result<void> OpenGLContext::LoadGLAD()
    {
        if (m_GLADLoaded)
        {
            return Result<void>::Success();
        }

        // Load OpenGL functions using GLAD
        // GLAD requires a current OpenGL context to load functions
        if (!IsCurrent())
        {
            auto makeCurrentResult = MakeCurrent();
            if (makeCurrentResult.IsFailure())
            {
                return Result<void>::Failure(
                    ErrorCode::GraphicsContextCreationFailed,
                    "Failed to make context current for GLAD loading"
                );
            }
        }

        // Load GLAD - this populates all OpenGL function pointers
        if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress))
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsContextCreationFailed,
                "Failed to load OpenGL functions with GLAD"
            );
        }

        m_GLADLoaded = true;

        // Get OpenGL version from context (already set during creation)
        SB_CORE_INFO("GLAD loaded successfully. OpenGL {}.{}", m_MajorVersion, m_MinorVersion);
        
        const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
        const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
        if (version) SB_CORE_INFO("OpenGL Version: {}", version);
        if (renderer) SB_CORE_INFO("OpenGL Renderer: {}", renderer);
        if (vendor) SB_CORE_INFO("OpenGL Vendor: {}", vendor);

        return Result<void>::Success();
    }

} // namespace Sabora
