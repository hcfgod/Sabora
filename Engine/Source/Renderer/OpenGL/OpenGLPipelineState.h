#pragma once

#include "Renderer/Resources/PipelineState.h"
#include "Renderer/OpenGL/OpenGLShaderProgram.h"
#include <memory>

namespace Sabora
{
    /**
     * @brief OpenGL implementation of pipeline state.
     * 
     * OpenGLPipelineState manages a complete graphics pipeline configuration
     * including shader program, vertex layout, and render state (blend, depth,
     * rasterizer). OpenGL doesn't have explicit pipeline objects like Vulkan,
     * so we simulate them by caching and applying state when the pipeline is bound.
     */
    class OpenGLPipelineState : public PipelineState
    {
    public:
        /**
         * @brief Create a pipeline state from individual shaders.
         * @param vertexShader Vertex shader (required).
         * @param fragmentShader Fragment shader (required).
         * @param vertexLayout Vertex layout description.
         * @param topology Primitive topology.
         * @param blendState Blend state configuration.
         * @param depthStencilState Depth/stencil state configuration.
         * @param rasterizerState Rasterizer state configuration.
         * @return Result containing the created pipeline state, or an error.
         * 
         * This creates an OpenGLShaderProgram internally by linking the shaders.
         */
        [[nodiscard]] static Result<std::unique_ptr<OpenGLPipelineState>> Create(
            Shader* vertexShader,
            Shader* fragmentShader,
            const VertexLayout& vertexLayout,
            PrimitiveTopology topology,
            const BlendState& blendState = BlendState{},
            const DepthStencilState& depthStencilState = DepthStencilState{},
            const RasterizerState& rasterizerState = RasterizerState{}
        );

        /**
         * @brief Destructor.
         */
        ~OpenGLPipelineState() override;

        // Non-copyable, movable
        OpenGLPipelineState(const OpenGLPipelineState&) = delete;
        OpenGLPipelineState& operator=(const OpenGLPipelineState&) = delete;
        OpenGLPipelineState(OpenGLPipelineState&&) noexcept = default;
        OpenGLPipelineState& operator=(OpenGLPipelineState&&) noexcept = default;

        //==========================================================================
        // PipelineState Interface Implementation
        //==========================================================================

        [[nodiscard]] Shader* GetVertexShader() const override;
        [[nodiscard]] Shader* GetFragmentShader() const override;
        [[nodiscard]] const VertexLayout& GetVertexLayout() const override { return m_VertexLayout; }
        [[nodiscard]] PrimitiveTopology GetTopology() const override { return m_Topology; }
        [[nodiscard]] const BlendState& GetBlendState() const override { return m_BlendState; }
        [[nodiscard]] const DepthStencilState& GetDepthStencilState() const override { return m_DepthStencilState; }
        [[nodiscard]] const RasterizerState& GetRasterizerState() const override { return m_RasterizerState; }

        [[nodiscard]] void* GetNativeHandle() const override;
        [[nodiscard]] bool IsValid() const override;

        /**
         * @brief Get the shader program.
         * @return Pointer to the shader program.
         */
        [[nodiscard]] OpenGLShaderProgram* GetShaderProgram() const noexcept { return m_ShaderProgram.get(); }

        /**
         * @brief Get the VAO ID (for internal use by renderer).
         * @return VAO ID, or 0 if not created.
         */
        [[nodiscard]] uint32_t GetVAO() const noexcept { return m_VAO; }

        /**
         * @brief Bind this pipeline state (internal use by renderer).
         * @return Result indicating success or failure.
         * 
         * This applies all pipeline state to OpenGL. Should only be called
         * from the renderer on the main thread.
         */
        [[nodiscard]] Result<void> Bind();

    private:
        /**
         * @brief Private constructor - use Create() factory method.
         * @param shaderProgram Shader program (owned by this pipeline).
         * @param vertexShader Vertex shader (for interface, not owned).
         * @param fragmentShader Fragment shader (for interface, not owned).
         * @param vertexLayout Vertex layout.
         * @param topology Primitive topology.
         * @param blendState Blend state.
         * @param depthStencilState Depth/stencil state.
         * @param rasterizerState Rasterizer state.
         */
        OpenGLPipelineState(
            std::unique_ptr<OpenGLShaderProgram> shaderProgram,
            Shader* vertexShader,
            Shader* fragmentShader,
            const VertexLayout& vertexLayout,
            PrimitiveTopology topology,
            const BlendState& blendState,
            const DepthStencilState& depthStencilState,
            const RasterizerState& rasterizerState
        ) noexcept;

        /**
         * @brief Apply blend state to OpenGL.
         */
        void ApplyBlendState() const;

        /**
         * @brief Apply depth/stencil state to OpenGL.
         */
        void ApplyDepthStencilState() const;

        /**
         * @brief Apply rasterizer state to OpenGL.
         */
        void ApplyRasterizerState() const;

        /**
         * @brief Setup vertex attributes from vertex layout.
         */
        void SetupVertexAttributes() const;

        /**
         * @brief Get OpenGL primitive type from topology.
         * @return OpenGL primitive type enum.
         */
        [[nodiscard]] uint32_t GetGLPrimitiveType() const;

        /**
         * @brief Get OpenGL blend factor from BlendFactor enum.
         * @param factor Blend factor.
         * @return OpenGL blend factor enum.
         */
        [[nodiscard]] static uint32_t GetGLBlendFactor(BlendFactor factor);

        /**
         * @brief Get OpenGL blend equation from BlendOp enum.
         * @param op Blend operation.
         * @return OpenGL blend equation enum.
         */
        [[nodiscard]] static uint32_t GetGLBlendEquation(BlendOp op);

        /**
         * @brief Get OpenGL compare function from CompareFunc enum.
         * @param func Compare function.
         * @return OpenGL compare function enum.
         */
        [[nodiscard]] static uint32_t GetGLCompareFunc(CompareFunc func);

        /**
         * @brief Get OpenGL stencil operation from StencilOp enum.
         * @param op Stencil operation.
         * @return OpenGL stencil operation enum.
         */
        [[nodiscard]] static uint32_t GetGLStencilOp(StencilOp op);

        /**
         * @brief Get OpenGL vertex attribute type from VertexAttributeType enum.
         * @param type Attribute type.
         * @return OpenGL type enum.
         */
        [[nodiscard]] static uint32_t GetGLAttributeType(VertexAttributeType type);

        std::unique_ptr<OpenGLShaderProgram> m_ShaderProgram;  ///< Shader program (owned)
        Shader* m_VertexShader;                    ///< Vertex shader (for interface, not owned)
        Shader* m_FragmentShader;                 ///< Fragment shader (for interface, not owned)
        VertexLayout m_VertexLayout;               ///< Vertex layout
        PrimitiveTopology m_Topology;              ///< Primitive topology
        BlendState m_BlendState;                   ///< Blend state
        DepthStencilState m_DepthStencilState;     ///< Depth/stencil state
        RasterizerState m_RasterizerState;         ///< Rasterizer state
        uint32_t m_VAO = 0;                        ///< Vertex Array Object (0 = not created)
    };

} // namespace Sabora
