#pragma once

#include <glad/glad.h>

#include <string>
#include <unordered_map>

class ModeIconManager
{
public:
    ModeIconManager() = default;
    ~ModeIconManager();

    ModeIconManager(const ModeIconManager&) = delete;
    ModeIconManager& operator=(const ModeIconManager&) = delete;

    bool load(const std::string& id, const std::string& path);
    GLuint getTexture(const std::string& id) const;

private:
    std::unordered_map<std::string, GLuint> m_textures;
};