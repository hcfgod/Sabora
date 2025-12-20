#pragma once

#include <cstdint>
#include <string>

namespace Sabora
{
    //==========================================================================
    // Renderer API Types
    //==========================================================================

    /**
     * @brief Supported graphics APIs.
     */
    enum class RendererAPI : uint8_t
    {
        None = 0,
        OpenGL,
        Vulkan,
        DirectX12,
        Metal,
    };

    /**
     * @brief Get a string representation of a RendererAPI.
     * @param api The renderer API.
     * @return String representation.
     */
    inline const char* RendererAPIToString(RendererAPI api) noexcept
    {
        switch (api)
        {
            case RendererAPI::None:      return "None";
            case RendererAPI::OpenGL:   return "OpenGL";
            case RendererAPI::Vulkan:   return "Vulkan";
            case RendererAPI::DirectX12: return "DirectX12";
            case RendererAPI::Metal:    return "Metal";
            default:                    return "Unknown";
        }
    }

    //==========================================================================
    // Texture Types
    //==========================================================================

    /**
     * @brief Texture dimension types.
     */
    enum class TextureType : uint8_t
    {
        Texture1D = 0,
        Texture2D,
        Texture3D,
        TextureCube,
        Texture2DArray,
        TextureCubeArray,
    };

    /**
     * @brief Texture pixel formats.
     */
    enum class TextureFormat : uint8_t
    {
        // 8-bit formats
        R8,
        RG8,
        RGB8,
        RGBA8,
        
        // 16-bit formats
        R16,
        RG16,
        RGB16,
        RGBA16,
        R16F,
        RG16F,
        RGB16F,
        RGBA16F,
        
        // 32-bit formats
        R32F,
        RG32F,
        RGB32F,
        RGBA32F,
        
        // Depth formats
        Depth16,
        Depth24,
        Depth32,
        Depth24Stencil8,
        Depth32F,
        Depth32FStencil8,
        
        // Compressed formats
        BC1,      // DXT1
        BC2,      // DXT3
        BC3,      // DXT5
        BC4,
        BC5,
        BC6H,
        BC7,
        
        // sRGB formats
        SRGB8,
        SRGBA8,
    };

    /**
     * @brief Texture usage hints.
     */
    enum class TextureUsage : uint8_t
    {
        None = 0,
        ShaderRead = 1 << 0,
        ShaderWrite = 1 << 1,
        RenderTarget = 1 << 2,
        DepthStencil = 1 << 3,
    };

    inline TextureUsage operator|(TextureUsage a, TextureUsage b) noexcept
    {
        return static_cast<TextureUsage>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    inline TextureUsage operator&(TextureUsage a, TextureUsage b) noexcept
    {
        return static_cast<TextureUsage>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }

    //==========================================================================
    // Buffer Types
    //==========================================================================

    /**
     * @brief Buffer types.
     */
    enum class BufferType : uint8_t
    {
        Vertex = 0,
        Index,
        Uniform,
        Storage,
        Indirect,
    };

    /**
     * @brief Buffer usage hints.
     */
    enum class BufferUsage : uint8_t
    {
        Static = 0,   ///< Data is set once and used many times
        Dynamic,      ///< Data is modified repeatedly and used many times
        Stream,       ///< Data is modified once and used at most a few times
    };

    //==========================================================================
    // Shader Types
    //==========================================================================

    /**
     * @brief Shader stage types.
     */
    enum class ShaderStage : uint8_t
    {
        Vertex = 0,
        Fragment,
        Geometry,
        Compute,
        TessellationControl,
        TessellationEvaluation,
    };

    /**
     * @brief Get a string representation of a ShaderStage.
     * @param stage The shader stage.
     * @return String representation.
     */
    inline const char* ShaderStageToString(ShaderStage stage) noexcept
    {
        switch (stage)
        {
            case ShaderStage::Vertex:                  return "Vertex";
            case ShaderStage::Fragment:                 return "Fragment";
            case ShaderStage::Geometry:                 return "Geometry";
            case ShaderStage::Compute:                  return "Compute";
            case ShaderStage::TessellationControl:     return "TessellationControl";
            case ShaderStage::TessellationEvaluation:   return "TessellationEvaluation";
            default:                                    return "Unknown";
        }
    }

    //==========================================================================
    // Primitive Types
    //==========================================================================

    /**
     * @brief Primitive topology types.
     */
    enum class PrimitiveTopology : uint8_t
    {
        Points = 0,
        Lines,
        LineStrip,
        Triangles,
        TriangleStrip,
        TriangleFan,
        LinesAdjacency,
        LineStripAdjacency,
        TrianglesAdjacency,
        TriangleStripAdjacency,
        Patches,
    };

    //==========================================================================
    // Blend States
    //==========================================================================

    /**
     * @brief Blend factor types.
     */
    enum class BlendFactor : uint8_t
    {
        Zero = 0,
        One,
        SrcColor,
        OneMinusSrcColor,
        DstColor,
        OneMinusDstColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DstAlpha,
        OneMinusDstAlpha,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha,
        SrcAlphaSaturate,
    };

    /**
     * @brief Blend operation types.
     */
    enum class BlendOp : uint8_t
    {
        Add = 0,
        Subtract,
        ReverseSubtract,
        Min,
        Max,
    };

    /**
     * @brief Color write mask.
     */
    enum class ColorWriteMask : uint8_t
    {
        None = 0,
        Red = 1 << 0,
        Green = 1 << 1,
        Blue = 1 << 2,
        Alpha = 1 << 3,
        All = Red | Green | Blue | Alpha,
    };

    inline ColorWriteMask operator|(ColorWriteMask a, ColorWriteMask b) noexcept
    {
        return static_cast<ColorWriteMask>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    inline ColorWriteMask operator&(ColorWriteMask a, ColorWriteMask b) noexcept
    {
        return static_cast<ColorWriteMask>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }

    //==========================================================================
    // Depth/Stencil States
    //==========================================================================

    /**
     * @brief Depth comparison function.
     */
    enum class CompareFunc : uint8_t
    {
        Never = 0,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always,
    };

    /**
     * @brief Stencil operation.
     */
    enum class StencilOp : uint8_t
    {
        Keep = 0,
        Zero,
        Replace,
        IncrementClamp,
        DecrementClamp,
        IncrementWrap,
        DecrementWrap,
        Invert,
    };

    //==========================================================================
    // Rasterizer States
    //==========================================================================

    /**
     * @brief Polygon fill mode.
     */
    enum class FillMode : uint8_t
    {
        Solid = 0,
        Wireframe,
    };

    /**
     * @brief Face culling mode.
     */
    enum class CullMode : uint8_t
    {
        None = 0,
        Front,
        Back,
        FrontAndBack,
    };

    /**
     * @brief Front face winding order.
     */
    enum class FrontFace : uint8_t
    {
        Clockwise = 0,
        CounterClockwise,
    };

    //==========================================================================
    // Viewport and Scissor
    //==========================================================================

    /**
     * @brief Viewport structure.
     */
    struct Viewport
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float minDepth = 0.0f;
        float maxDepth = 1.0f;
    };

    /**
     * @brief Scissor rectangle structure.
     */
    struct ScissorRect
    {
        int32_t x = 0;
        int32_t y = 0;
        int32_t width = 0;
        int32_t height = 0;
    };

    //==========================================================================
    // Clear Values
    //==========================================================================

    /**
     * @brief Clear color value.
     */
    struct ClearColor
    {
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 0.0f;
    };

    /**
     * @brief Clear depth/stencil value.
     */
    struct ClearDepthStencil
    {
        float depth = 1.0f;
        uint8_t stencil = 0;
    };

    /**
     * @brief Clear flags.
     */
    enum class ClearFlags : uint8_t
    {
        None = 0,
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 2,
        All = Color | Depth | Stencil,
    };

    inline ClearFlags operator|(ClearFlags a, ClearFlags b) noexcept
    {
        return static_cast<ClearFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    inline ClearFlags operator&(ClearFlags a, ClearFlags b) noexcept
    {
        return static_cast<ClearFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }

} // namespace Sabora
