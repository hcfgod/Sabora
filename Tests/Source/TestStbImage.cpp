/**
 * @file TestStbImage.cpp
 * @brief Basic load test for stb_image to validate integration.
 */

#include "doctest.h"

#define STBI_NO_STDIO
#include <stb_image.h>

// Minimal 1x1 transparent PNG (RGBA = 0,0,0,0)
// Sourced from a canonical tiny test fixture.
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

TEST_SUITE("stb_image")
{
    TEST_CASE("Load tiny PNG from memory")
    {
        int width = 0;
        int height = 0;
        int channels = 0;

        unsigned char* data = stbi_load_from_memory(
            kPng1x1Transparent,
            static_cast<int>(sizeof(kPng1x1Transparent)),
            &width,
            &height,
            &channels,
            4); // force RGBA

        REQUIRE(data != nullptr);
        CHECK(width == 1);
        CHECK(height == 1);
        CHECK(channels == 4);

        // Pixel should be fully transparent
        CHECK(data[0] == 0);
        CHECK(data[1] == 0);
        CHECK(data[2] == 0);
        CHECK(data[3] == 0);

        stbi_image_free(data);
    }
}
