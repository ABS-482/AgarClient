#pragma once

struct InputState
{
    float mouseX = 0.0f;
    float mouseY = 0.0f;

    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;

    bool leftButton = false;
    bool rightButton = false;

    bool leftButtonJustPressed = false;

    float mouseWheel = 0.0f;
};