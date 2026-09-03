#include "InputManager.h"

#include <SDL3/SDL.h>

bool InputManager::poll(InputState& state)
{
    m_mouseEventsThisFrame = 0;
    state.leftButtonJustPressed = false;
    state.mouseWheel = 0.0f;

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

        default:
            break;
        }
    }

    return true;
}