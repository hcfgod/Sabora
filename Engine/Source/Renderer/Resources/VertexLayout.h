#pragma once

#include "Renderer/Core/RendererTypes.h"
#include <vector>
#include <cstdint>

namespace Sabora
{
    /**
     * @brief Vertex attribute data types.
     */
    enum class VertexAttributeType : uint8_t
    {
        Float = 0,
        Float2,
        Float3,
        Float4,
        Int,
        Int2,
        Int3,
        Int4,
        UInt,
        UInt2,
        UInt3,
        UInt4,
        Byte,
        Byte2,
        Byte4,
        UByte,
        UByte2,
        UByte4,
        Short,
        Short2,
        Short4,
        UShort,
        UShort2,
        UShort4,
    };

    /**
     * @brief Get the size in bytes of a vertex attribute type.
     * @param type The attribute type.
     * @return Size in bytes.
     */
    inline uint32_t GetVertexAttributeSize(VertexAttributeType type) noexcept
    {
        switch (type)
        {
            case VertexAttributeType::Float:    return 4;
            case VertexAttributeType::Float2:   return 8;
            case VertexAttributeType::Float3:   return 12;
            case VertexAttributeType::Float4:   return 16;
            case VertexAttributeType::Int:      return 4;
            case VertexAttributeType::Int2:    return 8;
            case VertexAttributeType::Int3:    return 12;
            case VertexAttributeType::Int4:    return 16;
            case VertexAttributeType::UInt:    return 4;
            case VertexAttributeType::UInt2:   return 8;
            case VertexAttributeType::UInt3:   return 12;
            case VertexAttributeType::UInt4:   return 16;
            case VertexAttributeType::Byte:   return 1;
            case VertexAttributeType::Byte2:   return 2;
            case VertexAttributeType::Byte4:   return 4;
            case VertexAttributeType::UByte:  return 1;
            case VertexAttributeType::UByte2:  return 2;
            case VertexAttributeType::UByte4: return 4;
            case VertexAttributeType::Short:   return 2;
            case VertexAttributeType::Short2:  return 4;
            case VertexAttributeType::Short4: return 8;
            case VertexAttributeType::UShort:  return 2;
            case VertexAttributeType::UShort2: return 4;
            case VertexAttributeType::UShort4: return 8;
            default:                            return 0;
        }
    }

    /**
     * @brief Vertex attribute description.
     */
    struct VertexAttribute
    {
        uint32_t location = 0;              ///< Attribute location (shader binding)
        VertexAttributeType type = VertexAttributeType::Float3; ///< Data type
        uint32_t offset = 0;               ///< Offset in bytes from start of vertex
        bool normalized = false;            ///< Whether integer values should be normalized
    };

    /**
     * @brief Vertex layout description.
     * 
     * Describes the layout of vertex data, including attribute locations,
     * types, and offsets. Used to create vertex input layouts for the
     * graphics pipeline.
     */
    class VertexLayout
    {
    public:
        /**
         * @brief Create an empty vertex layout.
         */
        VertexLayout() = default;

        /**
         * @brief Create a vertex layout with attributes.
         * @param attributes List of vertex attributes.
         * @param stride Vertex stride in bytes (0 for automatic calculation).
         */
        explicit VertexLayout(
            std::vector<VertexAttribute> attributes,
            uint32_t stride = 0
        )
            : m_Attributes(std::move(attributes))
            , m_Stride(stride)
        {
            if (m_Stride == 0)
            {
                CalculateStride();
            }
        }

        /**
         * @brief Add a vertex attribute.
         * @param location Attribute location (shader binding).
         * @param type Attribute data type.
         * @param offset Offset in bytes from start of vertex.
         * @param normalized Whether integer values should be normalized.
         * @return Reference to this layout for chaining.
         */
        VertexLayout& AddAttribute(
            uint32_t location,
            VertexAttributeType type,
            uint32_t offset = 0,
            bool normalized = false
        )
        {
            VertexAttribute attr;
            attr.location = location;
            attr.type = type;
            attr.offset = offset;
            attr.normalized = normalized;

            m_Attributes.push_back(attr);

            // Recalculate stride if it was auto-calculated
            if (m_Stride == 0 || m_Stride == m_AutoStride)
            {
                CalculateStride();
            }

            return *this;
        }

        /**
         * @brief Get all vertex attributes.
         * @return Reference to the attributes vector.
         */
        [[nodiscard]] const std::vector<VertexAttribute>& GetAttributes() const noexcept
        {
            return m_Attributes;
        }

        /**
         * @brief Get the vertex stride in bytes.
         * @return Stride in bytes.
         */
        [[nodiscard]] uint32_t GetStride() const noexcept
        {
            return m_Stride > 0 ? m_Stride : m_AutoStride;
        }

        /**
         * @brief Set the vertex stride manually.
         * @param stride Stride in bytes.
         */
        void SetStride(uint32_t stride) noexcept
        {
            m_Stride = stride;
        }

        /**
         * @brief Check if the layout is empty.
         * @return True if there are no attributes.
         */
        [[nodiscard]] bool IsEmpty() const noexcept
        {
            return m_Attributes.empty();
        }

    private:
        /**
         * @brief Calculate stride automatically from attributes.
         */
        void CalculateStride()
        {
            if (m_Attributes.empty())
            {
                m_AutoStride = 0;
                return;
            }

            // Find the maximum offset + size
            uint32_t maxEnd = 0;
            for (const auto& attr : m_Attributes)
            {
                uint32_t attrSize = GetVertexAttributeSize(attr.type);
                uint32_t attrEnd = attr.offset + attrSize;
                if (attrEnd > maxEnd)
                {
                    maxEnd = attrEnd;
                }
            }

            m_AutoStride = maxEnd;
        }

        std::vector<VertexAttribute> m_Attributes;
        uint32_t m_Stride = 0;          ///< Manual stride (0 = auto-calculate)
        uint32_t m_AutoStride = 0;      ///< Auto-calculated stride
    };

} // namespace Sabora
