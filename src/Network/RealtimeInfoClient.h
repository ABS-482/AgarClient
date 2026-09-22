#pragma once

#include "../Game/ServerListEntry.h"

#include <ixwebsocket/IXWebSocket.h>

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// Realtime-обновление online серверов через RTS WebSocket сервера
// (аналог Java RealtimeInfoSocket). Ничем, кроме online/totalOnline,
// не занимается: список серверов по-прежнему получает и владеет им
// ServerListFetcher/main.cpp, этот класс только правит поля в уже
// существующем std::vector<ServerListEntry> по ссылке.
class RealtimeInfoClient
{
public:
    RealtimeInfoClient();
    ~RealtimeInfoClient();

    // serverList должен жить не меньше, чем сам RealtimeInfoClient.
    // Строит внутренний id -> index кэш один раз (список после fetch()
    // не меняется по составу — меняются только поля внутри элементов).
    void setServerList(std::vector<ServerListEntry>& serverList);

    void connect();
    void disconnect();

    // Вызывать раз в кадр из главного цикла. Переносит то, что успело
    // накопиться в фоновом потоке ixWebSocket, в serverList — вся
    // запись в ServerListEntry происходит только отсюда, из main thread.
    void update();

    int totalOnline() const { return m_totalOnline; }

private:
    void handleMessage(const std::string& json);

    ix::WebSocket m_webSocket;

    std::vector<ServerListEntry>* m_serverList = nullptr;
    std::unordered_map<std::string, size_t> m_idToIndex;

    std::mutex m_pendingMutex;
    std::optional<int> m_pendingTotal;
    std::unordered_map<std::string, int> m_pendingOnlineById;

    int m_totalOnline = 0;
};