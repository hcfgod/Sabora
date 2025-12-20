#pragma once

#include "Core/Result.h"
#include "Renderer/Core/RendererTypes.h"
#include "Renderer/Resources/Shader.h"
#include "Renderer/Resources/VertexLayout.h"
#include <memory>

namespace Sabora
{
    /**
     * @brief Blend state configuration.
     */
    struct BlendState
    {
        bool enabled = false;
        BlendFactor srcColor = BlendFactor::One;
        BlendFactor dstColor = BlendFactor::Zero;
        BlendFactor srcAlpha = BlendFactor::One;
        BlendFactor dstAlpha = BlendFactor::Zero;
        BlendOp colorOp = BlendOp::Add;
        BlendOp alphaOp = BlendOp::Add;
        ColorWriteMask writeMask = ColorWriteMask::All;
    };

    /**
     * @brief Depth/stencil state configuration.
     */
    struct DepthStencilState
    {
        // Depth
        bool depthTestEnabled = true;
        bool depthWriteEnabled = true;
        CompareFunc depthFunc = CompareFunc::Less;

        // Stencil
        bool stencilTestEnabled = false;
        uint8_t stencilReadMask = 0xFF;
        uint8_t stencilWriteMask = 0xFF;

        struct StencilFaceState
        {
            StencilOp failOp = StencilOp::Keep;
            StencilOp depthFailOp = StencilOp::Keep;
            StencilOp passOp = StencilOp::Keep;
            CompareFunc func = CompareFunc::Always;
        };

        StencilFaceState frontFace;
        StencilFaceState backFace;
    };

    /**
     * @brief Rasterizer state configuration.
     */
    struct RasterizerState
    {
        FillMode fillMode = FillMode::Solid;
        CullMode cullMode = CullMode::Back;
        FrontFace frontFace = FrontFace::CounterClockwise;
        bool depthClipEnabled = true;
        bool scissorEnabled = false;
        float depthBias = 0.0f;
        float depthBiasClamp = 0.0f;
        float slopeScaledDepthBias = 0.0f;
    };

    /**
     * @brief Abstract pipeline state interface.
     * 
     * PipelineState represents a complete graphics pipeline configuration,
     * including shaders, vertex layout, and render state (blend, depth, etc.).
     * It provides a unified interface for pipeline operations across different APIs.
     * 
     * Thread Safety:
     * - Pipeline state creation/destruction is thread-safe (uses MainThreadDispatcher)
     * - Pipeline state operations are thread-safe
     */
    class PipelineState
    {
    public:
        virtual ~PipelineState() = default;

        /**
         * @brief Get the vertex shader.
         * @return Pointer to the vertex shader, or nullptr if not set.
         */
        [[nodiscard]] virtual Shader* GetVertexShader() const = 0;

        /**
         * @brief Get the fragment shader.
         * @return Pointer to the fragment shader, or nullptr if not set.
         */
        [[nodiscard]] virtual Shader* GetFragmentShader() const = 0;

        /**
         * @brief Get the vertex layout.
         * @return Reference to the vertex layout.
         */
        [[nodiscard]] virtual const VertexLayout& GetVertexLayout() const = 0;

        /**
         * @brief Get the primitive topology.
         * @return The primitive topology.
         */
        [[nodiscard]] virtual PrimitiveTopology GetTopology() const = 0;

        /**
         * @brief Get the blend state.
         * @return Reference to the blend state.
         */
        [[nodiscard]] virtual const BlendState& GetBlendState() const = 0;

        /**
         * @brief Get the depth/stencil state.
         * @return Reference to the depth/stencil state.
         */
        [[nodiscard]] virtual const DepthStencilState& GetDepthStencilState() const = 0;

        /**
         * @brief Get the rasterizer state.
         * @return Reference to the rasterizer state.
         */
        [[nodiscard]] virtual const RasterizerState& GetRasterizerState() const = 0;

        /**
         * @brief Get the native API handle (for advanced use).
         * @return Opaque pointer to the native pipeline handle.
         */
        [[nodiscard]] virtual void* GetNativeHandle() const = 0;

        /**
         * @brief Check if the pipeline state is valid.
         * @return True if the pipeline state is valid and can be used.
         */
        [[nodiscard]] virtual bool IsValid() const = 0;

    protected:
        PipelineState() = default;

        // Non-copyable, movable
        PipelineState(const PipelineState&) = delete;
        PipelineState& operator=(const PipelineState&) = delete;
        PipelineState(PipelineState&&) noexcept = default;
        PipelineState& operator=(PipelineState&&) noexcept = default;
    };

} // namespace Sabora
