#pragma once

#include "Renderer/Core/RendererTypes.h"
#include "Renderer/Resources/Shader.h"
#include "Core/Result.h"
#include <string>
#include <vector>
#include <memory>

namespace Sabora
{
    /**
     * @brief Shader compilation and cross-compilation system.
     * 
     * ShaderCompiler uses shaderc to compile GLSL to SPIR-V, and SPIRV-Cross
     * to convert SPIR-V to GLSL (for OpenGL compatibility). This allows us to:
     * - Use modern GLSL features
     * - Support future Vulkan backend with same shader source
     * - Perform shader reflection to extract uniforms, samplers, etc.
     */
    class ShaderCompiler
    {
    public:
        /**
         * @brief Compilation result containing compiled shader data.
         */
        struct CompilationResult
        {
            std::string glslSource;           ///< Final GLSL source (for OpenGL)
            std::vector<uint32_t> spirvCode;  ///< SPIR-V bytecode (for Vulkan/future)
            ShaderReflection reflection;       ///< Reflection information
            bool success = false;             ///< Whether compilation succeeded
            std::string errorMessage;         ///< Error message if compilation failed
        };

        /**
         * @brief Compile a shader stage from GLSL source.
         * @param stage Shader stage (vertex, fragment, etc.).
         * @param source GLSL source code.
         * @param sourceName Name of the source (for error messages).
         * @param targetGLSLVersion Target GLSL version for OpenGL (e.g., 330, 450).
         * @return Result containing compilation result, or an error.
         * 
         * This method:
         * 1. Compiles GLSL to SPIR-V using shaderc
         * 2. Converts SPIR-V back to GLSL using SPIRV-Cross (for OpenGL)
         * 3. Extracts reflection information (uniforms, samplers, attributes)
         */
        [[nodiscard]] static Result<CompilationResult> Compile(
            ShaderStage stage,
            const std::string& source,
            const std::string& sourceName = "shader",
            uint32_t targetGLSLVersion = 330
        );

        /**
         * @brief Compile GLSL source directly to SPIR-V.
         * @param stage Shader stage.
         * @param source GLSL source code.
         * @param sourceName Name of the source.
         * @return Result containing SPIR-V bytecode, or an error.
         */
        [[nodiscard]] static Result<std::vector<uint32_t>> CompileToSPIRV(
            ShaderStage stage,
            const std::string& source,
            const std::string& sourceName = "shader"
        );

        /**
         * @brief Convert SPIR-V bytecode to GLSL.
         * @param spirvCode SPIR-V bytecode.
         * @param targetVersion Target GLSL version (e.g., 330, 450).
         * @param isES Whether to target GLSL ES.
         * @return Result containing GLSL source, or an error.
         */
        [[nodiscard]] static Result<std::string> CrossCompileToGLSL(
            const std::vector<uint32_t>& spirvCode,
            uint32_t targetVersion = 330,
            bool isES = false
        );

        /**
         * @brief Extract reflection information from SPIR-V.
         * @param spirvCode SPIR-V bytecode.
         * @return Result containing reflection data, or an error.
         */
        [[nodiscard]] static Result<ShaderReflection> Reflect(
            const std::vector<uint32_t>& spirvCode
        );

    private:
        /**
         * @brief Convert ShaderStage to shaderc shader kind.
         * @param stage Shader stage.
         * @return shaderc shader kind.
         */
        [[nodiscard]] static uint32_t ShaderStageToShadercKind(ShaderStage stage);
    };

} // namespace Sabora
