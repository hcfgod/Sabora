#include "Application.h"
#include "Log.h"
#include <SDL3/SDL.h>

// Test shader compilation libraries
#include <shaderc/shaderc.hpp>
#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>
#include <spirv_hlsl.hpp>
#include <spirv_msl.hpp>

// Test MSDF atlas generation library
#include <msdf-atlas-gen/msdf-atlas-gen.h>

// Test OpenAL Soft audio library
#include <AL/al.h>
#include <AL/alc.h>

namespace Sabora 
{
    //==========================================================================
    // SDLContext Implementation
    //==========================================================================

    SDLContext::SDLContext() noexcept
        : m_Initialized(false)
    {
    }

    SDLContext::SDLContext(SDLContext&& other) noexcept
        : m_Initialized(other.m_Initialized)
    {
        other.m_Initialized = false;
    }

    SDLContext& SDLContext::operator=(SDLContext&& other) noexcept
    {
        if (this != &other)
        {
            // Clean up current state if initialized
            if (m_Initialized)
            {
                SDL_Quit();
            }

            m_Initialized = other.m_Initialized;
            other.m_Initialized = false;
        }
        return *this;
    }

    SDLContext::~SDLContext()
    {
        if (m_Initialized)
        {
            SB_CORE_INFO("Shutting down SDL3...");
            SDL_Quit();
            m_Initialized = false;
        }
    }

    Result<std::unique_ptr<SDLContext>> SDLContext::Create(uint32_t flags)
    {
        // Create the context (private constructor access via factory)
        auto context = std::unique_ptr<SDLContext>(new SDLContext());

        // Disable GameInput to prevent crashes on some Windows configurations
        SDL_SetHint(SDL_HINT_WINDOWS_GAMEINPUT, "0");

        // Note: SDL3 uses 'bool' return type for SDL_Init unlike SDL2 which used 'int'
        if (!SDL_Init(flags))
        {
            return Result<std::unique_ptr<SDLContext>>::Failure(
                ErrorCode::PlatformSDLError,
                fmt::format("SDL3 could not initialize! SDL_Error: {}", SDL_GetError())
            );
        }

        context->m_Initialized = true;
        SB_CORE_INFO("SDL3 initialized successfully! Version: {}", SDL_GetRevision());

        return Result<std::unique_ptr<SDLContext>>::Success(std::move(context));
    }

    //==========================================================================
    // Application Implementation
    //==========================================================================

    Application::Application(const ApplicationConfig& config) 
    {
        // Initialize logging system first (needed for error reporting)
        Sabora::Log::Initialize();
        SB_CORE_INFO("Application created with name: {}", config.name);
    }

    Result<void> Application::Initialize()
    {
        SB_CORE_INFO("Initializing application systems...");

        // Initialize SDL using RAII wrapper
        auto sdlResult = SDLContext::Create(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
        if (sdlResult.IsFailure())
        {
            SB_CORE_ERROR("Failed to initialize SDL: {}", sdlResult.GetError().ToString());
            return Result<void>::Failure(sdlResult.GetError());
        }

        m_SDLContext = std::move(sdlResult).Value();

        // Test shader compilation libraries
        SB_CORE_INFO("Testing shader compilation libraries...");
        TestShaderLibraries();

        // Test MSDF atlas generation library
        SB_CORE_INFO("Testing MSDF atlas generation library...");
        TestMSDFAtlasGen();

        // Test OpenAL Soft audio library
        SB_CORE_INFO("Testing OpenAL Soft audio library...");
        TestOpenALSoft();

        SB_CORE_INFO("Application initialization complete.");
        return Result<void>::Success();
    }

    void Application::Run() 
    {
        // Ensure application is initialized before running
        if (!m_SDLContext || !m_SDLContext->IsInitialized())
        {
            SB_CORE_ERROR("Application::Run() called before successful initialization!");
            return;
        }

        m_Running = true;
        m_LastFrame = Clock::now();

        SB_CORE_INFO("Entering main application loop...");

        while (m_Running) 
        {
            // Calculate delta time
            const auto now = Clock::now();
            [[maybe_unused]] const float deltaTime = 
                std::chrono::duration<float>(now - m_LastFrame).count();
            m_LastFrame = now;

            // Process SDL events
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_EVENT_QUIT)
                {
                    RequestClose();
                }
            }

            // Future: Update game systems with deltaTime
            // Future: Render frame
        }

        SB_CORE_INFO("Exited main application loop.");
    }

    void Application::RequestClose() 
    {
        m_Running = false;
        SB_CORE_INFO("Application close requested.");
    }

    void Application::TestShaderLibraries()
    {
        // Test shaderc - create a compiler instance
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        
        // Simple vertex shader in GLSL
        const std::string vertexShader = R"(
            #version 450
            layout(location = 0) in vec3 inPosition;
            layout(location = 0) out vec3 fragColor;
            
            void main() {
                gl_Position = vec4(inPosition, 1.0);
                fragColor = vec3(1.0, 0.0, 0.0);
            }
        )";

        // Compile GLSL to SPIR-V
        shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
            vertexShader, 
            shaderc_vertex_shader, 
            "test_shader", 
            options
        );

        if (result.GetCompilationStatus() == shaderc_compilation_status_success)
        {
            std::vector<uint32_t> spirvCode(result.cbegin(), result.cend());
            SB_CORE_INFO("✓ shaderc: Successfully compiled GLSL to SPIR-V ({} words)", spirvCode.size());
            
            // Test SPIRV-Cross - parse SPIR-V and convert to GLSL
            try
            {
                spirv_cross::CompilerGLSL glslCompiler(spirvCode);
                spirv_cross::CompilerGLSL::Options glslOptions;
                glslOptions.version = 330;
                glslOptions.es = false;
                glslCompiler.set_common_options(glslOptions);
                
                std::string glslOutput = glslCompiler.compile();
                SB_CORE_INFO("✓ SPIRV-Cross: Successfully converted SPIR-V to GLSL ({} bytes)", glslOutput.size());
                
                // Test HLSL output
                spirv_cross::CompilerHLSL hlslCompiler(spirvCode);
                spirv_cross::CompilerHLSL::Options hlslOptions;
                hlslOptions.shader_model = 50;
                hlslCompiler.set_hlsl_options(hlslOptions);
                
                std::string hlslOutput = hlslCompiler.compile();
                SB_CORE_INFO("✓ SPIRV-Cross: Successfully converted SPIR-V to HLSL ({} bytes)", hlslOutput.size());
                
                // Test MSL output (for macOS/iOS)
                spirv_cross::CompilerMSL mslCompiler(spirvCode);
                spirv_cross::CompilerMSL::Options mslOptions;
                mslOptions.set_msl_version(2, 0);
                mslCompiler.set_msl_options(mslOptions);
                
                std::string mslOutput = mslCompiler.compile();
                SB_CORE_INFO("✓ SPIRV-Cross: Successfully converted SPIR-V to MSL ({} bytes)", mslOutput.size());
            }
            catch (const std::exception& e)
            {
                SB_CORE_WARN("SPIRV-Cross test failed: {}", e.what());
            }
        }
        else
        {
            SB_CORE_WARN("shaderc compilation failed: {}", result.GetErrorMessage());
        }
    }

    void Application::TestMSDFAtlasGen()
    {
        try
        {
            // Test that we can create basic types from msdf-atlas-gen
            // Create a simple rectangle packer to verify linking
            msdf_atlas::RectanglePacker packer(1024, 1024);
            
            // Test that we can create a font geometry structure
            msdf_atlas::FontGeometry fontGeometry;
            
            // Test that we can access types and create rectangles
            msdf_atlas::Rectangle testRect;
            testRect.x = 0;
            testRect.y = 0;
            testRect.w = 10;
            testRect.h = 10;
            
            msdf_atlas::GlyphBox glyphBox;
            glyphBox.rect = testRect;
            glyphBox.index = 0;
            glyphBox.advance = 10.0;
            
            // Test enum types
            [[maybe_unused]] msdf_atlas::ImageType imageType = msdf_atlas::ImageType::MSDF;
            [[maybe_unused]] msdf_atlas::ImageFormat imageFormat = msdf_atlas::ImageFormat::PNG;
            
            SB_CORE_INFO("✓ msdf-atlas-gen: Library linked successfully, types accessible");
            SB_CORE_INFO("  - RectanglePacker: OK (1024x1024)");
            SB_CORE_INFO("  - FontGeometry: OK");
            SB_CORE_INFO("  - GlyphBox: OK");
            SB_CORE_INFO("  - ImageType enum: OK (MSDF)");
            SB_CORE_INFO("  - ImageFormat enum: OK (PNG)");
        }
        catch (const std::exception& e)
        {
            SB_CORE_WARN("msdf-atlas-gen test failed: {}", e.what());
        }
        catch (...)
        {
            SB_CORE_WARN("msdf-atlas-gen test failed: Unknown exception");
        }
    }

    void Application::TestOpenALSoft()
    {
        // OpenAL Soft test - initialize device and context
        ALCdevice* device = nullptr;
        ALCcontext* context = nullptr;
        
        try
        {
            // Open default audio device
            device = alcOpenDevice(nullptr);
            if (device == nullptr)
            {
                SB_CORE_WARN("OpenAL Soft: Failed to open default audio device");
                return;
            }
            
            // Create audio context
            context = alcCreateContext(device, nullptr);
            if (context == nullptr)
            {
                SB_CORE_WARN("OpenAL Soft: Failed to create audio context");
                alcCloseDevice(device);
                return;
            }
            
            // Make context current
            if (!alcMakeContextCurrent(context))
            {
                SB_CORE_WARN("OpenAL Soft: Failed to make context current");
                alcDestroyContext(context);
                alcCloseDevice(device);
                return;
            }
            
            // Get OpenAL version and renderer info
            const ALCchar* version = alGetString(AL_VERSION);
            const ALCchar* renderer = alGetString(AL_RENDERER);
            const ALCchar* vendor = alGetString(AL_VENDOR);
            
            SB_CORE_INFO("✓ OpenAL Soft: Library linked successfully");
            SB_CORE_INFO("  - Version: {}", version ? version : "Unknown");
            SB_CORE_INFO("  - Renderer: {}", renderer ? renderer : "Unknown");
            SB_CORE_INFO("  - Vendor: {}", vendor ? vendor : "Unknown");
            
            // Clean up
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(context);
            alcCloseDevice(device);
        }
        catch (const std::exception& e)
        {
            SB_CORE_WARN("OpenAL Soft test failed: {}", e.what());
            // Clean up on error
            if (context != nullptr)
            {
                alcMakeContextCurrent(nullptr);
                alcDestroyContext(context);
            }
            if (device != nullptr)
            {
                alcCloseDevice(device);
            }
        }
        catch (...)
        {
            SB_CORE_WARN("OpenAL Soft test failed: Unknown exception");
            // Clean up on error
            if (context != nullptr)
            {
                alcMakeContextCurrent(nullptr);
                alcDestroyContext(context);
            }
            if (device != nullptr)
            {
                alcCloseDevice(device);
            }
        }
    }


    Application::~Application() 
    {
        SB_CORE_INFO("Application shutting down...");

        // SDLContext is automatically cleaned up via unique_ptr destructor (RAII)
        // The destructor will call SDL_Quit() if SDL was initialized

        // Shutdown logging system last
        Sabora::Log::Shutdown();
    }

} // namespace Sabora
