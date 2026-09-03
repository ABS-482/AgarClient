#pragma once

#include <glad/glad.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class SkinManager
{
public:
    SkinManager() = default;
    ~SkinManager();

    SkinManager(const SkinManager&) = delete;
    SkinManager& operator=(const SkinManager&) = delete;

    GLuint getTexture(uint32_t skinId);

private:
    GLuint loadTexture(uint32_t skinId);
    bool downloadSkin(uint32_t skinId, std::vector<unsigned char>& data);
    std::string buildSkinUrl(uint32_t skinId) const;

    std::unordered_map<uint32_t, GLuint> m_textures;
};