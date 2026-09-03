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
    void sendSpectatePosition(double worldX, double worldY);

    bool isConnected() const { return m_connected; }

private:
    void sendRaw(const uint8_t* data, size_t size);
    void sendHandshake();
    void sendPacket(uint8_t opcode);
    void sendPlayerPassword();

    std::string m_playerPassword;

    PacketHandler& m_handler;
    ix::WebSocket m_webSocket;
    bool m_connected = false;
};