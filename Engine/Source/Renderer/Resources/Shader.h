#pragma once

#include "Core/Result.h"
#include "Renderer/Core/RendererTypes.h"
#include <string>
#include <memory>
#include <vector>

namespace Sabora
{
    /**
     * @brief Abstract shader interface.
     * 
     * Shader represents a compiled shader program stage (vertex, fragment, etc.).
     * It provides a unified interface for shader operations across different APIs.
     * 
     * Thread Safety:
     * - Shader creation/destruction is thread-safe (uses MainThreadDispatcher)
     * - Shader operations are thread-safe
     */
    class Shader
    {
    public:
        virtual ~Shader() = default;

        /**
         * @brief Get the shader stage.
         * @return The shader stage (vertex, fragment, etc.).
         */
        [[nodiscard]] virtual ShaderStage GetStage() const = 0;

        /**
         * @brief Get the shader source code (if available).
         * @return The original source code, or empty string if not available.
         */
        [[nodiscard]] virtual std::string GetSource() const = 0;

        /**
         * @brief Get the entry point name.
         * @return The entry point name (e.g., "main" for GLSL).
         */
        [[nodiscard]] virtual std::string GetEntryPoint() const = 0;

        /**
         * @brief Check if the shader compiled successfully.
         * @return True if the shader is valid and compiled.
         */
        [[nodiscard]] virtual bool IsValid() const = 0;

        /**
         * @brief Get compilation error message (if compilation failed).
         * @return Error message, or empty string if compilation succeeded.
         */
        [[nodiscard]] virtual std::string GetCompileError() const = 0;

        /**
         * @brief Get the native API handle (for advanced use).
         * @return Opaque pointer to the native shader handle.
         */
        [[nodiscard]] virtual void* GetNativeHandle() const = 0;

    protected:
        Shader() = default;

        // Non-copyable, movable
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&&) noexcept = default;
        Shader& operator=(Shader&&) noexcept = default;
    };

    /**
     * @brief Shader reflection information.
     * 
     * Contains information about shader resources (uniforms, samplers, etc.)
     * extracted through shader reflection.
     */
    struct ShaderReflection
    {
        /**
         * @brief Uniform buffer information.
         */
        struct UniformBuffer
        {
            std::string name;
            uint32_t binding = 0;
            size_t size = 0;
        };

        /**
         * @brief Sampler/texture information.
         */
        struct Sampler
        {
            std::string name;
            uint32_t binding = 0;
            TextureType type = TextureType::Texture2D;
        };

        std::vector<UniformBuffer> uniformBuffers;
        std::vector<Sampler> samplers;
    };

} // namespace Sabora
