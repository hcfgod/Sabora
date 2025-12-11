/**
 * @file TestLibsndfile.cpp
 * @brief Unit tests for libsndfile audio file I/O library.
 * 
 * These tests verify that libsndfile is properly linked and can read/write
 * audio files, query format information, and handle various audio formats.
 */

#include "doctest.h"
#include "Sabora.h"

// libsndfile audio file I/O library
#include <sndfile.h>

#include <string>
#include <vector>
#include <cstring>
#include <cmath>
#include <filesystem>

using namespace Sabora;

TEST_SUITE("libsndfile")
{
    TEST_CASE("Get library version")
    {
        const char* version = sf_version_string();
        CHECK(version != nullptr);
        
        if (version != nullptr)
        {
            std::string versionStr(version);
            CHECK(versionStr.length() > 0);
        }
    }

    TEST_CASE("Get format major count")
    {
        int formatCount = 0;
        int result = sf_command(nullptr, SFC_GET_FORMAT_MAJOR_COUNT, &formatCount, sizeof(int));
        
        CHECK(result == 0); // Success
        CHECK(formatCount > 0); // Should support at least some formats
        
        // Verify we can query format info for at least one format
        if (formatCount > 0)
        {
            SF_FORMAT_INFO formatInfo = {};
            formatInfo.format = SF_FORMAT_WAV;
            result = sf_command(nullptr, SFC_GET_FORMAT_INFO, &formatInfo, sizeof(SF_FORMAT_INFO));
            CHECK(result == 0);
        }
    }

    TEST_CASE("Query WAV format information")
    {
        SF_FORMAT_INFO formatInfo = {};
        formatInfo.format = SF_FORMAT_WAV;
        
        int result = sf_command(nullptr, SFC_GET_FORMAT_INFO, &formatInfo, sizeof(SF_FORMAT_INFO));
        
        // Should succeed and provide format information
        CHECK(result == 0);
        CHECK(formatInfo.format == SF_FORMAT_WAV);
        CHECK(formatInfo.name != nullptr);
    }

    TEST_CASE("Query AIFF format information")
    {
        SF_FORMAT_INFO formatInfo = {};
        formatInfo.format = SF_FORMAT_AIFF;
        
        int result = sf_command(nullptr, SFC_GET_FORMAT_INFO, &formatInfo, sizeof(SF_FORMAT_INFO));
        
        // Should succeed and provide format information
        CHECK(result == 0);
        CHECK(formatInfo.format == SF_FORMAT_AIFF);
        CHECK(formatInfo.name != nullptr);
    }

    TEST_CASE("Query FLAC format information")
    {
        SF_FORMAT_INFO formatInfo = {};
        formatInfo.format = SF_FORMAT_FLAC;
        
        int result = sf_command(nullptr, SFC_GET_FORMAT_INFO, &formatInfo, sizeof(SF_FORMAT_INFO));
        
        // May or may not be available depending on build configuration
        if (result == 0)
        {
            CHECK(formatInfo.format == SF_FORMAT_FLAC);
            CHECK(formatInfo.name != nullptr);
        }
        else
        {
            WARN("FLAC format not available (may require external libraries)");
        }
    }

    TEST_CASE("Query OGG format information")
    {
        SF_FORMAT_INFO formatInfo = {};
        formatInfo.format = SF_FORMAT_OGG;
        
        int result = sf_command(nullptr, SFC_GET_FORMAT_INFO, &formatInfo, sizeof(SF_FORMAT_INFO));
        
        // May or may not be available depending on build configuration
        if (result == 0)
        {
            CHECK(formatInfo.format == SF_FORMAT_OGG);
            CHECK(formatInfo.name != nullptr);
        }
        else
        {
            WARN("OGG format not available (may require external libraries)");
        }
    }

    TEST_CASE("Create and write WAV file")
    {
        // Create a temporary test file path
        std::string testFilePath = "test_libsndfile_output.wav";
        
        // Remove file if it exists
        std::filesystem::remove(testFilePath);
        
        // Set up file info for WAV format
        SF_INFO sfinfo = {};
        sfinfo.samplerate = 44100;
        sfinfo.channels = 2;
        sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        
        // Open file for writing
        SNDFILE* file = sf_open(testFilePath.c_str(), SFM_WRITE, &sfinfo);
        
        if (file == nullptr)
        {
            // File creation might fail in some test environments
            WARN("Could not create test WAV file");
            return;
        }
        
        CHECK(file != nullptr);
        
        // Generate a simple sine wave (1 second of audio)
        const int sampleCount = sfinfo.samplerate * sfinfo.channels;
        std::vector<short> samples(sampleCount);
        
        const double frequency = 440.0; // A4 note
        for (int i = 0; i < sfinfo.samplerate; ++i)
        {
            double sample = 0.3 * 32767.0 * sin(2.0 * 3.14159265358979323846 * frequency * i / sfinfo.samplerate);
            samples[i * 2] = static_cast<short>(sample);     // Left channel
            samples[i * 2 + 1] = static_cast<short>(sample); // Right channel
        }
        
        // Write samples to file
        sf_count_t framesWritten = sf_write_short(file, samples.data(), sfinfo.samplerate);
        CHECK(framesWritten == sfinfo.samplerate);
        
        // Close file
        int closeResult = sf_close(file);
        CHECK(closeResult == 0);
        
        // Verify file was created
        CHECK(std::filesystem::exists(testFilePath));
        
        // Clean up
        std::filesystem::remove(testFilePath);
    }

    TEST_CASE("Read WAV file")
    {
        // First create a test file
        std::string testFilePath = "test_libsndfile_read.wav";
        std::filesystem::remove(testFilePath);
        
        // Create a simple WAV file
        {
            SF_INFO sfinfo = {};
            sfinfo.samplerate = 44100;
            sfinfo.channels = 1;
            sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
            
            SNDFILE* file = sf_open(testFilePath.c_str(), SFM_WRITE, &sfinfo);
            if (file == nullptr)
            {
                WARN("Could not create test WAV file for reading test");
                return;
            }
            
            // Write a few samples
            std::vector<short> samples(1000, 0);
            sf_write_short(file, samples.data(), 1000);
            sf_close(file);
        }
        
        // Now read it back
        SF_INFO sfinfo = {};
        SNDFILE* file = sf_open(testFilePath.c_str(), SFM_READ, &sfinfo);
        
        if (file == nullptr)
        {
            WARN("Could not open test WAV file for reading");
            std::filesystem::remove(testFilePath);
            return;
        }
        
        CHECK(file != nullptr);
        CHECK(sfinfo.samplerate == 44100);
        CHECK(sfinfo.channels == 1);
        CHECK(sfinfo.format == (SF_FORMAT_WAV | SF_FORMAT_PCM_16));
        CHECK(sfinfo.frames > 0);
        
        // Read some samples
        std::vector<short> readSamples(1000);
        sf_count_t framesRead = sf_read_short(file, readSamples.data(), 1000);
        CHECK(framesRead == 1000);
        
        // Close file
        int closeResult = sf_close(file);
        CHECK(closeResult == 0);
        
        // Clean up
        std::filesystem::remove(testFilePath);
    }

    TEST_CASE("Query file information")
    {
        // Create a test file
        std::string testFilePath = "test_libsndfile_info.wav";
        std::filesystem::remove(testFilePath);
        
        SF_INFO sfinfo = {};
        sfinfo.samplerate = 48000;
        sfinfo.channels = 2;
        sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        
        SNDFILE* file = sf_open(testFilePath.c_str(), SFM_WRITE, &sfinfo);
        if (file == nullptr)
        {
            WARN("Could not create test WAV file for info test");
            return;
        }
        
        // Write some data
        std::vector<short> samples(100, 0);
        sf_write_short(file, samples.data(), 100);
        
        // Query file information
        SF_INFO queryInfo = {};
        int result = sf_command(file, SFC_GET_CURRENT_SF_INFO, &queryInfo, sizeof(SF_INFO));
        CHECK(result == 0);
        CHECK(queryInfo.samplerate == 48000);
        CHECK(queryInfo.channels == 2);
        
        sf_close(file);
        std::filesystem::remove(testFilePath);
    }

    TEST_CASE("Error handling - invalid file path")
    {
        SF_INFO sfinfo = {};
        sfinfo.samplerate = 44100;
        sfinfo.channels = 1;
        sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        
        // Try to open a file in a non-existent directory
        SNDFILE* file = sf_open("/nonexistent/path/test.wav", SFM_READ, &sfinfo);
        CHECK(file == nullptr);
        
        // Check error code
        int error = sf_error(nullptr);
        CHECK(error != SF_ERR_NO_ERROR);
    }

    TEST_CASE("Error handling - invalid format")
    {
        std::string testFilePath = "test_invalid_format.wav";
        std::filesystem::remove(testFilePath);
        
        SF_INFO sfinfo = {};
        sfinfo.samplerate = 44100;
        sfinfo.channels = 1;
        sfinfo.format = 0xFFFFFFFF; // Invalid format
        
        SNDFILE* file = sf_open(testFilePath.c_str(), SFM_WRITE, &sfinfo);
        CHECK(file == nullptr);
        
        // Check error code
        int error = sf_error(nullptr);
        CHECK(error != SF_ERR_NO_ERROR);
    }

    TEST_CASE("Get string error message")
    {
        // Generate an error by trying to open non-existent file
        SF_INFO sfinfo = {};
        SNDFILE* file = sf_open("nonexistent_file.wav", SFM_READ, &sfinfo);
        CHECK(file == nullptr);
        
        // Get error string
        const char* errorStr = sf_strerror(nullptr);
        CHECK(errorStr != nullptr);
        
        if (errorStr != nullptr)
        {
            std::string errorString(errorStr);
            CHECK(errorString.length() > 0);
        }
    }

    TEST_CASE("Seek in file")
    {
        // Create a test file
        std::string testFilePath = "test_libsndfile_seek.wav";
        std::filesystem::remove(testFilePath);
        
        SF_INFO sfinfo = {};
        sfinfo.samplerate = 44100;
        sfinfo.channels = 1;
        sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        
        SNDFILE* file = sf_open(testFilePath.c_str(), SFM_WRITE, &sfinfo);
        if (file == nullptr)
        {
            WARN("Could not create test WAV file for seek test");
            return;
        }
        
        // Write samples
        std::vector<short> samples(1000);
        for (int i = 0; i < 1000; ++i)
        {
            samples[i] = static_cast<short>(i);
        }
        sf_write_short(file, samples.data(), 1000);
        sf_close(file);
        
        // Open for reading
        file = sf_open(testFilePath.c_str(), SFM_READ, &sfinfo);
        REQUIRE(file != nullptr);
        
        // Seek to middle of file
        sf_count_t seekPos = sf_seek(file, 500, SEEK_SET);
        CHECK(seekPos == 500);
        
        // Read from seek position
        short sample = 0;
        sf_count_t framesRead = sf_read_short(file, &sample, 1);
        CHECK(framesRead == 1);
        CHECK(sample == 500); // Should match the value we wrote
        
        sf_close(file);
        std::filesystem::remove(testFilePath);
    }

    TEST_CASE("Get file frame count")
    {
        // Create a test file
        std::string testFilePath = "test_libsndfile_frames.wav";
        std::filesystem::remove(testFilePath);
        
        SF_INFO sfinfo = {};
        sfinfo.samplerate = 44100;
        sfinfo.channels = 1;
        sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        
        SNDFILE* file = sf_open(testFilePath.c_str(), SFM_WRITE, &sfinfo);
        if (file == nullptr)
        {
            WARN("Could not create test WAV file for frame count test");
            return;
        }
        
        const sf_count_t expectedFrames = 2000;
        std::vector<short> samples(expectedFrames, 0);
        sf_write_short(file, samples.data(), expectedFrames);
        sf_close(file);
        
        // Open for reading
        file = sf_open(testFilePath.c_str(), SFM_READ, &sfinfo);
        REQUIRE(file != nullptr);
        
        CHECK(sfinfo.frames == expectedFrames);
        
        sf_close(file);
        std::filesystem::remove(testFilePath);
    }
}

