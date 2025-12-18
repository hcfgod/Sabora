#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <typeindex>
#include <unordered_map>
#include <cstdint>

// Forward declaration of SDL types
union SDL_Event;

namespace Sabora
{
    /**
     * @brief Base class for all application events.
     * 
     * Events are used to communicate between systems in the engine.
     * All events should inherit from this base class.
     */
    class Event
    {
    public:
        virtual ~Event() = default;

        /**
         * @brief Check if the event has been handled.
         * @return True if the event has been marked as handled.
         */
        [[nodiscard]] bool IsHandled() const noexcept { return m_Handled; }

        /**
         * @brief Mark the event as handled.
         */
        void MarkHandled() noexcept { m_Handled = true; }

    protected:
        Event() = default;

    private:
        bool m_Handled = false;
    };

    /**
     * @brief Window close event - fired when the window is requested to close.
     */
    class WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent() = default;
    };

    /**
     * @brief Window resize event - fired when the window is resized.
     */
    class WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(int32_t width, int32_t height)
            : m_Width(width)
            , m_Height(height)
        {
        }

        [[nodiscard]] int32_t GetWidth() const noexcept { return m_Width; }
        [[nodiscard]] int32_t GetHeight() const noexcept { return m_Height; }

    private:
        int32_t m_Width = 0;
        int32_t m_Height = 0;
    };

    /**
     * @brief Key event - fired when a keyboard key is pressed or released.
     */
    class KeyEvent : public Event
    {
    public:
        KeyEvent(int32_t key, bool pressed, bool repeat)
            : m_Key(key)
            , m_Pressed(pressed)
            , m_Repeat(repeat)
        {
        }

        [[nodiscard]] int32_t GetKey() const noexcept { return m_Key; }
        [[nodiscard]] bool IsPressed() const noexcept { return m_Pressed; }
        [[nodiscard]] bool IsRepeat() const noexcept { return m_Repeat; }

    private:
        int32_t m_Key = 0;
        bool m_Pressed = false;
        bool m_Repeat = false;
    };

    /**
     * @brief Mouse button event - fired when a mouse button is pressed or released.
     */
    class MouseButtonEvent : public Event
    {
    public:
        MouseButtonEvent(uint8_t button, bool pressed, float x, float y)
            : m_Button(button)
            , m_Pressed(pressed)
            , m_X(x)
            , m_Y(y)
        {
        }

        [[nodiscard]] uint8_t GetButton() const noexcept { return m_Button; }
        [[nodiscard]] bool IsPressed() const noexcept { return m_Pressed; }
        [[nodiscard]] float GetX() const noexcept { return m_X; }
        [[nodiscard]] float GetY() const noexcept { return m_Y; }

    private:
        uint8_t m_Button = 0;
        bool m_Pressed = false;
        float m_X = 0.0f;
        float m_Y = 0.0f;
    };

    /**
     * @brief Mouse move event - fired when the mouse moves.
     */
    class MouseMoveEvent : public Event
    {
    public:
        MouseMoveEvent(float x, float y, float deltaX, float deltaY)
            : m_X(x)
            , m_Y(y)
            , m_DeltaX(deltaX)
            , m_DeltaY(deltaY)
        {
        }

        [[nodiscard]] float GetX() const noexcept { return m_X; }
        [[nodiscard]] float GetY() const noexcept { return m_Y; }
        [[nodiscard]] float GetDeltaX() const noexcept { return m_DeltaX; }
        [[nodiscard]] float GetDeltaY() const noexcept { return m_DeltaY; }

    private:
        float m_X = 0.0f;
        float m_Y = 0.0f;
        float m_DeltaX = 0.0f;
        float m_DeltaY = 0.0f;
    };

    /**
     * @brief Event dispatcher for handling and routing events.
     * 
     * EventDispatcher provides a centralized system for event handling.
     * Events can be dispatched and listeners can subscribe to specific event types.
     * 
     * Usage:
     * @code
     *   EventDispatcher dispatcher;
     *   
     *   // Subscribe to window close events
     *   dispatcher.Subscribe<WindowCloseEvent>([](const WindowCloseEvent& e) {
     *       // Handle window close
     *   });
     *   
     *   // Dispatch an event
     *   WindowCloseEvent closeEvent;
     *   dispatcher.Dispatch(closeEvent);
     * @endcode
     */
    class EventDispatcher
    {
    public:
        /**
         * @brief Subscribe to events of type T.
         * @param callback Function to call when the event is dispatched.
         * @return Subscription ID (can be used to unsubscribe).
         */
        template<typename T>
        [[nodiscard]] size_t Subscribe(std::function<void(const T&)> callback)
        {
            static_assert(std::is_base_of_v<Event, T>, "T must inherit from Event");
            
            auto typeIndex = std::type_index(typeid(T));
            size_t id = m_NextSubscriptionId++;
            
            m_Subscriptions[typeIndex].push_back({
                id,
                [callback](const Event& e) {
                    callback(static_cast<const T&>(e));
                }
            });
            
            return id;
        }

        /**
         * @brief Unsubscribe from events using a subscription ID.
         * @param typeIndex Type index of the event type.
         * @param subscriptionId ID returned from Subscribe().
         */
        void Unsubscribe(std::type_index typeIndex, size_t subscriptionId)
        {
            auto it = m_Subscriptions.find(typeIndex);
            if (it != m_Subscriptions.end())
            {
                auto& callbacks = it->second;
                callbacks.erase(
                    std::remove_if(
                        callbacks.begin(),
                        callbacks.end(),
                        [subscriptionId](const auto& sub) {
                            return sub.id == subscriptionId;
                        }
                    ),
                    callbacks.end()
                );
            }
        }

        /**
         * @brief Unsubscribe from events of type T using a subscription ID.
         * @param subscriptionId ID returned from Subscribe().
         */
        template<typename T>
        void Unsubscribe(size_t subscriptionId)
        {
            static_assert(std::is_base_of_v<Event, T>, "T must inherit from Event");
            Unsubscribe(std::type_index(typeid(T)), subscriptionId);
        }

        /**
         * @brief Dispatch an event to all subscribed listeners.
         * @param event The event to dispatch.
         * @return True if the event was handled by at least one listener.
         */
        bool Dispatch(Event& event)
        {
            auto typeIndex = std::type_index(typeid(event));
            auto it = m_Subscriptions.find(typeIndex);
            
            if (it != m_Subscriptions.end())
            {
                for (const auto& subscription : it->second)
                {
                    subscription.callback(event);
                    if (event.IsHandled())
                    {
                        return true;
                    }
                }
            }
            
            return false;
        }

        /**
         * @brief Process all pending SDL events and dispatch them.
         * 
         * This method polls SDL events and converts them to engine events,
         * then dispatches them to registered listeners.
         */
        void ProcessSDLEvents();

    private:
        /**
         * @brief Process a single SDL event and dispatch it.
         * @param sdlEvent The SDL event to process.
         */
        void ProcessSDLEvent(const SDL_Event& sdlEvent);

        struct Subscription
        {
            size_t id;
            std::function<void(const Event&)> callback;
        };

        std::unordered_map<std::type_index, std::vector<Subscription>> m_Subscriptions;
        size_t m_NextSubscriptionId = 1;
    };

} // namespace Sabora