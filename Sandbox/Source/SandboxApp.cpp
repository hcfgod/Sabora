#include "pch.h"
#include "Sabora.h"
#include "Input/KeyCode.h"
#include "Renderer/Core/Renderer.h"
#include "Renderer/Core/RendererTypes.h"
#include "Renderer/Resources/Buffer.h"
#include "Renderer/Resources/VertexLayout.h"
#include "Renderer/Resources/Shader.h"
#include "Renderer/Resources/PipelineState.h"
#include <SDL3/SDL.h>
#include <memory>

using namespace Sabora;

/**
 * @brief Constants used by the Sandbox application.
 */
namespace SandboxConstants
{
    /**
     * @brief Default clear color for the render target.
     * Dark blue color: RGB(0.1, 0.1, 0.2) with full alpha.
     */
    constexpr ClearColor DefaultClearColor{0.1f, 0.1f, 0.2f, 1.0f};
}

/**
 * @brief Sandbox application for testing engine features.
 */
class SandboxApp : public Application
{
public:
    SandboxApp() : Application({ "Sabora Sandbox", WindowConfig{ "Sabora Sandbox", 1280, 720, false, true, false, true } }), m_RenderingInitialized(false)
    {
        // Subscribe to events using EventManager singleton
        // Note: EventManager::Subscribe requires const reference in callback signature,
        // but the underlying event is non-const, so we use const_cast to match OnWindowClose signature.
        // This is safe because Dispatch() passes a non-const event to the callback.
        [[maybe_unused]] auto closeSubId = EventManager::Get().Subscribe<WindowCloseEvent>
        (
            [this](const WindowCloseEvent& event) 
            {
                // Safe const_cast: EventDispatcher::Dispatch passes non-const event to callbacks
                WindowCloseEvent& mutableEvent = const_cast<WindowCloseEvent&>(event);
                OnWindowClose(mutableEvent);
            }
        );

        [[maybe_unused]] auto keySubId = EventManager::Get().Subscribe<KeyEvent>
        (
            [this](const KeyEvent& event) 
            {
                // Handle key events
                // Convert SDL keycode to KeyCode for comparison
                KeyCode keyCode = SDLToKeyCode(static_cast<SDL_Keycode>(event.GetKey()));
                if (keyCode == KeyCode::Escape && event.IsPressed())
                {
                    RequestClose();
                }
            }
        );

    }

    void InitializeRendering()
    {
        // Initialize rendering resources after renderer is ready
        auto* renderer = GetRenderer();
        if (!renderer)
        {
            SB_CORE_ERROR("Renderer is not available!");
            return;
        }

        // Simple vertex shader - renders a triangle
        const std::string vertexShaderSource = 
        R"(
            #version 330 core

            layout(location = 0) in vec3 a_Position;
            layout(location = 1) in vec3 a_Color;

            out vec3 v_Color;

            void main()
            {
                gl_Position = vec4(a_Position, 1.0);
                v_Color = a_Color;
            }
        )";

                    // Simple fragment shader
        const std::string fragmentShaderSource = 
        R"(
            #version 330 core

            in vec3 v_Color;
            out vec4 FragColor;

            void main()
            {
                FragColor = vec4(v_Color, 1.0);
            }
        )";

        // Create shaders
        auto vertexShaderResult = renderer->CreateShader(ShaderStage::Vertex, vertexShaderSource);
        if (vertexShaderResult.IsFailure())
        {
            SB_CORE_ERROR("Failed to create vertex shader: {}", vertexShaderResult.GetError().ToString());
            return;
        }
        m_VertexShader = std::move(vertexShaderResult).Value();

        auto fragmentShaderResult = renderer->CreateShader(ShaderStage::Fragment, fragmentShaderSource);
        if (fragmentShaderResult.IsFailure())
        {
            SB_CORE_ERROR("Failed to create fragment shader: {}", fragmentShaderResult.GetError().ToString());
            return;
        }
        m_FragmentShader = std::move(fragmentShaderResult).Value();

        // Create vertex layout
        // Position (vec3) at location 0, Color (vec3) at location 1
        VertexLayout vertexLayout;
        vertexLayout.AddAttribute(0, VertexAttributeType::Float3, 0);  // Position
        vertexLayout.AddAttribute(1, VertexAttributeType::Float3, 12); // Color (offset after vec3 = 12 bytes)

        // Create pipeline state
        auto pipelineResult = renderer->CreatePipelineState(
            m_VertexShader.get(),
            m_FragmentShader.get(),
            vertexLayout,
            PrimitiveTopology::Triangles
        );
        if (pipelineResult.IsFailure())
        {
            SB_CORE_ERROR("Failed to create pipeline state: {}", pipelineResult.GetError().ToString());
            return;
        }
        m_PipelineState = std::move(pipelineResult).Value();

        // Create vertex buffer with triangle data
        // Each vertex: position (vec3) + color (vec3) = 6 floats = 24 bytes
        struct Vertex
        {
            float position[3];
            float color[3];
        };

        Vertex vertices[] = {
            // Bottom-left (red)
            { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
            // Bottom-right (green)
            { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
            // Top-center (blue)
            { {  0.0f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
        };

        auto vertexBufferResult = renderer->CreateBuffer(
            BufferType::Vertex,
            sizeof(vertices),
            BufferUsage::Static,
            vertices
        );
        if (vertexBufferResult.IsFailure())
        {
            SB_CORE_ERROR("Failed to create vertex buffer: {}", vertexBufferResult.GetError().ToString());
            return;
        }
        m_VertexBuffer = std::move(vertexBufferResult).Value();

        SB_CORE_INFO("Triangle rendering setup complete!");
    }

    void OnUpdate(float deltaTime) override
    {
        auto* renderer = GetRenderer();
        if (!renderer)
        {
            return;
        }

        // Initialize rendering resources on first update (after renderer is ready)
        if (!m_RenderingInitialized)
        {
            InitializeRendering();
            m_RenderingInitialized = true;
        }

        if (!m_PipelineState || !m_VertexBuffer)
        {
            return;
        }

        // Clear the screen with a dark blue color and depth buffer
        // Note: Viewport is automatically updated by the engine when window is resized
        ClearColor clearColor = SandboxConstants::DefaultClearColor;
        
        ClearDepthStencil clearDepthStencil;
        clearDepthStencil.depth = 1.0f;  // Clear depth to 1.0 (far plane)
        clearDepthStencil.stencil = 0;
        
        auto clearResult = renderer->Clear(ClearFlags::Color | ClearFlags::Depth, clearColor, clearDepthStencil);
        if (clearResult.IsFailure())
        {
            SB_CORE_ERROR("Failed to clear render target: {}", clearResult.GetError().ToString());
            return;
        }

        // Set pipeline state
        auto pipelineResult = renderer->SetPipelineState(m_PipelineState.get());
        if (pipelineResult.IsFailure())
        {
            SB_CORE_ERROR("Failed to set pipeline state: {}", pipelineResult.GetError().ToString());
            return;
        }

        // Set vertex buffer
        auto bufferResult = renderer->SetVertexBuffer(m_VertexBuffer.get());
        if (bufferResult.IsFailure())
        {
            SB_CORE_ERROR("Failed to set vertex buffer: {}", bufferResult.GetError().ToString());
            return;
        }

        // Draw the triangle (3 vertices)
        auto drawResult = renderer->Draw(3);
        if (drawResult.IsFailure())
        {
            SB_CORE_ERROR("Failed to draw: {}", drawResult.GetError().ToString());
            return;
        }
    }

    void OnWindowClose(WindowCloseEvent& event) override
    {
        SB_CORE_INFO("Window close requested");
        // You can mark the event as handled to prevent default close behavior
        // event.MarkHandled();
    }

private:
    bool m_RenderingInitialized;
    std::unique_ptr<Shader> m_VertexShader;
    std::unique_ptr<Shader> m_FragmentShader;
    std::unique_ptr<PipelineState> m_PipelineState;
    std::unique_ptr<Buffer> m_VertexBuffer;
};

/**
 * @brief Create the application instance.
 * 
 * This function is called by EntryPoint::Main() to create your application.
 * Simply return a new instance of your Application-derived class.
 */
Sabora::Application* Sabora::CreateApplication()
{
    return new SandboxApp();
}