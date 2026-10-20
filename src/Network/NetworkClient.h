#pragma once

#include "PacketHandler.h"

#include <ixwebsocket/IXWebSocket.h>

#include <string>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstdint>

enum class ConnectionMode
{
    Play,
    Spectate
};

class NetworkClient
{
public:
    explicit NetworkClient(PacketHandler& handler);
    ~NetworkClient();

    void connect(
        const std::string& url,
        const std::string& serverPass = "",
        ConnectionMode mode = ConnectionMode::Play
    );

    void disconnect();

    void requestSpectate();
    void requestPlay();

    void setPlayerPassword(
        const std::string& password
    )
    {
        m_playerPassword = password;
    }

    void sendAimPosition(double worldX, double worldY);

    void setNickname(
        const std::string& nickname
    )
    {
        m_nickname = nickname;
    }

    void requestSplit();
    void requestEjectMass();
    void requestSpawnFromSpectate();
    void requestDelayedNickResend();
    void update();

    void setDonateCredentials(
        uint32_t donateId,
        const std::string& donatePass
    )
    {
        m_donateId = donateId;
        m_donatePass = donatePass;
    }

    void setPlayerColor(
        uint8_t colorIndex
    )
    {
        m_playerColor = colorIndex;
    }

    void requestSpawn();
    void sendChat(const std::string& text);

    void setGameMode(
        uint8_t gameMode
    )
    {
        m_gameMode = gameMode;
    }

    uint8_t gameMode() const
    {
        return m_gameMode;
    }

    bool isConnected() const
    {
        return m_connected;
    }

    bool consumeConnectionFailed();

    const std::string& currentUrl() const
    {
        return m_currentUrl;
    }

    int pingMs() const noexcept
    {
        return m_pingMs.load(std::memory_order_relaxed);
    }

    // Вызывается PacketHandler при получении ответа opcode 115.
    void handlePingResponse(uint32_t hash) noexcept;

private:
    void sendNick();
    void sendDonate();
    void sendPlayerColor();
    void sendRaw(const uint8_t* data, size_t size);
    void sendHandshake();
    void sendPacket(uint8_t opcode);
    void sendPlayerPassword();
    void sendPing();

    std::chrono::steady_clock::time_point m_lastChatTime{};

    std::mutex m_pendingMutex;

    bool m_pendingNickResend = false;

    std::chrono::steady_clock::time_point
        m_nickResendTime{};

    std::string m_nickname;

    uint32_t m_donateId = 0;
    std::string m_donatePass;

    uint8_t m_playerColor = 0;

    std::string m_playerPassword;

    ConnectionMode m_connectionMode =
        ConnectionMode::Play;

    uint8_t m_gameMode = 0;

    PacketHandler& m_handler;
    ix::WebSocket m_webSocket;

    bool m_connected = false;
    bool m_connectionFailed = false;
    bool m_intentionalDisconnect = false;

    std::string m_currentUrl;
    // Ping
    std::chrono::steady_clock::time_point m_lastPingAttempt{};

    std::atomic<uint32_t> m_lastPingHash{ 0 };
    std::atomic<int64_t> m_lastPingSentAtMs{ 0 };
    std::atomic<int> m_pingMs{ -1 };
};
