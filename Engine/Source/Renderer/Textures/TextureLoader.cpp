#include "pch.h"
#include "TextureLoader.h"
#include "Core/AsyncIO.h"
#include "Core/Log.h"
#include <stb_image.h>
#include <algorithm>

namespace Sabora
{
    //==========================================================================
    // Constructor
    //==========================================================================

    TextureLoader::TextureLoader(bool generateMipmaps)
        : m_GenerateMipmaps(generateMipmaps)
    {
    }

    //==========================================================================
    // IAssetLoader Implementation
    //==========================================================================

    Result<std::unique_ptr<OpenGLTexture>> TextureLoader::Load(
        const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
        {
            return Result<std::unique_ptr<OpenGLTexture>>::Failure(
                ErrorCode::FileNotFound,
                fmt::format("Texture file not found: {}", path.string())
            );
        }

        // Read file data
        auto readResult = AsyncIO::ReadBinaryFile(path);
        if (readResult.IsFailure())
        {
            return Result<std::unique_ptr<OpenGLTexture>>::Failure(
                readResult.GetError().Code(),
                fmt::format("Failed to read texture file: {} - {}", path.string(), readResult.GetError().ToString())
            );
        }

        std::vector<uint8_t> fileData = readResult.Value();

        // Load image using stb_image
        int width = 0;
        int height = 0;
        int channels = 0;
        int desiredChannels = 4; // Force RGBA for consistency

        unsigned char* imageData = stbi_load_from_memory(
            fileData.data(),
            static_cast<int>(fileData.size()),
            &width,
            &height,
            &channels,
            desiredChannels
        );

        if (imageData == nullptr)
        {
            const char* error = stbi_failure_reason();
            return Result<std::unique_ptr<OpenGLTexture>>::Failure(
                ErrorCode::GraphicsTextureCreationFailed,
                fmt::format("Failed to load image {}: {}", path.string(), error ? error : "Unknown error")
            );
        }

        // Validate dimensions
        if (width <= 0 || height <= 0)
        {
            stbi_image_free(imageData);
            return Result<std::unique_ptr<OpenGLTexture>>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                fmt::format("Invalid image dimensions: {}x{}", width, height)
            );
        }

        // OpenGL expects images flipped vertically, so flip it
        FlipImageVertically(imageData, width, height, desiredChannels);

        // Determine texture format
        TextureFormat format = GetFormatFromChannels(desiredChannels);

        // Create texture
        auto textureResult = OpenGLTexture::Create(
            TextureType::Texture2D,
            format,
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            1, // depth
            m_GenerateMipmaps ? 0 : 1, // mip levels (0 = auto if generating)
            TextureUsage::ShaderRead | TextureUsage::RenderTarget,
            imageData
        );

        // Free stb_image data
        stbi_image_free(imageData);

        if (textureResult.IsFailure())
        {
            return Result<std::unique_ptr<OpenGLTexture>>::Failure(
                textureResult.GetError().Code(),
                fmt::format("Failed to create OpenGL texture from image: {} - {}", path.string(), textureResult.GetError().ToString())
            );
        }

        auto texture = std::move(textureResult).Value();

        // Generate mipmaps if requested
        if (m_GenerateMipmaps)
        {
            auto mipResult = texture->GenerateMipmaps();
            if (mipResult.IsFailure())
            {
                SB_CORE_WARN("Failed to generate mipmaps for texture {}: {}", 
                    path.string(), mipResult.GetError().ToString());
                // Don't fail - texture is still usable without mipmaps
            }
        }

        return Result<std::unique_ptr<OpenGLTexture>>::Success(std::move(texture));
    }

    std::vector<std::string> TextureLoader::GetSupportedExtensions() const
    {
        return { ".png", ".jpg", ".jpeg", ".bmp", ".tga", ".gif", ".hdr" };
    }

    //==========================================================================
    // Helper Methods
    //==========================================================================

    TextureFormat TextureLoader::GetFormatFromChannels(int channels)
    {
        switch (channels)
        {
            case 1: return TextureFormat::R8;
            case 2: return TextureFormat::RG8;
            case 3: return TextureFormat::RGB8;
            case 4: return TextureFormat::RGBA8;
            default:
                SB_CORE_WARN("Unknown channel count: {}, defaulting to RGBA8", channels);
                return TextureFormat::RGBA8;
        }
    }

    void TextureLoader::FlipImageVertically(unsigned char* data, int width, int height, int channels)
    {
        if (data == nullptr || width <= 0 || height <= 0 || channels <= 0)
        {
            return;
        }

        int rowSize = width * channels;
        int halfHeight = height / 2;

        for (int y = 0; y < halfHeight; ++y)
        {
            unsigned char* topRow = data + y * rowSize;
            unsigned char* bottomRow = data + (height - 1 - y) * rowSize;

            // Swap rows
            for (int x = 0; x < rowSize; ++x)
            {
                std::swap(topRow[x], bottomRow[x]);
            }
        }
    }

} // namespace Sabora
