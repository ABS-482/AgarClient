#pragma once

#include "PacketReader.h"
#include "../Game/World.h"

#include <cstdint>

class PacketHandler
{
public:
    PacketHandler(int serverProtocol, World& world)
        : m_serverProtocol(serverProtocol)
        , m_world(world)
    {
    }

    // Аналог вашего parse(reader) из JS.
    void handleMessage(const uint8_t* data, size_t size);

private:
    void handleWorldUpdate(PacketReader& reader);

    int m_serverProtocol;
    World& m_world;
};