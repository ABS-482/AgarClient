#include "NetworkClient.h"

#include <iostream>
#include <cstring>
#include <random>

namespace
{
    // UTF-8 → UTF-16LE, с поддержкой суррогатных пар — честный аналог
    // того, что JS-строка уже даёт "бесплатно" через charCodeAt.
    void appendUtf16LE(std::vector<uint8_t>& buf, const std::string& utf8Text)
    {
        size_t i = 0;

        while (i < utf8Text.size())
        {
            unsigned char c0 = static_cast<unsigned char>(utf8Text[i]);
            uint32_t codepoint = 0;
            size_t extra = 0;

            if (c0 < 0x80) { codepoint = c0; extra = 0; }
            else if ((c0 & 0xE0) == 0xC0) { codepoint = c0 & 0x1F; extra = 1; }
            else if ((c0 & 0xF0) == 0xE0) { codepoint = c0 & 0x0F; extra = 2; }
            else if ((c0 & 0xF8) == 0xF0) { codepoint = c0 & 0x07; extra = 3; }
            else { ++i; continue; }

            if (i + extra >= utf8Text.size())
                break;

            bool valid = true;

            for (size_t k = 1; k <= extra; ++k)
            {
                unsigned char cc = static_cast<unsigned char>(utf8Text[i + k]);

                if ((cc & 0xC0) != 0x80) { valid = false; break; }

                codepoint = (codepoint << 6) | (cc & 0x3F);
            }

            i += extra + 1;

            if (!valid)
                continue;

            if (codepoint <= 0xFFFF)
            {
                uint16_t unit = static_cast<uint16_t>(codepoint);
                buf.push_back(static_cast<uint8_t>(unit & 0xFF));
                buf.push_back(static_cast<uint8_t>((unit >> 8) & 0xFF));
            }
            else
            {
                uint32_t v = codepoint - 0x10000;
                uint16_t high = static_cast<uint16_t>(0xD800 + (v >> 10));
                uint16_t low = static_cast<uint16_t>(0xDC00 + (v & 0x3FF));

                buf.push_back(static_cast<uint8_t>(high & 0xFF));
                buf.push_back(static_cast<uint8_t>((high >> 8) & 0xFF));
                buf.push_back(static_cast<uint8_t>(low & 0xFF));
                buf.push_back(static_cast<uint8_t>((low >> 8) & 0xFF));
            }
        }
    }
}

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

void NetworkClient::sendNick()
{
    if (m_nickname.empty())
        return;

    std::vector<uint8_t> buf;
    buf.push_back(0);

    appendUtf16LE(buf, m_nickname);

    sendRaw(buf.data(), buf.size());
}

void NetworkClient::sendDonate()
{
    if (m_donatePass.empty())
        return; // как и в JS — не отправляем, если mp == null/пусто

    std::vector<uint8_t> buf;
    buf.push_back(78);

    buf.push_back(static_cast<uint8_t>(m_donateId & 0xFF));
    buf.push_back(static_cast<uint8_t>((m_donateId >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((m_donateId >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((m_donateId >> 24) & 0xFF));

    appendUtf16LE(buf, m_donatePass);

    sendRaw(buf.data(), buf.size());
}

void NetworkClient::sendPlayerColor()
{
    uint8_t buf[2] = { 79, m_playerColor };
    sendRaw(buf, sizeof(buf));
}

void NetworkClient::requestSpawn()
{
    sendNick();
    sendPlayerPassword(); // уже реализовано ранее, переиспользуем — эквивалент doSendPass()
    sendDonate();
    sendPlayerColor();
    sendChat("***playerenter***");
    sendChat("***playerenter***");
}

void NetworkClient::sendChat(const std::string& text)
{
    if (text.empty() || text.size() >= 200)
        return;

    auto now = std::chrono::steady_clock::now();
    auto sinceLastChat = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_lastChatTime
    ).count();

    if (sinceLastChat < 5000)
        return;

    m_lastChatTime = now;

    std::vector<uint8_t> buf;
    buf.push_back(99);
    buf.push_back(0);

    appendUtf16LE(buf, text);

    sendRaw(buf.data(), buf.size());
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

void NetworkClient::sendAimPosition(double worldX, double worldY)
{
    uint8_t buf[21];
    buf[0] = 16;

    std::memcpy(buf + 1, &worldX, sizeof(double));
    std::memcpy(buf + 9, &worldY, sizeof(double));

    uint32_t zero = 0;
    std::memcpy(buf + 17, &zero, sizeof(uint32_t));

    sendRaw(buf, sizeof(buf));
}