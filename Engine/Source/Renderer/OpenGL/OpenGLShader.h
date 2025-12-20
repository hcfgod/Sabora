#pragma once

#include "Renderer/Resources/Shader.h"
#include <cstdint>
#include <string>

namespace Sabora
{
    /**
     * @brief OpenGL implementation of an individual shader stage.
     * 
     * OpenGLShader represents a single compiled shader stage (vertex, fragment, etc.).
     * Multiple shader stages are linked together into an OpenGLShaderProgram.
     */
    class OpenGLShader : public Shader
    {
    public:
        /**
         * @brief Create and compile a shader from GLSL source.
         * @param stage Shader stage (vertex, fragment, etc.).
         * @param source GLSL source code.
         * @return Result containing the created shader, or an error.
         */
        [[nodiscard]] static Result<std::unique_ptr<OpenGLShader>> Create(
            ShaderStage stage,
            const std::string& source
        );

        /**
         * @brief Destructor - destroys the OpenGL shader object.
         */
        ~OpenGLShader() override;

        // Non-copyable, movable
        OpenGLShader(const OpenGLShader&) = delete;
        OpenGLShader& operator=(const OpenGLShader&) = delete;
        OpenGLShader(OpenGLShader&& other) noexcept;
        OpenGLShader& operator=(OpenGLShader&& other) noexcept;

        //==========================================================================
        // Shader Interface Implementation
        //==========================================================================

        [[nodiscard]] ShaderStage GetStage() const override { return m_Stage; }
        [[nodiscard]] std::string GetSource() const override { return m_Source; }
        [[nodiscard]] std::string GetEntryPoint() const override { return "main"; }
        [[nodiscard]] bool IsValid() const override { return m_ShaderId != 0 && m_Compiled; }
        [[nodiscard]] std::string GetCompileError() const override { return m_CompileError; }
        [[nodiscard]] void* GetNativeHandle() const override;

        /**
         * @brief Get the OpenGL shader ID.
         * @return OpenGL shader ID.
         */
        [[nodiscard]] uint32_t GetShaderId() const noexcept { return m_ShaderId; }

    private:
        /**
         * @brief Private constructor - use Create() factory method.
         * @param shaderId OpenGL shader ID.
         * @param stage Shader stage.
         * @param source Shader source code.
         * @param compiled Whether compilation succeeded.
         * @param compileError Compilation error message (if any).
         */
        OpenGLShader(
            uint32_t shaderId,
            ShaderStage stage,
            const std::string& source,
            bool compiled,
            const std::string& compileError
        ) noexcept;

        /**
         * @brief Compile GLSL source to OpenGL shader object.
         * @param stage Shader stage.
         * @param source GLSL source code.
         * @return Tuple of (shaderId, success, errorMessage).
         */
        [[nodiscard]] static std::tuple<uint32_t, bool, std::string> CompileShader(
            ShaderStage stage,
            const std::string& source
        );

        /**
         * @brief Get OpenGL shader type for a shader stage.
         * @param stage Shader stage.
         * @return OpenGL shader type enum.
         */
        [[nodiscard]] static uint32_t GetGLShaderType(ShaderStage stage);

        uint32_t m_ShaderId = 0;      ///< OpenGL shader ID
        ShaderStage m_Stage;          ///< Shader stage
        std::string m_Source;         ///< Original source code
        bool m_Compiled = false;      ///< Whether compilation succeeded
        std::string m_CompileError;   ///< Compilation error message
    };

} // namespace Sabora
