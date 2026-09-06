#pragma once

#include <cstdint>
#include <string>

struct ChatMessage
{
    uint32_t msgId = 0;
    uint32_t playerID = 0;
    uint8_t pwd = 0;
    int32_t donateID = 0;

    uint8_t colorR = 255;
    uint8_t colorG = 255;
    uint8_t colorB = 255;

    uint32_t skin = 0;
    uint16_t userLevel = 0;
    uint16_t userLevelSeason = 0;

    uint8_t toxicity = 0;
    uint8_t profanity = 0;
    uint8_t insult = 0;
    uint8_t avgScore = 0;

    std::string name;
    std::string message;
    std::string recipient; // непусто только для приватных (opcode 205)
    std::string language; // "en", "ru", "fr", "nl", "cn" — пусто, если суффикс не распознан

    bool isPrivate = false;
    bool isPlayerEnter = false;
};