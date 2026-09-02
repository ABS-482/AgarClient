#include "World.h"

Blob& World::getOrCreateBlob(uint32_t id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_blobs.find(id);

    if (it != m_blobs.end())
        return it->second;

    Blob blob;
    blob.id = id;

    auto result = m_blobs.emplace(id, blob);
    return result.first->second;
}

void World::removeBlob(uint32_t id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_blobs.erase(id);
}

std::unordered_map<uint32_t, Blob> World::snapshot() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_blobs; // копия под локом — короткая блокировка
}