#pragma once

#include "Assets/IAssetLoader.h"
#include "Renderer/OpenGL/OpenGLTexture.h"
#include <filesystem>

namespace Sabora
{
    /**
     * @brief Asset loader for textures using stb_image.
     * 
     * TextureLoader implements IAssetLoader<OpenGLTexture> to load textures
     * from image files through the Asset System. It uses stb_image to support
     * common formats: PNG, JPEG, BMP, TGA, GIF, HDR.
     */
    class TextureLoader : public IAssetLoader<OpenGLTexture>
    {
    public:
        /**
         * @brief Constructor.
         * @param generateMipmaps Whether to automatically generate mipmaps after loading.
         */
        explicit TextureLoader(bool generateMipmaps = true);

        /**
         * @brief Load a texture from an image file.
         * @param path Path to the image file.
         * @return Result containing the loaded texture, or an error.
         * 
         * This method:
         * 1. Loads image data using stb_image
         * 2. Determines texture format from channel count
         * 3. Creates OpenGLTexture with the image data
         * 4. Optionally generates mipmaps
         */
        [[nodiscard]] Result<std::unique_ptr<OpenGLTexture>> Load(
            const std::filesystem::path& path
        ) override;

        /**
         * @brief Get the asset type name.
         * @return "Texture"
         */
        [[nodiscard]] std::string GetAssetTypeName() const override { return "Texture"; }

        /**
         * @brief Get supported file extensions.
         * @return Vector of supported extensions.
         */
        [[nodiscard]] std::vector<std::string> GetSupportedExtensions() const override;

    private:
        /**
         * @brief Convert stb_image channel count to TextureFormat.
         * @param channels Number of channels (1-4).
         * @return Texture format.
         */
        [[nodiscard]] static TextureFormat GetFormatFromChannels(int channels);

        /**
         * @brief Flip image vertically (OpenGL expects bottom-to-top).
         * @param data Image pixel data.
         * @param width Image width.
         * @param height Image height.
         * @param channels Number of channels.
         */
        static void FlipImageVertically(unsigned char* data, int width, int height, int channels);

        bool m_GenerateMipmaps;  ///< Whether to generate mipmaps after loading
    };

} // namespace Sabora
