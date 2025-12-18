#pragma once

#include "Event.h"
#include <functional>
#include <typeindex>
#include <utility>
#include <vector>

namespace Sabora
{
    /**
     * @brief Singleton event manager for easy event subscription and dispatch.
     * 
     * EventManager provides a global, easy-to-use interface for event handling.
     * It automatically manages subscriptions and provides convenient methods
     * for subscribing to and dispatching events.
     * 
     * Usage example:
     *   EventManager::Get().Subscribe<WindowCloseEvent>([](const WindowCloseEvent& e) {
     *       // Handle event
     *   });
     *   
     *   KeyEvent event(key, true, false);
     *   EventManager::Get().Dispatch(event);
     */
    class EventManager
    {
    public:
        /**
         * @brief Get the singleton instance of EventManager.
         * @return Reference to the EventManager instance.
         */
        [[nodiscard]] static EventManager& Get()
        {
            static EventManager instance;
            return instance;
        }

        /**
         * @brief Set the event dispatcher to use.
         * 
         * This should be called once during application initialization
         * with the Application's event dispatcher.
         * 
         * @param dispatcher Reference to the EventDispatcher to use.
         */
        void SetDispatcher(EventDispatcher& dispatcher)
        {
            m_Dispatcher = &dispatcher;
        }

        /**
         * @brief Get the underlying event dispatcher.
         * @return Pointer to the EventDispatcher, or nullptr if not set.
         */
        [[nodiscard]] EventDispatcher* GetDispatcher() const noexcept
        {
            return m_Dispatcher;
        }

        /**
         * @brief Subscribe to events of type T.
         * @param callback Function to call when the event is dispatched.
         * @return Subscription ID (can be used to unsubscribe later).
         */
        template<typename T>
        [[nodiscard]] size_t Subscribe(std::function<void(const T&)> callback)
        {
            if (m_Dispatcher == nullptr)
            {
                return 0;
            }
            return m_Dispatcher->Subscribe<T>(callback);
        }

        /**
         * @brief Unsubscribe from events of type T using a subscription ID.
         * @param subscriptionId ID returned from Subscribe().
         */
        template<typename T>
        void Unsubscribe(size_t subscriptionId)
        {
            if (m_Dispatcher != nullptr)
            {
                m_Dispatcher->Unsubscribe<T>(subscriptionId);
            }
        }

        /**
         * @brief Dispatch an event to all subscribed listeners.
         * @param event The event to dispatch.
         * @return True if the event was handled by at least one listener.
         */
        template<typename T>
        bool Dispatch(T& event)
        {
            if (m_Dispatcher == nullptr)
            {
                return false;
            }
            return m_Dispatcher->Dispatch(event);
        }

        /**
         * @brief Process all pending SDL events and dispatch them.
         * 
         * This method polls SDL events and converts them to engine events,
         * then dispatches them to registered listeners.
         */
        void ProcessSDLEvents()
        {
            if (m_Dispatcher != nullptr)
            {
                m_Dispatcher->ProcessSDLEvents();
            }
        }

    private:
        EventManager() = default;
        ~EventManager() = default;

        // Non-copyable, non-movable
        EventManager(const EventManager&) = delete;
        EventManager& operator=(const EventManager&) = delete;
        EventManager(EventManager&&) = delete;
        EventManager& operator=(EventManager&&) = delete;

        EventDispatcher* m_Dispatcher = nullptr;
    };

} // namespace Sabora