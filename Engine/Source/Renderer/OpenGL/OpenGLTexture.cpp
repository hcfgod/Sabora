#include "pch.h"
#include "OpenGLTexture.h"
#include "Core/Log.h"
#include "Core/MainThreadDispatcher.h"
#include <glad/gl.h>
#include <algorithm>
#include <cmath>

namespace Sabora
{
    //==========================================================================
    // Factory Method
    //==========================================================================

    Result<std::unique_ptr<OpenGLTexture>> OpenGLTexture::Create(
        TextureType type,
        TextureFormat format,
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        uint32_t mipLevels,
        TextureUsage usage,
        const void* data)
    {
        if (width == 0 || height == 0 || depth == 0)
        {
            return Result<std::unique_ptr<OpenGLTexture>>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "Texture dimensions must be greater than 0"
            );
        }

        // Calculate mip levels if not specified
        if (mipLevels == 0)
        {
            mipLevels = CalculateMipLevels(width, height, depth);
        }

        // Create texture on main thread
        uint32_t textureId = 0;
        bool success = false;
        std::string errorMessage;

        auto createFunc = [&textureId, &success, &errorMessage, type, format, width, height, depth, mipLevels, data]() {
            // Generate texture
            glGenTextures(1, &textureId);

            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                success = false;
                errorMessage = fmt::format("Failed to generate OpenGL texture: error code {}", error);
                return;
            }

            // Get target and format info
            uint32_t target = 0;
            switch (type)
            {
                case TextureType::Texture1D:        target = GL_TEXTURE_1D; break;
                case TextureType::Texture2D:        target = GL_TEXTURE_2D; break;
                case TextureType::Texture3D:        target = GL_TEXTURE_3D; break;
                case TextureType::TextureCube:     target = GL_TEXTURE_CUBE_MAP; break;
                case TextureType::Texture2DArray:  target = GL_TEXTURE_2D_ARRAY; break;
                case TextureType::TextureCubeArray: target = GL_TEXTURE_CUBE_MAP_ARRAY; break;
                default:
                    success = false;
                    errorMessage = "Invalid texture type";
                    glDeleteTextures(1, &textureId);
                    textureId = 0;
                    return;
            }

            // Bind texture
            glBindTexture(target, textureId);

            // Get format info
            uint32_t internalFormat = 0;
            uint32_t glFormat = 0;
            uint32_t dataType = 0;

            // Helper lambda to set format info
            auto setFormat = [&](uint32_t internal, uint32_t format, uint32_t type) {
                internalFormat = internal;
                glFormat = format;
                dataType = type;
            };

            // Map texture format to OpenGL formats
            switch (format)
            {
                case TextureFormat::R8:        setFormat(GL_R8, GL_RED, GL_UNSIGNED_BYTE); break;
                case TextureFormat::RG8:        setFormat(GL_RG8, GL_RG, GL_UNSIGNED_BYTE); break;
                case TextureFormat::RGB8:      setFormat(GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE); break;
                case TextureFormat::RGBA8:      setFormat(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE); break;
                case TextureFormat::R16:        setFormat(GL_R16, GL_RED, GL_UNSIGNED_SHORT); break;
                case TextureFormat::RG16:        setFormat(GL_RG16, GL_RG, GL_UNSIGNED_SHORT); break;
                case TextureFormat::RGB16:      setFormat(GL_RGB16, GL_RGB, GL_UNSIGNED_SHORT); break;
                case TextureFormat::RGBA16:     setFormat(GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT); break;
                case TextureFormat::R16F:       setFormat(GL_R16F, GL_RED, GL_HALF_FLOAT); break;
                case TextureFormat::RG16F:      setFormat(GL_RG16F, GL_RG, GL_HALF_FLOAT); break;
                case TextureFormat::RGB16F:     setFormat(GL_RGB16F, GL_RGB, GL_HALF_FLOAT); break;
                case TextureFormat::RGBA16F:    setFormat(GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT); break;
                case TextureFormat::R32F:       setFormat(GL_R32F, GL_RED, GL_FLOAT); break;
                case TextureFormat::RG32F:     setFormat(GL_RG32F, GL_RG, GL_FLOAT); break;
                case TextureFormat::RGB32F:     setFormat(GL_RGB32F, GL_RGB, GL_FLOAT); break;
                case TextureFormat::RGBA32F:    setFormat(GL_RGBA32F, GL_RGBA, GL_FLOAT); break;
                case TextureFormat::Depth16:    setFormat(GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT); break;
                case TextureFormat::Depth24:    setFormat(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT); break;
                case TextureFormat::Depth32:     setFormat(GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT); break;
                case TextureFormat::Depth24Stencil8: setFormat(GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8); break;
                case TextureFormat::Depth32F:    setFormat(GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT); break;
                case TextureFormat::Depth32FStencil8: setFormat(GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV); break;
                case TextureFormat::SRGB8:      setFormat(GL_SRGB8, GL_RGB, GL_UNSIGNED_BYTE); break;
                case TextureFormat::SRGBA8:     setFormat(GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE); break;
                default:
                    success = false;
                    errorMessage = "Unsupported texture format";
                    glDeleteTextures(1, &textureId);
                    textureId = 0;
                    return;
            }

            // Create texture storage based on type
            switch (type)
            {
                case TextureType::Texture1D:
                    glTexStorage1D(target, static_cast<GLsizei>(mipLevels), internalFormat, static_cast<GLsizei>(width));
                    if (data)
                    {
                        glTexSubImage1D(target, 0, 0, static_cast<GLsizei>(width), glFormat, dataType, data);
                    }
                    break;

                case TextureType::Texture2D:
                    glTexStorage2D(target, static_cast<GLsizei>(mipLevels), internalFormat, 
                        static_cast<GLsizei>(width), static_cast<GLsizei>(height));
                    if (data)
                    {
                        glTexSubImage2D(target, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 
                            glFormat, dataType, data);
                    }
                    break;

                case TextureType::Texture3D:
                    glTexStorage3D(target, static_cast<GLsizei>(mipLevels), internalFormat,
                        static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth));
                    if (data)
                    {
                        glTexSubImage3D(target, 0, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 
                            static_cast<GLsizei>(depth), glFormat, dataType, data);
                    }
                    break;

                case TextureType::TextureCube:
                    glTexStorage2D(target, static_cast<GLsizei>(mipLevels), internalFormat,
                        static_cast<GLsizei>(width), static_cast<GLsizei>(height));
                    // Cube maps require data for each face - handled separately if needed
                    break;

                case TextureType::Texture2DArray:
                    glTexStorage3D(target, static_cast<GLsizei>(mipLevels), internalFormat,
                        static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth));
                    if (data)
                    {
                        glTexSubImage3D(target, 0, 0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                            static_cast<GLsizei>(depth), glFormat, dataType, data);
                    }
                    break;

                case TextureType::TextureCubeArray:
                    glTexStorage3D(target, static_cast<GLsizei>(mipLevels), internalFormat,
                        static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth));
                    break;

                default:
                    success = false;
                    errorMessage = "Invalid texture type";
                    glDeleteTextures(1, &textureId);
                    textureId = 0;
                    return;
            }

            // Set default texture parameters
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, mipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
            if (type == TextureType::Texture3D || type == TextureType::Texture2DArray || type == TextureType::TextureCubeArray)
            {
                glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_REPEAT);
            }

            // Check for errors
            error = glGetError();
            if (error != GL_NO_ERROR)
            {
                success = false;
                errorMessage = fmt::format("Failed to create texture storage: error code {}", error);
                glDeleteTextures(1, &textureId);
                textureId = 0;
                return;
            }

            success = true;
        };

        // Execute on main thread
        MainThreadDispatcher::Get().DispatchSync(createFunc);

        if (!success)
        {
            return Result<std::unique_ptr<OpenGLTexture>>::Failure(
                ErrorCode::GraphicsTextureCreationFailed,
                errorMessage
            );
        }

        auto texture = std::unique_ptr<OpenGLTexture>(
            new OpenGLTexture(textureId, type, format, width, height, depth, mipLevels, usage)
        );

        return Result<std::unique_ptr<OpenGLTexture>>::Success(std::move(texture));
    }

    //==========================================================================
    // Constructor/Destructor
    //==========================================================================

    OpenGLTexture::OpenGLTexture(
        uint32_t textureId,
        TextureType type,
        TextureFormat format,
        uint32_t width,
        uint32_t height,
        uint32_t depth,
        uint32_t mipLevels,
        TextureUsage usage) noexcept
        : m_TextureId(textureId)
        , m_Type(type)
        , m_Format(format)
        , m_Width(width)
        , m_Height(height)
        , m_Depth(depth)
        , m_MipLevels(mipLevels)
        , m_Usage(usage)
    {
    }

    OpenGLTexture::~OpenGLTexture()
    {
        if (m_TextureId != 0)
        {
            // Clean up texture on main thread
            // Use DispatchSync to ensure synchronous cleanup even during shutdown
            uint32_t textureId = m_TextureId;
            MainThreadDispatcher::Get().DispatchSync([textureId]() {
                glDeleteTextures(1, &textureId);
            });
            m_TextureId = 0;
        }
    }

    OpenGLTexture::OpenGLTexture(OpenGLTexture&& other) noexcept
        : m_TextureId(other.m_TextureId)
        , m_Type(other.m_Type)
        , m_Format(other.m_Format)
        , m_Width(other.m_Width)
        , m_Height(other.m_Height)
        , m_Depth(other.m_Depth)
        , m_MipLevels(other.m_MipLevels)
        , m_Usage(other.m_Usage)
    {
        other.m_TextureId = 0;
        other.m_Width = 0;
        other.m_Height = 0;
        other.m_Depth = 0;
    }

    OpenGLTexture& OpenGLTexture::operator=(OpenGLTexture&& other) noexcept
    {
        if (this != &other)
        {
            if (m_TextureId != 0)
            {
                uint32_t textureId = m_TextureId;
                MainThreadDispatcher::Get().DispatchSync([textureId]() {
                    glDeleteTextures(1, &textureId);
                });
            }

            m_TextureId = other.m_TextureId;
            m_Type = other.m_Type;
            m_Format = other.m_Format;
            m_Width = other.m_Width;
            m_Height = other.m_Height;
            m_Depth = other.m_Depth;
            m_MipLevels = other.m_MipLevels;
            m_Usage = other.m_Usage;

            other.m_TextureId = 0;
            other.m_Width = 0;
            other.m_Height = 0;
            other.m_Depth = 0;
        }

        return *this;
    }

    //==========================================================================
    // Texture Interface Implementation
    //==========================================================================

    Result<void> OpenGLTexture::UpdateData(
        const void* data,
        size_t size,
        uint32_t mipLevel,
        uint32_t x,
        uint32_t y,
        uint32_t z,
        uint32_t width,
        uint32_t height,
        uint32_t depth)
    {
        if (data == nullptr)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "UpdateData: data pointer is null"
            );
        }

        // Use full texture dimensions if width/height/depth are 0
        if (width == 0) width = m_Width;
        if (height == 0) height = m_Height;
        if (depth == 0) depth = m_Depth;

        bool success = false;
        std::string errorMessage;

        auto updateFunc = [this, &success, &errorMessage, data, mipLevel, x, y, z, width, height, depth]() {
            uint32_t target = GetGLTarget();
            glBindTexture(target, m_TextureId);

            uint32_t glFormat = GetGLFormat();
            uint32_t dataType = GetGLDataType();

            switch (m_Type)
            {
                case TextureType::Texture1D:
                    glTexSubImage1D(target, static_cast<GLint>(mipLevel), static_cast<GLint>(x),
                        static_cast<GLsizei>(width), glFormat, dataType, data);
                    break;

                case TextureType::Texture2D:
                    glTexSubImage2D(target, static_cast<GLint>(mipLevel), static_cast<GLint>(x), static_cast<GLint>(y),
                        static_cast<GLsizei>(width), static_cast<GLsizei>(height), glFormat, dataType, data);
                    break;

                case TextureType::Texture3D:
                case TextureType::Texture2DArray:
                case TextureType::TextureCubeArray:
                    glTexSubImage3D(target, static_cast<GLint>(mipLevel), static_cast<GLint>(x), 
                        static_cast<GLint>(y), static_cast<GLint>(z),
                        static_cast<GLsizei>(width), static_cast<GLsizei>(height), static_cast<GLsizei>(depth),
                        glFormat, dataType, data);
                    break;

                case TextureType::TextureCube:
                    // Cube maps require face specification - use GL_TEXTURE_CUBE_MAP_POSITIVE_X + face
                    // For now, update all faces (can be extended)
                    for (uint32_t face = 0; face < 6; ++face)
                    {
                        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, static_cast<GLint>(mipLevel),
                            static_cast<GLint>(x), static_cast<GLint>(y),
                            static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                            glFormat, dataType, data);
                    }
                    break;

                default:
                    success = false;
                    errorMessage = "Invalid texture type for UpdateData";
                    return;
            }

            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                success = false;
                errorMessage = fmt::format("Failed to update texture data: error code {}", error);
                return;
            }

            success = true;
        };

        MainThreadDispatcher::Get().DispatchSync(updateFunc);

        if (!success)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                errorMessage
            );
        }

        return Result<void>::Success();
    }

    Result<void> OpenGLTexture::GenerateMipmaps()
    {
        if (m_MipLevels <= 1)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "Cannot generate mipmaps for texture with only one mip level"
            );
        }

        bool success = false;
        std::string errorMessage;

        auto genFunc = [this, &success, &errorMessage]() {
            uint32_t target = GetGLTarget();
            glBindTexture(target, m_TextureId);
            glGenerateMipmap(target);

            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                success = false;
                errorMessage = fmt::format("Failed to generate mipmaps: error code {}", error);
                return;
            }

            success = true;
        };

        MainThreadDispatcher::Get().DispatchSync(genFunc);

        if (!success)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                errorMessage
            );
        }

        return Result<void>::Success();
    }

    void* OpenGLTexture::GetNativeHandle() const
    {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(m_TextureId));
    }

    bool OpenGLTexture::IsValid() const
    {
        return m_TextureId != 0;
    }

    //==========================================================================
    // Helper Methods
    //==========================================================================

    uint32_t OpenGLTexture::GetGLTarget() const
    {
        switch (m_Type)
        {
            case TextureType::Texture1D:        return GL_TEXTURE_1D;
            case TextureType::Texture2D:        return GL_TEXTURE_2D;
            case TextureType::Texture3D:        return GL_TEXTURE_3D;
            case TextureType::TextureCube:     return GL_TEXTURE_CUBE_MAP;
            case TextureType::Texture2DArray:  return GL_TEXTURE_2D_ARRAY;
            case TextureType::TextureCubeArray: return GL_TEXTURE_CUBE_MAP_ARRAY;
            default:                            return 0;
        }
    }

    uint32_t OpenGLTexture::GetGLInternalFormat() const
    {
        switch (m_Format)
        {
            case TextureFormat::R8:        return GL_R8;
            case TextureFormat::RG8:        return GL_RG8;
            case TextureFormat::RGB8:      return GL_RGB8;
            case TextureFormat::RGBA8:      return GL_RGBA8;
            case TextureFormat::R16:        return GL_R16;
            case TextureFormat::RG16:        return GL_RG16;
            case TextureFormat::RGB16:      return GL_RGB16;
            case TextureFormat::RGBA16:     return GL_RGBA16;
            case TextureFormat::R16F:       return GL_R16F;
            case TextureFormat::RG16F:      return GL_RG16F;
            case TextureFormat::RGB16F:     return GL_RGB16F;
            case TextureFormat::RGBA16F:    return GL_RGBA16F;
            case TextureFormat::R32F:       return GL_R32F;
            case TextureFormat::RG32F:     return GL_RG32F;
            case TextureFormat::RGB32F:     return GL_RGB32F;
            case TextureFormat::RGBA32F:    return GL_RGBA32F;
            case TextureFormat::Depth16:    return GL_DEPTH_COMPONENT16;
            case TextureFormat::Depth24:    return GL_DEPTH_COMPONENT24;
            case TextureFormat::Depth32:     return GL_DEPTH_COMPONENT32;
            case TextureFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
            case TextureFormat::Depth32F:    return GL_DEPTH_COMPONENT32F;
            case TextureFormat::Depth32FStencil8: return GL_DEPTH32F_STENCIL8;
            case TextureFormat::SRGB8:      return GL_SRGB8;
            case TextureFormat::SRGBA8:     return GL_SRGB8_ALPHA8;
            default:                        return 0;
        }
    }

    uint32_t OpenGLTexture::GetGLFormat() const
    {
        switch (m_Format)
        {
            case TextureFormat::R8:
            case TextureFormat::R16:
            case TextureFormat::R16F:
            case TextureFormat::R32F:
            case TextureFormat::Depth16:
            case TextureFormat::Depth24:
            case TextureFormat::Depth32:
            case TextureFormat::Depth32F:
                return GL_RED;

            case TextureFormat::RG8:
            case TextureFormat::RG16:
            case TextureFormat::RG16F:
            case TextureFormat::RG32F:
                return GL_RG;

            case TextureFormat::RGB8:
            case TextureFormat::RGB16:
            case TextureFormat::RGB16F:
            case TextureFormat::RGB32F:
            case TextureFormat::SRGB8:
                return GL_RGB;

            case TextureFormat::RGBA8:
            case TextureFormat::RGBA16:
            case TextureFormat::RGBA16F:
            case TextureFormat::RGBA32F:
            case TextureFormat::SRGBA8:
                return GL_RGBA;

            case TextureFormat::Depth24Stencil8:
            case TextureFormat::Depth32FStencil8:
                return GL_DEPTH_STENCIL;

            default:
                return GL_RGBA;
        }
    }

    uint32_t OpenGLTexture::GetGLDataType() const
    {
        switch (m_Format)
        {
            case TextureFormat::R8:
            case TextureFormat::RG8:
            case TextureFormat::RGB8:
            case TextureFormat::RGBA8:
            case TextureFormat::SRGB8:
            case TextureFormat::SRGBA8:
                return GL_UNSIGNED_BYTE;

            case TextureFormat::R16:
            case TextureFormat::RG16:
            case TextureFormat::RGB16:
            case TextureFormat::RGBA16:
            case TextureFormat::Depth16:
                return GL_UNSIGNED_SHORT;

            case TextureFormat::Depth24:
            case TextureFormat::Depth32:
            case TextureFormat::Depth24Stencil8:
                return GL_UNSIGNED_INT;

            case TextureFormat::R16F:
            case TextureFormat::RG16F:
            case TextureFormat::RGB16F:
            case TextureFormat::RGBA16F:
                return GL_HALF_FLOAT;

            case TextureFormat::R32F:
            case TextureFormat::RG32F:
            case TextureFormat::RGB32F:
            case TextureFormat::RGBA32F:
            case TextureFormat::Depth32F:
                return GL_FLOAT;

            case TextureFormat::Depth32FStencil8:
                return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;

            default:
                return GL_UNSIGNED_BYTE;
        }
    }

    uint32_t OpenGLTexture::CalculateMipLevels(uint32_t width, uint32_t height, uint32_t depth)
    {
        uint32_t maxDim = std::max({ width, height, depth });
        return static_cast<uint32_t>(std::floor(std::log2(static_cast<double>(maxDim)))) + 1;
    }

} // namespace Sabora
