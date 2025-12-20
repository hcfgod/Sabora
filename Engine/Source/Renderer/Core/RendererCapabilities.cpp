#include "pch.h"
#include "Renderer/Core/RendererCapabilities.h"
#include <sstream>
#include <spdlog/fmt/fmt.h>

namespace Sabora
{
    std::string RendererCapabilities::ToString() const
    {
        std::ostringstream ss;
        
        ss << fmt::format("Renderer Capabilities:\n");
        ss << fmt::format("  API: {} {}\n", apiName, apiVersion);
        ss << fmt::format("  Vendor: {}\n", vendorName);
        ss << fmt::format("  Device: {}\n", deviceName);
        ss << fmt::format("  Driver: {}\n", driverVersion);
        ss << fmt::format("\nFeatures:\n");
        ss << fmt::format("  Compute Shaders: {}\n", supportsComputeShaders ? "Yes" : "No");
        ss << fmt::format("  Geometry Shaders: {}\n", supportsGeometryShaders ? "Yes" : "No");
        ss << fmt::format("  Tessellation Shaders: {}\n", supportsTessellationShaders ? "Yes" : "No");
        ss << fmt::format("  Instancing: {}\n", supportsInstancing ? "Yes" : "No");
        ss << fmt::format("  Multi-Draw Indirect: {}\n", supportsMultiDrawIndirect ? "Yes" : "No");
        ss << fmt::format("  MSAA: {}\n", supportsMSAA ? "Yes" : "No");
        ss << fmt::format("  Anisotropic Filtering: {}\n", supportsAnisotropicFiltering ? "Yes" : "No");
        ss << fmt::format("  Multiple Render Targets: {}\n", supportsMultipleRenderTargets ? "Yes" : "No");
        ss << fmt::format("\nLimits:\n");
        ss << fmt::format("  Max Texture Size: {}\n", maxTextureSize);
        ss << fmt::format("  Max 3D Texture Size: {}\n", max3DTextureSize);
        ss << fmt::format("  Max Cube Map Size: {}\n", maxCubeMapSize);
        ss << fmt::format("  Max Color Attachments: {}\n", maxColorAttachments);
        ss << fmt::format("  Max Uniform Buffer Size: {} bytes\n", maxUniformBufferSize);
        ss << fmt::format("  Max Storage Buffer Size: {} bytes\n", maxStorageBufferSize);
        ss << fmt::format("  Max Vertex Attributes: {}\n", maxVertexAttributes);
        ss << fmt::format("  Max MSAA Samples: {}\n", maxSamples);
        
        return ss.str();
    }

} // namespace Sabora
