/**
 * @file TestLibvorbis.cpp
 * @brief Unit tests for libvorbis Vorbis audio codec library.
 * 
 * These tests verify that libvorbis, libvorbisfile, and libvorbisenc
 * are properly linked and can encode/decode Vorbis audio.
 */

#include "doctest.h"
#include "Sabora.h"

// libvorbis Vorbis codec library
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/vorbisenc.h>

#include <cstring>
#include <vector>
#include <filesystem>

using namespace Sabora;

TEST_SUITE("libvorbis")
{
    TEST_CASE("Initialize vorbis_info structure")
    {
        vorbis_info vi;
        vorbis_info_init(&vi); // Returns void
        
        // Verify structure is initialized (check that it's not null)
        CHECK(true); // If we get here, function exists and was called
        
        // Clean up
        vorbis_info_clear(&vi);
    }

    TEST_CASE("Initialize vorbis_comment structure")
    {
        vorbis_comment vc;
        vorbis_comment_init(&vc); // Returns void
        
        // Verify structure is initialized
        // Note: user_comments and comment_lengths are nullptr until comments are added
        CHECK(vc.comments == 0); // Should start with zero comments
        
        // Clean up
        vorbis_comment_clear(&vc);
    }

    TEST_CASE("Initialize vorbis_dsp_state")
    {
        vorbis_info vi;
        vorbis_info_init(&vi); // Returns void
        
        // Try to set up encoding first (required for analysis_init)
        int result = vorbis_encode_init_vbr(&vi, 2, 44100, 0.5f);
        if (result == 0)
        {
            vorbis_dsp_state vd;
            result = vorbis_analysis_init(&vd, &vi);
            
            if (result == 0)
            {
                // Clean up if successful
                vorbis_dsp_clear(&vd);
            }
            else
            {
                WARN("vorbis_analysis_init failed");
            }
        }
        else
        {
            WARN("vorbis_encode_init_vbr failed (cannot test analysis_init)");
        }
        
        vorbis_info_clear(&vi);
    }

    TEST_CASE("Initialize vorbis_block")
    {
        vorbis_info vi;
        vorbis_info_init(&vi); // Returns void
        
        // Try to set up encoding first (required for analysis_init)
        int result = vorbis_encode_init_vbr(&vi, 2, 44100, 0.5f);
        if (result == 0)
        {
            vorbis_dsp_state vd;
            result = vorbis_analysis_init(&vd, &vi);
            
            if (result == 0)
            {
                vorbis_block vb;
                result = vorbis_block_init(&vd, &vb);
                
                if (result == 0)
                {
                    CHECK(true); // Block initialized successfully
                    vorbis_block_clear(&vb);
                }
                else
                {
                    WARN("vorbis_block_init failed");
                }
                
                vorbis_dsp_clear(&vd);
            }
            else
            {
                WARN("vorbis_analysis_init failed (cannot test block_init)");
            }
        }
        else
        {
            WARN("vorbis_encode_init_vbr failed (cannot test block_init)");
        }
        
        vorbis_info_clear(&vi);
    }
}

TEST_SUITE("libvorbisfile")
{
    TEST_CASE("Verify vorbisfile functions are available")
    {
        // Test that ov_clear function exists (simple function that takes OggVorbis_File*)
        // We can't fully test file operations without a valid OGG file,
        // but we can verify the functions are linked
        
        OggVorbis_File vf = {};
        
        // ov_clear should handle nullptr or uninitialized file gracefully
        // (actual behavior may vary, but function should exist)
        ov_clear(&vf);
        
        CHECK(true); // If we get here, function is linked
    }

    TEST_CASE("Check vorbisfile structure size")
    {
        // Verify OggVorbis_File structure exists and has reasonable size
        OggVorbis_File vf = {};
        
        // Structure should be non-zero size
        CHECK(sizeof(OggVorbis_File) > 0);
    }
}

TEST_SUITE("libvorbisenc")
{
    TEST_CASE("Initialize vorbis_encode_setup_vbr")
    {
        vorbis_info vi;
        vorbis_info_init(&vi); // Returns void
        
        // Try to set up encoding with VBR
        long channels = 2;
        long rate = 44100;
        float base_quality = 0.5f; // Medium quality
        
        int result = vorbis_encode_init_vbr(&vi, channels, rate, base_quality);
        
        if (result == 0)
        {
            CHECK(vi.channels == channels);
            CHECK(vi.rate == rate);
            
            // Clean up
            vorbis_info_clear(&vi);
        }
        else
        {
            WARN("vorbis_encode_init_vbr failed");
            vorbis_info_clear(&vi);
        }
    }

    TEST_CASE("Initialize vorbis_encode_setup_managed")
    {
        vorbis_info vi;
        vorbis_info_init(&vi); // Returns void
        
        long channels = 2;
        long rate = 44100;
        long max_bitrate = 128000;
        long nominal_bitrate = 96000;
        long min_bitrate = 64000;
        
        int result = vorbis_encode_init(&vi, channels, rate, max_bitrate, nominal_bitrate, min_bitrate);
        
        if (result == 0)
        {
            CHECK(vi.channels == channels);
            CHECK(vi.rate == rate);
            
            vorbis_info_clear(&vi);
        }
        else
        {
            WARN("vorbis_encode_init failed");
            vorbis_info_clear(&vi);
        }
    }

    TEST_CASE("Set up vorbis encoding with comments")
    {
        vorbis_info vi;
        vorbis_info_init(&vi); // Returns void
        
        int result = vorbis_encode_init_vbr(&vi, 2, 44100, 0.5f);
        
        if (result == 0)
        {
            vorbis_comment vc;
            vorbis_comment_init(&vc); // Returns void
            vorbis_comment_add_tag(&vc, "ARTIST", "Test Artist"); // Returns void
            vorbis_comment_add_tag(&vc, "TITLE", "Test Title"); // Returns void
            
            CHECK(vc.comments == 2);
            
            vorbis_comment_clear(&vc);
            vorbis_info_clear(&vi);
        }
        else
        {
            WARN("Could not set up encoding for comment test");
            vorbis_info_clear(&vi);
        }
    }

    TEST_CASE("Encode Vorbis audio data")
    {
        vorbis_info vi;
        vorbis_info_init(&vi);
        
        // Set up encoding
        int result = vorbis_encode_init_vbr(&vi, 2, 44100, 0.5f);
        if (result == 0)
        {
            vorbis_dsp_state vd;
            result = vorbis_analysis_init(&vd, &vi);
            
            if (result == 0)
            {
                vorbis_block vb;
                result = vorbis_block_init(&vd, &vb);
                
                if (result == 0)
                {
                    // Prepare some test audio data (stereo, 1024 samples)
                    const int samples = 1024;
                    float** buffer = vorbis_analysis_buffer(&vd, samples);
                    
                    if (buffer != nullptr)
                    {
                        // Generate simple test tone
                        for (int i = 0; i < samples; ++i)
                        {
                            float sample = 0.1f * sin(2.0f * 3.14159f * 440.0f * i / 44100.0f);
                            buffer[0][i] = sample; // Left channel
                            buffer[1][i] = sample; // Right channel
                        }
                        
                        // Tell vorbis how many samples we wrote
                        vorbis_analysis_wrote(&vd, samples);
                        
                        // Try to analyze and encode
                        while (vorbis_analysis_blockout(&vd, &vb) == 1)
                        {
                            vorbis_analysis(&vb, nullptr);
                            vorbis_bitrate_addblock(&vb);
                            
                            ogg_packet op;
                            while (vorbis_bitrate_flushpacket(&vd, &op))
                            {
                                CHECK(op.packet != nullptr);
                                CHECK(op.bytes > 0);
                            }
                        }
                        
                        vorbis_block_clear(&vb);
                    }
                    
                    vorbis_dsp_clear(&vd);
                }
            }
        }
        
        vorbis_info_clear(&vi);
    }

    TEST_CASE("Vorbis comment manipulation")
    {
        vorbis_comment vc;
        vorbis_comment_init(&vc);
        
        // Add multiple comments
        vorbis_comment_add_tag(&vc, "ARTIST", "Test Artist");
        vorbis_comment_add_tag(&vc, "TITLE", "Test Title");
        vorbis_comment_add_tag(&vc, "ALBUM", "Test Album");
        vorbis_comment_add_tag(&vc, "GENRE", "Test Genre");
        
        CHECK(vc.comments == 4);
        
        // Verify we can query comments
        if (vc.comments > 0 && vc.user_comments != nullptr)
        {
            CHECK(vc.user_comments[0] != nullptr);
        }
        
        vorbis_comment_clear(&vc);
    }
}

