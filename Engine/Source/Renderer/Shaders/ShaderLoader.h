#pragma once

#include "Assets/IAssetLoader.h"
#include "Renderer/OpenGL/OpenGLShaderProgram.h"
#include <filesystem>

namespace Sabora
{
    /**
     * @brief Asset loader for shader programs.
     * 
     * ShaderLoader implements IAssetLoader<OpenGLShaderProgram> to load shaders
     * through the Asset System. It supports:
     * - Single-file format (.glsl, .shader) with #type markers
     * - Multi-file format (.vert, .frag, .geom, .comp) - separate files per stage
     * 
     * The loader automatically detects the format based on file extension and
     * uses ShaderParser for single-file format or direct loading for multi-file.
     */
    class ShaderLoader : public IAssetLoader<OpenGLShaderProgram>
    {
    public:
        /**
         * @brief Constructor.
         * @param targetGLSLVersion Target GLSL version for compilation (default: 330).
         */
        explicit ShaderLoader(uint32_t targetGLSLVersion = 330);

        /**
         * @brief Load a shader program from a file.
         * @param path Path to the shader file.
         * @return Result containing the loaded shader program, or an error.
         * 
         * This method:
         * 1. Detects file format (single-file vs multi-file)
         * 2. Loads and parses shader source(s)
         * 3. Compiles shader stages using ShaderCompiler
         * 4. Links shader stages into OpenGLShaderProgram
         */
        [[nodiscard]] Result<std::unique_ptr<OpenGLShaderProgram>> Load(
            const std::filesystem::path& path
        ) override;

        /**
         * @brief Get the asset type name.
         * @return "ShaderProgram"
         */
        [[nodiscard]] std::string GetAssetTypeName() const override { return "ShaderProgram"; }

        /**
         * @brief Get supported file extensions.
         * @return Vector of supported extensions.
         */
        [[nodiscard]] std::vector<std::string> GetSupportedExtensions() const override;

    private:
        /**
         * @brief Load a single-file shader format.
         * @param path Path to the shader file.
         * @return Result containing the shader program, or an error.
         */
        [[nodiscard]] Result<std::unique_ptr<OpenGLShaderProgram>> LoadSingleFile(
            const std::filesystem::path& path
        );

        /**
         * @brief Load a multi-file shader format.
         * @param path Path to one of the shader files (e.g., .vert or .frag).
         * @return Result containing the shader program, or an error.
         */
        [[nodiscard]] Result<std::unique_ptr<OpenGLShaderProgram>> LoadMultiFile(
            const std::filesystem::path& path
        );

        /**
         * @brief Check if a file extension indicates single-file format.
         * @param extension File extension (with or without dot).
         * @return True if single-file format.
         */
        [[nodiscard]] static bool IsSingleFileFormat(const std::string& extension);

        /**
         * @brief Get the shader stage from file extension.
         * @param extension File extension (with or without dot).
         * @return Shader stage, or ShaderStage::Vertex if unknown.
         */
        [[nodiscard]] static ShaderStage GetStageFromExtension(const std::string& extension);

        /**
         * @brief Get base path without extension.
         * @param path File path.
         * @return Path without extension.
         */
        [[nodiscard]] static std::filesystem::path GetBasePath(const std::filesystem::path& path);

        uint32_t m_TargetGLSLVersion; ///< Target GLSL version for compilation
    };

} // namespace Sabora
