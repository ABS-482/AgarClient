#include "World.h"

Blob& World::getOrCreateBlob(uint32_t id)
{
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
    m_blobs.erase(id);
}