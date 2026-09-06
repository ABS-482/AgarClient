#include "PacketHandler.h"

#include <iostream>

void PacketHandler::handleMessage(const uint8_t* data, size_t size)
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
            handleWorldUpdate(reader);
            break;

        case 49:
            handleUsersList(reader);
            break;
    
        case 64:
            handleMapBounds(reader);
            break;

        default:
            std::cerr << "Unknown opcode: "
                << static_cast<int>(opcode) << '\n';
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

void PacketHandler::handleMapBounds(PacketReader& reader)
{
    double minX = reader.readFloat64LE();
    double minY = reader.readFloat64LE();
    double maxX = reader.readFloat64LE();
    double maxY = reader.readFloat64LE();

    m_world.setMapBounds(minX, minY, maxX, maxY);
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

    // Легаси-формат: доп. поля встроены прямо в имя, через ":::".
    for (auto& user : users)
    {
        if (user.name.find(":::") == std::string::npos)
            continue;

        std::vector<std::string> parts;
        size_t start = 0;

        for (;;)
        {
            size_t pos = user.name.find(":::", start);
            parts.push_back(user.name.substr(start, pos - start));

            if (pos == std::string::npos)
                break;

            start = pos + 3;
        }

        if (parts.size() == 4 || parts.size() == 5)
        {
            user.name = parts[0];

            try { user.userLevel = static_cast<uint16_t>(std::stoi(parts[1])); }
            catch (...) {}

            if (!parts[2].empty())
                user.userType = static_cast<uint8_t>(parts[2][0]);

            try { user.urlId = static_cast<uint32_t>(std::stoul(parts[3])); }
            catch (...) {}

            if (parts.size() == 5)
            {
                try
                {
                    user.userLevelSeason = static_cast<uint16_t>(std::stoi(parts[4]));
                    user.hasSeason = true;
                }
                catch (...) {}
            }
        }
    }

    // Опциональный бинарный блок — если есть, ПЕРЕЗАПИСЫВАЕТ поля выше.
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
            users[i].userType = reader.readUint8();
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

void PacketHandler::handleWorldUpdate(PacketReader& reader)
{
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
        int16_t x = reader.readInt16LE();
        int16_t y = reader.readInt16LE();
        int8_t size = reader.readInt8();
        uint8_t colorIndex = reader.readUint8();

        m_world.updateFoodBlob(
            id,
            x,
            y,
            size,
            colorIndex,
            cellType
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
        int16_t size = reader.readInt16LE();
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
            name
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
        int16_t size = reader.readInt16LE();
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
            name
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