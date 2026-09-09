#pragma once

#include "PacketHandler.h"

#include <ixwebsocket/IXWebSocket.h>

#include <string>

class NetworkClient
{
public:
    explicit NetworkClient(PacketHandler& handler);
    ~NetworkClient();

    void connect(const std::string& url, const std::string& serverPass = "");
    void disconnect();
    void requestSpectate();
    void setPlayerPassword(const std::string& password) { m_playerPassword = password; }
    void sendAimPosition(double worldX, double worldY);
    void setNickname(const std::string& nickname) { m_nickname = nickname; }

    void setDonateCredentials(uint32_t donateId, const std::string& donatePass)
    {
        m_donateId = donateId;
        m_donatePass = donatePass;
    }

    void setPlayerColor(uint8_t colorIndex) { m_playerColor = colorIndex; }

    void requestSpawn();
    void sendChat(const std::string& text);

    bool isConnected() const { return m_connected; }

private:
    void sendNick();
    void sendDonate();
    void sendPlayerColor();

    std::string m_nickname;
    uint32_t m_donateId = 0;
    std::string m_donatePass;
    uint8_t m_playerColor = 0;
    void sendRaw(const uint8_t* data, size_t size);
    void sendHandshake();
    void sendPacket(uint8_t opcode);
    void sendPlayerPassword();
    std::chrono::steady_clock::time_point m_lastChatTime{};

    std::string m_playerPassword;

    PacketHandler& m_handler;
    ix::WebSocket m_webSocket;
    bool m_connected = false;
};