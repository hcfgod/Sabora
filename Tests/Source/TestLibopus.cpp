/**
 * @file TestLibopus.cpp
 * @brief Unit tests for libopus audio codec library.
 * 
 * These tests verify that libopus is properly linked and can encode/decode
 * Opus audio data. libopus provides the best quality/compression ratio for audio.
 */

#include "doctest.h"
#include "Sabora.h"

// libopus audio codec library
#include <opus/opus.h>

#include <vector>
#include <cstring>

using namespace Sabora;

TEST_SUITE("libopus")
{
    TEST_CASE("Create Opus encoder")
    {
        int error = 0;
        OpusEncoder* encoder = opus_encoder_create(48000, 2, OPUS_APPLICATION_AUDIO, &error);
        
        CHECK(encoder != nullptr);
        CHECK(error == OPUS_OK);
        
        if (encoder != nullptr)
        {
            opus_encoder_destroy(encoder);
        }
    }

    TEST_CASE("Create Opus decoder")
    {
        int error = 0;
        OpusDecoder* decoder = opus_decoder_create(48000, 2, &error);
        
        CHECK(decoder != nullptr);
        CHECK(error == OPUS_OK);
        
        if (decoder != nullptr)
        {
            opus_decoder_destroy(decoder);
        }
    }

    TEST_CASE("Configure Opus encoder settings")
    {
        int error = 0;
        OpusEncoder* encoder = opus_encoder_create(48000, 2, OPUS_APPLICATION_AUDIO, &error);
        REQUIRE(encoder != nullptr);
        REQUIRE(error == OPUS_OK);
        
        // Set bitrate
        error = opus_encoder_ctl(encoder, OPUS_SET_BITRATE(64000));
        CHECK(error == OPUS_OK);
        
        // Set complexity
        error = opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(5));
        CHECK(error == OPUS_OK);
        
        // Set signal type
        error = opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
        CHECK(error == OPUS_OK);
        
        opus_encoder_destroy(encoder);
    }

    TEST_CASE("Query Opus encoder settings")
    {
        int error = 0;
        OpusEncoder* encoder = opus_encoder_create(48000, 2, OPUS_APPLICATION_AUDIO, &error);
        REQUIRE(encoder != nullptr);
        REQUIRE(error == OPUS_OK);
        
        // Get sample rate
        opus_int32 sampleRate = 0;
        error = opus_encoder_ctl(encoder, OPUS_GET_SAMPLE_RATE(&sampleRate));
        CHECK(error == OPUS_OK);
        CHECK(sampleRate == 48000);
        
        // Get bitrate
        opus_int32 bitrate = 0;
        error = opus_encoder_ctl(encoder, OPUS_GET_BITRATE(&bitrate));
        CHECK(error == OPUS_OK);
        
        opus_encoder_destroy(encoder);
    }

    TEST_CASE("Verify Opus application types")
    {
        // Verify application type constants are defined
        CHECK(OPUS_APPLICATION_VOIP >= 0);
        CHECK(OPUS_APPLICATION_AUDIO >= 0);
        CHECK(OPUS_APPLICATION_RESTRICTED_LOWDELAY >= 0);
    }

    TEST_CASE("Verify Opus signal types")
    {
        // Verify signal type constants are defined
        // Note: OPUS_AUTO is not a constant, it's the default (0)
        CHECK(OPUS_SIGNAL_VOICE >= 0);
        CHECK(OPUS_SIGNAL_MUSIC >= 0);
    }

    TEST_CASE("Test Opus encode/decode cycle")
    {
        int error = 0;
        
        // Create encoder and decoder
        OpusEncoder* encoder = opus_encoder_create(48000, 2, OPUS_APPLICATION_AUDIO, &error);
        REQUIRE(encoder != nullptr);
        REQUIRE(error == OPUS_OK);
        
        OpusDecoder* decoder = opus_decoder_create(48000, 2, &error);
        REQUIRE(decoder != nullptr);
        REQUIRE(error == OPUS_OK);
        
        // Set encoder bitrate
        opus_encoder_ctl(encoder, OPUS_SET_BITRATE(64000));
        
        // Create test PCM data (silence)
        const int frameSize = 960; // 20ms at 48kHz
        const int channels = 2;
        std::vector<opus_int16> inputPCM(frameSize * channels, 0);
        std::vector<unsigned char> encodedData(4000, 0);
        std::vector<opus_int16> outputPCM(frameSize * channels, 0);
        
        // Encode
        int encodedBytes = opus_encode(encoder, inputPCM.data(), frameSize, 
                                       encodedData.data(), static_cast<opus_int32>(encodedData.size()));
        CHECK(encodedBytes > 0);
        
        // Decode
        if (encodedBytes > 0)
        {
            int decodedSamples = opus_decode(decoder, encodedData.data(), encodedBytes,
                                             outputPCM.data(), frameSize, 0);
            CHECK(decodedSamples == frameSize);
        }
        
        opus_encoder_destroy(encoder);
        opus_decoder_destroy(decoder);
    }

    TEST_CASE("Test Opus with different sample rates")
    {
        int error = 0;
        
        // Opus supports: 8kHz, 12kHz, 16kHz, 24kHz, and 48kHz
        // Test 8kHz (narrowband)
        OpusEncoder* encoder8 = opus_encoder_create(8000, 2, OPUS_APPLICATION_AUDIO, &error);
        CHECK(encoder8 != nullptr);
        CHECK(error == OPUS_OK);
        if (encoder8 != nullptr) opus_encoder_destroy(encoder8);
        
        // Test 16kHz (wideband)
        OpusEncoder* encoder16 = opus_encoder_create(16000, 2, OPUS_APPLICATION_AUDIO, &error);
        CHECK(encoder16 != nullptr);
        CHECK(error == OPUS_OK);
        if (encoder16 != nullptr) opus_encoder_destroy(encoder16);
        
        // Test 48kHz (fullband) - most common for high quality
        OpusEncoder* encoder48 = opus_encoder_create(48000, 2, OPUS_APPLICATION_AUDIO, &error);
        CHECK(encoder48 != nullptr);
        CHECK(error == OPUS_OK);
        if (encoder48 != nullptr) opus_encoder_destroy(encoder48);
    }

    TEST_CASE("Test Opus with different channel configurations")
    {
        int error = 0;
        
        // Test mono
        OpusEncoder* encoderMono = opus_encoder_create(48000, 1, OPUS_APPLICATION_AUDIO, &error);
        CHECK(encoderMono != nullptr);
        CHECK(error == OPUS_OK);
        if (encoderMono != nullptr) opus_encoder_destroy(encoderMono);
        
        // Test stereo
        OpusEncoder* encoderStereo = opus_encoder_create(48000, 2, OPUS_APPLICATION_AUDIO, &error);
        CHECK(encoderStereo != nullptr);
        CHECK(error == OPUS_OK);
        if (encoderStereo != nullptr) opus_encoder_destroy(encoderStereo);
    }

    TEST_CASE("Verify Opus library version")
    {
        // Get library version string
        const char* version = opus_get_version_string();
        CHECK(version != nullptr);
        
        if (version != nullptr)
        {
            std::string versionStr(version);
            CHECK(versionStr.length() > 0);
        }
    }

    TEST_CASE("Test Opus error codes")
    {
        // Verify error code constants are defined
        CHECK(OPUS_OK == 0);
        CHECK(OPUS_BAD_ARG < 0);
        CHECK(OPUS_BUFFER_TOO_SMALL < 0);
        CHECK(OPUS_INTERNAL_ERROR < 0);
        CHECK(OPUS_INVALID_PACKET < 0);
        CHECK(OPUS_UNIMPLEMENTED < 0);
        CHECK(OPUS_INVALID_STATE < 0);
    }
}
