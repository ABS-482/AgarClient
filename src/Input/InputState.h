#pragma once

#include "GameAction.h"
#include "ActionState.h"

#include <unordered_map>
#include <string>

struct InputState
{
    float mouseX = 0.0f;
    float mouseY = 0.0f;

    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;

    float mouseWheel = 0.0f;

    bool leftButton = false;
    bool rightButton = false;

    bool cycleFpsLimitPressed = false;

    bool spawnRequestPressed = false;

    bool leftButtonJustPressed = false;
    bool leftButtonJustReleased = false;
    bool leftButtonDoubleClicked = false;

    bool splitRequested = false;
    bool ejectMassRequested = false;

    bool ejectMassKeyHeld = false;

    bool menuUpPressed = false;
    bool menuDownPressed = false;
    bool menuConfirmPressed = false;
    bool menuTogglePressed = false;
    bool splitPreviewHeld = false;
    bool fullscreenPressed = false;
    // Ввод текста в активном поле
    std::string textInput;

    bool textBackspacePressed = false;
    bool textEnterPressed = false;
    bool textEscapePressed = false;
    bool textSelectAllPressed = false;
    bool textCopyPressed = false;
    bool textCutPressed = false;
    bool textPastePressed = false;

    bool textDeletePressed = false;
    bool textLeftPressed = false;
    bool textRightPressed = false;
    bool textHomePressed = false;
    bool textEndPressed = false;

    // Shift при нажатии стрелки или Home/End.
    bool textShiftHeld = false;

    // Состояние передаётся из MainMenu до InputManager::poll().
    bool textInputActive = false;

    std::unordered_map<GameAction, ActionState> actions;

    ActionState& action(GameAction gameAction)
    {
        return actions[gameAction];
    }

    const ActionState& action(GameAction gameAction) const
    {
        static const ActionState empty{};

        auto it = actions.find(gameAction);

        if (it == actions.end())
            return empty;

        return it->second;
    }
};