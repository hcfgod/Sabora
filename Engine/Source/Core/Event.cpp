#include "Event.h"
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
                WindowResizeEvent event(
                    static_cast<int32_t>(sdlEvent.window.data1),
                    static_cast<int32_t>(sdlEvent.window.data2)
                );
                Dispatch(event);
                break;
            }
            
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
            {
                KeyEvent event(
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
                MouseButtonEvent event(
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
                MouseMoveEvent event(
                    sdlEvent.motion.x,
                    sdlEvent.motion.y,
                    sdlEvent.motion.xrel,
                    sdlEvent.motion.yrel
                );
                Dispatch(event);
                break;
            }
            
            default:
                // Unhandled SDL event type
                break;
        }
    }

} // namespace Sabora
