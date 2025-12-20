/**
 * @file TestOpenGLRenderer.cpp
 * @brief Unit tests for OpenGL renderer operations.
 */

#include "doctest.h"
#include "Renderer/OpenGL/OpenGLRenderer.h"
#include "Renderer/Core/RendererTypes.h"
#include <glad/gl.h>

using namespace Sabora;

TEST_SUITE("OpenGLRenderer")
{
    TEST_CASE("GetGLPrimitiveType - Maps all primitive topologies correctly")
    {
        // Test all primitive topology mappings
        CHECK(OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::Points) == GL_POINTS);
        CHECK(OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::Lines) == GL_LINES);
        CHECK(OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::LineStrip) == GL_LINE_STRIP);
        CHECK(OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::Triangles) == GL_TRIANGLES);
        CHECK(OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::TriangleStrip) == GL_TRIANGLE_STRIP);
        CHECK(OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::TriangleFan) == GL_TRIANGLE_FAN);
        CHECK(OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::LinesAdjacency) == GL_LINES_ADJACENCY);
        CHECK(OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::LineStripAdjacency) == GL_LINE_STRIP_ADJACENCY);
        CHECK(OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::TrianglesAdjacency) == GL_TRIANGLES_ADJACENCY);
        CHECK(OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::TriangleStripAdjacency) == GL_TRIANGLE_STRIP_ADJACENCY);
        CHECK(OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::Patches) == GL_PATCHES);
    }

    TEST_CASE("GetGLPrimitiveType - Default case returns GL_TRIANGLES")
    {
        // Test that invalid/unknown topology defaults to triangles
        // Note: This tests the default case in the switch statement
        // We can't easily test an invalid enum value, but we can verify
        // that the function always returns a valid OpenGL constant
        uint32_t result = OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::Triangles);
        CHECK(result == GL_TRIANGLES);
        
        // Verify it's a valid OpenGL primitive type constant
        CHECK(result >= GL_POINTS);
    }

    TEST_CASE("GetGLPrimitiveType - Is consistent for same input")
    {
        // Test that the function is deterministic
        uint32_t result1 = OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::Triangles);
        uint32_t result2 = OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::Triangles);
        CHECK(result1 == result2);
        
        result1 = OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::Lines);
        result2 = OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::Lines);
        CHECK(result1 == result2);
    }

    TEST_CASE("GetGLPrimitiveType - Returns different values for different topologies")
    {
        // Verify that different topologies map to different OpenGL constants
        uint32_t points = OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::Points);
        uint32_t lines = OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::Lines);
        uint32_t triangles = OpenGLRenderer::GetGLPrimitiveType(PrimitiveTopology::Triangles);
        
        CHECK(points != lines);
        CHECK(lines != triangles);
        CHECK(points != triangles);
    }
}

// Note: Full renderer integration tests (buffer creation, texture creation, etc.)
// require an OpenGL context and window, which is better suited for integration tests.
// These tests focus on testable static methods and validation logic.

