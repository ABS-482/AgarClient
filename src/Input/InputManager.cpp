#include "InputManager.h"

#include <SDL3/SDL.h>

bool InputManager::poll(InputState& state)
{
    m_mouseEventsThisFrame = 0;
    state.spawnRequestPressed = false;
    state.leftButtonJustPressed = false;
    state.mouseWheel = 0.0f;
    state.cycleFpsLimitPressed = false;
    state.splitRequested = false;
    state.ejectMassRequested = false;
    state.menuUpPressed = false;
    state.menuDownPressed = false;
    state.menuConfirmPressed = false;
    state.menuTogglePressed = false;
    state.leftButtonDoubleClicked = false;

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
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                state.leftButton = true;
                state.leftButtonJustPressed = true;
                state.leftButtonDoubleClicked = (event.button.clicks == 2);
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                state.leftButton = false;
            }
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            state.mouseWheel += event.wheel.y;
            break;

        case SDL_EVENT_KEY_DOWN:
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
            break;

        case SDL_EVENT_KEY_UP:
            if (event.key.scancode == SDL_SCANCODE_Q)
            {
                state.ejectMassKeyHeld = false;
            }
            break;

        default:
            break;
        }
    }

    return true;
}