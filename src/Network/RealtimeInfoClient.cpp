#include "RealtimeInfoClient.h"

#include <nlohmann/json.hpp>

#include <cctype>
#include <iostream>

namespace
{
    // Endpoint зафиксирован задачей — не wss, не rts-cf.*.
    constexpr const char* kRtsUrl = "ws://rts.petridish.pw:8082";
    constexpr int kReconnectDelayMs = 5000;
}

RealtimeInfoClient::RealtimeInfoClient()
{
    m_webSocket.setUrl(kRtsUrl);

    // ixWebSocket по умолчанию сам переподключается с экспоненциальным
    // backoff'ом. Прижимаем min и max к одному и тому же значению —
    // получаем фиксированные 5 секунд, как в Java
    // (Threads.runAfter(5000L, ...)), без ручного таймера.
    m_webSocket.setMinWaitBetweenReconnectionRetries(kReconnectDelayMs);
    m_webSocket.setMaxWaitBetweenReconnectionRetries(kReconnectDelayMs);

    m_webSocket.setOnMessageCallback(
        [this](const ix::WebSocketMessagePtr& msg)
        {
            switch (msg->type)
            {
            case ix::WebSocketMessageType::Open:
                std::cout << "[RTS] Connected\n";
                // Обязательно на КАЖДОЕ новое подключение, в т.ч. после
                // автопереподключения — Open стреляет и тогда тоже.
                m_webSocket.send("1");
                std::cout << "[RTS] Sent: 1\n";
                break;

            case ix::WebSocketMessageType::Close:
                std::cout << "[RTS] Connection closed\n";
                std::cout << "[RTS] Reconnecting in 5 seconds\n";
                break;

            case ix::WebSocketMessageType::Error:
                std::cerr << "[RTS] error: " << msg->errorInfo.reason << '\n';
                break;

            case ix::WebSocketMessageType::Message:
                if (!msg->binary)
                {
                    std::cout << "[RTS] Message received\n";
                    handleMessage(msg->str);
                }
                break;

            default:
                break;
            }
        }
    );
}

RealtimeInfoClient::~RealtimeInfoClient()
{
    disconnect();
}

void RealtimeInfoClient::setServerList(std::vector<ServerListEntry>& serverList)
{
    m_serverList = &serverList;

    m_idToIndex.clear();
    m_idToIndex.reserve(serverList.size());

    for (size_t i = 0; i < serverList.size(); ++i)
        m_idToIndex[serverList[i].id] = i;
}

void RealtimeInfoClient::connect()
{
    std::cout << "[RTS] Connecting to " << kRtsUrl << '\n';
    m_webSocket.start();
}

void RealtimeInfoClient::disconnect()
{
    m_webSocket.stop();
}

// Вызывается из фонового потока ixWebSocket — только копит данные
// под мьютексом, никакой записи в serverList отсюда.
void RealtimeInfoClient::handleMessage(const std::string& json)
{
    nlohmann::json parsed;

    try
    {
        parsed = nlohmann::json::parse(json);
    }
    catch (const std::exception& e)
    {
        std::cerr << "[RTS] failed to parse JSON: " << e.what() << '\n';
        return;
    }

    if (!parsed.is_object())
        return;

    std::lock_guard<std::mutex> lock(m_pendingMutex);

    for (auto it = parsed.begin(); it != parsed.end(); ++it)
    {
        const std::string& key = it.key();

        if (key == "total")
        {
            if (it->is_number())
            {
                m_pendingTotal = it->get<int>();
            }
            else if (it->is_string())
            {
                try { m_pendingTotal = static_cast<int>(std::stof(it->get<std::string>())); }
                catch (...) {}
            }

            continue;
        }

        // "<id>-level" / "<id>-lastlevel" и подобные суффиксы —
        // supernovaLevel полей в ServerListEntry нет, пропускаем,
        // как и договаривались.
        if (key.find('-') != std::string::npos)
            continue;

        if (it->is_number())
        {
            m_pendingOnlineById[key] = it->get<int>();
        }
        else if (it->is_string() && !it->get<std::string>().empty())
        {
            try { m_pendingOnlineById[key] = std::stoi(it->get<std::string>()); }
            catch (...) {}
        }
    }
}

// Вызывается из главного потока раз в кадр.
void RealtimeInfoClient::update()
{
    std::optional<int> newTotal;
    std::unordered_map<std::string, int> newOnline;

    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);

        if (m_pendingTotal.has_value())
        {
            newTotal = m_pendingTotal;
            m_pendingTotal.reset();
        }

        if (!m_pendingOnlineById.empty())
            newOnline.swap(m_pendingOnlineById);
    }

    if (newTotal.has_value())
    {
        std::cout << "[RTS] Total online: " << *newTotal << '\n';
        m_totalOnline = *newTotal;
    }

    if (!m_serverList)
        return;

    for (const auto& [id, online] : newOnline)
    {
        auto found = m_idToIndex.find(id);

        if (found == m_idToIndex.end())
            continue;

        ServerListEntry& entry = (*m_serverList)[found->second];

        if (entry.online != online)
        {
            std::cout << "[RTS] Server " << id << ": "
                << entry.online << " -> " << online << '\n';

            entry.online = online;
        }
    }
}