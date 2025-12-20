#pragma once

#include "Renderer/Resources/Texture.h"
#include <cstdint>

namespace Sabora
{
    /**
     * @brief OpenGL implementation of the Texture interface.
     * 
     * OpenGLTexture provides OpenGL-specific texture operations including
     * 1D, 2D, 3D, Cube, and Array textures with various formats and mipmaps.
     */
    class OpenGLTexture : public Texture
    {
    public:
        /**
         * @brief Create a new OpenGL texture.
         * @param type Texture type (2D, 3D, Cube, etc.).
         * @param format Texture pixel format.
         * @param width Texture width.
         * @param height Texture height (1 for 1D textures).
         * @param depth Texture depth (1 for 2D textures).
         * @param mipLevels Number of mipmap levels (0 for automatic).
         * @param usage Texture usage flags.
         * @param data Optional initial pixel data.
         * @return Result containing the created texture, or an error.
         */
        [[nodiscard]] static Result<std::unique_ptr<OpenGLTexture>> Create(
            TextureType type,
            TextureFormat format,
            uint32_t width,
            uint32_t height,
            uint32_t depth,
            uint32_t mipLevels,
            TextureUsage usage,
            const void* data = nullptr
        );

        /**
         * @brief Destructor - destroys the OpenGL texture.
         */
        ~OpenGLTexture() override;

        // Non-copyable, movable
        OpenGLTexture(const OpenGLTexture&) = delete;
        OpenGLTexture& operator=(const OpenGLTexture&) = delete;
        OpenGLTexture(OpenGLTexture&& other) noexcept;
        OpenGLTexture& operator=(OpenGLTexture&& other) noexcept;

        //==========================================================================
        // Texture Interface Implementation
        //==========================================================================

        [[nodiscard]] TextureType GetType() const override { return m_Type; }
        [[nodiscard]] TextureFormat GetFormat() const override { return m_Format; }
        [[nodiscard]] uint32_t GetWidth() const override { return m_Width; }
        [[nodiscard]] uint32_t GetHeight() const override { return m_Height; }
        [[nodiscard]] uint32_t GetDepth() const override { return m_Depth; }
        [[nodiscard]] uint32_t GetMipLevels() const override { return m_MipLevels; }
        [[nodiscard]] TextureUsage GetUsage() const override { return m_Usage; }

        [[nodiscard]] Result<void> UpdateData(
            const void* data,
            size_t size,
            uint32_t mipLevel = 0,
            uint32_t x = 0,
            uint32_t y = 0,
            uint32_t z = 0,
            uint32_t width = 0,
            uint32_t height = 0,
            uint32_t depth = 0
        ) override;

        [[nodiscard]] Result<void> GenerateMipmaps() override;

        [[nodiscard]] void* GetNativeHandle() const override;
        [[nodiscard]] bool IsValid() const override;

    private:
        /**
         * @brief Private constructor - use Create() factory method.
         * @param textureId OpenGL texture ID.
         * @param type Texture type.
         * @param format Texture format.
         * @param width Texture width.
         * @param height Texture height.
         * @param depth Texture depth.
         * @param mipLevels Number of mipmap levels.
         * @param usage Texture usage flags.
         */
        OpenGLTexture(
            uint32_t textureId,
            TextureType type,
            TextureFormat format,
            uint32_t width,
            uint32_t height,
            uint32_t depth,
            uint32_t mipLevels,
            TextureUsage usage
        ) noexcept;

        /**
         * @brief Get OpenGL texture target for this texture type.
         * @return OpenGL texture target enum.
         */
        [[nodiscard]] uint32_t GetGLTarget() const;

        /**
         * @brief Get OpenGL internal format for this texture format.
         * @return OpenGL internal format enum.
         */
        [[nodiscard]] uint32_t GetGLInternalFormat() const;

        /**
         * @brief Get OpenGL format (for glTexImage calls).
         * @return OpenGL format enum.
         */
        [[nodiscard]] uint32_t GetGLFormat() const;

        /**
         * @brief Get OpenGL data type for this texture format.
         * @return OpenGL data type enum.
         */
        [[nodiscard]] uint32_t GetGLDataType() const;

        /**
         * @brief Calculate number of mipmap levels from dimensions.
         * @param width Texture width.
         * @param height Texture height.
         * @param depth Texture depth.
         * @return Number of mipmap levels.
         */
        [[nodiscard]] static uint32_t CalculateMipLevels(uint32_t width, uint32_t height, uint32_t depth);

        uint32_t m_TextureId = 0;     ///< OpenGL texture ID
        TextureType m_Type;           ///< Texture type
        TextureFormat m_Format;       ///< Texture format
        uint32_t m_Width = 0;         ///< Texture width
        uint32_t m_Height = 0;        ///< Texture height
        uint32_t m_Depth = 0;         ///< Texture depth
        uint32_t m_MipLevels = 0;     ///< Number of mipmap levels
        TextureUsage m_Usage;         ///< Texture usage flags
    };

} // namespace Sabora
