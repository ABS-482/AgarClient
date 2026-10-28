#include "InputManager.h"

#include <SDL3/SDL.h>

bool InputManager::poll(InputState& state)
{
    m_mouseEventsThisFrame = 0;
    state.spawnRequestPressed = false;
    state.leftButtonJustPressed = false;
    state.leftButtonJustReleased = false;
    state.mouseWheel = 0.0f;
    state.cycleFpsLimitPressed = false;
    state.splitRequested = false;
    state.ejectMassRequested = false;
    state.menuUpPressed = false;
    state.menuDownPressed = false;
    state.menuConfirmPressed = false;
    state.menuTogglePressed = false;
    state.leftButtonDoubleClicked = false;
    state.fullscreenPressed = false;
    state.textInput.clear();
    state.textBackspacePressed = false;
    state.textEnterPressed = false;
    state.textEscapePressed = false;
    state.textSelectAllPressed = false;
    state.textCopyPressed = false;
    state.textCutPressed = false;
    state.textPastePressed = false;

    state.textDeletePressed = false;
    state.textLeftPressed = false;
    state.textRightPressed = false;
    state.textHomePressed = false;
    state.textEndPressed = false;
    state.textShiftHeld = false;

    for (auto& [action, stateAction] : state.actions)
    {
        stateAction.pressed = false;
        stateAction.released = false;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            return false;

        case SDL_EVENT_MOUSE_MOTION:
            state.mouseX = event.motion.x;
            state.mouseY = event.motion.y;
            state.mouseDeltaX += event.motion.xrel;
            state.mouseDeltaY += event.motion.yrel;
            m_mouseEventsThisFrame++;
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            state.mouseX = event.button.x;
            state.mouseY = event.button.y;

            if (event.button.button == SDL_BUTTON_LEFT)
            {
                state.leftButton = true;
                state.leftButtonJustPressed = true;
                state.leftButtonDoubleClicked =
                    (event.button.clicks == 2);
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            state.mouseX = event.button.x;
            state.mouseY = event.button.y;

            if (event.button.button == SDL_BUTTON_LEFT)
            {
                state.leftButton = false;
                state.leftButtonJustReleased = true;
            }
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            state.mouseWheel += event.wheel.y;
            break;

        case SDL_EVENT_KEY_DOWN:
            if (state.textInputActive)
            {
                const bool ctrl =
                    (event.key.mod & SDL_KMOD_CTRL) != 0;

                const bool shift =
                    (event.key.mod & SDL_KMOD_SHIFT) != 0;

                if (ctrl && !event.key.repeat &&
                    event.key.scancode == SDL_SCANCODE_A)
                {
                    state.textSelectAllPressed = true;
                    break;
                }
                if (ctrl && !event.key.repeat &&
                    event.key.scancode == SDL_SCANCODE_C)
                {
                    state.textCopyPressed = true;
                    break;
                }
                if (ctrl && !event.key.repeat &&
                    event.key.scancode == SDL_SCANCODE_X)
                {
                    state.textCutPressed = true;
                    break;
                }
                if (ctrl && !event.key.repeat &&
                    event.key.scancode == SDL_SCANCODE_V)
                {
                    state.textPastePressed = true;
                    break;
                }

                if (event.key.scancode == SDL_SCANCODE_BACKSPACE)
                {
                    state.textBackspacePressed = true;
                }
                else if (event.key.scancode == SDL_SCANCODE_DELETE)
                {
                    state.textDeletePressed = true;
                }
                else if (event.key.scancode == SDL_SCANCODE_LEFT)
                {
                    state.textLeftPressed = true;
                    state.textShiftHeld = shift;
                }
                else if (event.key.scancode == SDL_SCANCODE_RIGHT)
                {
                    state.textRightPressed = true;
                    state.textShiftHeld = shift;
                }
                else if (event.key.scancode == SDL_SCANCODE_HOME)
                {
                    state.textHomePressed = true;
                    state.textShiftHeld = shift;
                }
                else if (event.key.scancode == SDL_SCANCODE_END)
                {
                    state.textEndPressed = true;
                    state.textShiftHeld = shift;
                }
                else if (
                    !event.key.repeat &&
                    (
                        event.key.scancode == SDL_SCANCODE_RETURN ||
                        event.key.scancode == SDL_SCANCODE_KP_ENTER
                        ))
                {
                    state.textEnterPressed = true;
                }
                else if (
                    !event.key.repeat &&
                    event.key.scancode == SDL_SCANCODE_ESCAPE)
                {
                    state.textEscapePressed = true;
                }
                else if (
                    !event.key.repeat &&
                    event.key.scancode == SDL_SCANCODE_F11)
                {
                    state.fullscreenPressed = true;
                }

                // Пока поле активно, игровые клавиши не обрабатываем.
                break;
            }
            if (event.key.scancode == SDL_SCANCODE_F1 && !event.key.repeat)
            {
                state.cycleFpsLimitPressed = true;
            }
            else if (event.key.scancode == SDL_SCANCODE_F2 && !event.key.repeat)
            {
                state.spawnRequestPressed = true;
            }
            else if (event.key.scancode == SDL_SCANCODE_SPACE && !event.key.repeat)
            {
                state.splitRequested = true;

                state.action(GameAction::Split).pressed = true;
                state.action(GameAction::Split).held = true;
            }
            else if (event.key.scancode == SDL_SCANCODE_W && !event.key.repeat)
            {
                state.ejectMassRequested = true;
            }
            else if (event.key.scancode == SDL_SCANCODE_Q)
            {
                state.ejectMassKeyHeld = true;
            }
            else if (event.key.scancode == SDL_SCANCODE_UP && !event.key.repeat)
            {
                state.menuUpPressed = true;
            }
            else if (event.key.scancode == SDL_SCANCODE_DOWN && !event.key.repeat)
            {
                state.menuDownPressed = true;
            }
            else if (event.key.scancode == SDL_SCANCODE_RETURN && !event.key.repeat)
            {
                state.menuConfirmPressed = true;
            }
            else if (event.key.scancode == SDL_SCANCODE_ESCAPE && !event.key.repeat)
            {
                state.menuTogglePressed = true;
            }
            else if (event.key.scancode == SDL_SCANCODE_E)
            {
                state.action(GameAction::MacroE).held = true;
            }
            else if (event.key.scancode == SDL_SCANCODE_LSHIFT ||
                event.key.scancode == SDL_SCANCODE_RSHIFT)
            {
                state.splitPreviewHeld = true;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN &&
                !event.key.repeat &&
                event.key.scancode == SDL_SCANCODE_F11)
            {
                state.fullscreenPressed = true;
            }
            break;


        case SDL_EVENT_KEY_UP:
            if (event.key.scancode == SDL_SCANCODE_Q)
            {
                state.ejectMassKeyHeld = false;
            }
            if (event.key.scancode == SDL_SCANCODE_SPACE)
            {
                state.action(GameAction::Split).released = true;
                state.action(GameAction::Split).held = false;
            }
            if (event.key.scancode == SDL_SCANCODE_E)
            {
                state.action(GameAction::MacroE).released = true;
                state.action(GameAction::MacroE).held = false;
            }
            if (event.key.scancode == SDL_SCANCODE_LSHIFT ||
                event.key.scancode == SDL_SCANCODE_RSHIFT)
            {
                state.splitPreviewHeld = false;
            }
            break;

        case SDL_EVENT_TEXT_INPUT:
            if (state.textInputActive && event.text.text)
            {
                state.textInput += event.text.text;
            }
            break;

        default:
            break;
        }
    }

    return true;
}