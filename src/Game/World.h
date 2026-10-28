#pragma once

#include "Blob.h"
#include "ChatMessage.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include "Leaderboard.h"
#include <vector>
#include <chrono>

#include <cassert>
#include <thread>

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

    struct ChatMessagesView
    {
        std::shared_ptr<const std::vector<ChatMessage>> messages;
        std::uint64_t version = 0;
        std::uint64_t resetVersion = 0;
    };

    void updateFoodBlob(
        uint32_t id,
        uint16_t x,
        uint16_t y,
        uint8_t size,
        uint8_t colorIndex,
        uint8_t cellType,
        std::chrono::steady_clock::time_point updateTime
    );

    void updateVirusBlob(
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
    );

    void updatePlayerBlob(
        uint32_t id,
        uint16_t x,
        uint16_t y,
        uint16_t size,
        uint8_t flags,
        uint16_t playerID,
        const std::string& name,
        std::chrono::steady_clock::time_point updateTime
    );

    void setPlayerName(uint16_t playerID, const std::string& name);
    void setPlayerSkin(uint16_t playerID, uint32_t skin);
    void setPlayerColorIndex(uint16_t playerID, uint8_t colorIndex);
    void setPlayerSticker(uint16_t playerID, uint32_t sticker);
    void removePlayerMeta(uint16_t playerID);
    void addOwnedBlob(uint32_t id);
    void removeBlob(uint32_t id);
    void setLeaderboard(std::vector<LeaderboardEntry> entries);
    void addChatMessage(ChatMessage message);
    std::vector<ChatMessage> getChatMessages() const;
    std::shared_ptr<const std::vector<ChatMessage>>
        getChatMessagesSnapshot() const;
    ChatMessagesView getChatMessagesView() const;
    std::shared_ptr<const std::vector<LeaderboardEntry>>
        getLeaderboardSnapshot() const;

    // Возвращает разделяемый неизменяемый снимок мира. Если мир не менялся
    // с прошлого вызова — копирования НЕ происходит, отдаётся тот же shared_ptr.
    WorldSnapshot snapshot() const;

    const std::unordered_map<uint32_t, Blob>& blobs() const
    {
        check();
        return m_blobs;
    }

    uint64_t version() const
    {
        check();
        return m_version;
    }

    uint64_t foodVersion() const
    {
        check();
        return m_foodVersion;
    }

    void setMapBounds(double minX, double minY, double maxX, double maxY);
    MapBounds getMapBounds() const;
    void reset();

    std::vector<uint32_t> getOwnedIds() const;

private:
    std::unordered_map<uint32_t, Blob> m_blobs;
    std::unordered_map<uint16_t, std::string> playerNames;
    std::unordered_map<uint16_t, uint32_t> playerSkins;
    std::unordered_map<uint16_t, uint8_t> playerColorIndexes;
    std::unordered_map<uint16_t, uint32_t> playerStickers;
    std::vector<uint32_t> ownedIds;
    std::shared_ptr<const std::vector<LeaderboardEntry>> m_leaderboard =
        std::make_shared<const std::vector<LeaderboardEntry>>();
    std::vector<ChatMessage> m_chatMessages;
    uint64_t m_chatMessagesVersion = 1;
    std::uint64_t m_chatMessagesResetVersion = 0;

    mutable uint64_t m_cachedChatMessagesVersion = 0;

    mutable std::shared_ptr<const std::vector<ChatMessage>>
        m_cachedChatMessages;
    MapBounds m_mapBounds;

    // Версия состояния мира.
    uint64_t m_version = 0;

    // Версия визуального состояния статической еды.
    uint64_t m_foodVersion = 0;

    #ifndef NDEBUG
        std::thread::id m_owner = std::this_thread::get_id();

        void check() const
        {
            assert(std::this_thread::get_id() == m_owner);
        }
    #else
        void check() const noexcept {}
    #endif
};