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

    if (inserted)
    {
        blob.id = id;
        blob.x = static_cast<float>(x);
        blob.y = static_cast<float>(y);
        blob.size = static_cast<float>(size);

        blob.targetX = blob.x;
        blob.targetY = blob.y;
        blob.targetSize = blob.size;
    }
    else
    {
        blob.x = static_cast<float>(x);
        blob.y = static_cast<float>(y);
        blob.size = static_cast<float>(size);

        blob.targetX = blob.x;
        blob.targetY = blob.y;
        blob.targetSize = blob.size;
    }

    blob.cellType = static_cast<CellType>(cellType);
    blob.colorIndex = colorIndex;
    blob.sticker = 0;
    blob.skin = 0;
    blob.isVirus = false;
    blob.flags = 0;
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

    blob.x = static_cast<float>(x);
    blob.y = static_cast<float>(y);
    blob.size = static_cast<float>(size);

    blob.targetX = blob.x;
    blob.targetY = blob.y;
    blob.targetSize = blob.size;

    blob.cellType = CellType::Virus;
    blob.colorIndex = colorIndex;
    blob.skin = static_cast<uint32_t>(skin);
    blob.playerID = playerID;
    blob.isVirus = (flags & 1) != 0;
    blob.flags = flags;

    if (!name.empty())
        blob.name = name;

    blob.sticker = 0;
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

    blob.x = static_cast<float>(x);
    blob.y = static_cast<float>(y);
    blob.size = static_cast<float>(size);

    blob.targetX = blob.x;
    blob.targetY = blob.y;
    blob.targetSize = blob.size;

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
}

std::unordered_map<uint32_t, Blob> World::snapshot() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_blobs; // копия под локом — короткая блокировка
}