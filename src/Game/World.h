#pragma once

#include "Blob.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class World
{
public:
    Blob& getOrCreateBlob(uint32_t id);
    void removeBlob(uint32_t id);

    std::unordered_map<uint32_t, Blob>& blobs() { return m_blobs; }
    const std::unordered_map<uint32_t, Blob>& blobs() const { return m_blobs; }

    // Метаданные по playerID, приходящие отдельными списками
    // и применяемые к клеткам через playerID.
    std::unordered_map<uint16_t, std::string> playerNames;
    std::unordered_map<uint16_t, uint32_t> playerSkins;
    std::unordered_map<uint16_t, uint8_t> playerColorIndexes;
    std::unordered_map<uint16_t, uint32_t> playerStickers;

    // ID клеток, помеченных flags&32 — предположительно "свои" клетки
    // (актуально для spawn-режима, сейчас не используется).
    std::vector<uint32_t> ownedIds;

private:
    std::unordered_map<uint32_t, Blob> m_blobs;
};