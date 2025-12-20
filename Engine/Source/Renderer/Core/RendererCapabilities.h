#pragma once

#include <cstdint>
#include <string>

namespace Sabora
{
    /**
     * @brief Renderer capabilities and feature support information.
     * 
     * This structure contains information about what features and capabilities
     * are supported by the current renderer implementation.
     */
    struct RendererCapabilities
    {
        // API Information
        std::string apiName;              ///< API name (e.g., "OpenGL", "Vulkan")
        std::string apiVersion;           ///< API version string
        std::string vendorName;           ///< GPU vendor name
        std::string deviceName;           ///< GPU device name
        std::string driverVersion;        ///< Driver version string

        // Feature Support
        bool supportsComputeShaders = false;      ///< Compute shader support
        bool supportsGeometryShaders = false;     ///< Geometry shader support
        bool supportsTessellationShaders = false; ///< Tessellation shader support
        bool supportsInstancing = false;        ///< Instanced rendering support
        bool supportsMultiDrawIndirect = false;   ///< Multi-draw indirect support
        bool supportsTextureArrays = false;       ///< Texture array support
        bool supportsCubeMaps = false;           ///< Cube map texture support
        bool supports3DTextures = false;          ///< 3D texture support
        bool supportsMSAA = false;               ///< Multisample anti-aliasing support
        bool supportsAnisotropicFiltering = false; ///< Anisotropic texture filtering
        bool supportsSRGB = false;                ///< sRGB color space support
        bool supportsFloatTextures = false;       ///< Floating-point texture support
        bool supportsDepthTextures = false;       ///< Depth texture support
        bool supportsStencilTextures = false;     ///< Stencil texture support
        bool supportsMultipleRenderTargets = false; ///< Multiple render targets (MRT)

        // Limits
        uint32_t maxTextureSize = 0;              ///< Maximum texture dimension
        uint32_t max3DTextureSize = 0;           ///< Maximum 3D texture dimension
        uint32_t maxCubeMapSize = 0;             ///< Maximum cube map texture size
        uint32_t maxArrayTextureLayers = 0;       ///< Maximum texture array layers
        uint32_t maxTextureAnisotropy = 0;       ///< Maximum anisotropic filtering level
        uint32_t maxColorAttachments = 0;        ///< Maximum color attachments (MRT)
        uint32_t maxUniformBufferSize = 0;        ///< Maximum uniform buffer size
        uint32_t maxStorageBufferSize = 0;        ///< Maximum storage buffer size
        uint32_t maxComputeWorkGroupSize[3] = {0, 0, 0}; ///< Maximum compute work group size
        uint32_t maxComputeWorkGroupCount[3] = {0, 0, 0}; ///< Maximum compute work group count
        uint32_t maxViewports = 0;               ///< Maximum number of viewports
        uint32_t maxSamples = 0;                 ///< Maximum MSAA samples

        // Shader Limits
        uint32_t maxVertexAttributes = 0;        ///< Maximum vertex attributes
        uint32_t maxVertexUniformComponents = 0; ///< Maximum vertex uniform components
        uint32_t maxFragmentUniformComponents = 0; ///< Maximum fragment uniform components
        uint32_t maxGeometryOutputVertices = 0;  ///< Maximum geometry shader output vertices

        /**
         * @brief Get a string representation of all capabilities.
         * @return Formatted string with all capability information.
         */
        [[nodiscard]] std::string ToString() const;
    };

} // namespace Sabora
