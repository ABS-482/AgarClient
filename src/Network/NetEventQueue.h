
#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

struct NetEvent
{
    enum class Type : uint8_t
    {
        Open,
        Close,
        Error,
        Packet
    };

    Type type = Type::Packet;
    std::chrono::steady_clock::time_point recvTime{};
    std::string data;
    int code = 0;
};

class NetEventQueue
{
public:
    void push(NetEvent&& event)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_incoming.push_back(std::move(event));
    }

    // Забирает накопленные события одним обменом векторов.
    // out перед вызовом должен быть пустым.
    void drain(std::vector<NetEvent>& out)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        out.swap(m_incoming);
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_incoming.clear();
    }

private:
    std::mutex m_mutex;
    std::vector<NetEvent> m_incoming;
};
