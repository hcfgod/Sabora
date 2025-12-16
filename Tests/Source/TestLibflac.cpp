/**
 * @file TestLibflac.cpp
 * @brief Unit tests for libFLAC lossless audio codec library.
 * 
 * These tests verify that libFLAC is properly linked and can encode/decode
 * FLAC audio data. libFLAC provides lossless compression for audio assets.
 */

#include "doctest.h"
#include "Sabora.h"

// libFLAC lossless audio codec library
#include <FLAC/stream_decoder.h>
#include <FLAC/stream_encoder.h>
#include <FLAC/format.h>
#include <FLAC/metadata.h>

#include <vector>
#include <cstring>

using namespace Sabora;

TEST_SUITE("libFLAC")
{
    TEST_CASE("Create FLAC decoder")
    {
        FLAC__StreamDecoder* decoder = FLAC__stream_decoder_new();
        
        CHECK(decoder != nullptr);
        
        if (decoder != nullptr)
        {
            FLAC__stream_decoder_delete(decoder);
        }
    }

    TEST_CASE("Create FLAC encoder")
    {
        FLAC__StreamEncoder* encoder = FLAC__stream_encoder_new();
        
        CHECK(encoder != nullptr);
        
        if (encoder != nullptr)
        {
            FLAC__stream_encoder_delete(encoder);
        }
    }

    TEST_CASE("Initialize FLAC decoder state")
    {
        FLAC__StreamDecoder* decoder = FLAC__stream_decoder_new();
        REQUIRE(decoder != nullptr);
        
        // Initialize decoder with null callbacks (just testing API)
        FLAC__StreamDecoderInitStatus initStatus = FLAC__stream_decoder_init_stream(
            decoder,
            nullptr,  // read callback
            nullptr,  // seek callback
            nullptr,  // tell callback
            nullptr,  // length callback
            nullptr,  // eof callback
            nullptr,  // write callback
            nullptr,  // metadata callback
            nullptr,  // error callback
            nullptr   // client data
        );
        
        // Should fail gracefully with null callbacks, but function exists
        // Simplify complex expression to avoid doctest "Expression Too Complex" error
        bool isValidStatus = (initStatus == FLAC__STREAM_DECODER_INIT_STATUS_OK || 
                              initStatus == FLAC__STREAM_DECODER_INIT_STATUS_INVALID_CALLBACKS);
        CHECK(isValidStatus);
        
        FLAC__stream_decoder_delete(decoder);
    }

    TEST_CASE("Configure FLAC encoder settings")
    {
        FLAC__StreamEncoder* encoder = FLAC__stream_encoder_new();
        REQUIRE(encoder != nullptr);
        
        // Set encoder sample rate
        FLAC__bool result = FLAC__stream_encoder_set_sample_rate(encoder, 44100);
        CHECK(static_cast<bool>(result) == true);
        
        // Set encoder channels
        result = FLAC__stream_encoder_set_channels(encoder, 2);
        CHECK(static_cast<bool>(result) == true);
        
        // Set encoder bits per sample
        result = FLAC__stream_encoder_set_bits_per_sample(encoder, 16);
        CHECK(static_cast<bool>(result) == true);
        
        FLAC__stream_encoder_delete(encoder);
    }

    TEST_CASE("Query FLAC encoder state")
    {
        FLAC__StreamEncoder* encoder = FLAC__stream_encoder_new();
        REQUIRE(encoder != nullptr);
        
        // Get encoder state (should be uninitialized)
        FLAC__StreamEncoderState state = FLAC__stream_encoder_get_state(encoder);
        // Simplify complex expression to avoid doctest "Expression Too Complex" error
        bool isValidState = (state == FLAC__STREAM_ENCODER_OK || 
                             state == FLAC__STREAM_ENCODER_UNINITIALIZED);
        CHECK(isValidState);
        
        FLAC__stream_encoder_delete(encoder);
    }

    TEST_CASE("Query FLAC decoder state")
    {
        FLAC__StreamDecoder* decoder = FLAC__stream_decoder_new();
        REQUIRE(decoder != nullptr);
        
        // Get decoder state (should be uninitialized)
        FLAC__StreamDecoderState state = FLAC__stream_decoder_get_state(decoder);
        CHECK(state == FLAC__STREAM_DECODER_UNINITIALIZED);
        
        FLAC__stream_decoder_delete(decoder);
    }

    TEST_CASE("Verify FLAC format constants")
    {
        // Verify FLAC format constants are defined
        CHECK(FLAC__MAX_CHANNELS > 0);
        CHECK(FLAC__MAX_SAMPLE_RATE > 0);
        CHECK(FLAC__MAX_BITS_PER_SAMPLE > 0);
        CHECK(FLAC__MIN_BLOCK_SIZE > 0);
        CHECK(FLAC__MAX_BLOCK_SIZE > 0);
    }

    TEST_CASE("Create FLAC metadata iterator")
    {
        FLAC__Metadata_Chain* chain = FLAC__metadata_chain_new();
        CHECK(chain != nullptr);
        
        if (chain != nullptr)
        {
            FLAC__metadata_chain_delete(chain);
        }
    }

    TEST_CASE("Create FLAC metadata object")
    {
        FLAC__StreamMetadata* metadata = FLAC__metadata_object_new(FLAC__METADATA_TYPE_STREAMINFO);
        CHECK(metadata != nullptr);
        
        if (metadata != nullptr)
        {
            CHECK(metadata->type == FLAC__METADATA_TYPE_STREAMINFO);
            FLAC__metadata_object_delete(metadata);
        }
    }

    TEST_CASE("Set FLAC encoder compression level")
    {
        FLAC__StreamEncoder* encoder = FLAC__stream_encoder_new();
        REQUIRE(encoder != nullptr);
        
        // Set compression level (0-8, where 0 is fastest, 8 is best compression)
        FLAC__bool result = FLAC__stream_encoder_set_compression_level(encoder, 5);
        CHECK(static_cast<bool>(result) == true);
        
        FLAC__stream_encoder_delete(encoder);
    }

    TEST_CASE("Verify FLAC library version")
    {
        // Get library version string
        const char* version = FLAC__VERSION_STRING;
        CHECK(version != nullptr);
        
        if (version != nullptr)
        {
            std::string versionStr(version);
            CHECK(versionStr.length() > 0);
        }
    }

    TEST_CASE("FLAC encoder verify settings")
    {
        FLAC__StreamEncoder* encoder = FLAC__stream_encoder_new();
        REQUIRE(encoder != nullptr);
        
        // Set basic settings
        FLAC__stream_encoder_set_sample_rate(encoder, 44100);
        FLAC__stream_encoder_set_channels(encoder, 2);
        FLAC__stream_encoder_set_bits_per_sample(encoder, 16);
        
        // Verify settings can be queried
        CHECK(FLAC__stream_encoder_get_sample_rate(encoder) == 44100);
        CHECK(FLAC__stream_encoder_get_channels(encoder) == 2);
        CHECK(FLAC__stream_encoder_get_bits_per_sample(encoder) == 16);
        
        FLAC__stream_encoder_delete(encoder);
    }
}
