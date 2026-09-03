#include "SkinManager.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>

#include <stb_image.h>

#include <iostream>
#include <iterator>

#pragma comment(lib, "winhttp.lib")

std::string SkinManager::buildSkinUrl(uint32_t skinId) const
{
    std::string id = std::to_string(skinId);
    char lastDigit = id.back();

    return
        "http://skins" +
        std::string(1, lastDigit) +
        "cached.petri-dish.ru/engine/serverskins/" +
        id +
        ".png";
}

GLuint SkinManager::getTexture(uint32_t skinId)
{
    if (skinId == 0)
        return 0;

    auto it = m_textures.find(skinId);

    if (it != m_textures.end())
        return it->second;

    GLuint texture = loadTexture(skinId);

    if (texture != 0)
        m_textures.emplace(skinId, texture);

    return texture;
}

GLuint SkinManager::loadTexture(uint32_t skinId)
{
    std::vector<unsigned char> data;

    if (!downloadSkin(skinId, data))
        return 0;

    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* pixels = stbi_load_from_memory(
        data.data(),
        static_cast<int>(data.size()),
        &width,
        &height,
        &channels,
        4
    );

    if (!pixels)
    {
        std::cerr
            << "Failed to decode skin " << skinId
            << ": " << stbi_failure_reason()
            << '\n';

        return 0;
    }

    GLuint texture = 0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE
    );

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels
    );

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);

    std::cout
        << "Loaded skin " << skinId
        << " (" << width << "x" << height << ")"
        << '\n';

    return texture;
}

bool SkinManager::downloadSkin(
    uint32_t skinId,
    std::vector<unsigned char>& data
)
{
    std::string url = buildSkinUrl(skinId);

    std::wstring wideUrl(
        url.begin(),
        url.end()
    );

    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);

    wchar_t hostName[256]{};
    wchar_t urlPath[1024]{};

    components.lpszHostName = hostName;
    components.dwHostNameLength = static_cast<DWORD>(
        std::size(hostName)
        );

    components.lpszUrlPath = urlPath;
    components.dwUrlPathLength = static_cast<DWORD>(
        std::size(urlPath)
        );

    if (!WinHttpCrackUrl(
        wideUrl.c_str(),
        0,
        0,
        &components))
    {
        std::cerr
            << "Failed to parse skin URL: "
            << url
            << '\n';

        return false;
    }

    HINTERNET session = WinHttpOpen(
        L"AgarClient/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!session)
        return false;

    HINTERNET connection = WinHttpConnect(
        session,
        components.lpszHostName,
        components.nPort,
        0
    );

    if (!connection)
    {
        WinHttpCloseHandle(session);
        return false;
    }

    DWORD flags = 0;

    if (components.nScheme == INTERNET_SCHEME_HTTPS)
        flags |= WINHTTP_FLAG_SECURE;

    HINTERNET request = WinHttpOpenRequest(
        connection,
        L"GET",
        components.lpszUrlPath,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );

    if (!request)
    {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    BOOL sent = WinHttpSendRequest(
        request,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    );

    if (!sent ||
        !WinHttpReceiveResponse(request, nullptr))
    {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);

    if (!WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_STATUS_CODE |
        WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &statusCodeSize,
        WINHTTP_NO_HEADER_INDEX))
    {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    if (statusCode != 200)
    {
        std::cerr
            << "Skin " << skinId
            << " returned HTTP "
            << statusCode
            << '\n';

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    while (true)
    {
        DWORD available = 0;

        if (!WinHttpQueryDataAvailable(
            request,
            &available))
        {
            break;
        }

        if (available == 0)
            break;

        std::size_t oldSize = data.size();

        data.resize(oldSize + available);

        DWORD downloaded = 0;

        if (!WinHttpReadData(
            request,
            data.data() + oldSize,
            available,
            &downloaded))
        {
            data.resize(oldSize);
            break;
        }

        if (downloaded != available)
            data.resize(oldSize + downloaded);
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return !data.empty();
}

SkinManager::~SkinManager()
{
    for (const auto& [skinId, texture] : m_textures)
    {
        if (texture != 0)
            glDeleteTextures(1, &texture);
    }
}