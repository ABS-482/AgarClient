#include "NetworkClient.h"

#include <iostream>
#include <random>

NetworkClient::NetworkClient(PacketHandler& handler)
    : m_handler(handler)
{
    ix::SocketTLSOptions tlsOptions;
    tlsOptions.caFile = "NONE";
    m_webSocket.setTLSOptions(tlsOptions);

    ix::WebSocketHttpHeaders headers;
    headers["Origin"] = "http://petridish.pw";
    m_webSocket.setExtraHeaders(headers);

    m_webSocket.setOnMessageCallback(
        [this](const ix::WebSocketMessagePtr& msg)
        {
            switch (msg->type)
            {
            case ix::WebSocketMessageType::Open:
                m_connected = true;
                std::cout << "WebSocket connected.\n";
                sendHandshake();
                requestSpectate();
                sendPlayerPassword();
                break;

            case ix::WebSocketMessageType::Close:
                m_connected = false;
                std::cout << "WebSocket closed.\n";
                break;

            case ix::WebSocketMessageType::Error:
                std::cerr << "WebSocket error: "
                    << msg->errorInfo.reason << '\n';
                break;

            case ix::WebSocketMessageType::Message:
                if (msg->binary)
                {
                    m_handler.handleMessage(
                        reinterpret_cast<const uint8_t*>(msg->str.data()),
                        msg->str.size()
                    );
                }
                break;

            default:
                break;
            }
        }
    );
}

NetworkClient::~NetworkClient()
{
    disconnect();
}

void NetworkClient::connect(const std::string& url, const std::string& serverPass)
{
    std::string fullUrl = url + "/connect?serverpass=" + serverPass;
    m_webSocket.setUrl(fullUrl);
    m_webSocket.start();
}

void NetworkClient::disconnect()
{
    m_webSocket.stop();
    m_connected = false;
}

void NetworkClient::sendRaw(const uint8_t* data, size_t size)
{
    m_webSocket.sendBinary(
        std::string(reinterpret_cast<const char*>(data), size)
    );
}

void NetworkClient::sendHandshake()
{
    constexpr uint32_t snurmd = 102;
    constexpr uint32_t tokernad = 77897631;

    // Пакет 1: opcode 254 + snurmd (uint32 LE)
    {
        uint8_t buf[5];
        buf[0] = 254;
        buf[1] = static_cast<uint8_t>(snurmd & 0xFF);
        buf[2] = static_cast<uint8_t>((snurmd >> 8) & 0xFF);
        buf[3] = static_cast<uint8_t>((snurmd >> 16) & 0xFF);
        buf[4] = static_cast<uint8_t>((snurmd >> 24) & 0xFF);
        sendRaw(buf, sizeof(buf));
    }

    // Пакет 2: opcode 255 + tokernad (uint32 LE)
    {
        uint8_t buf[5];
        buf[0] = 255;
        buf[1] = static_cast<uint8_t>(tokernad & 0xFF);
        buf[2] = static_cast<uint8_t>((tokernad >> 8) & 0xFF);
        buf[3] = static_cast<uint8_t>((tokernad >> 16) & 0xFF);
        buf[4] = static_cast<uint8_t>((tokernad >> 24) & 0xFF);
        sendRaw(buf, sizeof(buf));
    }

    // Пакет 3: opcode 253 + версия клиента (15)
    {
        uint8_t buf[2] = { 253, 15 };
        sendRaw(buf, sizeof(buf));
    }

    std::cout << "Handshake sent (snurmd=" << snurmd
        << ", tokernad=" << tokernad << ")\n";
}

void NetworkClient::sendPacket(uint8_t opcode)
{
    sendRaw(&opcode, 1);
}

void NetworkClient::requestSpectate()
{
    sendPacket(1);
}

void NetworkClient::sendPlayerPassword()
{
    std::vector<uint8_t> buf;
    buf.reserve(1 + 2 * m_playerPassword.size());
    buf.push_back(77);

    for (char c : m_playerPassword)
    {
        uint16_t code = static_cast<uint16_t>(static_cast<unsigned char>(c));
        buf.push_back(static_cast<uint8_t>(code & 0xFF));
        buf.push_back(static_cast<uint8_t>((code >> 8) & 0xFF));
    }

    sendRaw(buf.data(), buf.size());
}