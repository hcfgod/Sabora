#include "pch.h"
#include "ShaderCompiler.h"
#include "Core/Log.h"
#include <shaderc/shaderc.hpp>
#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>
#include <algorithm>

namespace Sabora
{
    //==========================================================================
    // Compile Methods
    //==========================================================================

    Result<ShaderCompiler::CompilationResult> ShaderCompiler::Compile(
        ShaderStage stage,
        const std::string& source,
        const std::string& sourceName,
        uint32_t targetGLSLVersion)
    {
        CompilationResult result;

        // Step 1: Compile GLSL to SPIR-V
        auto spirvResult = CompileToSPIRV(stage, source, sourceName);
        if (spirvResult.IsFailure())
        {
            result.success = false;
            result.errorMessage = spirvResult.GetError().ToString();
            return Result<CompilationResult>::Failure(spirvResult.GetError().Code(), result.errorMessage);
        }

        result.spirvCode = spirvResult.Value();

        // Step 2: Cross-compile SPIR-V to GLSL for OpenGL
        auto glslResult = CrossCompileToGLSL(result.spirvCode, targetGLSLVersion, false);
        if (glslResult.IsFailure())
        {
            result.success = false;
            result.errorMessage = glslResult.GetError().ToString();
            return Result<CompilationResult>::Failure(glslResult.GetError().Code(), result.errorMessage);
        }

        result.glslSource = glslResult.Value();

        // Step 3: Extract reflection information
        auto reflectionResult = Reflect(result.spirvCode);
        if (reflectionResult.IsSuccess())
        {
            result.reflection = reflectionResult.Value();
        }
        // Reflection failure is not critical - continue without it

        result.success = true;
        return Result<CompilationResult>::Success(std::move(result));
    }

    Result<std::vector<uint32_t>> ShaderCompiler::CompileToSPIRV(
        ShaderStage stage,
        const std::string& source,
        const std::string& sourceName)
    {
        if (source.empty())
        {
            return Result<std::vector<uint32_t>>::Failure(
                ErrorCode::CoreInvalidArgument,
                "Shader source is empty"
            );
        }

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        // Set optimization level (optional - can be made configurable)
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        // Convert shader stage
        shaderc_shader_kind kind = static_cast<shaderc_shader_kind>(ShaderStageToShadercKind(stage));
        if (kind == shaderc_glsl_infer_from_source)
        {
            return Result<std::vector<uint32_t>>::Failure(
                ErrorCode::GraphicsShaderCompilationFailed,
                fmt::format("Invalid shader stage for compilation: {}", static_cast<int>(stage))
            );
        }

        // Compile GLSL to SPIR-V
        shaderc::SpvCompilationResult compileResult = compiler.CompileGlslToSpv(
            source,
            kind,
            sourceName.c_str(),
            options
        );

        // Check compilation status
        if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            std::string errorMsg = compileResult.GetErrorMessage();
            if (errorMsg.empty())
            {
                errorMsg = fmt::format("Shader compilation failed with status: {}", 
                    static_cast<int>(compileResult.GetCompilationStatus()));
            }

            return Result<std::vector<uint32_t>>::Failure(
                ErrorCode::GraphicsShaderCompilationFailed,
                errorMsg
            );
        }

        // Convert result to vector
        std::vector<uint32_t> spirvCode(compileResult.cbegin(), compileResult.cend());
        
        if (spirvCode.empty())
        {
            return Result<std::vector<uint32_t>>::Failure(
                ErrorCode::GraphicsShaderCompilationFailed,
                "Compiled SPIR-V code is empty"
            );
        }

        return Result<std::vector<uint32_t>>::Success(std::move(spirvCode));
    }

    Result<std::string> ShaderCompiler::CrossCompileToGLSL(
        const std::vector<uint32_t>& spirvCode,
        uint32_t targetVersion,
        bool isES)
    {
        if (spirvCode.empty())
        {
            return Result<std::string>::Failure(
                ErrorCode::CoreInvalidArgument,
                "SPIR-V code is empty"
            );
        }

        try
        {
            // Create SPIRV-Cross compiler
            spirv_cross::CompilerGLSL glslCompiler(spirvCode);

            // Set GLSL options
            spirv_cross::CompilerGLSL::Options options;
            options.version = static_cast<int>(targetVersion);
            options.es = isES;
            options.vulkan_semantics = false; // OpenGL semantics
            glslCompiler.set_common_options(options);

            // Compile to GLSL
            std::string glslSource = glslCompiler.compile();

            if (glslSource.empty())
            {
                return Result<std::string>::Failure(
                    ErrorCode::GraphicsShaderCompilationFailed,
                    "Cross-compiled GLSL source is empty"
                );
            }

            return Result<std::string>::Success(std::move(glslSource));
        }
        catch (const std::exception& e)
        {
            return Result<std::string>::Failure(
                ErrorCode::GraphicsShaderCompilationFailed,
                fmt::format("SPIRV-Cross compilation error: {}", e.what())
            );
        }
    }

    Result<ShaderReflection> ShaderCompiler::Reflect(
        const std::vector<uint32_t>& spirvCode)
    {
        if (spirvCode.empty())
        {
            return Result<ShaderReflection>::Failure(
                ErrorCode::CoreInvalidArgument,
                "SPIR-V code is empty"
            );
        }

        try
        {
            ShaderReflection reflection;

            // Create SPIRV-Cross compiler for reflection
            spirv_cross::CompilerGLSL compiler(spirvCode);
            spirv_cross::ShaderResources resources = compiler.get_shader_resources();

            // Extract uniform buffers
            for (const auto& resource : resources.uniform_buffers)
            {
                ShaderReflection::UniformBuffer ubo;
                ubo.name = resource.name;
                ubo.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                
                // Get buffer size
                const auto& type = compiler.get_type(resource.base_type_id);
                ubo.size = static_cast<size_t>(compiler.get_declared_struct_size(type));

                reflection.uniformBuffers.push_back(ubo);
            }

            // Extract samplers/textures
            for (const auto& resource : resources.sampled_images)
            {
                ShaderReflection::Sampler sampler;
                sampler.name = resource.name;
                sampler.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                
                // Determine texture type from image dimension
                const auto& type = compiler.get_type(resource.type_id);
                if (type.image.dim == spv::Dim2D)
                {
                    sampler.type = TextureType::Texture2D;
                }
                else if (type.image.dim == spv::DimCube)
                {
                    sampler.type = TextureType::TextureCube;
                }
                else if (type.image.dim == spv::Dim3D)
                {
                    sampler.type = TextureType::Texture3D;
                }
                else
                {
                    sampler.type = TextureType::Texture2D; // Default
                }

                reflection.samplers.push_back(sampler);
            }

            return Result<ShaderReflection>::Success(std::move(reflection));
        }
        catch (const std::exception& e)
        {
            return Result<ShaderReflection>::Failure(
                ErrorCode::GraphicsShaderCompilationFailed,
                fmt::format("Shader reflection error: {}", e.what())
            );
        }
    }

    //==========================================================================
    // Helper Methods
    //==========================================================================

    uint32_t ShaderCompiler::ShaderStageToShadercKind(ShaderStage stage)
    {
        switch (stage)
        {
            case ShaderStage::Vertex:                  return shaderc_vertex_shader;
            case ShaderStage::Fragment:                 return shaderc_fragment_shader;
            case ShaderStage::Geometry:                 return shaderc_geometry_shader;
            case ShaderStage::Compute:                  return shaderc_compute_shader;
            case ShaderStage::TessellationControl:     return shaderc_tess_control_shader;
            case ShaderStage::TessellationEvaluation:   return shaderc_tess_evaluation_shader;
            default:                                    return shaderc_glsl_infer_from_source;
        }
    }

} // namespace Sabora
