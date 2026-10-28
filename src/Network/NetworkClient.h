#pragma once

#include "PacketHandler.h"

#include <ixwebsocket/IXWebSocket.h>

#include "NetEventQueue.h"
#include <vector>
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

    void setGameMode(uint8_t gameMode) noexcept
    {
        m_gameMode.store(gameMode, std::memory_order_relaxed);
    }

    uint8_t gameMode() const noexcept
    {
        return m_gameMode.load(std::memory_order_relaxed);
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

    std::int64_t connectedDurationSeconds() const noexcept;

    int serverPlayersOnline() const noexcept
    {
        return m_serverPlayersOnline.load(std::memory_order_relaxed);
    }

    int serverUptimeMinutes() const noexcept
    {
        return m_serverUptimeMinutes.load(std::memory_order_relaxed);
    }

    int serverTotalCells() const noexcept
    {
        return m_serverTotalCells.load(std::memory_order_relaxed);
    }

    int serverTotalMass() const noexcept
    {
        return m_serverTotalMass.load(std::memory_order_relaxed);
    }

    void setServerRealtimeBase(
        int playersOnline,
        int uptimeMinutes
    ) noexcept
    {
        m_serverPlayersOnline.store(
            playersOnline,
            std::memory_order_relaxed
        );

        m_serverUptimeMinutes.store(
            uptimeMinutes,
            std::memory_order_relaxed
        );
    }

    void setServerTotalCells(int value) noexcept
    {
        m_serverTotalCells.store(value, std::memory_order_relaxed);
    }

    void setServerTotalMass(int value) noexcept
    {
        m_serverTotalMass.store(value, std::memory_order_relaxed);
    }

    // Вызывается PacketHandler при получении ответа opcode 115.
    void handlePingResponse(
        uint32_t hash,
        std::chrono::steady_clock::time_point recvTime
    ) noexcept;
    void sendPlayerColor();

private:
    void sendNick();
    void sendDonate();
    void sendRaw(const uint8_t* data, size_t size);
    void sendHandshake();
    void sendPacket(uint8_t opcode);
    void sendPlayerPassword();
    void sendPing();

    void processEvents();

    void onOpen(
        std::chrono::steady_clock::time_point recvTime
    );

    void onClose();

    void onError(const std::string& reason);

    NetEventQueue m_events;
    std::vector<NetEvent> m_batch;

    std::chrono::steady_clock::time_point m_lastChatTime{};

    std::mutex m_pendingMutex;

    bool m_pendingNickResend = false;

    std::chrono::steady_clock::time_point
        m_nickResendTime{};

    std::string m_nickname;
    std::string m_playerPassword;

    // Последние отправленные значения.
    std::string m_lastSentNickname;
    std::string m_lastSentPlayerPassword;

    bool m_hasSentNickname = false;
    bool m_hasSentPlayerPassword = false;

    uint32_t m_donateId = 0;
    std::string m_donatePass;

    uint8_t m_playerColor = 0;

    ConnectionMode m_connectionMode =
        ConnectionMode::Play;

    std::atomic<uint8_t> m_gameMode{ 0 };

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
    std::atomic<std::int64_t> m_connectedAtMs{ -1 };
    std::atomic<int> m_serverPlayersOnline{ -1 };
    std::atomic<int> m_serverUptimeMinutes{ -1 };
    std::atomic<int> m_serverTotalCells{ -1 };
    std::atomic<int> m_serverTotalMass{ -1 };
};
