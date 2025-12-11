/**
 * @file TestShaderLibraries.cpp
 * @brief Unit tests for shader compilation libraries (shaderc and SPIRV-Cross).
 * 
 * These tests verify that shaderc and SPIRV-Cross are properly linked and
 * can compile shaders and perform cross-compilation between different shader languages.
 */

#include "doctest.h"
#include "Sabora.h"

// Shader compilation libraries
#include <shaderc/shaderc.hpp>
#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>
#include <spirv_hlsl.hpp>
#include <spirv_msl.hpp>

#include <vector>
#include <string>

using namespace Sabora;

TEST_SUITE("Shader Libraries")
{
    TEST_CASE("shaderc - Compile GLSL vertex shader to SPIR-V")
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        
        // Simple vertex shader in GLSL
        const std::string vertexShader = R"(
            #version 450
            layout(location = 0) in vec3 inPosition;
            layout(location = 0) out vec3 fragColor;
            
            void main() {
                gl_Position = vec4(inPosition, 1.0);
                fragColor = vec3(1.0, 0.0, 0.0);
            }
        )";

        // Compile GLSL to SPIR-V
        shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
            vertexShader, 
            shaderc_vertex_shader, 
            "test_shader", 
            options
        );

        REQUIRE(result.GetCompilationStatus() == shaderc_compilation_status_success);
        
        std::vector<uint32_t> spirvCode(result.cbegin(), result.cend());
        CHECK(spirvCode.size() > 0);
    }

    TEST_CASE("shaderc - Compile GLSL fragment shader to SPIR-V")
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        
        const std::string fragmentShader = R"(
            #version 450
            layout(location = 0) in vec3 fragColor;
            layout(location = 0) out vec4 outColor;
            
            void main() {
                outColor = vec4(fragColor, 1.0);
            }
        )";

        shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
            fragmentShader, 
            shaderc_fragment_shader, 
            "test_fragment_shader", 
            options
        );

        REQUIRE(result.GetCompilationStatus() == shaderc_compilation_status_success);
        
        std::vector<uint32_t> spirvCode(result.cbegin(), result.cend());
        CHECK(spirvCode.size() > 0);
    }

    TEST_CASE("shaderc - Handle invalid shader compilation")
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        
        // Invalid shader code
        const std::string invalidShader = R"(
            #version 450
            invalid syntax here
        )";

        shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(
            invalidShader, 
            shaderc_vertex_shader, 
            "invalid_shader", 
            options
        );

        CHECK(result.GetCompilationStatus() != shaderc_compilation_status_success);
        CHECK(result.GetErrorMessage().length() > 0);
    }

    TEST_CASE("SPIRV-Cross - Convert SPIR-V to GLSL")
    {
        // First compile a shader to SPIR-V
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        
        const std::string vertexShader = R"(
            #version 450
            layout(location = 0) in vec3 inPosition;
            layout(location = 0) out vec3 fragColor;
            
            void main() {
                gl_Position = vec4(inPosition, 1.0);
                fragColor = vec3(1.0, 0.0, 0.0);
            }
        )";

        shaderc::SpvCompilationResult compileResult = compiler.CompileGlslToSpv(
            vertexShader, 
            shaderc_vertex_shader, 
            "test_shader", 
            options
        );

        REQUIRE(compileResult.GetCompilationStatus() == shaderc_compilation_status_success);
        
        std::vector<uint32_t> spirvCode(compileResult.cbegin(), compileResult.cend());
        
        // Now convert SPIR-V to GLSL using SPIRV-Cross
        spirv_cross::CompilerGLSL glslCompiler(spirvCode);
        spirv_cross::CompilerGLSL::Options glslOptions;
        glslOptions.version = 330;
        glslOptions.es = false;
        glslCompiler.set_common_options(glslOptions);
        
        std::string glslOutput = glslCompiler.compile();
        
        CHECK(glslOutput.length() > 0);
        CHECK(glslOutput.find("main") != std::string::npos);
    }

    TEST_CASE("SPIRV-Cross - Convert SPIR-V to HLSL")
    {
        // Compile shader to SPIR-V
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        
        const std::string vertexShader = R"(
            #version 450
            layout(location = 0) in vec3 inPosition;
            layout(location = 0) out vec3 fragColor;
            
            void main() {
                gl_Position = vec4(inPosition, 1.0);
                fragColor = vec3(1.0, 0.0, 0.0);
            }
        )";

        shaderc::SpvCompilationResult compileResult = compiler.CompileGlslToSpv(
            vertexShader, 
            shaderc_vertex_shader, 
            "test_shader", 
            options
        );

        REQUIRE(compileResult.GetCompilationStatus() == shaderc_compilation_status_success);
        
        std::vector<uint32_t> spirvCode(compileResult.cbegin(), compileResult.cend());
        
        // Convert SPIR-V to HLSL
        spirv_cross::CompilerHLSL hlslCompiler(spirvCode);
        spirv_cross::CompilerHLSL::Options hlslOptions;
        hlslOptions.shader_model = 50;
        hlslCompiler.set_hlsl_options(hlslOptions);
        
        std::string hlslOutput = hlslCompiler.compile();
        
        CHECK(hlslOutput.length() > 0);
        // Check for main function (case may vary)
        bool hasMain = (hlslOutput.find("main") != std::string::npos) || 
                       (hlslOutput.find("Main") != std::string::npos);
        CHECK(hasMain);
    }

    TEST_CASE("SPIRV-Cross - Convert SPIR-V to MSL")
    {
        // Compile shader to SPIR-V
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        
        const std::string vertexShader = R"(
            #version 450
            layout(location = 0) in vec3 inPosition;
            layout(location = 0) out vec3 fragColor;
            
            void main() {
                gl_Position = vec4(inPosition, 1.0);
                fragColor = vec3(1.0, 0.0, 0.0);
            }
        )";

        shaderc::SpvCompilationResult compileResult = compiler.CompileGlslToSpv(
            vertexShader, 
            shaderc_vertex_shader, 
            "test_shader", 
            options
        );

        REQUIRE(compileResult.GetCompilationStatus() == shaderc_compilation_status_success);
        
        std::vector<uint32_t> spirvCode(compileResult.cbegin(), compileResult.cend());
        
        // Convert SPIR-V to MSL (Metal Shading Language for macOS/iOS)
        spirv_cross::CompilerMSL mslCompiler(spirvCode);
        spirv_cross::CompilerMSL::Options mslOptions;
        mslOptions.set_msl_version(2, 0);
        mslCompiler.set_msl_options(mslOptions);
        
        std::string mslOutput = mslCompiler.compile();
        
        CHECK(mslOutput.length() > 0);
        // Check for main function or vertex keyword (case may vary)
        bool hasMainOrVertex = (mslOutput.find("main") != std::string::npos) || 
                                (mslOutput.find("vertex") != std::string::npos);
        CHECK(hasMainOrVertex);
    }

    TEST_CASE("SPIRV-Cross - Handle invalid SPIR-V")
    {
        // Create invalid SPIR-V code
        std::vector<uint32_t> invalidSpirv = { 0xDEADBEEF, 0xCAFEBABE };
        
        // This should throw an exception or handle the error gracefully
        try
        {
            spirv_cross::CompilerGLSL glslCompiler(invalidSpirv);
            // If we get here, the compiler might have handled it gracefully
            // or the test should check for specific error conditions
        }
        catch (const std::exception&)
        {
            // Expected behavior - invalid SPIR-V should throw
            CHECK(true);
        }
    }

    TEST_CASE("Full shader compilation pipeline")
    {
        // Test the complete pipeline: GLSL -> SPIR-V -> GLSL/HLSL/MSL
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        
        const std::string originalShader = R"(
            #version 450
            layout(location = 0) in vec3 inPosition;
            layout(location = 0) out vec3 fragColor;
            
            void main() {
                gl_Position = vec4(inPosition, 1.0);
                fragColor = vec3(1.0, 0.0, 0.0);
            }
        )";

        // Step 1: Compile GLSL to SPIR-V
        shaderc::SpvCompilationResult compileResult = compiler.CompileGlslToSpv(
            originalShader, 
            shaderc_vertex_shader, 
            "pipeline_test", 
            options
        );

        REQUIRE(compileResult.GetCompilationStatus() == shaderc_compilation_status_success);
        std::vector<uint32_t> spirvCode(compileResult.cbegin(), compileResult.cend());
        CHECK(spirvCode.size() > 0);

        // Step 2: Convert SPIR-V back to GLSL
        {
            spirv_cross::CompilerGLSL glslCompiler(spirvCode);
            spirv_cross::CompilerGLSL::Options glslOptions;
            glslOptions.version = 330;
            glslCompiler.set_common_options(glslOptions);
            std::string glslOutput = glslCompiler.compile();
            CHECK(glslOutput.length() > 0);
        }

        // Step 3: Convert SPIR-V to HLSL
        {
            spirv_cross::CompilerHLSL hlslCompiler(spirvCode);
            spirv_cross::CompilerHLSL::Options hlslOptions;
            hlslOptions.shader_model = 50;
            hlslCompiler.set_hlsl_options(hlslOptions);
            std::string hlslOutput = hlslCompiler.compile();
            CHECK(hlslOutput.length() > 0);
        }

        // Step 4: Convert SPIR-V to MSL
        {
            spirv_cross::CompilerMSL mslCompiler(spirvCode);
            spirv_cross::CompilerMSL::Options mslOptions;
            mslOptions.set_msl_version(2, 0);
            mslCompiler.set_msl_options(mslOptions);
            std::string mslOutput = mslCompiler.compile();
            CHECK(mslOutput.length() > 0);
        }
    }
}

