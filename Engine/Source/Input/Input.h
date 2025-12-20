#pragma once

#include "KeyCode.h"
#include <array>
#include <cstdint>
#include <mutex>
#include <atomic>

// We use int for scancode parameters in the header to avoid requiring SDL headers.
// Scancodes are just integer values, so using int is safe.
// The implementation file will cast to the actual SDL_Scancode enum when needed.
using Scancode = int;

namespace Sabora
{
    /**
     * @brief Mouse button enumeration.
     * 
     * Represents the different mouse buttons that can be pressed.
     * Values match SDL's SDL_BUTTON_* constants (1-5).
     */
    enum class MouseButton : uint8_t
    {
        Left = 1,    // matches SDL_BUTTON_LEFT
        Middle = 2,  // matches SDL_BUTTON_MIDDLE
        Right = 3,   // matches SDL_BUTTON_RIGHT
        X1 = 4,      // matches SDL_BUTTON_X1
        X2 = 5       // matches SDL_BUTTON_X2
    };

    /**
     * @brief Input system for polling keyboard and mouse state.
     * 
     * Similar to Unity's new input system, this provides a centralized way
     * to query input state through polling. The engine automatically updates
     * the input state from SDL events, so applications can simply query the
     * current state at any time.
     * 
     * @note Thread-safe - all methods can be called from any thread safely.
     * 
     * Usage example:
     *   // Using KeyCode (recommended)
     *   if (Input::Get().IsKeyPressed(KeyCode::W))
     *   {
     *       // Move forward
     *   }
     *   
     *   // Using scancodes (layout-independent)
     *   if (Input::Get().IsKeyPressed(SDL_SCANCODE_W))
     *   {
     *       // Move forward (works regardless of keyboard layout)
     *   }
     *   
     *   if (Input::Get().IsMouseButtonDown(MouseButton::Left))
     *   {
     *       // Handle mouse click
     *   }
     *   
     *   auto mousePos = Input::Get().GetMousePosition();
     *   auto mouseDelta = Input::Get().GetMouseDelta();
     */
    class Input
    {
    public:
        /**
         * @brief Get the singleton instance of Input.
         * @return Reference to the Input instance.
         */
        [[nodiscard]] static Input& Get()
        {
            static Input instance;
            return instance;
        }

        /**
         * @brief Check if a key is currently pressed (held down).
         * @param scancode The SDL scancode to check.
         * @return True if the key is currently pressed.
         * 
         * @note This uses scancodes which are layout-independent.
         *       For example, SDL_SCANCODE_W will always be the W key
         *       regardless of keyboard layout.
         */
        [[nodiscard]] bool IsKeyPressed(Scancode scancode) const noexcept;

        /**
         * @brief Check if a key was just pressed this frame (was up, now down).
         * @param scancode The scancode to check.
         * @return True if the key was just pressed this frame.
         */
        [[nodiscard]] bool IsKeyDown(Scancode scancode) const noexcept;

        /**
         * @brief Check if a key was just released this frame (was down, now up).
         * @param scancode The scancode to check.
         * @return True if the key was just released this frame.
         */
        [[nodiscard]] bool IsKeyUp(Scancode scancode) const noexcept;

        /**
         * @brief Check if a key is currently pressed using a KeyCode.
         * @param keyCode The KeyCode to check.
         * @return True if the key is currently pressed.
         * 
         * @note This is the preferred way to check keys. KeyCode provides
         *       a platform-independent API that abstracts away SDL dependencies.
         * 
         * Usage:
         *   if (Input::Get().IsKeyPressed(KeyCode::W))
         *   {
         *       // Move forward
         *   }
         */
        [[nodiscard]] bool IsKeyPressed(KeyCode keyCode) const noexcept;

        /**
         * @brief Check if a key was just pressed this frame using a KeyCode.
         * @param keyCode The KeyCode to check.
         * @return True if the key was just pressed this frame.
         */
        [[nodiscard]] bool IsKeyDown(KeyCode keyCode) const noexcept;

        /**
         * @brief Check if a key was just released this frame using a KeyCode.
         * @param keyCode The KeyCode to check.
         * @return True if the key was just released this frame.
         */
        [[nodiscard]] bool IsKeyUp(KeyCode keyCode) const noexcept;

        /**
         * @brief Check if a key is currently pressed using an SDL keycode.
         * @param keycode The SDL keycode to check.
         * @return True if the key is currently pressed.
         * 
         * @deprecated Prefer using KeyCode overloads instead.
         * @note Keycodes are layout-dependent. SDLK_W on a QWERTY keyboard
         *       might be different on an AZERTY keyboard.
         */
        [[nodiscard]] bool IsKeyPressed(SDL_Keycode keycode) const noexcept;

        /**
         * @brief Check if a key was just pressed this frame using an SDL keycode.
         * @param keycode The SDL keycode to check.
         * @return True if the key was just pressed this frame.
         * 
         * @deprecated Prefer using KeyCode overloads instead.
         */
        [[nodiscard]] bool IsKeyDown(SDL_Keycode keycode) const noexcept;

        /**
         * @brief Check if a key was just released this frame using an SDL keycode.
         * @param keycode The SDL keycode to check.
         * @return True if the key was just released this frame.
         * 
         * @deprecated Prefer using KeyCode overloads instead.
         */
        [[nodiscard]] bool IsKeyUp(SDL_Keycode keycode) const noexcept;

        /**
         * @brief Check if a mouse button is currently pressed (held down).
         * @param button The mouse button to check.
         * @return True if the button is currently pressed.
         */
        [[nodiscard]] bool IsMouseButtonPressed(MouseButton button) const noexcept;

        /**
         * @brief Check if a mouse button was just pressed this frame.
         * @param button The mouse button to check.
         * @return True if the button was just pressed this frame.
         */
        [[nodiscard]] bool IsMouseButtonDown(MouseButton button) const noexcept;

        /**
         * @brief Check if a mouse button was just released this frame.
         * @param button The mouse button to check.
         * @return True if the button was just released this frame.
         */
        [[nodiscard]] bool IsMouseButtonUp(MouseButton button) const noexcept;

        /**
         * @brief Get the current mouse position in window coordinates.
         * @return Pair of (x, y) coordinates.
         */
        [[nodiscard]] std::pair<float, float> GetMousePosition() const noexcept;

        /**
         * @brief Get the mouse position delta (movement) since last frame.
         * @return Pair of (deltaX, deltaY) values.
         */
        [[nodiscard]] std::pair<float, float> GetMouseDelta() const noexcept;

        /**
         * @brief Get the mouse scroll wheel delta since last frame.
         * @return Pair of (scrollX, scrollY) values.
         */
        [[nodiscard]] std::pair<float, float> GetMouseScrollDelta() const noexcept;

        /**
         * @brief Check if the mouse is currently locked (confined to window).
         * @return True if the mouse is locked.
         */
        [[nodiscard]] bool IsMouseLocked() const noexcept;

        /**
         * @brief Update keyboard state from SDL keyboard state.
         * 
         * This should be called once per frame to update the current
         * keyboard state. The engine handles this automatically.
         */
        void UpdateKeyboardState();

        /**
         * @brief Update frame-specific input state (key down/up, button down/up).
         * 
         * This should be called at the beginning of each frame to reset
         * frame-specific state. The engine handles this automatically.
         */
        void BeginFrame();

        /**
         * @brief Update keyboard state when a key is pressed.
         * @param scancode The scancode of the key that was pressed.
         * @param isRepeat True if this is a key repeat event, false for initial press.
         * 
         * This is called automatically by the engine from SDL events.
         * Repeat events update the held state but don't trigger IsKeyDown.
         */
        void OnKeyPressed(Scancode scancode, bool isRepeat = false);

        /**
         * @brief Update keyboard state when a key is released.
         * @param scancode The scancode of the key that was released.
         * 
         * This is called automatically by the engine from SDL events.
         */
        void OnKeyReleased(Scancode scancode);

        /**
         * @brief Update mouse button state when a button is pressed.
         * @param button The mouse button that was pressed.
         * 
         * This is called automatically by the engine from SDL events.
         */
        void OnMouseButtonPressed(MouseButton button);

        /**
         * @brief Update mouse button state when a button is released.
         * @param button The mouse button that was released.
         * 
         * This is called automatically by the engine from SDL events.
         */
        void OnMouseButtonReleased(MouseButton button);

        /**
         * @brief Update mouse position.
         * @param x The new X coordinate.
         * @param y The new Y coordinate.
         * @param deltaX The change in X since last frame.
         * @param deltaY The change in Y since last frame.
         * 
         * This is called automatically by the engine from SDL events.
         */
        void OnMouseMoved(float x, float y, float deltaX, float deltaY);

        /**
         * @brief Update mouse scroll wheel state.
         * @param scrollX The horizontal scroll amount.
         * @param scrollY The vertical scroll amount.
         * 
         * This is called automatically by the engine from SDL events.
         */
        void OnMouseScrolled(float scrollX, float scrollY);

    private:
        Input() = default;
        ~Input() = default;

        // Non-copyable, non-movable
        Input(const Input&) = delete;
        Input& operator=(const Input&) = delete;
        Input(Input&&) = delete;
        Input& operator=(Input&&) = delete;

        // Thread safety: Input state is accessed from multiple threads
        // - Main thread: Reads state, updates from events
        // - Other threads: May read state (e.g., for game logic)
        // We use a mutex to protect all state access
        mutable std::mutex m_StateMutex;

        // Keyboard state
        // We use a large array to track all possible scancodes
        // SDL_SCANCODE_COUNT is typically 512, but we use a safe constant
        // that will be validated in Input.cpp
        static constexpr size_t MAX_SCANCODES = 512;
        std::array<bool, MAX_SCANCODES> m_KeyStates{};
        std::array<bool, MAX_SCANCODES> m_KeyDownThisFrame{};
        std::array<bool, MAX_SCANCODES> m_KeyUpThisFrame{};

        // Mouse state
        float m_MouseX = 0.0f;
        float m_MouseY = 0.0f;
        float m_MouseDeltaX = 0.0f;
        float m_MouseDeltaY = 0.0f;
        float m_ScrollDeltaX = 0.0f;
        float m_ScrollDeltaY = 0.0f;

        // Mouse button state (using uint8_t to match SDL button values)
        static constexpr size_t MAX_MOUSE_BUTTONS = 5;
        std::array<bool, MAX_MOUSE_BUTTONS> m_MouseButtonStates{};
        std::array<bool, MAX_MOUSE_BUTTONS> m_MouseButtonDownThisFrame{};
        std::array<bool, MAX_MOUSE_BUTTONS> m_MouseButtonUpThisFrame{};
    };

} // namespace Sabora
