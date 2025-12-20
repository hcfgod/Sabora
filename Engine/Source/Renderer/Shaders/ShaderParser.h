#pragma once

#include "Renderer/Core/RendererTypes.h"
#include "Core/Result.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

namespace Sabora
{
    /**
     * @brief Parser for single-file shader format with #type markers.
     * 
     * ShaderParser extracts shader stage code from a single file that contains
     * multiple shader stages separated by #type markers (e.g., #type vertex,
     * #type fragment). This format is inspired by The Cherno's shader system.
     * 
     * Example shader file:
     * @code
     *   #type vertex
     *   #version 450
     *   layout(location = 0) in vec3 aPosition;
     *   void main() { ... }
     *   
     *   #type fragment
     *   #version 450
     *   layout(location = 0) out vec4 FragColor;
     *   void main() { ... }
     * @endcode
     */
    class ShaderParser
    {
    public:
        /**
         * @brief Parse a shader file and extract shader stages.
         * @param source The shader source code.
         * @return Result containing a map of ShaderStage -> source code, or an error.
         * 
         * The parser looks for #type markers and extracts the code between markers.
         * Supported types: vertex, fragment, geometry, compute.
         */
        [[nodiscard]] static Result<std::unordered_map<ShaderStage, std::string>> Parse(
            const std::string& source
        );

        /**
         * @brief Parse a shader file from a file path.
         * @param filePath Path to the shader file.
         * @return Result containing a map of ShaderStage -> source code, or an error.
         */
        [[nodiscard]] static Result<std::unordered_map<ShaderStage, std::string>> ParseFromFile(
            const std::filesystem::path& filePath
        );

    private:
        /**
         * @brief Parse a shader type string to ShaderStage enum.
         * @param typeStr The type string (e.g., "vertex", "fragment").
         * @return ShaderStage enum, or ShaderStage::Vertex if invalid.
         */
        [[nodiscard]] static ShaderStage ParseShaderType(const std::string& typeStr);

        /**
         * @brief Trim whitespace from the beginning and end of a string.
         * @param str The string to trim.
         * @return Trimmed string.
         */
        [[nodiscard]] static std::string Trim(const std::string& str);
    };

} // namespace Sabora
