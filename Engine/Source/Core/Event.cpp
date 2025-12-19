#include "pch.h"
#include "Event.h"
#include "Input/Input.h"
#include <SDL3/SDL.h>

namespace Sabora
{
    void EventDispatcher::ProcessSDLEvents()
    {
        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent))
        {
            ProcessSDLEvent(sdlEvent);
        }
    }

    void EventDispatcher::ProcessSDLEvent(const SDL_Event& sdlEvent)
    {
        // Update Input system from SDL events
        Input& input = Input::Get();

        switch (sdlEvent.type)
        {
            case SDL_EVENT_QUIT:
            {
                WindowCloseEvent event;
                Dispatch(event);

                break;
            }
            
            case SDL_EVENT_WINDOW_RESIZED:
            {
                WindowResizeEvent event
                (
                    static_cast<int32_t>(sdlEvent.window.data1),
                    static_cast<int32_t>(sdlEvent.window.data2)
                );
                Dispatch(event);

                break;
            }
            
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
            {
                // Update Input system
                // Only process non-repeat events for frame-specific state (IsKeyDown/IsKeyUp)
                // Repeat events should only update held state (IsKeyPressed)
                if (sdlEvent.type == SDL_EVENT_KEY_DOWN)
                {
                    // Check if this is a repeat event
                    bool isRepeat = sdlEvent.key.repeat != 0;
                    // Cast SDL_Scancode to our Scancode type (both are int-compatible)
                    input.OnKeyPressed(static_cast<Scancode>(sdlEvent.key.scancode), isRepeat);
                }
                else
                {
                    input.OnKeyReleased(static_cast<Scancode>(sdlEvent.key.scancode));
                }

                // Dispatch event for event-based systems
                KeyEvent event
                (
                    sdlEvent.key.key,
                    sdlEvent.type == SDL_EVENT_KEY_DOWN,
                    sdlEvent.key.repeat != 0
                );
                Dispatch(event);

                break;
            }
            
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                // Update Input system
                MouseButton button = static_cast<MouseButton>(sdlEvent.button.button);
                if (sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                {
                    input.OnMouseButtonPressed(button);
                }
                else
                {
                    input.OnMouseButtonReleased(button);
                }

                // Dispatch event for event-based systems
                MouseButtonEvent event
                (
                    sdlEvent.button.button,
                    sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN,
                    sdlEvent.button.x,
                    sdlEvent.button.y
                );
                Dispatch(event);

                break;
            }
            
            case SDL_EVENT_MOUSE_MOTION:
            {
                // Update Input system
                input.OnMouseMoved(
                    sdlEvent.motion.x,
                    sdlEvent.motion.y,
                    sdlEvent.motion.xrel,
                    sdlEvent.motion.yrel
                );

                // Dispatch event for event-based systems
                MouseMoveEvent event
                (
                    sdlEvent.motion.x,
                    sdlEvent.motion.y,
                    sdlEvent.motion.xrel,
                    sdlEvent.motion.yrel
                );

                Dispatch(event);
                break;
            }

            case SDL_EVENT_MOUSE_WHEEL:
            {
                // Update Input system
                input.OnMouseScrolled(
                    sdlEvent.wheel.x,
                    sdlEvent.wheel.y
                );

                // Note: We don't have a MouseWheelEvent in the event system yet,
                // but the Input system handles it for polling
                break;
            }
            
            default:
                // Unhandled SDL event type
                break;
        }
    }

} // namespace Sabora