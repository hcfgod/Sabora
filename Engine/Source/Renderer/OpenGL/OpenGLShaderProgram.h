#pragma once

#include "Renderer/Resources/Shader.h"
#include "Renderer/OpenGL/OpenGLShader.h"
#include "Core/Result.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace Sabora
{
    /**
     * @brief OpenGL shader program (linked shader stages).
     * 
     * OpenGLShaderProgram represents a complete shader program consisting of
     * multiple linked shader stages (typically vertex + fragment). This is the
     * object that gets used for rendering.
     */
    class OpenGLShaderProgram
    {
    public:
        /**
         * @brief Create a shader program from shader stages.
         * @param shaders Vector of shader stages to link.
         * @return Result containing the created program, or an error.
         */
        [[nodiscard]] static Result<std::unique_ptr<OpenGLShaderProgram>> Create(
            std::vector<std::unique_ptr<OpenGLShader>> shaders
        );

        /**
         * @brief Create a shader program from an existing OpenGL program ID.
         * @param programId Existing OpenGL program ID (must be valid and linked).
         * @return Result containing the created program, or an error.
         * 
         * This is used when linking shaders externally (e.g., in pipeline creation).
         * The program ID must already be linked and valid.
         */
        [[nodiscard]] static Result<std::unique_ptr<OpenGLShaderProgram>> CreateFromProgramId(
            uint32_t programId
        );

        /**
         * @brief Destructor - destroys the OpenGL program.
         */
        ~OpenGLShaderProgram();

        // Non-copyable, movable
        OpenGLShaderProgram(const OpenGLShaderProgram&) = delete;
        OpenGLShaderProgram& operator=(const OpenGLShaderProgram&) = delete;
        OpenGLShaderProgram(OpenGLShaderProgram&& other) noexcept;
        OpenGLShaderProgram& operator=(OpenGLShaderProgram&& other) noexcept;

        /**
         * @brief Get the OpenGL program ID.
         * @return OpenGL program ID.
         */
        [[nodiscard]] uint32_t GetProgramId() const noexcept { return m_ProgramId; }

        /**
         * @brief Check if the program is valid and linked.
         * @return True if the program is valid.
         */
        [[nodiscard]] bool IsValid() const noexcept { return m_ProgramId != 0 && m_Linked; }

        /**
         * @brief Get link error message (if linking failed).
         * @return Error message, or empty string if linking succeeded.
         */
        [[nodiscard]] const std::string& GetLinkError() const noexcept { return m_LinkError; }

        /**
         * @brief Get uniform location by name.
         * @param name Uniform name.
         * @return Uniform location, or -1 if not found.
         */
        [[nodiscard]] int32_t GetUniformLocation(const std::string& name);

        /**
         * @brief Get attribute location by name.
         * @param name Attribute name.
         * @return Attribute location, or -1 if not found.
         */
        [[nodiscard]] int32_t GetAttributeLocation(const std::string& name);

        /**
         * @brief Get the native API handle.
         * @return Opaque pointer to the native program handle.
         */
        [[nodiscard]] void* GetNativeHandle() const;

        /**
         * @brief Get reflection information.
         * @return Reference to reflection data.
         */
        [[nodiscard]] const ShaderReflection& GetReflection() const noexcept { return m_Reflection; }

    private:
        /**
         * @brief Private constructor - use Create() factory method.
         * @param programId OpenGL program ID.
         * @param linked Whether linking succeeded.
         * @param linkError Link error message (if any).
         * @param reflection Reflection information.
         */
        OpenGLShaderProgram(
            uint32_t programId,
            bool linked,
            const std::string& linkError,
            const ShaderReflection& reflection
        ) noexcept;

        /**
         * @brief Link shader stages into a program.
         * @param shaders Vector of shader stages.
         * @return Tuple of (programId, success, errorMessage).
         */
        [[nodiscard]] static std::tuple<uint32_t, bool, std::string> LinkProgram(
            const std::vector<OpenGLShader*>& shaders
        );

        /**
         * @brief Cache uniform and attribute locations.
         */
        void CacheLocations();

        uint32_t m_ProgramId = 0;                    ///< OpenGL program ID
        bool m_Linked = false;                        ///< Whether linking succeeded
        std::string m_LinkError;                      ///< Link error message
        ShaderReflection m_Reflection;                ///< Reflection information
        
        // Cached locations
        std::unordered_map<std::string, int32_t> m_UniformLocations;
        std::unordered_map<std::string, int32_t> m_AttributeLocations;
    };

} // namespace Sabora
