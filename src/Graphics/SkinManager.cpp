#include "SkinManager.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>

#include <stb_image.h>

#include <iostream>
#include <iterator>
#include <string>
#include <cstddef>
#include <utility>
#include "../Core/AssetPath.h"

namespace
{
    constexpr std::size_t MaxSkinDownloadBytes =
        2u * 1024u * 1024u;

    constexpr int MaxSkinDimension = 1024;
}

#pragma comment(lib, "winhttp.lib")

SkinManager::SkinManager(int workerThreads)
{
    GLuint bombTexture = loadLocalTexture(AssetPath::get("skins/bomb.png").c_str());

    m_textures.emplace(63895, bombTexture);

    m_httpSession = WinHttpOpen(
        L"AgarClient/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (m_httpSession)
    {
        const HINTERNET session =
            reinterpret_cast<HINTERNET>(m_httpSession);

        if (!WinHttpSetTimeouts(
            session,
            5000, // DNS-разрешение
            5000, // подключение
            5000, // отправка
            5000  // получение данных
        ))
        {
            std::cerr
                << "WinHttpSetTimeouts failed: "
                << GetLastError()
                << '\n';
        }
    }
    else
    {
        std::cerr
            << "WinHttpOpen failed: "
            << GetLastError()
            << '\n';
    }

    for (int i = 0; i < workerThreads; ++i)
        m_workers.emplace_back(&SkinManager::workerLoop, this);
}

SkinManager::~SkinManager()
{
    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_stopping = true;
    }

    m_requestCv.notify_all();

    for (auto& worker : m_workers)
    {
        if (worker.joinable())
            worker.join();
    }

    if (m_httpSession)
    {
        WinHttpCloseHandle(
            reinterpret_cast<HINTERNET>(m_httpSession)
        );

        m_httpSession = nullptr;
    }

    for (const auto& [skinId, texture] : m_textures)
    {
        if (texture != 0)
            glDeleteTextures(1, &texture);
    }
}

std::string SkinManager::buildSkinUrl(
    uint32_t skinId
) const
{
    const std::string id =
        std::to_string(skinId);

    const uint32_t shard =
        (skinId % 8u) + 1u;

    return
        "https://skins" +
        std::to_string(shard) +
        "cached.petridish.info/engine/serverskins/" +
        id +
        ".png";
}

GLuint SkinManager::loadLocalTexture(const char* path)
{
    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char* pixels = stbi_load(
        path,
        &width,
        &height,
        &channels,
        4
    );

    if (!pixels)
    {
        std::cerr
            << "Failed to load local skin: "
            << path
            << " : "
            << stbi_failure_reason()
            << '\n';

        return 0;
    }

    GLuint texture = 0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

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

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
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

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);

    return texture;
}

GLuint SkinManager::getTexture(uint32_t skinId)
{
    if (skinId == 0)
        return 0;

    auto it = m_textures.find(skinId);

    if (it != m_textures.end())
        return it->second;

    if (m_requested.find(skinId) == m_requested.end())
    {
        m_requested.insert(skinId);

        {
            std::lock_guard<std::mutex> lock(m_requestMutex);
            m_requestQueue.push(skinId);
        }

        m_requestCv.notify_one();
    }

    return 0; // пока не готова — этот кадр рисуем без скина
}

void SkinManager::processCompleted()
{
    constexpr std::size_t maxUploadsPerFrame = 3;

    std::queue<DecodedImage> ready;

    // Забираем не больше трёх завершённых загрузок за кадр.
    // Остальные остаются в очереди для следующих кадров.
    {
        std::lock_guard<std::mutex> lock(m_resultMutex);

        for (
            std::size_t i = 0;
            i < maxUploadsPerFrame && !m_resultQueue.empty();
            ++i
            )
        {
            ready.push(std::move(m_resultQueue.front()));
            m_resultQueue.pop();
        }
    }

    while (!ready.empty())
    {
        DecodedImage& img = ready.front();

        if (!img.pixels.empty())
        {
            GLuint texture = 0;

            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA8,
                img.width,
                img.height,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                img.pixels.data()
            );

            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(
                GL_TEXTURE_2D,
                GL_TEXTURE_MIN_FILTER,
                GL_LINEAR_MIPMAP_LINEAR
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

            glBindTexture(GL_TEXTURE_2D, 0);

            m_textures.emplace(img.skinId, texture);
        }
        else
        {
            // Ошибочные и отклонённые изображения кэшируем,
            // чтобы не запрашивать их повторно каждый кадр.
            m_textures.emplace(img.skinId, 0);

            std::cerr
                << "Failed to load skin "
                << img.skinId
                << '\n';
        }

        ready.pop();
    }
}

void SkinManager::workerLoop()
{
    while (!m_stopping)
    {
        uint32_t skinId = 0;

        {
            std::unique_lock<std::mutex> lock(m_requestMutex);

            m_requestCv.wait(lock, [this]
                {
                    return m_stopping || !m_requestQueue.empty();
                });

            if (m_stopping)
                return;

            skinId = m_requestQueue.front();
            m_requestQueue.pop();
        }

        DecodedImage result;
        result.skinId = skinId;

        std::vector<unsigned char> raw;

        if (downloadSkin(skinId, raw) && !raw.empty())
        {
            int width = 0;
            int height = 0;
            int channels = 0;

            // Сначала проверяем заголовок, ещё не декодируя всё изображение.
            const bool imageInfoValid =
                stbi_info_from_memory(
                    raw.data(),
                    static_cast<int>(raw.size()),
                    &width,
                    &height,
                    &channels
                ) != 0;

            const bool dimensionsValid =
                imageInfoValid &&
                width > 0 &&
                height > 0 &&
                width <= MaxSkinDimension &&
                height <= MaxSkinDimension;

            if (dimensionsValid)
            {
                int decodedWidth = 0;
                int decodedHeight = 0;
                int decodedChannels = 0;

                unsigned char* pixels = stbi_load_from_memory(
                    raw.data(),
                    static_cast<int>(raw.size()),
                    &decodedWidth,
                    &decodedHeight,
                    &decodedChannels,
                    4
                );

                if (pixels)
                {
                    // Дополнительно проверяем фактические размеры результата.
                    const bool decodedDimensionsValid =
                        decodedWidth > 0 &&
                        decodedHeight > 0 &&
                        decodedWidth <= MaxSkinDimension &&
                        decodedHeight <= MaxSkinDimension;

                    if (decodedDimensionsValid)
                    {
                        const std::size_t pixelCount =
                            static_cast<std::size_t>(decodedWidth) *
                            static_cast<std::size_t>(decodedHeight);

                        result.width = decodedWidth;
                        result.height = decodedHeight;

                        result.pixels.assign(
                            pixels,
                            pixels + pixelCount * 4u
                        );
                    }

                    stbi_image_free(pixels);
                }
            }

            if (result.pixels.empty())
            {
                std::cerr
                    << "Rejected invalid or oversized skin "
                    << skinId << '\n';
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_resultMutex);
            m_resultQueue.push(std::move(result));
        }
    }
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

    HINTERNET session =
        reinterpret_cast<HINTERNET>(m_httpSession);

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
        return false;
    }

    bool downloadSucceeded = true;

    while (true)
    {
        DWORD available = 0;

        if (!WinHttpQueryDataAvailable(request, &available))
        {
            downloadSucceeded = false;
            break;
        }

        if (available == 0)
            break;

        const std::size_t oldSize = data.size();
        const std::size_t availableSize =
            static_cast<std::size_t>(available);

        // Не позволяем накопить слишком большой файл.
        if (
            oldSize > MaxSkinDownloadBytes ||
            availableSize > MaxSkinDownloadBytes - oldSize
            )
        {
            std::cerr
                << "Skin " << skinId
                << " exceeds the download size limit\n";

            data.clear();
            downloadSucceeded = false;
            break;
        }

        data.resize(oldSize + availableSize);

        DWORD downloaded = 0;

        if (!WinHttpReadData(
            request,
            data.data() + oldSize,
            available,
            &downloaded))
        {
            data.resize(oldSize);
            downloadSucceeded = false;
            break;
        }

        data.resize(
            oldSize + static_cast<std::size_t>(downloaded)
        );
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);

    if (!downloadSucceeded)
    {
        data.clear();
        return false;
    }

    return !data.empty();
}