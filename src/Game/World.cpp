#include "World.h"
#include <algorithm>

void World::updateFoodBlob(
    uint32_t id,
    uint16_t x,
    uint16_t y,
    uint8_t size,
    uint8_t colorIndex,
    uint8_t cellType,
    std::chrono::steady_clock::time_point updateTime
)
{
    check();

    const float newX = static_cast<float>(x);
    const float newY = static_cast<float>(y);
    const float newSize = static_cast<float>(size);
    const CellType newCellType =
        static_cast<CellType>(cellType);

    auto [it, inserted] = m_blobs.try_emplace(id);
    Blob& blob = it->second;

    const bool wasFood =
        !inserted && blob.cellType == CellType::Food;

    // Перестраивать статическую еду нужно только тогда,
    // когда меняются её видимые данные или тип сущности.
    const bool foodVisualChanged =
        (wasFood || newCellType == CellType::Food) &&
        (
            inserted ||
            blob.cellType != newCellType ||
            blob.targetX != newX ||
            blob.targetY != newY ||
            blob.targetSize != newSize ||
            blob.colorIndex != colorIndex
            );

    blob.id = id;
    blob.targetX = newX;
    blob.targetY = newY;
    blob.targetSize = newSize;
    blob.lastUpdateTime = updateTime;

    blob.cellType = newCellType;
    blob.colorIndex = colorIndex;
    blob.sticker = 0;
    blob.skin = 0;
    blob.isVirus = false;
    blob.flags = 0;

    if (foodVisualChanged)
        ++m_foodVersion;

    ++m_version;
}
void World::updateVirusBlob(
    uint32_t id,
    uint16_t x,
    uint16_t y,
    uint16_t size,
    int32_t skin,
    uint8_t colorIndex,
    uint8_t flags,
    uint16_t playerID,
    const std::string& name,
    std::chrono::steady_clock::time_point updateTime
)
{
    check();

    auto [it, inserted] = m_blobs.try_emplace(id);
    Blob& blob = it->second;

    blob.id = id;
    blob.targetX = static_cast<float>(x);
    blob.targetY = static_cast<float>(y);
    blob.targetSize = static_cast<float>(size);
    blob.lastUpdateTime =
        updateTime;

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
    uint16_t size,
    uint8_t flags,
    uint16_t playerID,
    const std::string& name,
    std::chrono::steady_clock::time_point updateTime
)
{
    check();

    auto [it, inserted] = m_blobs.try_emplace(id);
    Blob& blob = it->second;

    blob.id = id;
    blob.targetX = static_cast<float>(x);
    blob.targetY = static_cast<float>(y);
    blob.targetSize = static_cast<float>(size);
    blob.lastUpdateTime =
        updateTime;

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
    check();
    playerNames[playerID] = name;
}

void World::setPlayerSkin(uint16_t playerID, uint32_t skin)
{
    check();
    playerSkins[playerID] = skin;
}

void World::setPlayerColorIndex(uint16_t playerID, uint8_t colorIndex)
{
    check();
    playerColorIndexes[playerID] = colorIndex;
}

void World::setPlayerSticker(uint16_t playerID, uint32_t sticker)
{
    check();
    playerStickers[playerID] = sticker;
}

void World::addOwnedBlob(uint32_t id)
{
    check();

    for (uint32_t ownedId : ownedIds)
    {
        if (ownedId == id)
            return;
    }

    ownedIds.push_back(id);
}

void World::removeBlob(uint32_t id)
{
    check();

    const auto it = m_blobs.find(id);

    if (it != m_blobs.end())
    {
        if (it->second.cellType == CellType::Food)
            ++m_foodVersion;

        m_blobs.erase(it);
    }

    ownedIds.erase(
        std::remove(ownedIds.begin(), ownedIds.end(), id),
        ownedIds.end()
    );

    ++m_version;
}

void World::reset()
{
    check();

    m_blobs.clear();
    playerNames.clear();
    playerSkins.clear();
    playerColorIndexes.clear();
    playerStickers.clear();
    ownedIds.clear();
    m_mapBounds = MapBounds{};
    m_chatMessages.clear();
    ++m_chatMessagesVersion;
    ++m_chatMessagesResetVersion;
    m_leaderboard =
        std::make_shared<const std::vector<LeaderboardEntry>>();

    ++m_version;
    ++m_foodVersion;
}

void World::setMapBounds(double minX, double minY, double maxX, double maxY)
{
    check();
    m_mapBounds = { minX, minY, maxX, maxY, true };
}

World::MapBounds World::getMapBounds() const
{
    check();
    return m_mapBounds;
}

void World::addChatMessage(ChatMessage message)
{
    check();

    m_chatMessages.push_back(std::move(message));

    constexpr size_t maxMessages = 50;

    if (m_chatMessages.size() > maxMessages)
    {
        m_chatMessages.erase(m_chatMessages.begin());
    }

    ++m_chatMessagesVersion;
}

std::vector<ChatMessage> World::getChatMessages() const
{
    check();
    return m_chatMessages;
}

World::ChatMessagesView World::getChatMessagesView() const
{
    check();

    if (!m_cachedChatMessages ||
        m_cachedChatMessagesVersion != m_chatMessagesVersion)
    {
        m_cachedChatMessages =
            std::make_shared<const std::vector<ChatMessage>>(
                m_chatMessages
            );

        m_cachedChatMessagesVersion = m_chatMessagesVersion;
    }

    return {
        m_cachedChatMessages,
        m_chatMessagesVersion,
        m_chatMessagesResetVersion
    };
}

std::shared_ptr<const std::vector<ChatMessage>>
World::getChatMessagesSnapshot() const
{
    return getChatMessagesView().messages;
}

std::vector<uint32_t> World::getOwnedIds() const
{
    check();
    return ownedIds;
}

void World::removePlayerMeta(uint16_t playerID)
{
    check();

    // В точности как в JS removePidName — sticker НЕ трогаем,
    // там удаляются только name/skin/colorIndex.
    playerNames.erase(playerID);
    playerSkins.erase(playerID);
    playerColorIndexes.erase(playerID);
}

WorldSnapshot World::snapshot() const
{
    check();

    return WorldSnapshot(
        std::shared_ptr<const std::unordered_map<uint32_t, Blob>>{},
        & m_blobs
    );
}

void World::setLeaderboard(std::vector<LeaderboardEntry> entries)
{
    auto snapshot =
        std::make_shared<const std::vector<LeaderboardEntry>>(
            std::move(entries)
        );

    check();
    m_leaderboard = std::move(snapshot);
}

std::shared_ptr<const std::vector<LeaderboardEntry>>
World::getLeaderboardSnapshot() const
{
    check();
    return m_leaderboard;
}