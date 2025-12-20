#pragma once

#include "Core/Result.h"
#include "Renderer/Core/RendererTypes.h"
#include <cstdint>
#include <memory>

namespace Sabora
{
    /**
     * @brief Abstract texture interface.
     * 
     * Texture represents a GPU texture that can store image data. It supports
     * various texture types (1D, 2D, 3D, Cube, Array) and formats.
     * 
     * Thread Safety:
     * - Texture creation/destruction is thread-safe (uses MainThreadDispatcher)
     * - Texture updates should be done from the main thread or via MainThreadDispatcher
     * - Read operations are thread-safe
     */
    class Texture
    {
    public:
        virtual ~Texture() = default;

        /**
         * @brief Get the texture type.
         * @return The texture type (2D, 3D, Cube, etc.).
         */
        [[nodiscard]] virtual TextureType GetType() const = 0;

        /**
         * @brief Get the texture format.
         * @return The pixel format.
         */
        [[nodiscard]] virtual TextureFormat GetFormat() const = 0;

        /**
         * @brief Get the texture width.
         * @return Width in pixels.
         */
        [[nodiscard]] virtual uint32_t GetWidth() const = 0;

        /**
         * @brief Get the texture height.
         * @return Height in pixels (1 for 1D textures).
         */
        [[nodiscard]] virtual uint32_t GetHeight() const = 0;

        /**
         * @brief Get the texture depth.
         * @return Depth in pixels (1 for 2D textures).
         */
        [[nodiscard]] virtual uint32_t GetDepth() const = 0;

        /**
         * @brief Get the number of mipmap levels.
         * @return Number of mipmap levels.
         */
        [[nodiscard]] virtual uint32_t GetMipLevels() const = 0;

        /**
         * @brief Get the texture usage flags.
         * @return Texture usage flags.
         */
        [[nodiscard]] virtual TextureUsage GetUsage() const = 0;

        /**
         * @brief Update texture data.
         * @param data Pointer to pixel data.
         * @param size Size of the data in bytes.
         * @param mipLevel Mipmap level to update (0 for base level).
         * @param x X offset in pixels.
         * @param y Y offset in pixels (0 for 1D textures).
         * @param z Z offset in pixels (0 for 2D textures).
         * @param width Width of the region to update.
         * @param height Height of the region to update (1 for 1D textures).
         * @param depth Depth of the region to update (1 for 2D textures).
         * @return Result indicating success or failure.
         */
        [[nodiscard]] virtual Result<void> UpdateData(
            const void* data,
            size_t size,
            uint32_t mipLevel = 0,
            uint32_t x = 0,
            uint32_t y = 0,
            uint32_t z = 0,
            uint32_t width = 0,
            uint32_t height = 0,
            uint32_t depth = 0
        ) = 0;

        /**
         * @brief Generate mipmaps for this texture.
         * @return Result indicating success or failure.
         * 
         * This generates mipmap levels for the texture. The base level (level 0)
         * must already contain valid data.
         */
        [[nodiscard]] virtual Result<void> GenerateMipmaps() = 0;

        /**
         * @brief Get the native API handle (for advanced use).
         * @return Opaque pointer to the native texture handle.
         */
        [[nodiscard]] virtual void* GetNativeHandle() const = 0;

        /**
         * @brief Check if the texture is valid.
         * @return True if the texture is valid and can be used.
         */
        [[nodiscard]] virtual bool IsValid() const = 0;

    protected:
        Texture() = default;

        // Non-copyable, movable
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) noexcept = default;
        Texture& operator=(Texture&&) noexcept = default;
    };

} // namespace Sabora
