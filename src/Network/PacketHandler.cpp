#include "PacketHandler.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <limits>

#include "../Network/NetworkClient.h"

void PacketHandler::handleMessage(const uint8_t* data, size_t size,
    std::chrono::steady_clock::time_point recvTime)
{
    if (size == 0)
        return;

    try
    {
        PacketReader reader(data, size);

        // Маркер-заголовок: если первый байт буфера — 0xF0 (240),
        // пропускаем следующие 5 байт и переходим сразу к opcode.
        if (data[0] == 0xF0)
        {
            reader.skip(6); // 1 байт маркера + 5 байт заголовка
        }

        uint8_t opcode = reader.readUint8();

        switch (opcode)
        {
        case 6:
            handleColorsViaPid(reader);
            break;

        case 7:
            handleSkinsViaPid(reader);
            break;

        case 9:
            handleRemovePidName(reader);
            break;

        case 10:
            handleNamesViaPid(reader);
            break;

        case 11:
            handleWorldUpdate(reader, recvTime);
            break;

        case 49:
            handleUsersList(reader);
            break;
    
        case 64:
            handleMapBounds(reader);
            break;

        case 90:
            handleServerUptime(reader);
            break;

        case 96:
            handleGameMode(reader);
            break;

        case 97:
            handleDelayedNickRequest(reader);
            break;

        case 115:
        {
            if (reader.remaining() < 4)
                throw std::out_of_range(
                    "Ping response: expected 4 bytes for hash"
                );

            const uint32_t hash = reader.readUint32LE();

            if (m_networkClient)
                m_networkClient->handlePingResponse(hash, recvTime);

            break;
        }

        case 199:
            handleChatMessage(reader, false);
            break;

        case 205:
            handleChatMessage(reader, true);
            break;

        default:
            break;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "PacketHandler error: " << e.what() << '\n';
    }


}

void PacketHandler::handleColorsViaPid(PacketReader& reader)
{
    while (reader.hasMore())
    {
        uint16_t id = reader.readUint16LE();
        uint8_t color = reader.readUint8();

        if (id > 0)
            m_world.setPlayerColorIndex(id, color);
    }
}

void PacketHandler::handleDelayedNickRequest(PacketReader& reader)
{
    uint8_t request = reader.readUint8();
    (void)request; // пока не используется, но байт нужно потребить из потока

    if (m_networkClient)
    {
        m_networkClient->requestDelayedNickResend();
    }
}

void PacketHandler::handleMapBounds(PacketReader& reader)
{
    double minX = reader.readFloat64LE();
    double minY = reader.readFloat64LE();
    double maxX = reader.readFloat64LE();
    double maxY = reader.readFloat64LE();

    m_world.setMapBounds(minX, minY, maxX, maxY);
}

void PacketHandler::handleGameMode(PacketReader& reader)
{
    uint8_t gameMode =
        reader.readUint8();

    if (m_networkClient)
    {
        m_networkClient->setGameMode(gameMode);
    }
}

void PacketHandler::handleServerUptime(PacketReader& reader)
{
    if (reader.remaining() < 16)
    {
        throw std::out_of_range(
            "Opcode 90: expected at least two doubles"
        );
    }

    const double uptimeSeconds = reader.readFloat64LE();
    const double playersOnlineValue = reader.readFloat64LE();

    if (!std::isfinite(uptimeSeconds) ||
        !std::isfinite(playersOnlineValue) ||
        uptimeSeconds < 0.0 ||
        uptimeSeconds > 1.0e9 ||
        playersOnlineValue < 0.0 ||
        playersOnlineValue > 1.0e7)
    {
        throw std::runtime_error(
            "Opcode 90: invalid uptime or playersOnline"
        );
    }

    const int uptimeMinutes =
        static_cast<int>(uptimeSeconds / 60.0 + 1.0);

    const int playersOnline =
        static_cast<int>(playersOnlineValue);

    std::optional<int> totalCells;
    std::optional<int> totalMass;

    if (reader.remaining() >= 8)
    {
        const double value = reader.readFloat64LE();

        if (std::isfinite(value) &&
            value >= 0.0 &&
            value <= 1.0e9)
        {
            totalCells = static_cast<int>(value);
        }
    }

    if (reader.remaining() >= 8)
    {
        const double value = reader.readFloat64LE();

        if (std::isfinite(value) &&
            value >= 0.0 &&
            value <= 1.0e12)
        {
            totalMass = static_cast<int>(value);
        }
    }

    if (!m_networkClient)
        return;

    m_networkClient->setServerRealtimeBase(
        playersOnline,
        uptimeMinutes
    );

    if (totalCells.has_value())
    {
        m_networkClient->setServerTotalCells(*totalCells);
    }

    if (totalMass.has_value())
    {
        m_networkClient->setServerTotalMass(*totalMass);
    }
}

void PacketHandler::handleSkinsViaPid(PacketReader& reader)
{
    while (reader.hasMore())
    {
        uint16_t id = reader.readUint16LE();
        uint32_t skin = reader.readUint32LE();

        if (id > 0)
            m_world.setPlayerSkin(id, skin);
    }
}

void PacketHandler::handleRemovePidName(PacketReader& reader)
{
    if (!reader.hasMore())
        return;

    uint16_t id = reader.readUint16LE();

    if (id > 0)
        m_world.removePlayerMeta(id);
}

void PacketHandler::handleNamesViaPid(PacketReader& reader)
{
    while (reader.hasMore())
    {
        uint16_t id = reader.readUint16LE();
        std::string name = reader.readUtf16String();

        if (id > 0)
            m_world.setPlayerName(id, name);
    }
}

void PacketHandler::handleChatMessage(PacketReader& reader, bool isPrivate)
{
    ChatMessage msg;

    msg.msgId = reader.readUint32BE();
    msg.playerID = reader.readUint32BE();
    msg.pwd = reader.readUint8();
    msg.donateID = reader.readInt32BE();

    msg.colorR = reader.readUint8();
    msg.colorG = reader.readUint8();
    msg.colorB = reader.readUint8();

    msg.skin = reader.readUint32BE();
    msg.userLevel = reader.readUint16BE();
    msg.userLevelSeason = reader.readUint16BE();

    msg.toxicity = reader.readUint8();
    msg.profanity = reader.readUint8();
    msg.insult = reader.readUint8();
    msg.avgScore = reader.readUint8();

    msg.name = reader.readUtf16String();

    std::string rawMessage = reader.readUtf16String();
    std::string displayMessage = rawMessage;

    if (rawMessage.size() >= 4 && rawMessage[rawMessage.size() - 3] == ':')
    {
        std::string code = rawMessage.substr(rawMessage.size() - 2, 2);

        static const std::array<std::string, 7> knownLanguageCodes =
        { "ru", "en", "fr", "nl", "cn", "ar", "gl" };

        if (std::find(knownLanguageCodes.begin(), knownLanguageCodes.end(), code)
            != knownLanguageCodes.end())
        {
            msg.language = code;
            displayMessage = rawMessage.substr(0, rawMessage.size() - 4);
        }
    }

    msg.message = displayMessage;

    msg.isPlayerEnter = (msg.message == "***playerenter***");

    if (isPrivate)
    {
        msg.recipient = reader.readUtf16String();
    }

    msg.isPrivate = isPrivate;

    // original_message / hash — есть не всегда, пока не используем,
    // но дочитываем, чтобы буфер был потреблён корректно.
    if (reader.hasMore())
        reader.readUtf16String();

    if (reader.hasMore())
        reader.readUtf16String();

    m_world.addChatMessage(std::move(msg));
}

void PacketHandler::handleUsersList(PacketReader& reader)
{
    uint32_t count = reader.readUint32LE();

    if (count > reader.remaining() / 8)
    {
        throw std::out_of_range("handleUsersList: count слишком большой для оставшихся данных");
    }

    std::vector<LeaderboardEntry> users;
    users.reserve(count);

    for (uint32_t i = 0; i < count; ++i)
    {
        LeaderboardEntry entry;
        entry.id = reader.readUint32LE();
        entry.name = reader.readUtf16String();
        users.push_back(std::move(entry));
    }


    // Legacy format, identical to Java:
    // 4 fields: name:::level:::userType:::urlId
    // 5 fields: name:::level:::userType:::urlId:::seasonLevel
    for (auto& user : users)
    {
        if (user.name.find(":::") == std::string::npos)
            continue;

        std::vector<std::string> parts;
        size_t start = 0;

        for (;;)
        {
            const size_t pos = user.name.find(":::", start);

            parts.push_back(
                user.name.substr(
                    start,
                    pos == std::string::npos
                    ? std::string::npos
                    : pos - start
                )
            );

            if (pos == std::string::npos)
                break;

            start = pos + 3;
        }

        if (parts.size() != 4 && parts.size() != 5)
            continue;

        // Field 0: player name.
        user.name = parts[0];

        // A four-field entry has no seasonal level.
        user.hasSeason = false;

        // Field 1: regular level.
        try
        {
            const int level = std::stoi(parts[1]);

            if (level >= 0 &&
                level <= std::numeric_limits<uint16_t>::max())
            {
                user.userLevel = static_cast<uint16_t>(level);
            }
        }
        catch (...)
        {
        }

        // Field 2 (userType) is intentionally ignored,
        // matching the Java text parser.

        // Field 3: URL ID.
        try
        {
            const unsigned long long urlId =
                std::stoull(parts[3]);

            if (urlId <= std::numeric_limits<uint32_t>::max())
            {
                user.urlId = static_cast<uint32_t>(urlId);
            }
        }
        catch (...)
        {
        }

        // Field 4: seasonal level, present only in the 5-field format.
        if (parts.size() == 5)
        {
            try
            {
                const int seasonLevel = std::stoi(parts[4]);

                if (seasonLevel >= 0 &&
                    seasonLevel <= std::numeric_limits<uint16_t>::max())
                {
                    user.userLevelSeason =
                        static_cast<uint16_t>(seasonLevel);

                    user.hasSeason = true;
                }
            }
            catch (...)
            {
                user.hasSeason = false;
            }
        }
    }

    // Optional binary block follows below.

    if (reader.hasMore())
    {
        uint32_t entryCount = reader.readUint32LE();
        size_t remaining = reader.remaining();

        if (entryCount > 0 && remaining / entryCount < 7)
        {
            entryCount = 0; // не похоже на реальный блок — пропускаем, не читаем
        }

        size_t bytesPerEntry = (entryCount > 0) ? (remaining / entryCount) : 0;
        int version = 0;

        if (bytesPerEntry == 9) version = 2;
        else if (bytesPerEntry == 7) version = 1;

        for (uint32_t i = 0; i < entryCount && i < users.size() && version != 0; ++i)
        {
            users[i].userLevel = reader.readUint16LE();
            (void)reader.readUint8();
            users[i].urlId = reader.readUint32LE();

            if (version == 2)
            {
                users[i].userLevelSeason = reader.readUint16LE();
                users[i].hasSeason = true;
            }
        }
    }

    m_world.setLeaderboard(std::move(users));
}

void PacketHandler::handleWorldUpdate(PacketReader& reader,
    std::chrono::steady_clock::time_point recvTime)
{
    // ------------------------------------------------------------
    // Измерение интервала между World Update пакетами
    // ------------------------------------------------------------

    if (m_lastWorld != std::chrono::steady_clock::time_point{})
    {
        const float ms =
            std::chrono::duration<float, std::milli>(
                recvTime - m_lastWorld
            ).count();

        // Отбрасываем слишком короткие и слишком длинные интервалы.
        if (ms > 5.0f && ms < 500.0f)
        {
            const float tick = m_tickMs.load();

            // Экспоненциальное усреднение интервала.
            m_tickMs.store(
                tick + (ms - tick) * 0.1f
            );

            const float jitter = m_jitterMs.load();

            // Усреднённое абсолютное отклонение интервала.
            m_jitterMs.store(
                jitter + (std::abs(ms - tick) - jitter) * 0.1f
            );
        }
    }

    m_lastWorld = recvTime;

    const auto packetTimestamp = recvTime;

    // ------------------------------------------------------------
    // Поглощения (eaten)
    // ------------------------------------------------------------

    uint16_t blobsEaten = reader.readUint16LE();
    reader.skip(2); // выравнивание, как в оригинале (offset += 4 вместо 2)

    for (uint16_t i = 0; i < blobsEaten; ++i)
    {
        uint32_t eatenId = reader.readUint32LE();
        uint32_t eaterId = reader.readUint32LE();

        // TODO: визуальная анимация поглощения (позже, когда будет
        // интерполяция). Пока просто убираем поглощённую клетку сразу.
        m_world.removeBlob(eatenId);
        (void)eaterId;
    }

    // ------------------------------------------------------------
    // Еда
    // ------------------------------------------------------------

    uint32_t foodCells = reader.readUint32LE();

    for (uint32_t i = 0; i < foodCells; ++i)
    {
        uint32_t id = reader.readUint32LE();

        if (id == 0)
            break;

        uint8_t cellType = reader.readUint8();
        uint16_t x = reader.readUint16LE();
        uint16_t y = reader.readUint16LE();
        uint8_t size = reader.readUint8();
        uint8_t colorIndex = reader.readUint8();

        m_world.updateFoodBlob(
            id,
            x,
            y,
            size,
            colorIndex,
            cellType,
            packetTimestamp
        );
    }

    // ------------------------------------------------------------
    // Вирусы
    // ------------------------------------------------------------

    uint32_t virusCells = reader.readUint32LE();

    for (uint32_t i = 0; i < virusCells; ++i)
    {
        uint32_t id = reader.readUint32LE();

        if (id == 0)
            break;

        uint16_t x = reader.readUint16LE();
        uint16_t y = reader.readUint16LE();
        uint16_t size = reader.readUint16LE();
        int32_t skin = reader.readInt32LE();
        uint8_t colorIndex = reader.readUint8();
        uint8_t flags = reader.readUint8();
        uint16_t playerID = reader.readUint16LE();
        std::string name = reader.readUtf16String();

        m_world.updateVirusBlob(
            id,
            x,
            y,
            size,
            skin,
            colorIndex,
            flags,
            playerID,
            name,
            packetTimestamp
        );
    }

    // ------------------------------------------------------------
    // Смена имён / скинов / цветов / стикеров (по playerID)
    // ------------------------------------------------------------

    uint16_t nameChanges = reader.readUint16LE();
    for (uint16_t i = 0; i < nameChanges; ++i)
    {
        uint16_t id = reader.readUint16LE();
        std::string name = reader.readUtf16String();

        if (id > 0)
            m_world.setPlayerName(id, name);
    }

    uint16_t skinChanges = reader.readUint16LE();
    for (uint16_t i = 0; i < skinChanges; ++i)
    {
        uint16_t id = reader.readUint16LE();
        uint32_t skin = reader.readUint32LE();

        if (id > 0)
            m_world.setPlayerSkin(id, skin);
    }

    uint16_t colorChanges = reader.readUint16LE();
    for (uint16_t i = 0; i < colorChanges; ++i)
    {
        uint16_t id = reader.readUint16LE();
        uint8_t color = reader.readUint8();

        if (id > 0)
            m_world.setPlayerColorIndex(id, color);
    }

    uint16_t stickerChanges = reader.readUint16LE();
    for (uint16_t i = 0; i < stickerChanges; ++i)
    {
        uint16_t id = reader.readUint16LE();
        uint32_t sticker = reader.readUint32LE();

        if (id > 0)
            m_world.setPlayerSticker(id, sticker);
    }

    // ------------------------------------------------------------
    // Обычные клетки (игроки) — без count, идёт до id == 0
    // ------------------------------------------------------------

    for (;;)
    {
        uint32_t id = reader.readUint32LE();

        if (id == 0)
            break;

        uint16_t x = reader.readUint16LE();
        uint16_t y = reader.readUint16LE();
        uint16_t size = reader.readUint16LE();
        uint8_t flags = reader.readUint8();

        uint16_t playerID = reader.readUint16LE();
        reader.skip(1); // выравнивание (offset += 3 вместо 2)

        std::string name = reader.readUtf16String();

        m_world.updatePlayerBlob(
            id,
            x,
            y,
            size,
            flags,
            playerID,
            name,
            packetTimestamp
        );
    }

    // ------------------------------------------------------------
    // Удаление сущностей
    // ------------------------------------------------------------

    uint32_t removedCount = reader.readUint32LE();

    for (uint32_t i = 0; i < removedCount; ++i)
    {
        if (!reader.hasMore())
            break;

        uint32_t id = reader.readUint32LE();
        m_world.removeBlob(id);
    }
}