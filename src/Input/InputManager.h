#pragma once

#include "InputState.h"

class InputManager
{
public:
    // Опрашивает все события SDL за кадр, обновляет state.
    // Возвращает false, если пришёл запрос на выход (SDL_EVENT_QUIT).
    bool poll(InputState& state);

    int mouseEventsThisFrame() const { return m_mouseEventsThisFrame; }

private:
    int m_mouseEventsThisFrame = 0;
};