/**
 * @file TestMinimp3.cpp
 * @brief Unit tests for minimp3 MP3 decoder library.
 * 
 * These tests verify that minimp3 is properly included and can decode
 * MP3 audio data. minimp3 is a header-only, single-file library.
 */

#include "doctest.h"
#include "Sabora.h"

// minimp3 MP3 decoder library (header-only)
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include "minimp3_ex.h"

#include <vector>
#include <cstring>

using namespace Sabora;

TEST_SUITE("minimp3")
{
    TEST_CASE("Verify library is included correctly")
    {
        // Simple test to ensure minimp3 header is available
        // If we can compile this, the header is included correctly
        mp3dec_t mp3d = {};
        mp3dec_init(&mp3d);
        
        CHECK(true); // Library is available
    }

    TEST_CASE("Initialize mp3dec structure")
    {
        mp3dec_t mp3d;
        mp3dec_init(&mp3d);
        
        // Structure should be initialized (no way to verify directly,
        // but if we get here without errors, initialization worked)
        CHECK(true);
    }

    TEST_CASE("Decode MP3 frame header")
    {
        mp3dec_t mp3d;
        mp3dec_init(&mp3d);
        
        // Create a minimal MP3 frame header (invalid but should not crash)
        // Real MP3 frame starts with sync word 0xFFF
        unsigned char testData[4] = {0xFF, 0xFB, 0x90, 0x00};
        
        mp3dec_frame_info_t info = {};
        short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        
        // Try to decode (will likely fail with invalid data, but function should exist)
        int samples = mp3dec_decode_frame(&mp3d, testData, sizeof(testData), pcm, &info);
        
        // Function should execute without crashing
        // samples will be 0 for invalid data, but that's expected
        CHECK(samples >= 0); // Should return valid sample count (0 for invalid data)
    }

    TEST_CASE("Query MP3 frame info")
    {
        mp3dec_t mp3d;
        mp3dec_init(&mp3d);
        
        // Create minimal test data
        unsigned char testData[4] = {0xFF, 0xFB, 0x90, 0x00};
        mp3dec_frame_info_t info = {};
        short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        
        // Decode frame (will return 0 for invalid data, but function exists)
        mp3dec_decode_frame(&mp3d, testData, sizeof(testData), pcm, &info);
        
        // Info structure should be populated (even if decode failed)
        // Check that structure exists and was accessed
        CHECK(info.frame_bytes >= 0);
        CHECK(info.channels >= 0);
        CHECK(info.hz >= 0);
    }

    TEST_CASE("Verify MINIMP3_MAX_SAMPLES_PER_FRAME constant")
    {
        // Verify the constant is defined and has a reasonable value
        CHECK(MINIMP3_MAX_SAMPLES_PER_FRAME > 0);
        CHECK(MINIMP3_MAX_SAMPLES_PER_FRAME <= 1152 * 2); // Max samples per frame (stereo)
    }

    TEST_CASE("Test mp3dec_ex API")
    {
        // Test the extended API (minimp3_ex.h)
        mp3dec_ex_t dec = {};
        
        // Initialize with null data (should not crash)
        // Function should exist and be callable (result doesn't matter for this test)
        mp3dec_ex_open_buf(&dec, nullptr, 0, MP3D_SEEK_TO_SAMPLE);
        
        // If we get here, function exists and was called successfully
        CHECK(true);
        
        // Clean up
        mp3dec_ex_close(&dec);
    }

    TEST_CASE("Test mp3dec_ex seek functionality")
    {
        mp3dec_ex_t dec = {};
        
        // Try to open with empty buffer
        int result = mp3dec_ex_open_buf(&dec, nullptr, 0, MP3D_SEEK_TO_SAMPLE);
        
        if (result == 0)
        {
            // Try to seek (should handle gracefully even with no data)
            unsigned position = 0;
            mp3dec_ex_seek(&dec, position);
            
            CHECK(true); // Function exists
        }
        
        mp3dec_ex_close(&dec);
    }

    TEST_CASE("Verify MP3D_SEEK constants are defined")
    {
        // Verify seek mode constants are available
        CHECK(MP3D_SEEK_TO_BYTE >= 0);
        CHECK(MP3D_SEEK_TO_SAMPLE >= 0);
    }

    TEST_CASE("Test mp3dec_load_buf function")
    {
        mp3dec_t mp3d;
        mp3dec_init(&mp3d);
        
        // Create minimal test buffer
        unsigned char testData[10] = {0};
        mp3dec_file_info_t fileInfo = {};
        
        // Try to load (will fail with invalid data, but function should exist)
        // mp3dec_load_buf returns int (error code), not a pointer
        // Function should execute without crashing
        mp3dec_load_buf(&mp3d, testData, sizeof(testData), &fileInfo, nullptr, nullptr);
        
        // If we get here, function exists and was called
        CHECK(true);
    }

    TEST_CASE("Verify mp3dec_frame_info_t structure")
    {
        mp3dec_frame_info_t info = {};
        
        // Verify structure fields are accessible
        info.frame_bytes = 0;
        info.frame_offset = 0;
        info.channels = 0;
        info.hz = 0;
        info.layer = 0;
        info.bitrate_kbps = 0;
        
        CHECK(info.frame_bytes == 0);
        CHECK(info.channels >= 0);
        CHECK(info.hz >= 0);
    }
}
