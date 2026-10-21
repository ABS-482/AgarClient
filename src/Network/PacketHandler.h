#pragma once

#include "PacketReader.h"
#include "../Game/World.h"

#include <cstdint>
#include <atomic>
#include <chrono>

class NetworkClient;

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
    void setNetworkClient(NetworkClient& client)
    {
        m_networkClient = &client;
    }

    float tickMs() const
    {
        return m_tickMs.load();
    }

    float jitterMs() const
    {
        return m_jitterMs.load();
    }

private:
    void handleWorldUpdate(PacketReader& reader);
    void handleColorsViaPid(PacketReader& reader);
    void handleSkinsViaPid(PacketReader& reader);
    void handleRemovePidName(PacketReader& reader);
    void handleNamesViaPid(PacketReader& reader);
    void handleMapBounds(PacketReader& reader);
    void handleUsersList(PacketReader& reader);
    void handleChatMessage(PacketReader& reader, bool isPrivate);
    void handleDelayedNickRequest(PacketReader& reader);
    void handleGameMode(PacketReader& reader);
    void handleServerUptime(PacketReader& reader);

    int m_serverProtocol;
    World& m_world;
    NetworkClient* m_networkClient = nullptr;

    std::atomic<float> m_tickMs{ 50.0f };
    std::atomic<float> m_jitterMs{ 0.0f };
    std::chrono::steady_clock::time_point m_lastWorld{};
};