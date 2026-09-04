#pragma once

#include "Blob.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

using WorldSnapshot = std::shared_ptr<const std::unordered_map<uint32_t, Blob>>;

class World
{
public:
    struct MapBounds
    {
        double minX = 0.0;
        double minY = 0.0;
        double maxX = 0.0;
        double maxY = 0.0;
        bool valid = false;
    };

    void updateFoodBlob(
        uint32_t id, int16_t x, int16_t y, int8_t size,
        uint8_t colorIndex, uint8_t cellType
    );

    void updateVirusBlob(
        uint32_t id, uint16_t x, uint16_t y, int16_t size, int32_t skin,
        uint8_t colorIndex, uint8_t flags, uint16_t playerID, const std::string& name
    );

    void updatePlayerBlob(
        uint32_t id, uint16_t x, uint16_t y, int16_t size,
        uint8_t flags, uint16_t playerID, const std::string& name
    );

    void setPlayerName(uint16_t playerID, const std::string& name);
    void setPlayerSkin(uint16_t playerID, uint32_t skin);
    void setPlayerColorIndex(uint16_t playerID, uint8_t colorIndex);
    void setPlayerSticker(uint16_t playerID, uint32_t sticker);
    void removePlayerMeta(uint16_t playerID);
    void addOwnedBlob(uint32_t id);
    void removeBlob(uint32_t id);

    // Возвращает разделяемый неизменяемый снимок мира. Если мир не менялся
    // с прошлого вызова — копирования НЕ происходит, отдаётся тот же shared_ptr.
    WorldSnapshot snapshot() const;

    void setMapBounds(double minX, double minY, double maxX, double maxY);
    MapBounds getMapBounds() const;

private:
    mutable std::mutex m_mutex;
    std::unordered_map<uint32_t, Blob> m_blobs;
    std::unordered_map<uint16_t, std::string> playerNames;
    std::unordered_map<uint16_t, uint32_t> playerSkins;
    std::unordered_map<uint16_t, uint8_t> playerColorIndexes;
    std::unordered_map<uint16_t, uint32_t> playerStickers;
    std::vector<uint32_t> ownedIds;
    MapBounds m_mapBounds;

    // Версионирование для кэша снапшота.
    uint64_t m_version = 0;
    mutable WorldSnapshot m_cachedSnapshot;
    mutable uint64_t m_cachedVersion = 0;
};