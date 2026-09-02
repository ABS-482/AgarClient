#pragma once

#include "Blob.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class World
{
public:
    Blob& getOrCreateBlob(uint32_t id);
    void updateFoodBlob(
        uint32_t id,
        int16_t x,
        int16_t y,
        int8_t size,
        uint8_t colorIndex,
        uint8_t cellType
    );

    void updateVirusBlob(
        uint32_t id,
        uint16_t x,
        uint16_t y,
        int16_t size,
        int32_t skin,
        uint8_t colorIndex,
        uint8_t flags,
        uint16_t playerID,
        const std::string& name
    );

    void updatePlayerBlob(
        uint32_t id,
        uint16_t x,
        uint16_t y,
        int16_t size,
        uint8_t flags,
        uint16_t playerID,
        const std::string& name
    );
    void removeBlob(uint32_t id);

    // Потокобезопасный снапшот всех сущностей — вызывать из рендера.
    std::unordered_map<uint32_t, Blob> snapshot() const;

    // Метаданные по playerID — доступ к ним пока не защищён мьютексом,
    // так как они не читаются напрямую из рендера (только используются
    // внутри handleWorldUpdate при сборке Blob).
    std::unordered_map<uint16_t, std::string> playerNames;
    std::unordered_map<uint16_t, uint32_t> playerSkins;
    std::unordered_map<uint16_t, uint8_t> playerColorIndexes;
    std::unordered_map<uint16_t, uint32_t> playerStickers;

    std::vector<uint32_t> ownedIds;

private:
    mutable std::mutex m_mutex;
    std::unordered_map<uint32_t, Blob> m_blobs;
};