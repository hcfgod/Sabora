#include "pch.h"
#include "Input.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_scancode.h>

namespace Sabora
{
    bool Input::IsKeyPressed(Scancode scancode) const noexcept
    {
        if (scancode < 0 || static_cast<size_t>(scancode) >= MAX_SCANCODES)
        {
            return false;
        }
        return m_KeyStates[scancode];
    }

    bool Input::IsKeyDown(Scancode scancode) const noexcept
    {
        if (scancode < 0 || static_cast<size_t>(scancode) >= MAX_SCANCODES)
        {
            return false;
        }
        return m_KeyDownThisFrame[scancode];
    }

    bool Input::IsKeyUp(Scancode scancode) const noexcept
    {
        if (scancode < 0 || static_cast<size_t>(scancode) >= MAX_SCANCODES)
        {
            return false;
        }
        return m_KeyUpThisFrame[scancode];
    }

    bool Input::IsKeyPressed(SDL_Keycode keycode) const noexcept
    {
        // Convert keycode to scancode
        SDL_Scancode scancode = SDL_GetScancodeFromKey(keycode, nullptr);
        return IsKeyPressed(scancode);
    }

    bool Input::IsKeyDown(SDL_Keycode keycode) const noexcept
    {
        // Convert keycode to scancode
        SDL_Scancode scancode = SDL_GetScancodeFromKey(keycode, nullptr);
        return IsKeyDown(scancode);
    }

    bool Input::IsKeyUp(SDL_Keycode keycode) const noexcept
    {
        // Convert keycode to scancode
        SDL_Scancode scancode = SDL_GetScancodeFromKey(keycode, nullptr);
        return IsKeyUp(scancode);
    }

    bool Input::IsKeyPressed(KeyCode keyCode) const noexcept
    {
        // Convert KeyCode to SDL_Keycode, then to scancode
        SDL_Keycode sdlKeycode = KeyCodeToSDL(keyCode);
        SDL_Scancode scancode = SDL_GetScancodeFromKey(sdlKeycode, nullptr);
        return IsKeyPressed(scancode);
    }

    bool Input::IsKeyDown(KeyCode keyCode) const noexcept
    {
        // Convert KeyCode to SDL_Keycode, then to scancode
        SDL_Keycode sdlKeycode = KeyCodeToSDL(keyCode);
        SDL_Scancode scancode = SDL_GetScancodeFromKey(sdlKeycode, nullptr);
        return IsKeyDown(scancode);
    }

    bool Input::IsKeyUp(KeyCode keyCode) const noexcept
    {
        // Convert KeyCode to SDL_Keycode, then to scancode
        SDL_Keycode sdlKeycode = KeyCodeToSDL(keyCode);
        SDL_Scancode scancode = SDL_GetScancodeFromKey(sdlKeycode, nullptr);
        return IsKeyUp(scancode);
    }

    bool Input::IsMouseButtonPressed(MouseButton button) const noexcept
    {
        uint8_t buttonIndex = static_cast<uint8_t>(button) - 1; // SDL buttons are 1-indexed
        if (buttonIndex >= MAX_MOUSE_BUTTONS)
        {
            return false;
        }
        return m_MouseButtonStates[buttonIndex];
    }

    bool Input::IsMouseButtonDown(MouseButton button) const noexcept
    {
        uint8_t buttonIndex = static_cast<uint8_t>(button) - 1; // SDL buttons are 1-indexed
        if (buttonIndex >= MAX_MOUSE_BUTTONS)
        {
            return false;
        }
        return m_MouseButtonDownThisFrame[buttonIndex];
    }

    bool Input::IsMouseButtonUp(MouseButton button) const noexcept
    {
        uint8_t buttonIndex = static_cast<uint8_t>(button) - 1; // SDL buttons are 1-indexed
        if (buttonIndex >= MAX_MOUSE_BUTTONS)
        {
            return false;
        }
        return m_MouseButtonUpThisFrame[buttonIndex];
    }

    std::pair<float, float> Input::GetMousePosition() const noexcept
    {
        return { m_MouseX, m_MouseY };
    }

    std::pair<float, float> Input::GetMouseDelta() const noexcept
    {
        return { m_MouseDeltaX, m_MouseDeltaY };
    }

    std::pair<float, float> Input::GetMouseScrollDelta() const noexcept
    {
        return { m_ScrollDeltaX, m_ScrollDeltaY };
    }

    bool Input::IsMouseLocked() const noexcept
    {
        // SDL3 requires a window pointer to check relative mouse mode
        // Get the window with keyboard focus (which should be our main window)
        SDL_Window* window = SDL_GetKeyboardFocus();
        if (window != nullptr)
        {
            return SDL_GetWindowRelativeMouseMode(window);
        }
        return false;
    }

    void Input::UpdateKeyboardState()
    {
        // Get the current keyboard state from SDL
        // Note: SDL_PollEvent() automatically calls SDL_PumpEvents(), so the state should be up to date
        // This is a backup to ensure we're in sync with SDL's state, but event-based tracking is primary
        // 
        // IMPORTANT: This only updates m_KeyStates (for IsKeyPressed), not frame-specific arrays.
        // Frame-specific state (IsKeyDown/IsKeyUp) is only set by events to ensure accuracy.
        int numKeys = 0;
        const bool* keyboardState = SDL_GetKeyboardState(&numKeys);
        
        if (keyboardState != nullptr && numKeys > 0)
        {
            // Update our key states to match SDL's current state
            // This ensures we're in sync even if we miss events
            // Only update up to the minimum of numKeys and MAX_SCANCODES
            const int maxKeys = (numKeys < static_cast<int>(MAX_SCANCODES)) ? numKeys : static_cast<int>(MAX_SCANCODES);
            for (int i = 0; i < maxKeys; ++i)
            {
                // Only update if the state differs - this prevents unnecessary writes
                // This syncs the held state but doesn't interfere with frame-specific state
                if (m_KeyStates[i] != keyboardState[i])
                {
                    m_KeyStates[i] = keyboardState[i];
                }
            }
        }
    }

    void Input::BeginFrame()
    {
        // Reset frame-specific state
        m_KeyDownThisFrame.fill(false);
        m_KeyUpThisFrame.fill(false);
        m_MouseButtonDownThisFrame.fill(false);
        m_MouseButtonUpThisFrame.fill(false);
        
        // Reset mouse delta and scroll delta (they accumulate during the frame)
        m_MouseDeltaX = 0.0f;
        m_MouseDeltaY = 0.0f;
        m_ScrollDeltaX = 0.0f;
        m_ScrollDeltaY = 0.0f;
    }

    void Input::OnKeyPressed(Scancode scancode, bool isRepeat)
    {
        if (scancode < 0 || static_cast<size_t>(scancode) >= MAX_SCANCODES)
        {
            return;
        }

        // Check if this is a new press (key wasn't already pressed)
        bool wasAlreadyPressed = m_KeyStates[scancode];

        // If the key was already pressed, clear any stale "up" flag
        // This handles the case where a key might have been marked as up but is still pressed
        if (wasAlreadyPressed && m_KeyUpThisFrame[scancode])
        {
            m_KeyUpThisFrame[scancode] = false;
        }

        // Update the held state (for IsKeyPressed) - always update this
        m_KeyStates[scancode] = true;

        // Mark as down this frame ONLY if:
        // 1. It's not a repeat event (initial press)
        // 2. The key wasn't already pressed (new press, not a continuation)
        // 3. We haven't already marked it this frame (safety check)
        if (!isRepeat && !wasAlreadyPressed && !m_KeyDownThisFrame[scancode])
        {
            m_KeyDownThisFrame[scancode] = true;
        }
    }

    void Input::OnKeyReleased(Scancode scancode)
    {
        if (scancode < 0 || static_cast<size_t>(scancode) >= MAX_SCANCODES)
        {
            return;
        }

        // Check if the key was actually pressed before marking it as released
        // This ensures IsKeyUp only triggers when a key that was pressed is released
        bool wasPressed = m_KeyStates[scancode];

        // If the key wasn't pressed, clear any stale "down" flag
        // This handles edge cases where state might be inconsistent
        if (!wasPressed && m_KeyDownThisFrame[scancode])
        {
            m_KeyDownThisFrame[scancode] = false;
        }

        // Update the held state (for IsKeyPressed) - always update this
        m_KeyStates[scancode] = false;

        // Mark as up this frame ONLY if:
        // 1. The key was previously pressed (actual release, not spurious event)
        // 2. We haven't already marked it this frame (safety check)
        if (wasPressed && !m_KeyUpThisFrame[scancode])
        {
            m_KeyUpThisFrame[scancode] = true;
        }
    }

    void Input::OnMouseButtonPressed(MouseButton button)
    {
        uint8_t buttonIndex = static_cast<uint8_t>(button) - 1; // SDL buttons are 1-indexed
        if (buttonIndex >= MAX_MOUSE_BUTTONS)
        {
            return;
        }

        // Only mark as down this frame if it wasn't already pressed
        if (!m_MouseButtonStates[buttonIndex])
        {
            m_MouseButtonDownThisFrame[buttonIndex] = true;
        }

        m_MouseButtonStates[buttonIndex] = true;
    }

    void Input::OnMouseButtonReleased(MouseButton button)
    {
        uint8_t buttonIndex = static_cast<uint8_t>(button) - 1; // SDL buttons are 1-indexed
        if (buttonIndex >= MAX_MOUSE_BUTTONS)
        {
            return;
        }

        // Only mark as up this frame if it was previously pressed
        if (m_MouseButtonStates[buttonIndex])
        {
            m_MouseButtonUpThisFrame[buttonIndex] = true;
        }

        m_MouseButtonStates[buttonIndex] = false;
    }

    void Input::OnMouseMoved(float x, float y, float deltaX, float deltaY)
    {
        m_MouseX = x;
        m_MouseY = y;
        
        // Accumulate deltas (in case multiple move events occur in one frame)
        m_MouseDeltaX += deltaX;
        m_MouseDeltaY += deltaY;
    }

    void Input::OnMouseScrolled(float scrollX, float scrollY)
    {
        // Accumulate scroll deltas (in case multiple scroll events occur in one frame)
        m_ScrollDeltaX += scrollX;
        m_ScrollDeltaY += scrollY;
    }

} // namespace Sabora
