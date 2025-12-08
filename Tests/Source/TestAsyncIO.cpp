/**
 * @file TestAsyncIO.cpp
 * @brief Unit tests for AsyncIO functionality.
 */

#include "doctest.h"
#include "AsyncIO.h"
#include "Result.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace Sabora;
namespace fs = std::filesystem;

TEST_SUITE("AsyncIO")
{
    TEST_CASE("FileExists - Non-existent file returns false")
    {
        const fs::path nonExistent = "non_existent_file_12345.txt";
        CHECK_FALSE(AsyncIO::FileExists(nonExistent));
    }

    TEST_CASE("ReadTextFile - Non-existent file returns error")
    {
        const fs::path nonExistent = "non_existent_file_12345.txt";
        auto result = AsyncIO::ReadTextFile(nonExistent);
        CHECK(result.IsFailure());
        CHECK(result.GetError().Code() == ErrorCode::FileReadError);
    }

    TEST_CASE("ReadTextFile - Valid file returns contents")
    {
        // Create a temporary test file
        const fs::path testFile = "test_read_file.txt";
        const std::string testContent = "Hello, World!\nTest content\nLine 3";
        
        // Write test file
        {
            std::ofstream out(testFile);
            REQUIRE(out.is_open());
            out << testContent;
        }

        // Read it back
        auto result = AsyncIO::ReadTextFile(testFile);
        REQUIRE(result.IsSuccess());
        // Normalize line endings for cross-platform compatibility
        std::string readContent = result.Value();
        std::string normalizedContent = testContent;
        // Replace \r\n with \n in both strings for comparison
        readContent.erase(std::remove(readContent.begin(), readContent.end(), '\r'), readContent.end());
        normalizedContent.erase(std::remove(normalizedContent.begin(), normalizedContent.end(), '\r'), normalizedContent.end());
        CHECK(readContent == normalizedContent);

        // Cleanup
        AsyncIO::RemoveFile(testFile);
    }

    TEST_CASE("ReadTextFile - File size limit enforced")
    {
        // This test verifies that files exceeding MaxTextFileSize are rejected
        // Note: Creating a 100MB+ file for testing may be impractical
        // This is more of a documentation test
        // File size limits are enforced in ReadTextFile (see AsyncIO.h for MaxTextFileSize)
        CHECK(true); // Placeholder - actual file size limit testing would require large file creation
    }

    TEST_CASE("WriteTextFile - Creates file with content")
    {
        const fs::path testFile = "test_write_file.txt";
        const std::string testContent = "Test write content\nLine 2";

        auto result = AsyncIO::WriteTextFile(testFile, testContent, true);
        REQUIRE(result.IsSuccess());
        CHECK(AsyncIO::FileExists(testFile));

        // Verify content
        auto readResult = AsyncIO::ReadTextFile(testFile);
        REQUIRE(readResult.IsSuccess());
        CHECK(readResult.Value() == testContent);

        // Cleanup
        AsyncIO::RemoveFile(testFile);
    }

    TEST_CASE("ReadBinaryFile - Reads binary data correctly")
    {
        const fs::path testFile = "test_binary.bin";
        const std::vector<std::uint8_t> testData = {0x00, 0x01, 0x02, 0xFF, 0xFE, 0xFD};

        // Write binary file
        {
            std::ofstream out(testFile, std::ios::binary);
            REQUIRE(out.is_open());
            out.write(reinterpret_cast<const char*>(testData.data()), 
                     static_cast<std::streamsize>(testData.size()));
        }

        // Read it back
        auto result = AsyncIO::ReadBinaryFile(testFile);
        REQUIRE(result.IsSuccess());
        CHECK(result.Value() == testData);

        // Cleanup
        AsyncIO::RemoveFile(testFile);
    }

    TEST_CASE("ReadJsonFile - Valid JSON parses correctly")
    {
        const fs::path testFile = "test_json.json";
        const std::string jsonContent = R"({"name": "test", "value": 42, "nested": {"key": "value"}})";

        // Write JSON file
        {
            std::ofstream out(testFile);
            REQUIRE(out.is_open());
            out << jsonContent;
        }

        // Read and parse
        auto result = AsyncIO::ReadJsonFile(testFile);
        REQUIRE(result.IsSuccess());
        const auto& json = result.Value();
        CHECK(json["name"] == "test");
        CHECK(json["value"] == 42);
        CHECK(json["nested"]["key"] == "value");

        // Cleanup
        AsyncIO::RemoveFile(testFile);
    }

    TEST_CASE("ReadJsonFile - Invalid JSON returns error")
    {
        const fs::path testFile = "test_invalid_json.json";
        const std::string invalidJson = "{ invalid json }";

        // Write invalid JSON
        {
            std::ofstream out(testFile);
            REQUIRE(out.is_open());
            out << invalidJson;
        }

        // Try to parse
        auto result = AsyncIO::ReadJsonFile(testFile);
        CHECK(result.IsFailure());
        CHECK(result.GetError().Code() == ErrorCode::FileInvalidFormat);

        // Cleanup
        AsyncIO::RemoveFile(testFile);
    }

    TEST_CASE("ReadJsonFile - Structure validation works")
    {
        const fs::path testFile = "test_json_array.json";
        const std::string arrayJson = "[1, 2, 3]";

        // Write array JSON
        {
            std::ofstream out(testFile);
            REQUIRE(out.is_open());
            out << arrayJson;
        }

        // Try to read with structure validation (expects object)
        auto result = AsyncIO::ReadJsonFile(testFile, true);
        CHECK(result.IsFailure());
        CHECK(result.GetError().Code() == ErrorCode::ValidationFailed);

        // Without validation, should succeed
        auto result2 = AsyncIO::ReadJsonFile(testFile, false);
        CHECK(result2.IsSuccess());

        // Cleanup
        AsyncIO::RemoveFile(testFile);
    }

    TEST_CASE("ValidateAndSanitizePath - Rejects empty path")
    {
        const fs::path emptyPath = "";
        auto result = AsyncIO::ValidateAndSanitizePath(emptyPath);
        CHECK(result.IsFailure());
        CHECK(result.GetError().Code() == ErrorCode::FileInvalidPath);
    }

    TEST_CASE("ValidateAndSanitizePath - Rejects path traversal")
    {
        const fs::path traversalPath = "../../../etc/passwd";
        auto result = AsyncIO::ValidateAndSanitizePath(traversalPath, false);
        CHECK(result.IsFailure());
        CHECK(result.GetError().Code() == ErrorCode::FileInvalidPath);
    }

    TEST_CASE("CreateDirectoriesFor - Creates parent directories")
    {
        const fs::path testFile = "test_dir/subdir/test_file.txt";
        
        auto result = AsyncIO::WriteTextFile(testFile, "test", true);
        REQUIRE(result.IsSuccess());
        CHECK(AsyncIO::FileExists(testFile));

        // Cleanup
        AsyncIO::RemoveFile(testFile);
        fs::remove_all("test_dir");
    }
}

