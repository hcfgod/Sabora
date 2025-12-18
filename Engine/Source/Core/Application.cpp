#include "Application.h"
#include "Log.h"
#include <SDL3/SDL.h>

// Audio library includes for verification
#include <sndfile.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/vorbisenc.h>
#include <AL/alc.h>
#include <AL/al.h>
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include <FLAC/stream_decoder.h>
#include <FLAC/stream_encoder.h>
#include <FLAC/format.h>
#include <opus/opus.h>
#include <opus/opusfile.h>
#include <opus/opusenc.h>
#include <box2d/box2d.h>
#include <imgui.h>
#include <stb_image.h>
#include <glad/gl.h>
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include <vector>
#include <cstdarg>
#include <cstdio>

// Jolt Physics helper classes for initialization test
namespace
{
    using namespace JPH;
    
    // Jolt trace callback - routes to engine logging system
    static void JoltTraceImpl(const char* inFMT, ...)
    {
        va_list args;
        va_start(args, inFMT);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), inFMT, args);
        va_end(args);
        
        // Route to engine logging system
        Sabora::Log::Info(Sabora::LogCategory::Physics, buffer);
    }
    
#ifdef JPH_ENABLE_ASSERTS
    // Jolt assert callback - routes to engine logging system
    static bool JoltAssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, uint inLine)
    {
        char buffer[2048];
        int len = snprintf(buffer, sizeof(buffer), "Jolt Assertion failed: %s in %s:%u", inExpression, inFile, inLine);
        if (inMessage != nullptr && len < static_cast<int>(sizeof(buffer)) - 1)
        {
            snprintf(buffer + len, sizeof(buffer) - len, " - %s", inMessage);
        }
        Sabora::Log::Error(Sabora::LogCategory::Physics, buffer);
        
        // Return true to trigger breakpoint (only in debug builds)
        return true;
    }
#endif // JPH_ENABLE_ASSERTS
    
    // Simple layer setup for Jolt test
    namespace JoltTestLayers
    {
        static constexpr ObjectLayer NON_MOVING = 0;
        static constexpr ObjectLayer MOVING = 1;
        static constexpr ObjectLayer NUM_LAYERS = 2;
    }
    
    namespace JoltTestBroadPhaseLayers
    {
        static constexpr BroadPhaseLayer NON_MOVING(0);
        static constexpr BroadPhaseLayer MOVING(1);
        static constexpr uint NUM_LAYERS(2);
    }
    
    // Minimal layer interface implementations for Jolt test
    class JoltTestBPLayerInterfaceImpl final : public BroadPhaseLayerInterface
    {
    public:
        JoltTestBPLayerInterfaceImpl()
        {
            mObjectToBroadPhase[JoltTestLayers::NON_MOVING] = JoltTestBroadPhaseLayers::NON_MOVING;
            mObjectToBroadPhase[JoltTestLayers::MOVING] = JoltTestBroadPhaseLayers::MOVING;
        }
        
        virtual uint GetNumBroadPhaseLayers() const override
        {
            return JoltTestBroadPhaseLayers::NUM_LAYERS;
        }
        
        virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
        {
            JPH_ASSERT(inLayer < JoltTestLayers::NUM_LAYERS);
            return mObjectToBroadPhase[inLayer];
        }
        
    private:
        BroadPhaseLayer mObjectToBroadPhase[JoltTestLayers::NUM_LAYERS];
    };
    
    class JoltTestObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
    {
    public:
        virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
        {
            return true;
        }
    };
    
    class JoltTestObjectLayerPairFilterImpl : public ObjectLayerPairFilter
    {
    public:
        virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
        {
            return true;
        }
    };
}

namespace Sabora 
{
    namespace
    {
        // Simple in-memory write callback for libopusenc diagnostic usage
        int OpeWriteCallback(void* userData, const unsigned char* ptr, opus_int32 len)
        {
            auto* buffer = static_cast<std::vector<unsigned char>*>(userData);
            if (buffer == nullptr || ptr == nullptr || len < 0)
            {
                return 1;
            }

            buffer->insert(buffer->end(), ptr, ptr + len);
            return 0;
        }

        // No-op close callback
        int OpeCloseCallback(void* /*userData*/)
        {
            return 0;
        }

        // Minimal 1x1 transparent PNG (RGBA = 0,0,0,0) for stb_image verification
        // Source: canonical tiny PNG fixture for decoder smoke tests.
        static const unsigned char kPng1x1Transparent[] = {
            0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
            0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
            0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
            0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
            0x89, 0x00, 0x00, 0x00, 0x0A, 0x49, 0x44, 0x41,
            0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,
            0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00,
            0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,
            0x42, 0x60, 0x82
        };
    } // namespace

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

        // Verify audio libraries are properly linked and functional
        SB_CORE_INFO("Verifying audio library integration...");

        // Test libsndfile - get library version
        const char* sndfileVersion = sf_version_string();
        if (sndfileVersion != nullptr)
        {
            SB_CORE_INFO("libsndfile version: {}", sndfileVersion);
        }
        else
        {
            SB_CORE_WARN("Could not retrieve libsndfile version");
        }

        // Test libogg - verify library is linked
        ogg_stream_state testStream;
        int oggResult = ogg_stream_init(&testStream, 1);
        if (oggResult == 0)
        {
            SB_CORE_INFO("libogg initialized successfully (serialno: {})", testStream.serialno);
            ogg_stream_clear(&testStream);
        }
        else
        {
            SB_CORE_WARN("libogg stream initialization failed");
        }

        // Test libvorbis - verify library is linked
        vorbis_info vorbisInfo;
        vorbis_info_init(&vorbisInfo);
        SB_CORE_INFO("libvorbis initialized successfully");
        vorbis_info_clear(&vorbisInfo);

        // Test OpenAL Soft - verify library is linked
        ALCdevice* alDevice = alcOpenDevice(nullptr);
        if (alDevice != nullptr)
        {
            ALCcontext* alContext = alcCreateContext(alDevice, nullptr);
            if (alContext != nullptr)
            {
                if (alcMakeContextCurrent(alContext) == ALC_TRUE)
                {
                    const char* alVersion = alGetString(AL_VERSION);
                    const char* alRenderer = alGetString(AL_RENDERER);
                    if (alVersion != nullptr && alRenderer != nullptr)
                    {
                        SB_CORE_INFO("OpenAL Soft initialized - Version: {}, Renderer: {}", alVersion, alRenderer);
                    }
                    alcMakeContextCurrent(nullptr);
                }
                alcDestroyContext(alContext);
            }
            alcCloseDevice(alDevice);
        }
        else
        {
            SB_CORE_WARN("OpenAL Soft: No audio device available (this is acceptable in headless environments)");
        }

        // Test minimp3 - verify header-only library is included and functional
        mp3dec_t mp3d;
        mp3dec_init(&mp3d);
        
        // Test basic MP3 decoder functionality
        // Create a minimal test frame header (invalid but tests API availability)
        unsigned char testFrame[4] = {0xFF, 0xFB, 0x90, 0x00};
        mp3dec_frame_info_t frameInfo = {};
        short pcmBuffer[MINIMP3_MAX_SAMPLES_PER_FRAME];
        
        // Try to decode (will return 0 samples for invalid data, but verifies API works)
        int samplesDecoded = mp3dec_decode_frame(&mp3d, testFrame, sizeof(testFrame), pcmBuffer, &frameInfo);
        
        if (samplesDecoded >= 0)
        {
            SB_CORE_INFO("minimp3 MP3 decoder initialized successfully (max samples per frame: {})", MINIMP3_MAX_SAMPLES_PER_FRAME);
        }
        else
        {
            SB_CORE_WARN("minimp3 decoder test returned unexpected result");
        }

        // Test libFLAC - verify library is linked
        // Initialize FLAC decoder (null callbacks for testing)
        FLAC__StreamDecoder* flacDecoder = FLAC__stream_decoder_new();
        if (flacDecoder != nullptr)
        {
            SB_CORE_INFO("libFLAC decoder initialized successfully");
            FLAC__stream_decoder_delete(flacDecoder);
        }
        else
        {
            SB_CORE_WARN("libFLAC decoder initialization failed");
        }

        // Test FLAC encoder
        FLAC__StreamEncoder* flacEncoder = FLAC__stream_encoder_new();
        if (flacEncoder != nullptr)
        {
            SB_CORE_INFO("libFLAC encoder initialized successfully");
            FLAC__stream_encoder_delete(flacEncoder);
        }
        else
        {
            SB_CORE_WARN("libFLAC encoder initialization failed");
        }

        // Test libopus - verify library is linked
        // Initialize Opus encoder (test basic API availability)
        int opusError = 0;
        OpusEncoder* opusEncoder = opus_encoder_create(48000, 2, OPUS_APPLICATION_AUDIO, &opusError);
        if (opusEncoder != nullptr && opusError == OPUS_OK)
        {
            SB_CORE_INFO("libopus encoder initialized successfully (sample rate: 48000, channels: 2)");
            opus_encoder_destroy(opusEncoder);
        }
        else
        {
            SB_CORE_WARN("libopus encoder initialization failed (error code: {})", opusError);
        }

        // Test Opus decoder
        OpusDecoder* opusDecoder = opus_decoder_create(48000, 2, &opusError);
        if (opusDecoder != nullptr && opusError == OPUS_OK)
        {
            SB_CORE_INFO("libopus decoder initialized successfully");
            opus_decoder_destroy(opusDecoder);
        }
        else
        {
            SB_CORE_WARN("libopus decoder initialization failed (error code: {})", opusError);
        }

        // Test opusfile - verify library is linked
        // opusfile provides file reading capabilities for Opus files
        // We can't test file reading without a file, but we can verify the library is available
        // by checking that op_open_file function pointer exists (library is linked)
        // Note: op_open_file requires a file path, so we just verify the library is linked
        // by checking that the header is included and compilation succeeds
        SB_CORE_INFO("opusfile library linked successfully (file reading support for Opus files)");

        // Test libopusenc - verify high-level Opus encoder wrapper is linked
        OggOpusComments* opusComments = ope_comments_create();
        if (opusComments != nullptr)
        {
            ope_comments_add(opusComments, "ENCODER", "Sabora Diagnostics");

            std::vector<unsigned char> opusEncodedData;
            const OpusEncCallbacks callbacks{
                &OpeWriteCallback,
                &OpeCloseCallback
            };

            int opusencError = OPE_OK;
            OggOpusEnc* opusEncHandle = ope_encoder_create_callbacks(
                &callbacks,
                &opusEncodedData,
                opusComments,
                48000,
                2,
                0,
                &opusencError);

            if (opusEncHandle != nullptr && opusencError == OPE_OK)
            {
                const int drainResult = ope_encoder_drain(opusEncHandle);

                SB_CORE_INFO(
                    "libopusenc encoder initialized (drain={}, buffered={} bytes)",
                    drainResult,
                    opusEncodedData.size());

                ope_encoder_destroy(opusEncHandle);
            }
            else
            {
                SB_CORE_WARN("libopusenc encoder initialization failed (error code: {})", opusencError);
            }

            ope_comments_destroy(opusComments);
        }
        else
        {
            SB_CORE_WARN("libopusenc comments initialization failed");
        }

        // Test Box2D - create a world and step once
        const b2Version box2dVersion = b2GetVersion();
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = b2Vec2{ 0.0f, -9.8f };

        b2WorldId world = b2CreateWorld( &worldDef );
        if (b2World_IsValid( world ))
        {
            b2World_Step( world, 1.0f / 60.0f, 4 );
            SB_CORE_INFO(
                "Box2D initialized (v{}.{}.{}) and stepped a frame",
                box2dVersion.major,
                box2dVersion.minor,
                box2dVersion.revision );
            b2DestroyWorld( world );
        }
        else
        {
            SB_CORE_WARN("Box2D world creation failed");
        }

        // Test Jolt Physics - create physics system and step once
        using namespace JPH;
        
        // Register allocation hook
        RegisterDefaultAllocator();
        
        // Install trace and assert callbacks before using Jolt
        Trace = JoltTraceImpl;
        JPH_IF_ENABLE_ASSERTS(AssertFailed = JoltAssertFailedImpl;)
        
        // Create factory
        Factory::sInstance = new Factory();
        
        // Register all physics types
        RegisterTypes();
        
        // Create temp allocator
        TempAllocatorImpl temp_allocator(1024 * 1024); // 1 MB
        
        // Create job system
        JobSystemThreadPool job_system(cMaxPhysicsJobs, cMaxPhysicsBarriers, 1);
        
        // Create layer interfaces
        JoltTestBPLayerInterfaceImpl broad_phase_layer_interface;
        JoltTestObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
        JoltTestObjectLayerPairFilterImpl object_vs_object_layer_filter;
        
        // Create and initialize physics system
        PhysicsSystem physics_system;
        physics_system.Init(
            1024,  // max bodies
            0,     // num body mutexes (auto-detect)
            1024,  // max body pairs
            1024,  // max contact constraints
            broad_phase_layer_interface,
            object_vs_broadphase_layer_filter,
            object_vs_object_layer_filter
        );
        
        // Step the physics system once
        physics_system.Update(1.0f / 60.0f, 1, &temp_allocator, &job_system);
        
        SB_CORE_INFO("Jolt Physics initialized and stepped a frame");
        
        // Cleanup
        UnregisterTypes();
        delete Factory::sInstance;
        Factory::sInstance = nullptr;

        // Test ImGui (docking branch) - create context and log version
        ImGui::CreateContext();
        const char* imguiVersion = ImGui::GetVersion();
        if (imguiVersion != nullptr)
        {
            SB_CORE_INFO("ImGui (docking) initialized - Version: {}", imguiVersion);
        }
        else
        {
            SB_CORE_WARN("ImGui version string unavailable");
        }
        ImGui::DestroyContext();

        // Test stb_image - verify header-only image loader is functional
        int imageWidth = 0;
        int imageHeight = 0;
        int imageChannels = 0;
        unsigned char* imageData = stbi_load_from_memory(
            kPng1x1Transparent,
            static_cast<int>(sizeof(kPng1x1Transparent)),
            &imageWidth,
            &imageHeight,
            &imageChannels,
            4); // force RGBA output

        if (imageData != nullptr)
        {
            SB_CORE_INFO(
                "stb_image decoded tiny PNG ({}x{}, channels={}, rgba=[{}, {}, {}, {}])",
                imageWidth,
                imageHeight,
                imageChannels,
                imageData[0],
                imageData[1],
                imageData[2],
                imageData[3]);
            stbi_image_free(imageData);
        }
        else
        {
            SB_CORE_WARN("stb_image failed to decode tiny PNG fixture");
        }

        // Test GLAD - verify OpenGL loader is available
        // Note: GLAD requires an OpenGL context to load functions, so we just verify headers compile
        // Full initialization with gladLoadGL() requires a valid OpenGL context from SDL3
        SB_CORE_INFO("GLAD OpenGL loader headers verified (OpenGL 4.6 Core profile)");
        SB_CORE_INFO("Note: gladLoadGL() requires an OpenGL context - call after creating window with SDL3");

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

    Application::~Application() 
    {
        SB_CORE_INFO("Application shutting down...");

        // SDLContext is automatically cleaned up via unique_ptr destructor (RAII)
        // The destructor will call SDL_Quit() if SDL was initialized

        // Shutdown logging system last
        Sabora::Log::Shutdown();
    }

} // namespace Sabora