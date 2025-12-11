/**
 * @file TestOpenALSoft.cpp
 * @brief Unit tests for OpenAL Soft audio library.
 * 
 * These tests verify that OpenAL Soft is properly linked and can initialize
 * audio devices and contexts, and query audio system information.
 */

#include "doctest.h"
#include "Sabora.h"

// OpenAL Soft audio library
#include <AL/al.h>
#include <AL/alc.h>

#include <string>

using namespace Sabora;

TEST_SUITE("OpenAL Soft")
{
    TEST_CASE("Open default audio device")
    {
        ALCdevice* device = alcOpenDevice(nullptr);
        
        if (device == nullptr)
        {
            // Audio device might not be available in test environment
            // This is acceptable for CI/CD environments
            WARN("OpenAL Soft: No audio device available for testing");
            return;
        }
        
        CHECK(device != nullptr);
        
        // Clean up
        alcCloseDevice(device);
    }

    TEST_CASE("Create audio context")
    {
        ALCdevice* device = alcOpenDevice(nullptr);
        
        if (device == nullptr)
        {
            WARN("OpenAL Soft: No audio device available for testing");
            return;
        }
        
        ALCcontext* context = alcCreateContext(device, nullptr);
        CHECK(context != nullptr);
        
        // Clean up
        if (context != nullptr)
        {
            alcDestroyContext(context);
        }
        alcCloseDevice(device);
    }

    TEST_CASE("Make context current")
    {
        ALCdevice* device = alcOpenDevice(nullptr);
        
        if (device == nullptr)
        {
            WARN("OpenAL Soft: No audio device available for testing");
            return;
        }
        
        ALCcontext* context = alcCreateContext(device, nullptr);
        REQUIRE(context != nullptr);
        
        ALCboolean madeCurrent = alcMakeContextCurrent(context);
        CHECK(madeCurrent == ALC_TRUE);
        
        // Clean up
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        alcCloseDevice(device);
    }

    TEST_CASE("Query OpenAL version information")
    {
        ALCdevice* device = alcOpenDevice(nullptr);
        
        if (device == nullptr)
        {
            WARN("OpenAL Soft: No audio device available for testing");
            return;
        }
        
        ALCcontext* context = alcCreateContext(device, nullptr);
        REQUIRE(context != nullptr);
        
        ALCboolean madeCurrent = alcMakeContextCurrent(context);
        REQUIRE(madeCurrent == ALC_TRUE);
        
        // Get OpenAL version and renderer info
        const ALCchar* version = alGetString(AL_VERSION);
        const ALCchar* renderer = alGetString(AL_RENDERER);
        const ALCchar* vendor = alGetString(AL_VENDOR);
        
        // Verify we got valid strings (not null)
        CHECK(version != nullptr);
        CHECK(renderer != nullptr);
        CHECK(vendor != nullptr);
        
        // Verify strings are not empty
        std::string versionStr(version ? version : "");
        std::string rendererStr(renderer ? renderer : "");
        std::string vendorStr(vendor ? vendor : "");
        
        CHECK(versionStr.length() > 0);
        CHECK(rendererStr.length() > 0);
        CHECK(vendorStr.length() > 0);
        
        // Clean up
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        alcCloseDevice(device);
    }

    TEST_CASE("Query ALC device information")
    {
        ALCdevice* device = alcOpenDevice(nullptr);
        
        if (device == nullptr)
        {
            WARN("OpenAL Soft: No audio device available for testing");
            return;
        }
        
        // Get device specifier
        const ALCchar* deviceSpec = alcGetString(device, ALC_DEVICE_SPECIFIER);
        CHECK(deviceSpec != nullptr);
        
        if (deviceSpec != nullptr)
        {
            std::string deviceSpecStr(deviceSpec);
            CHECK(deviceSpecStr.length() > 0);
        }
        
        // Clean up
        alcCloseDevice(device);
    }

    TEST_CASE("Error handling - invalid device")
    {
        // Try to create context with null device (should fail gracefully)
        ALCcontext* context = alcCreateContext(nullptr, nullptr);
        CHECK(context == nullptr);
        
        // Clean up (should be safe even if context is null)
        if (context != nullptr)
        {
            alcDestroyContext(context);
        }
    }

    TEST_CASE("Multiple context creation and cleanup")
    {
        ALCdevice* device = alcOpenDevice(nullptr);
        
        if (device == nullptr)
        {
            WARN("OpenAL Soft: No audio device available for testing");
            return;
        }
        
        // Create multiple contexts
        ALCcontext* context1 = alcCreateContext(device, nullptr);
        ALCcontext* context2 = alcCreateContext(device, nullptr);
        
        CHECK(context1 != nullptr);
        CHECK(context2 != nullptr);
        
        // Make first context current
        ALCboolean madeCurrent = alcMakeContextCurrent(context1);
        CHECK(madeCurrent == ALC_TRUE);
        
        // Switch to second context
        madeCurrent = alcMakeContextCurrent(context2);
        CHECK(madeCurrent == ALC_TRUE);
        
        // Clean up
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context1);
        alcDestroyContext(context2);
        alcCloseDevice(device);
    }

    TEST_CASE("ALC error checking")
    {
        ALCdevice* device = alcOpenDevice(nullptr);
        
        if (device == nullptr)
        {
            WARN("OpenAL Soft: No audio device available for testing");
            return;
        }
        
        // Clear any existing errors
        alcGetError(device);
        
        // Create context (should not produce errors)
        ALCcontext* context = alcCreateContext(device, nullptr);
        REQUIRE(context != nullptr);
        
        ALCenum error = alcGetError(device);
        CHECK(error == ALC_NO_ERROR);
        
        // Clean up
        alcDestroyContext(context);
        alcCloseDevice(device);
    }

    TEST_CASE("AL error checking")
    {
        ALCdevice* device = alcOpenDevice(nullptr);
        
        if (device == nullptr)
        {
            WARN("OpenAL Soft: No audio device available for testing");
            return;
        }
        
        ALCcontext* context = alcCreateContext(device, nullptr);
        REQUIRE(context != nullptr);
        
        ALCboolean madeCurrent = alcMakeContextCurrent(context);
        REQUIRE(madeCurrent == ALC_TRUE);
        
        // Clear any existing errors
        alGetError();
        
        // Query version (should not produce errors)
        const ALCchar* version = alGetString(AL_VERSION);
        CHECK(version != nullptr);
        
        ALenum error = alGetError();
        CHECK(error == AL_NO_ERROR);
        
        // Clean up
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        alcCloseDevice(device);
    }
}

