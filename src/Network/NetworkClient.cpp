#include "NetworkClient.h"

#include <iostream>
#include <cstring>
#include <random>
#include <algorithm>
#include <cctype>
#include <limits>

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

    std::string urlEncodeQueryValue(const std::string& value)
    {
        constexpr char hex[] = "0123456789ABCDEF";

        std::string result;
        result.reserve(value.size());

        for (unsigned char c : value)
        {
            const bool unreserved =
                (c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') ||
                c == '-' || c == '_' || c == '.' || c == '~';

            if (unreserved)
            {
                result.push_back(static_cast<char>(c));
            }
            else
            {
                result.push_back('%');
                result.push_back(hex[c >> 4]);
                result.push_back(hex[c & 0x0F]);
            }
        }

        return result;
    }
}

NetworkClient::NetworkClient(PacketHandler& handler)
    : m_handler(handler)
{
    m_webSocket.disableAutomaticReconnection();

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
            {
                m_connected = true;
                m_connectionFailed = false;
                m_intentionalDisconnect = false;

                const auto connectedAtMs =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch()
                    ).count();

                m_connectedAtMs.store(
                    connectedAtMs,
                    std::memory_order_release
                );

                std::cout << "WebSocket connected.\n";

                sendHandshake();

                if (m_connectionMode == ConnectionMode::Play)
                {
                    requestPlay();
                }
                else
                {
                    requestSpectate();
                }

                break;
            }

            case ix::WebSocketMessageType::Close:
                m_connected = false;

                if (!m_intentionalDisconnect)
                    m_connectionFailed = true;

                m_connectedAtMs.store(-1, std::memory_order_release);

                std::cout << "WebSocket closed.\n";
                break;

            case ix::WebSocketMessageType::Error:
                m_connected = false;
                m_connectionFailed = true;

                m_connectedAtMs.store(-1, std::memory_order_release);

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

void NetworkClient::connect(
    const std::string& url,
    const std::string& serverPass,
    ConnectionMode mode
)
{
    m_currentUrl = url;
    m_connectionMode = mode;

    m_connectionFailed = false;
    m_intentionalDisconnect = false;
    m_lastPingAttempt = std::chrono::steady_clock::now();
    m_serverPlayersOnline.store(-1, std::memory_order_relaxed);
    m_serverUptimeMinutes.store(-1, std::memory_order_relaxed);
    m_serverTotalCells.store(-1, std::memory_order_relaxed);
    m_serverTotalMass.store(-1, std::memory_order_relaxed);
    m_lastPingHash.store(0, std::memory_order_relaxed);
    m_lastPingSentAtMs.store(0, std::memory_order_relaxed);
    m_pingMs.store(-1, std::memory_order_relaxed);

    std::string fullUrl =
        url + "?tynda=&serverpass=" +
        urlEncodeQueryValue(serverPass);

    m_webSocket.setUrl(fullUrl);
    m_webSocket.start();
}

void NetworkClient::disconnect()
{
    m_intentionalDisconnect = true;
    m_connectedAtMs.store(-1, std::memory_order_release);

    m_webSocket.stop();
    m_connected = false;
}

bool NetworkClient::consumeConnectionFailed()
{
    if (!m_connectionFailed)
        return false;

    m_connectionFailed = false;
    return true;
}

void NetworkClient::sendRaw(const uint8_t* data, size_t size)
{
    m_webSocket.sendBinary(
        std::string(reinterpret_cast<const char*>(data), size)
    );
}

void NetworkClient::sendPing()
{
    // Диапазон соответствует JS: Math.floor(Math.random() * 1000000).
    // Ноль исключён, чтобы не путать начальное состояние с запросом.
    static thread_local std::mt19937 generator{
        std::random_device{}()
    };

    std::uniform_int_distribution<uint32_t> distribution(
        1,
        999999
    );

    uint32_t hash;

    do
    {
        hash = distribution(generator);
    } while (
        hash == m_lastPingHash.load(
            std::memory_order_relaxed
        )
        );

    const auto now = std::chrono::steady_clock::now();

    const int64_t sentAtMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();

    // Сохраняем время и идентификатор до отправки.
    m_lastPingSentAtMs.store(
        sentAtMs,
        std::memory_order_relaxed
    );

    m_lastPingHash.store(
        hash,
        std::memory_order_release
    );

    // Opcode 115 + uint32 hash в little-endian.
    uint8_t packet[5];

    packet[0] = 115;
    packet[1] = static_cast<uint8_t>(hash & 0xFF);
    packet[2] = static_cast<uint8_t>((hash >> 8) & 0xFF);
    packet[3] = static_cast<uint8_t>((hash >> 16) & 0xFF);
    packet[4] = static_cast<uint8_t>((hash >> 24) & 0xFF);

    sendRaw(packet, sizeof(packet));
}

void NetworkClient::handlePingResponse(uint32_t hash) noexcept
{
    const uint32_t expectedHash =
        m_lastPingHash.load(
            std::memory_order_acquire
        );

    // Игнорируем устаревший или не соответствующий запросу ответ.
    if (hash != expectedHash || hash == 0)
        return;

    const int64_t sentAtMs =
        m_lastPingSentAtMs.load(
            std::memory_order_relaxed
        );

    if (sentAtMs <= 0)
        return;

    const int64_t nowMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();

    const int64_t elapsedMs = nowMs - sentAtMs;

    if (elapsedMs < 0 ||
        elapsedMs > std::numeric_limits<int>::max())
    {
        return;
    }

    m_pingMs.store(
        static_cast<int>(elapsedMs),
        std::memory_order_relaxed
    );
}

std::int64_t NetworkClient::connectedDurationSeconds() const noexcept
{
    const std::int64_t connectedAtMs =
        m_connectedAtMs.load(std::memory_order_acquire);

    if (connectedAtMs < 0)
        return -1;

    const std::int64_t nowMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();

    if (nowMs < connectedAtMs)
        return 0;

    return (nowMs - connectedAtMs) / 1000;
}

void NetworkClient::sendNick()
{
    std::string nick = m_nickname.empty() ? "PetriDish.pw" : m_nickname;

    std::string pass = m_playerPassword;
    std::transform(pass.begin(), pass.end(), pass.begin(),
        [](unsigned char c) { return std::tolower(c); });

    // m_playerColor: подставьте то же значение, что даёт #color в JS
    std::string full = nick + ":::::" + pass + ":::::" + std::to_string(m_playerColor);

    std::vector<uint8_t> buf;
    buf.push_back(0);
    appendUtf16LE(buf, full);
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
    requestSpawnFromSpectate();
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

    sendPlayerPassword();
    sendDonate();
    sendPlayerColor();

    sendChat("***playerenter***");
    sendChat("***playerenter***");
}

void NetworkClient::requestPlay()
{
    sendNick();
    sendPlayerPassword();
    sendDonate();
    sendPlayerColor();

    m_lastChatTime = std::chrono::steady_clock::now()
        - std::chrono::milliseconds(5001);

    sendChat("***playerenter***");
    sendChat("***playerenter***");
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

void NetworkClient::requestSplit()
{
    sendPacket(17);
}

void NetworkClient::requestEjectMass()
{
    sendPacket(21);
}

void NetworkClient::requestSpawnFromSpectate()
{
    sendPacket(2);
}

void NetworkClient::requestDelayedNickResend()
{
    std::lock_guard<std::mutex> lock(m_pendingMutex);
    m_pendingNickResend = true;
    m_nickResendTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
}

void NetworkClient::update()
{
    std::lock_guard<std::mutex> lock(m_pendingMutex);

    if (m_pendingNickResend && std::chrono::steady_clock::now() >= m_nickResendTime)
    {
        m_pendingNickResend = false;
        sendNick(); // приватный метод, но update() — тоже метод класса, доступ есть
    }
    const auto now = std::chrono::steady_clock::now();

    if (m_connected &&
        now - m_lastPingAttempt >= std::chrono::milliseconds(1000))
    {
        m_lastPingAttempt = now;
        sendPing();
    }
}