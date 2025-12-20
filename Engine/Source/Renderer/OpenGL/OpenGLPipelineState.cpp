#include "pch.h"
#include "OpenGLPipelineState.h"
#include "OpenGLShader.h"
#include "Core/Log.h"
#include "Core/MainThreadDispatcher.h"
#include <glad/gl.h>
#include <algorithm>

namespace Sabora
{
    //==========================================================================
    // Factory Method
    //==========================================================================

    Result<std::unique_ptr<OpenGLPipelineState>> OpenGLPipelineState::Create(
        Shader* vertexShader,
        Shader* fragmentShader,
        const VertexLayout& vertexLayout,
        PrimitiveTopology topology,
        const BlendState& blendState,
        const DepthStencilState& depthStencilState,
        const RasterizerState& rasterizerState)
    {
        if (!vertexShader || !vertexShader->IsValid())
        {
            return Result<std::unique_ptr<OpenGLPipelineState>>::Failure(
                ErrorCode::CoreInvalidArgument,
                "Vertex shader is null or invalid"
            );
        }

        if (!fragmentShader || !fragmentShader->IsValid())
        {
            return Result<std::unique_ptr<OpenGLPipelineState>>::Failure(
                ErrorCode::CoreInvalidArgument,
                "Fragment shader is null or invalid"
            );
        }

        // Cast to OpenGLShader
        OpenGLShader* glVertexShader = dynamic_cast<OpenGLShader*>(vertexShader);
        OpenGLShader* glFragmentShader = dynamic_cast<OpenGLShader*>(fragmentShader);

        if (!glVertexShader || !glFragmentShader)
        {
            return Result<std::unique_ptr<OpenGLPipelineState>>::Failure(
                ErrorCode::CoreInvalidArgument,
                "Shaders must be OpenGLShader instances"
            );
        }

        // Link shaders into a program on the main thread
        uint32_t linkedProgramId = 0;
        bool linkSuccess = false;
        std::string linkError;

        // Restructure: do the linking and get program ID
        auto linkResult = [glVertexShader, glFragmentShader, &linkedProgramId, &linkSuccess, &linkError]() {
            linkedProgramId = glCreateProgram();
            if (linkedProgramId == 0)
            {
                linkSuccess = false;
                linkError = "Failed to create shader program";
                return;
            }

            glAttachShader(linkedProgramId, glVertexShader->GetShaderId());
            glAttachShader(linkedProgramId, glFragmentShader->GetShaderId());
            glLinkProgram(linkedProgramId);

            GLint success = 0;
            glGetProgramiv(linkedProgramId, GL_LINK_STATUS, &success);

            if (success == GL_FALSE)
            {
                GLint logLength = 0;
                glGetProgramiv(linkedProgramId, GL_INFO_LOG_LENGTH, &logLength);

                if (logLength > 0)
                {
                    linkError.resize(static_cast<size_t>(logLength));
                    glGetProgramInfoLog(linkedProgramId, logLength, nullptr, linkError.data());
                    if (!linkError.empty() && linkError.back() == '\0')
                    {
                        linkError.pop_back();
                    }
                }
                else
                {
                    linkError = "Shader program linking failed";
                }

                glDetachShader(linkedProgramId, glVertexShader->GetShaderId());
                glDetachShader(linkedProgramId, glFragmentShader->GetShaderId());
                glDeleteProgram(linkedProgramId);
                linkedProgramId = 0;
                linkSuccess = false;
                return;
            }

            glDetachShader(linkedProgramId, glVertexShader->GetShaderId());
            glDetachShader(linkedProgramId, glFragmentShader->GetShaderId());
            linkSuccess = true;
        };

        MainThreadDispatcher::Get().DispatchSync(linkResult);

        if (!linkSuccess)
        {
            return Result<std::unique_ptr<OpenGLPipelineState>>::Failure(
                ErrorCode::GraphicsShaderLinkFailed,
                linkError
            );
        }

        // Create program from the linked program ID
        auto programResult = OpenGLShaderProgram::CreateFromProgramId(linkedProgramId);
        if (programResult.IsFailure())
        {
            // Clean up program
            MainThreadDispatcher::Get().DispatchSync([linkedProgramId]() {
                glDeleteProgram(linkedProgramId);
            });
            return Result<std::unique_ptr<OpenGLPipelineState>>::Failure(programResult.GetError());
        }

        auto pipeline = std::unique_ptr<OpenGLPipelineState>(
            new OpenGLPipelineState(
                std::move(programResult).Value(),
                vertexShader,
                fragmentShader,
                vertexLayout,
                topology,
                blendState,
                depthStencilState,
                rasterizerState
            )
        );

        // Create VAO for this pipeline state
        uint32_t vaoId = 0;
        bool vaoCreated = false;
        std::string vaoError;

        auto createVAOFunc = [&vaoId, &vaoCreated, &vaoError]() {
            glGenVertexArrays(1, &vaoId);
            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                vaoCreated = false;
                vaoError = fmt::format("Failed to create VAO: error code {}", error);
                return;
            }
            vaoCreated = true;
        };

        MainThreadDispatcher::Get().DispatchSync(createVAOFunc);

        if (!vaoCreated)
        {
            return Result<std::unique_ptr<OpenGLPipelineState>>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                vaoError
            );
        }

        pipeline->m_VAO = vaoId;

        return Result<std::unique_ptr<OpenGLPipelineState>>::Success(std::move(pipeline));
    }

    //==========================================================================
    // Constructor
    //==========================================================================

    OpenGLPipelineState::OpenGLPipelineState(
        std::unique_ptr<OpenGLShaderProgram> shaderProgram,
        Shader* vertexShader,
        Shader* fragmentShader,
        const VertexLayout& vertexLayout,
        PrimitiveTopology topology,
        const BlendState& blendState,
        const DepthStencilState& depthStencilState,
        const RasterizerState& rasterizerState) noexcept
        : m_ShaderProgram(std::move(shaderProgram))
        , m_VertexShader(vertexShader)
        , m_FragmentShader(fragmentShader)
        , m_VertexLayout(vertexLayout)
        , m_Topology(topology)
        , m_BlendState(blendState)
        , m_DepthStencilState(depthStencilState)
        , m_RasterizerState(rasterizerState)
        , m_VAO(0)
    {
    }

    OpenGLPipelineState::~OpenGLPipelineState()
    {
        // Clean up VAO on main thread
        if (m_VAO != 0)
        {
            uint32_t vaoId = m_VAO;
            MainThreadDispatcher::Get().Dispatch([vaoId]() {
                glDeleteVertexArrays(1, &vaoId);
            });
        }
    }

    //==========================================================================
    // PipelineState Interface Implementation
    //==========================================================================

    Shader* OpenGLPipelineState::GetVertexShader() const
    {
        return m_VertexShader;
    }

    Shader* OpenGLPipelineState::GetFragmentShader() const
    {
        return m_FragmentShader;
    }

    void* OpenGLPipelineState::GetNativeHandle() const
    {
        return m_ShaderProgram ? m_ShaderProgram->GetNativeHandle() : nullptr;
    }

    bool OpenGLPipelineState::IsValid() const
    {
        return m_ShaderProgram && m_ShaderProgram->IsValid();
    }

    Result<void> OpenGLPipelineState::Bind()
    {
        if (!m_ShaderProgram || !m_ShaderProgram->IsValid())
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                "Cannot bind invalid pipeline state"
            );
        }

        bool success = false;
        std::string errorMessage;

        auto bindFunc = [this, &success, &errorMessage]() {
            // Bind VAO (required for OpenGL 3.3+ core profile)
            if (m_VAO != 0)
            {
                glBindVertexArray(m_VAO);
            }
            else
            {
                success = false;
                errorMessage = "VAO not created for pipeline state";
                return;
            }

            // Bind shader program
            glUseProgram(m_ShaderProgram->GetProgramId());

            // Apply render state
            ApplyBlendState();
            ApplyDepthStencilState();
            ApplyRasterizerState();
            SetupVertexAttributes();

            GLenum error = glGetError();
            if (error != GL_NO_ERROR)
            {
                success = false;
                errorMessage = fmt::format("Failed to bind pipeline state: error code {}", error);
                return;
            }

            success = true;
        };

        MainThreadDispatcher::Get().DispatchSync(bindFunc);

        if (!success)
        {
            return Result<void>::Failure(
                ErrorCode::GraphicsInvalidOperation,
                errorMessage
            );
        }

        return Result<void>::Success();
    }

    //==========================================================================
    // State Application Methods
    //==========================================================================

    void OpenGLPipelineState::ApplyBlendState() const
    {
        if (m_BlendState.enabled)
        {
            glEnable(GL_BLEND);
            glBlendFuncSeparate(
                GetGLBlendFactor(m_BlendState.srcColor),
                GetGLBlendFactor(m_BlendState.dstColor),
                GetGLBlendFactor(m_BlendState.srcAlpha),
                GetGLBlendFactor(m_BlendState.dstAlpha)
            );
            glBlendEquationSeparate(
                GetGLBlendEquation(m_BlendState.colorOp),
                GetGLBlendEquation(m_BlendState.alphaOp)
            );

            // Color write mask
            GLboolean r = (m_BlendState.writeMask & ColorWriteMask::Red) != ColorWriteMask::None ? GL_TRUE : GL_FALSE;
            GLboolean g = (m_BlendState.writeMask & ColorWriteMask::Green) != ColorWriteMask::None ? GL_TRUE : GL_FALSE;
            GLboolean b = (m_BlendState.writeMask & ColorWriteMask::Blue) != ColorWriteMask::None ? GL_TRUE : GL_FALSE;
            GLboolean a = (m_BlendState.writeMask & ColorWriteMask::Alpha) != ColorWriteMask::None ? GL_TRUE : GL_FALSE;
            glColorMask(r, g, b, a);
        }
        else
        {
            glDisable(GL_BLEND);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }
    }

    void OpenGLPipelineState::ApplyDepthStencilState() const
    {
        // Depth state
        if (m_DepthStencilState.depthTestEnabled)
        {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GetGLCompareFunc(m_DepthStencilState.depthFunc));
            glDepthMask(m_DepthStencilState.depthWriteEnabled ? GL_TRUE : GL_FALSE);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
        }

        // Stencil state
        if (m_DepthStencilState.stencilTestEnabled)
        {
            glEnable(GL_STENCIL_TEST);
            glStencilMask(m_DepthStencilState.stencilWriteMask);

            // Front face
            glStencilFuncSeparate(
                GL_FRONT,
                GetGLCompareFunc(m_DepthStencilState.frontFace.func),
                0, // ref (set per draw)
                m_DepthStencilState.stencilReadMask
            );
            glStencilOpSeparate(
                GL_FRONT,
                GetGLStencilOp(m_DepthStencilState.frontFace.failOp),
                GetGLStencilOp(m_DepthStencilState.frontFace.depthFailOp),
                GetGLStencilOp(m_DepthStencilState.frontFace.passOp)
            );

            // Back face
            glStencilFuncSeparate(
                GL_BACK,
                GetGLCompareFunc(m_DepthStencilState.backFace.func),
                0, // ref (set per draw)
                m_DepthStencilState.stencilReadMask
            );
            glStencilOpSeparate(
                GL_BACK,
                GetGLStencilOp(m_DepthStencilState.backFace.failOp),
                GetGLStencilOp(m_DepthStencilState.backFace.depthFailOp),
                GetGLStencilOp(m_DepthStencilState.backFace.passOp)
            );
        }
        else
        {
            glDisable(GL_STENCIL_TEST);
        }
    }

    void OpenGLPipelineState::ApplyRasterizerState() const
    {
        // Fill mode
        switch (m_RasterizerState.fillMode)
        {
            case FillMode::Solid:
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                break;
            case FillMode::Wireframe:
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                break;
        }

        // Cull mode
        switch (m_RasterizerState.cullMode)
        {
            case CullMode::None:
                glDisable(GL_CULL_FACE);
                break;
            case CullMode::Front:
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT);
                break;
            case CullMode::Back:
                glEnable(GL_CULL_FACE);
                glCullFace(GL_BACK);
                break;
            case CullMode::FrontAndBack:
                glEnable(GL_CULL_FACE);
                glCullFace(GL_FRONT_AND_BACK);
                break;
        }

        // Front face
        glFrontFace(m_RasterizerState.frontFace == FrontFace::Clockwise ? GL_CW : GL_CCW);

        // Depth clamp
        if (m_RasterizerState.depthClipEnabled)
        {
            glDisable(GL_DEPTH_CLAMP);
        }
        else
        {
            glEnable(GL_DEPTH_CLAMP);
        }

        // Depth bias
        if (m_RasterizerState.depthBias != 0.0f || m_RasterizerState.slopeScaledDepthBias != 0.0f)
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(m_RasterizerState.slopeScaledDepthBias, m_RasterizerState.depthBias);
        }
        else
        {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }

    void OpenGLPipelineState::SetupVertexAttributes() const
    {
        // Vertex attributes are set up when binding vertex buffers
        // This is a placeholder - actual setup happens in draw calls
        // when vertex buffers are bound
    }

    //==========================================================================
    // Helper Methods
    //==========================================================================

    uint32_t OpenGLPipelineState::GetGLPrimitiveType() const
    {
        switch (m_Topology)
        {
            case PrimitiveTopology::Points:                return GL_POINTS;
            case PrimitiveTopology::Lines:                 return GL_LINES;
            case PrimitiveTopology::LineStrip:             return GL_LINE_STRIP;
            case PrimitiveTopology::Triangles:             return GL_TRIANGLES;
            case PrimitiveTopology::TriangleStrip:         return GL_TRIANGLE_STRIP;
            case PrimitiveTopology::TriangleFan:           return GL_TRIANGLE_FAN;
            case PrimitiveTopology::LinesAdjacency:        return GL_LINES_ADJACENCY;
            case PrimitiveTopology::LineStripAdjacency:    return GL_LINE_STRIP_ADJACENCY;
            case PrimitiveTopology::TrianglesAdjacency:    return GL_TRIANGLES_ADJACENCY;
            case PrimitiveTopology::TriangleStripAdjacency: return GL_TRIANGLE_STRIP_ADJACENCY;
            case PrimitiveTopology::Patches:               return GL_PATCHES;
            default:                                       return GL_TRIANGLES;
        }
    }

    uint32_t OpenGLPipelineState::GetGLBlendFactor(BlendFactor factor)
    {
        switch (factor)
        {
            case BlendFactor::Zero:                return GL_ZERO;
            case BlendFactor::One:                 return GL_ONE;
            case BlendFactor::SrcColor:            return GL_SRC_COLOR;
            case BlendFactor::OneMinusSrcColor:    return GL_ONE_MINUS_SRC_COLOR;
            case BlendFactor::DstColor:            return GL_DST_COLOR;
            case BlendFactor::OneMinusDstColor:    return GL_ONE_MINUS_DST_COLOR;
            case BlendFactor::SrcAlpha:           return GL_SRC_ALPHA;
            case BlendFactor::OneMinusSrcAlpha:    return GL_ONE_MINUS_SRC_ALPHA;
            case BlendFactor::DstAlpha:           return GL_DST_ALPHA;
            case BlendFactor::OneMinusDstAlpha:    return GL_ONE_MINUS_DST_ALPHA;
            case BlendFactor::ConstantColor:       return GL_CONSTANT_COLOR;
            case BlendFactor::OneMinusConstantColor: return GL_ONE_MINUS_CONSTANT_COLOR;
            case BlendFactor::ConstantAlpha:      return GL_CONSTANT_ALPHA;
            case BlendFactor::OneMinusConstantAlpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
            case BlendFactor::SrcAlphaSaturate:    return GL_SRC_ALPHA_SATURATE;
            default:                               return GL_ONE;
        }
    }

    uint32_t OpenGLPipelineState::GetGLBlendEquation(BlendOp op)
    {
        switch (op)
        {
            case BlendOp::Add:             return GL_FUNC_ADD;
            case BlendOp::Subtract:        return GL_FUNC_SUBTRACT;
            case BlendOp::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
            case BlendOp::Min:             return GL_MIN;
            case BlendOp::Max:             return GL_MAX;
            default:                       return GL_FUNC_ADD;
        }
    }

    uint32_t OpenGLPipelineState::GetGLCompareFunc(CompareFunc func)
    {
        switch (func)
        {
            case CompareFunc::Never:        return GL_NEVER;
            case CompareFunc::Less:         return GL_LESS;
            case CompareFunc::Equal:        return GL_EQUAL;
            case CompareFunc::LessEqual:    return GL_LEQUAL;
            case CompareFunc::Greater:     return GL_GREATER;
            case CompareFunc::NotEqual:    return GL_NOTEQUAL;
            case CompareFunc::GreaterEqual: return GL_GEQUAL;
            case CompareFunc::Always:      return GL_ALWAYS;
            default:                       return GL_LESS;
        }
    }

    uint32_t OpenGLPipelineState::GetGLStencilOp(StencilOp op)
    {
        switch (op)
        {
            case StencilOp::Keep:           return GL_KEEP;
            case StencilOp::Zero:           return GL_ZERO;
            case StencilOp::Replace:        return GL_REPLACE;
            case StencilOp::IncrementClamp: return GL_INCR;
            case StencilOp::DecrementClamp: return GL_DECR;
            case StencilOp::IncrementWrap:  return GL_INCR_WRAP;
            case StencilOp::DecrementWrap:  return GL_DECR_WRAP;
            case StencilOp::Invert:         return GL_INVERT;
            default:                        return GL_KEEP;
        }
    }

    uint32_t OpenGLPipelineState::GetGLAttributeType(VertexAttributeType type)
    {
        switch (type)
        {
            case VertexAttributeType::Float:
            case VertexAttributeType::Float2:
            case VertexAttributeType::Float3:
            case VertexAttributeType::Float4:
                return GL_FLOAT;

            case VertexAttributeType::Int:
            case VertexAttributeType::Int2:
            case VertexAttributeType::Int3:
            case VertexAttributeType::Int4:
                return GL_INT;

            case VertexAttributeType::UInt:
            case VertexAttributeType::UInt2:
            case VertexAttributeType::UInt3:
            case VertexAttributeType::UInt4:
                return GL_UNSIGNED_INT;

            case VertexAttributeType::Byte:
            case VertexAttributeType::Byte2:
            case VertexAttributeType::Byte4:
                return GL_BYTE;

            case VertexAttributeType::UByte:
            case VertexAttributeType::UByte2:
            case VertexAttributeType::UByte4:
                return GL_UNSIGNED_BYTE;

            case VertexAttributeType::Short:
            case VertexAttributeType::Short2:
            case VertexAttributeType::Short4:
                return GL_SHORT;

            case VertexAttributeType::UShort:
            case VertexAttributeType::UShort2:
            case VertexAttributeType::UShort4:
                return GL_UNSIGNED_SHORT;

            default:
                return GL_FLOAT;
        }
    }

} // namespace Sabora
