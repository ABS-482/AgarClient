#pragma once

struct InputState
{
    float mouseX = 0.0f;
    float mouseY = 0.0f;

    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;

    bool leftButton = false;
    bool rightButton = false;
    bool cycleFpsLimitPressed = false;

    bool spawnRequestPressed = false;

    bool leftButtonJustPressed = false;

    bool splitRequested = false;
    bool ejectMassRequested = false;

    bool ejectMassKeyHeld = false; // true, пока клавиша физически зажата

    float mouseWheel = 0.0f;
};