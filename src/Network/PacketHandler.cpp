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
        case 11:
            handleWorldUpdate(reader);
            break;

        default:
            std::cerr << "Unknown opcode: "
                << static_cast<int>(opcode) << '\n';
            break;
        }
    }
    catch (const std::out_of_range& e)
    {
        std::cerr << "PacketHandler error: " << e.what() << '\n';
    }


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

        Blob& blob = m_world.getOrCreateBlob(id);
        blob.x = blob.targetX = static_cast<float>(x);
        blob.y = blob.targetY = static_cast<float>(y);
        blob.size = blob.targetSize = static_cast<float>(size);
        blob.cellType = static_cast<CellType>(cellType);
        blob.colorIndex = colorIndex;
        blob.sticker = 0;
        blob.skin = 0;
        blob.isVirus = false;
        blob.flags = 0;
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
        bool isVirus = (flags & 1) != 0;
        uint16_t playerID = reader.readUint16LE();
        std::string name = reader.readUtf16String();

        Blob& blob = m_world.getOrCreateBlob(id);
        blob.x = blob.targetX = static_cast<float>(x);
        blob.y = blob.targetY = static_cast<float>(y);
        blob.size = blob.targetSize = static_cast<float>(size);
        blob.cellType = CellType::Virus;
        blob.colorIndex = colorIndex;
        blob.skin = static_cast<uint32_t>(skin);
        blob.playerID = playerID;
        blob.isVirus = isVirus;
        blob.flags = flags;

        if (!name.empty())
            blob.name = name;
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
            m_world.playerNames[id] = name;
    }

    uint16_t skinChanges = reader.readUint16LE();
    for (uint16_t i = 0; i < skinChanges; ++i)
    {
        uint16_t id = reader.readUint16LE();
        uint32_t skin = reader.readUint32LE();

        if (id > 0)
            m_world.playerSkins[id] = skin;
    }

    uint16_t colorChanges = reader.readUint16LE();
    for (uint16_t i = 0; i < colorChanges; ++i)
    {
        uint16_t id = reader.readUint16LE();
        uint8_t color = reader.readUint8();

        if (id > 0)
            m_world.playerColorIndexes[id] = color;
    }

    uint16_t stickerChanges = reader.readUint16LE();
    for (uint16_t i = 0; i < stickerChanges; ++i)
    {
        uint16_t id = reader.readUint16LE();
        uint32_t sticker = reader.readUint32LE();

        if (id > 0)
            m_world.playerStickers[id] = sticker;
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
        bool isVirus = (flags & 1) != 0;

        uint16_t playerID = reader.readUint16LE();
        reader.skip(1); // выравнивание (offset += 3 вместо 2)

        std::string name = reader.readUtf16String();

        Blob& blob = m_world.getOrCreateBlob(id);
        blob.x = blob.targetX = static_cast<float>(x);
        blob.y = blob.targetY = static_cast<float>(y);
        blob.size = blob.targetSize = static_cast<float>(size);
        blob.cellType = CellType::Player;
        blob.playerID = playerID;
        blob.isVirus = isVirus;
        blob.flags = flags;

        if (playerID > 0)
        {
            if (auto it = m_world.playerNames.find(playerID); it != m_world.playerNames.end())
                blob.name = it->second;
            else if (!name.empty())
                blob.name = name;

            if (auto it = m_world.playerSkins.find(playerID); it != m_world.playerSkins.end())
                blob.skin = it->second;

            if (auto it = m_world.playerColorIndexes.find(playerID); it != m_world.playerColorIndexes.end())
                blob.colorIndex = it->second;

            if (auto it = m_world.playerStickers.find(playerID); it != m_world.playerStickers.end())
                blob.sticker = it->second;
        }
        else if (!name.empty())
        {
            blob.name = name;
        }

        // Клетка помечена как "своя" (актуально для spawn-режима).
        if (flags & 32)
        {
            bool alreadyTracked = false;
            for (uint32_t ownedId : m_world.ownedIds)
            {
                if (ownedId == id)
                {
                    alreadyTracked = true;
                    break;
                }
            }

            if (!alreadyTracked)
                m_world.ownedIds.push_back(id);
        }
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