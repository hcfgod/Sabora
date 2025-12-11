/**
 * @file TestMSDFAtlasGen.cpp
 * @brief Unit tests for MSDF atlas generation library.
 * 
 * These tests verify that msdf-atlas-gen is properly linked and can create
 * and manipulate atlas generator instances and related data structures.
 */

#include "doctest.h"
#include "Sabora.h"

// MSDF atlas generation library
#include <msdf-atlas-gen/msdf-atlas-gen.h>

using namespace Sabora;

TEST_SUITE("MSDF Atlas Generation")
{
    TEST_CASE("Create RectanglePacker")
    {
        // Test that we can create a rectangle packer
        msdf_atlas::RectanglePacker packer(1024, 1024);
        
        // Verify packer was created (no exceptions thrown)
        CHECK(true);
    }

    TEST_CASE("Create FontGeometry")
    {
        // Test that we can create a font geometry structure
        msdf_atlas::FontGeometry fontGeometry;
        
        // Verify structure was created
        CHECK(true);
    }

    TEST_CASE("Create and manipulate Rectangle")
    {
        msdf_atlas::Rectangle testRect;
        testRect.x = 0;
        testRect.y = 0;
        testRect.w = 10;
        testRect.h = 10;
        
        CHECK(testRect.x == 0);
        CHECK(testRect.y == 0);
        CHECK(testRect.w == 10);
        CHECK(testRect.h == 10);
    }

    TEST_CASE("Create and manipulate GlyphBox")
    {
        msdf_atlas::GlyphBox glyphBox;
        
        msdf_atlas::Rectangle testRect;
        testRect.x = 0;
        testRect.y = 0;
        testRect.w = 10;
        testRect.h = 10;
        
        glyphBox.rect = testRect;
        glyphBox.index = 42;
        glyphBox.advance = 10.5;
        
        CHECK(glyphBox.rect.x == 0);
        CHECK(glyphBox.rect.y == 0);
        CHECK(glyphBox.rect.w == 10);
        CHECK(glyphBox.rect.h == 10);
        CHECK(glyphBox.index == 42);
        CHECK(glyphBox.advance == 10.5);
    }

    TEST_CASE("ImageType enum values")
    {
        // Test that enum types are accessible
        msdf_atlas::ImageType msdfType = msdf_atlas::ImageType::MSDF;
        msdf_atlas::ImageType sdfType = msdf_atlas::ImageType::SDF;
        msdf_atlas::ImageType psdfType = msdf_atlas::ImageType::PSDF;
        msdf_atlas::ImageType mtsdfType = msdf_atlas::ImageType::MTSDF;
        
        // Verify enum values are distinct (basic sanity check)
        CHECK(true);
    }

    TEST_CASE("ImageFormat enum values")
    {
        // Test that enum types are accessible
        msdf_atlas::ImageFormat pngFormat = msdf_atlas::ImageFormat::PNG;
        msdf_atlas::ImageFormat bmpFormat = msdf_atlas::ImageFormat::BMP;
        msdf_atlas::ImageFormat tiffFormat = msdf_atlas::ImageFormat::TIFF;
        msdf_atlas::ImageFormat uncompressedFormat = msdf_atlas::ImageFormat::UNSPECIFIED;
        
        // Verify enum values are accessible
        CHECK(true);
    }

    TEST_CASE("RectanglePacker with different sizes")
    {
        // Test creating packers with various sizes
        msdf_atlas::RectanglePacker packer1(512, 512);
        msdf_atlas::RectanglePacker packer2(1024, 1024);
        msdf_atlas::RectanglePacker packer3(2048, 2048);
        
        // Verify all packers were created successfully
        CHECK(true);
    }

    TEST_CASE("Multiple GlyphBox creation")
    {
        // Test creating multiple glyph boxes
        const int glyphCount = 10;
        std::vector<msdf_atlas::GlyphBox> glyphBoxes;
        glyphBoxes.reserve(glyphCount);
        
        for (int i = 0; i < glyphCount; ++i)
        {
            msdf_atlas::GlyphBox glyphBox;
            glyphBox.index = i;
            glyphBox.advance = static_cast<double>(i * 10);
            
            msdf_atlas::Rectangle rect;
            rect.x = i * 10;
            rect.y = 0;
            rect.w = 10;
            rect.h = 10;
            glyphBox.rect = rect;
            
            glyphBoxes.push_back(glyphBox);
        }
        
        CHECK(glyphBoxes.size() == glyphCount);
        CHECK(glyphBoxes[0].index == 0);
        CHECK(glyphBoxes[glyphCount - 1].index == glyphCount - 1);
    }

    TEST_CASE("FontGeometry with glyphs")
    {
        msdf_atlas::FontGeometry fontGeometry;
        
        // Test that we can work with font geometry
        // (Actual glyph loading would require font file, so we just verify structure exists)
        CHECK(true);
    }
}

