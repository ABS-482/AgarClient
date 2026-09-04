#include "World.h"

void World::updateFoodBlob(
    uint32_t id,
    int16_t x,
    int16_t y,
    int8_t size,
    uint8_t colorIndex,
    uint8_t cellType
)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto [it, inserted] = m_blobs.try_emplace(id);
    Blob& blob = it->second;

    blob.id = id;
    blob.targetX = static_cast<float>(x);
    blob.targetY = static_cast<float>(y);
    blob.targetSize = static_cast<float>(size);
    blob.lastUpdateTime = std::chrono::steady_clock::now();

    blob.cellType = static_cast<CellType>(cellType);
    blob.colorIndex = colorIndex;
    blob.sticker = 0;
    blob.skin = 0;
    blob.isVirus = false;
    blob.flags = 0;

    ++m_version;
}

void World::updateVirusBlob(
    uint32_t id,
    uint16_t x,
    uint16_t y,
    int16_t size,
    int32_t skin,
    uint8_t colorIndex,
    uint8_t flags,
    uint16_t playerID,
    const std::string& name
)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto [it, inserted] = m_blobs.try_emplace(id);
    Blob& blob = it->second;

    blob.id = id;
    blob.targetX = static_cast<float>(x);
    blob.targetY = static_cast<float>(y);
    blob.targetSize = static_cast<float>(size);
    blob.lastUpdateTime = std::chrono::steady_clock::now();

    blob.cellType = CellType::Virus;
    blob.colorIndex = colorIndex;
    blob.skin = 63895;
    blob.playerID = playerID;
    blob.isVirus = (flags & 1) != 0;
    blob.flags = flags;

    if (!name.empty())
        blob.name = name;

    blob.sticker = 0;

    ++m_version;
}

void World::updatePlayerBlob(
    uint32_t id,
    uint16_t x,
    uint16_t y,
    int16_t size,
    uint8_t flags,
    uint16_t playerID,
    const std::string& name
)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto [it, inserted] = m_blobs.try_emplace(id);
    Blob& blob = it->second;

    blob.id = id;
    blob.targetX = static_cast<float>(x);
    blob.targetY = static_cast<float>(y);
    blob.targetSize = static_cast<float>(size);
    blob.lastUpdateTime = std::chrono::steady_clock::now();

    blob.cellType = CellType::Player;
    blob.playerID = playerID;
    blob.isVirus = (flags & 1) != 0;
    blob.flags = flags;

    if (playerID > 0)
    {
        if (auto it = playerNames.find(playerID); it != playerNames.end())
            blob.name = it->second;
        else if (!name.empty())
            blob.name = name;

        if (auto it = playerSkins.find(playerID); it != playerSkins.end())
            blob.skin = it->second;

        if (auto it = playerColorIndexes.find(playerID); it != playerColorIndexes.end())
            blob.colorIndex = it->second;

        if (auto it = playerStickers.find(playerID); it != playerStickers.end())
            blob.sticker = it->second;
    }
    else if (!name.empty())
    {
        blob.name = name;
    }

    if (flags & 32)
    {
        bool alreadyTracked = false;

        for (uint32_t ownedId : ownedIds)
        {
            if (ownedId == id)
            {
                alreadyTracked = true;
                break;
            }
        }

        if (!alreadyTracked)
            ownedIds.push_back(id);
    }
    ++m_version;
}

void World::setPlayerName(uint16_t playerID, const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    playerNames[playerID] = name;
}

void World::setPlayerSkin(uint16_t playerID, uint32_t skin)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    playerSkins[playerID] = skin;
}

void World::setPlayerColorIndex(uint16_t playerID, uint8_t colorIndex)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    playerColorIndexes[playerID] = colorIndex;
}

void World::setPlayerSticker(uint16_t playerID, uint32_t sticker)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    playerStickers[playerID] = sticker;
}

void World::addOwnedBlob(uint32_t id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (uint32_t ownedId : ownedIds)
    {
        if (ownedId == id)
            return;
    }

    ownedIds.push_back(id);
}

void World::removeBlob(uint32_t id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_blobs.erase(id);
    ++m_version;
}

void World::setMapBounds(double minX, double minY, double maxX, double maxY)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mapBounds = { minX, minY, maxX, maxY, true };
}

World::MapBounds World::getMapBounds() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_mapBounds;
}

void World::removePlayerMeta(uint16_t playerID)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // В точности как в JS removePidName — sticker НЕ трогаем,
    // там удаляются только name/skin/colorIndex.
    playerNames.erase(playerID);
    playerSkins.erase(playerID);
    playerColorIndexes.erase(playerID);
}

WorldSnapshot World::snapshot() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_cachedSnapshot || m_cachedVersion != m_version)
    {
        m_cachedSnapshot = std::make_shared<std::unordered_map<uint32_t, Blob>>(m_blobs);
        m_cachedVersion = m_version;
    }

    return m_cachedSnapshot;
}