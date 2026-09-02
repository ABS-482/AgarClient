#include "InputManager.h"

#include <SDL3/SDL.h>

bool InputManager::poll(InputState& state)
{
    m_mouseEventsThisFrame = 0;

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

        default:
            break;
        }
    }

    return true;
}