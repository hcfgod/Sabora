#pragma once

#include "Core/Event.h"
#include "Renderer/Core/RendererTypes.h"
#include "Core/Result.h"
#include <string>

namespace Sabora
{
    /**
     * @brief Event dispatched when the renderer is initialized.
     */
    class RendererInitializedEvent : public Event
    {
    public:
        explicit RendererInitializedEvent(RendererAPI api)
            : m_API(api)
        {
        }

        [[nodiscard]] RendererAPI GetAPI() const noexcept { return m_API; }

    private:
        RendererAPI m_API;
    };

    /**
     * @brief Event dispatched when a renderer error occurs.
     */
    class RendererErrorEvent : public Event
    {
    public:
        explicit RendererErrorEvent(Error error)
            : m_Error(std::move(error))
        {
        }

        [[nodiscard]] const Error& GetError() const noexcept { return m_Error; }

    private:
        Error m_Error;
    };

    /**
     * @brief Event dispatched when the renderer is about to shutdown.
     */
    class RendererShutdownEvent : public Event
    {
    public:
        RendererShutdownEvent() = default;
    };

} // namespace Sabora
