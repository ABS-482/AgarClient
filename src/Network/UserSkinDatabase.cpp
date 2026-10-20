#include "UserSkinDatabase.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <charconv>
#include <cctype>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

namespace
{
    constexpr char kPersonalSkinsPath[] =
        "/engine/formobile/serverskins_pers_nocode_case.txt";

    constexpr char kClanSkinsPath[] =
        "/engine/formobile/serverskins_clan_nocode.txt";

    constexpr char kNickIdsPath[] =
        "/engine/formobile/nicksidsmobile_case.txt";

    constexpr char kClanIdsPath[] =
        "/engine/formobile/clansidsmobile.txt";

    constexpr std::size_t kMaxResponseSize =
        64u * 1024u * 1024u;

    constexpr int kHostAttempts = 3;

    std::string trimAscii(std::string value)
    {
        const auto isSpace = [](unsigned char c)
            {
                return std::isspace(c) != 0;
            };

        while (!value.empty() &&
            isSpace(static_cast<unsigned char>(value.front())))
        {
            value.erase(value.begin());
        }

        while (!value.empty() &&
            isSpace(static_cast<unsigned char>(value.back())))
        {
            value.pop_back();
        }

        return value;
    }

    bool parsePositiveId(
        const std::string& text,
        uint32_t& result
    )
    {
        const std::string value = trimAscii(text);

        if (value.empty())
            return false;

        uint32_t id = 0;

        const char* begin = value.data();
        const char* end = begin + value.size();

        const auto parsed = std::from_chars(begin, end, id);

        if (parsed.ec != std::errc{} ||
            parsed.ptr != end ||
            id == 0)
        {
            return false;
        }

        result = id;
        return true;
    }

    std::wstring widenAscii(const std::string& value)
    {
        return std::wstring(value.begin(), value.end());
    }

    void closeHandle(HINTERNET& handle)
    {
        if (handle)
        {
            WinHttpCloseHandle(handle);
            handle = nullptr;
        }
    }
}

UserSkinDatabase::UserSkinDatabase(std::string proxyMode)
    : m_proxyMode(std::move(proxyMode))
{
    if (m_proxyMode != "rus" && m_proxyMode != "cf")
        m_proxyMode.clear();

    // Сеть не блокирует главный игровой поток.
    m_worker = std::thread(
        &UserSkinDatabase::loadWorker,
        this
    );
}

UserSkinDatabase::~UserSkinDatabase()
{
    m_stopping.store(true, std::memory_order_release);

    if (m_worker.joinable())
        m_worker.join();
}

std::wstring UserSkinDatabase::buildDataHost(int number) const
{
    std::wstring host = L"data";
    host += std::to_wstring(number);

    if (m_proxyMode == "rus")
        host += L"ru";
    else if (m_proxyMode == "cf")
        host += L"cf";

    host += L".petridish.info";

    return host;
}

bool UserSkinDatabase::httpGet(
    const std::wstring& host,
    const std::wstring& path,
    std::string& contents
)
{
    contents.clear();

    HINTERNET session = WinHttpOpen(
        L"AgarClient/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!session)
        return false;

    // Ограничиваем время ожидания сетевых операций.
    WinHttpSetTimeouts(
        session,
        3000,
        3000,
        5000,
        5000
    );

    HINTERNET connection = WinHttpConnect(
        session,
        host.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT,
        0
    );

    if (!connection)
    {
        closeHandle(session);
        return false;
    }

    HINTERNET request = WinHttpOpenRequest(
        connection,
        L"GET",
        path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );

    if (!request)
    {
        closeHandle(connection);
        closeHandle(session);
        return false;
    }

    // Если сервер отдаёт сжатый ответ, WinHTTP его распакует.
    DWORD decompression =
        WINHTTP_DECOMPRESSION_FLAG_GZIP |
        WINHTTP_DECOMPRESSION_FLAG_DEFLATE;

    WinHttpSetOption(
        request,
        WINHTTP_OPTION_DECOMPRESSION,
        &decompression,
        sizeof(decompression)
    );

    bool success = WinHttpSendRequest(
        request,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    ) != FALSE;

    if (success)
    {
        success = WinHttpReceiveResponse(
            request,
            nullptr
        ) != FALSE;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);

    if (success)
    {
        success = WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_STATUS_CODE |
            WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode,
            &statusCodeSize,
            WINHTTP_NO_HEADER_INDEX
        ) != FALSE;

        success = success && statusCode == 200;
    }

    if (success)
    {
        char buffer[16384];

        while (!m_stopping.load(std::memory_order_acquire))
        {
            DWORD available = 0;

            if (!WinHttpQueryDataAvailable(
                request,
                &available))
            {
                success = false;
                break;
            }

            if (available == 0)
                break;

            while (available > 0)
            {
                const DWORD toRead =
                    (std::min)(
                        available,
                        static_cast<DWORD>(sizeof(buffer))
                        );

                DWORD downloaded = 0;

                if (!WinHttpReadData(
                    request,
                    buffer,
                    toRead,
                    &downloaded))
                {
                    success = false;
                    break;
                }

                if (downloaded == 0)
                {
                    success = false;
                    break;
                }

                if (contents.size() + downloaded >
                    kMaxResponseSize)
                {
                    success = false;
                    break;
                }

                contents.append(buffer, downloaded);
                available -= downloaded;
            }

            if (!success)
                break;
        }
    }

    closeHandle(request);
    closeHandle(connection);
    closeHandle(session);

    return success && !contents.empty();
}

bool UserSkinDatabase::downloadTable(
    const std::string& path,
    std::string& contents
)
{
    const std::wstring widePath = widenAscii(path);

    std::random_device rd;
    std::mt19937 rng(rd());

    std::uniform_int_distribution<int> distribution(1, 9);

    const int firstHost = distribution(rng);

    for (int attempt = 0;
        attempt < kHostAttempts;
        ++attempt)
    {
        if (m_stopping.load(std::memory_order_acquire))
            return false;

        // Пробуем разные data-хосты, начиная со случайного.
        const int hostNumber =
            ((firstHost - 1 + attempt) % 9) + 1;

        const std::wstring host =
            buildDataHost(hostNumber);

        if (httpGet(host, widePath, contents))
        {
            std::string narrowHost;
            narrowHost.reserve(host.size());

            for (wchar_t ch : host)
            {
                // Имя хоста состоит только из ASCII-символов.
                narrowHost.push_back(static_cast<char>(ch));
            }

            std::clog
                << "[UserSkinDatabase] Downloaded "
                << path
                << " from "
                << narrowHost
                << ", bytes="
                << contents.size()
                << '\n';

            return true;
        }

        contents.clear();
    }

    std::cerr
        << "[UserSkinDatabase] Failed to download "
        << path << '\n';

    return false;
}

std::string UserSkinDatabase::normalizeNick(
    std::string_view nickname
)
{
    if (nickname.empty())
        return {};

    // Java String.toLowerCase() работает с Unicode.
    // На Windows используем UTF-8 -> UTF-16 -> lowercase -> UTF-8.
    if (nickname.size() <=
        static_cast<std::size_t>(
            (std::numeric_limits<int>::max)()))
    {
        const int inputLength =
            static_cast<int>(nickname.size());

        const int wideLength = MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            nickname.data(),
            inputLength,
            nullptr,
            0
        );

        if (wideLength > 0)
        {
            std::vector<wchar_t> wide(
                static_cast<std::size_t>(wideLength)
            );

            if (MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                nickname.data(),
                inputLength,
                wide.data(),
                wideLength) == wideLength)
            {
                const int lowerLength = LCMapStringEx(
                    LOCALE_NAME_INVARIANT,
                    LCMAP_LOWERCASE,
                    wide.data(),
                    wideLength,
                    nullptr,
                    0,
                    nullptr,
                    nullptr,
                    0
                );

                if (lowerLength > 0)
                {
                    std::vector<wchar_t> lower(
                        static_cast<std::size_t>(lowerLength)
                    );

                    if (LCMapStringEx(
                        LOCALE_NAME_INVARIANT,
                        LCMAP_LOWERCASE,
                        wide.data(),
                        wideLength,
                        lower.data(),
                        lowerLength,
                        nullptr,
                        nullptr,
                        0) == lowerLength)
                    {
                        const int utf8Length =
                            WideCharToMultiByte(
                                CP_UTF8,
                                0,
                                lower.data(),
                                lowerLength,
                                nullptr,
                                0,
                                nullptr,
                                nullptr
                            );

                        if (utf8Length > 0)
                        {
                            std::string result(
                                static_cast<std::size_t>(utf8Length),
                                '\0'
                            );

                            if (WideCharToMultiByte(
                                CP_UTF8,
                                0,
                                lower.data(),
                                lowerLength,
                                result.data(),
                                utf8Length,
                                nullptr,
                                nullptr) == utf8Length)
                            {
                                return result;
                            }
                        }
                    }
                }
            }
        }
    }

    // Резервный вариант для обычных ASCII-ников.
    std::string result(nickname);

    for (char& c : result)
    {
        c = static_cast<char>(
            std::tolower(static_cast<unsigned char>(c))
            );
    }

    return result;
}

std::unordered_map<std::string, uint32_t>
UserSkinDatabase::parsePersonalTable(
    const std::string& contents
)
{
    std::unordered_map<std::string, uint32_t> result;

    std::size_t lineStart = 0;

    while (lineStart < contents.size())
    {
        const std::size_t lineEnd =
            contents.find('\n', lineStart);

        const std::size_t actualEnd =
            lineEnd == std::string::npos
            ? contents.size()
            : lineEnd;

        std::string line = contents.substr(
            lineStart,
            actualEnd - lineStart
        );

        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        const std::size_t separator = line.find(':');

        if (separator != std::string::npos && separator > 0)
        {
            const std::string nickname =
                normalizeNick(
                    std::string_view(line).substr(0, separator)
                );

            uint32_t skinId = 0;

            if (!nickname.empty() &&
                parsePositiveId(
                    line.substr(separator + 1),
                    skinId))
            {
                // Повторный ник перезаписывает предыдущую запись.
                result[nickname] = skinId;
            }
        }

        if (lineEnd == std::string::npos)
            break;

        lineStart = lineEnd + 1;
    }

    return result;
}

std::vector<std::pair<std::string, uint32_t>>
UserSkinDatabase::parseClanTable(
    const std::string& contents
)
{
    using ClanEntry = std::pair<std::string, uint32_t>;

    std::vector<ClanEntry> result;

    // Храним позиции, чтобы повторный префикс обновлял skinId,
    // но сохранял первоначальный порядок в таблице.
    std::unordered_map<std::string, std::size_t> indices;

    std::size_t lineStart = 0;

    while (lineStart < contents.size())
    {
        const std::size_t lineEnd =
            contents.find('\n', lineStart);

        const std::size_t actualEnd =
            lineEnd == std::string::npos
            ? contents.size()
            : lineEnd;

        std::string line = contents.substr(
            lineStart,
            actualEnd - lineStart
        );

        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        const std::size_t separator = line.find(':');

        if (separator != std::string::npos && separator > 0)
        {
            const std::string prefix =
                normalizeNick(
                    std::string_view(line).substr(0, separator)
                );

            uint32_t skinId = 0;

            if (!prefix.empty() &&
                parsePositiveId(
                    line.substr(separator + 1),
                    skinId))
            {
                const auto it = indices.find(prefix);

                if (it != indices.end())
                {
                    result[it->second].second = skinId;
                }
                else
                {
                    indices.emplace(prefix, result.size());
                    result.emplace_back(prefix, skinId);
                }
            }
        }

        if (lineEnd == std::string::npos)
            break;

        lineStart = lineEnd + 1;
    }

    return result;
}

std::vector<std::string>
UserSkinDatabase::parseNamesFromIdTable(
    const std::string& contents
)
{
    std::vector<std::string> result;

    std::size_t lineStart = 0;

    while (lineStart < contents.size())
    {
        const std::size_t lineEnd =
            contents.find('\n', lineStart);

        const std::size_t actualEnd =
            lineEnd == std::string::npos
            ? contents.size()
            : lineEnd;

        std::string line = contents.substr(
            lineStart,
            actualEnd - lineStart
        );

        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        const std::size_t separator = line.find(':');

        if (separator != std::string::npos && separator > 0)
        {
            const std::string name = trimAscii(
                line.substr(0, separator)
            );

            uint32_t id = 0;

            if (!name.empty() &&
                parsePositiveId(
                    line.substr(separator + 1),
                    id))
            {
                result.push_back(normalizeNick(name));
            }
        }

        if (lineEnd == std::string::npos)
            break;

        lineStart = lineEnd + 1;
    }

    return result;
}

void UserSkinDatabase::loadWorker()
{
    std::string personalContents;
    std::string clanContents;
    std::string protectedNickContents;
    std::string protectedClanContents;

    const bool personalDownloaded = downloadTable(
        kPersonalSkinsPath,
        personalContents
    );

    if (m_stopping.load(std::memory_order_acquire))
    {
        m_loaded.store(true, std::memory_order_release);
        return;
    }

    const bool clansDownloaded = downloadTable(
        kClanSkinsPath,
        clanContents
    );

    if (m_stopping.load(std::memory_order_acquire))
    {
        m_loaded.store(true, std::memory_order_release);
        return;
    }

    // ------------------------------------------------------------
    // Таблицы защищённых паролем ников и кланов
    // ------------------------------------------------------------

    const bool protectedNicksDownloaded = downloadTable(
        kNickIdsPath,
        protectedNickContents
    );

    if (m_stopping.load(std::memory_order_acquire))
    {
        m_loaded.store(true, std::memory_order_release);
        return;
    }

    const bool protectedClansDownloaded = downloadTable(
        kClanIdsPath,
        protectedClanContents
    );

    if (m_stopping.load(std::memory_order_acquire))
    {
        m_loaded.store(true, std::memory_order_release);
        return;
    }

    auto personal = personalDownloaded
        ? parsePersonalTable(personalContents)
        : std::unordered_map<std::string, uint32_t>{};

    auto clans = clansDownloaded
        ? parseClanTable(clanContents)
        : std::vector<std::pair<std::string, uint32_t>>{};

    // ------------------------------------------------------------
    // Разбор таблиц защищённых ников и кланов
    // ------------------------------------------------------------

    auto protectedNickList = protectedNicksDownloaded
        ? parseNamesFromIdTable(protectedNickContents)
        : std::vector<std::string>{};

    auto protectedClanList = protectedClansDownloaded
        ? parseNamesFromIdTable(protectedClanContents)
        : std::vector<std::string>{};

    // Персональные ники храним в unordered_set для быстрого поиска.
    std::unordered_set<std::string> protectedNicks;
    protectedNicks.reserve(protectedNickList.size());

    for (auto& nick : protectedNickList)
    {
        protectedNicks.insert(std::move(nick));
    }

    // Нормализуем клановые префиксы.
    for (auto& prefix : protectedClanList)
    {
        prefix = normalizeNick(prefix);
    }

    // ------------------------------------------------------------
    // Публикация загруженных данных
    // ------------------------------------------------------------

    {
        std::unique_lock lock(m_mutex);

        m_personalSkins.swap(personal);
        m_clanSkins.swap(clans);

        m_passwordProtectedNicks.swap(protectedNicks);
        m_passwordProtectedClanPrefixes.swap(protectedClanList);
    }

    std::clog
        << "[UserSkinDatabase] Ready. Personal skins: "
        << m_personalSkins.size()
        << ", clan skins: "
        << m_clanSkins.size()
        << '\n';

    m_loaded.store(true, std::memory_order_release);
}

std::optional<uint32_t> UserSkinDatabase::findSkinId(
    std::string_view nickname
) const
{
    const std::string normalized = normalizeNick(nickname);

    if (normalized.empty())
        return std::nullopt;

    std::shared_lock lock(m_mutex);

    // В Java ColorPickerButton задаёт текстуру клана первой,
    // а затем персональную. Поэтому персональная имеет приоритет.
    const auto personalIt =
        m_personalSkins.find(normalized);

    if (personalIt != m_personalSkins.end())
        return personalIt->second;

    // Если персонального скина нет, ищем клановый префикс.
    // При совпадении нескольких префиксов берём самый длинный.
    const std::pair<std::string, uint32_t>* bestMatch = nullptr;

    for (const auto& entry : m_clanSkins)
    {
        const std::string& prefix = entry.first;

        if (prefix.size() > normalized.size())
            continue;

        if (normalized.compare(0, prefix.size(), prefix) != 0)
            continue;

        if (!bestMatch ||
            prefix.size() > bestMatch->first.size())
        {
            bestMatch = &entry;
        }
    }

    if (bestMatch)
        return bestMatch->second;

    return std::nullopt;
}

std::size_t UserSkinDatabase::personalSkinCount() const
{
    std::shared_lock lock(m_mutex);
    return m_personalSkins.size();
}

std::size_t UserSkinDatabase::clanSkinCount() const
{
    std::shared_lock lock(m_mutex);
    return m_clanSkins.size();
}

bool UserSkinDatabase::requiresPassword(
    std::string_view nickname
) const
{
    if (nickname.empty())
        return false;

    // Java Chat$Message исключает эти имена из проверки Skin.
    // Сравнение намеренно чувствительно к регистру,
    // как в Java String.equals().
    const bool isPasswordErrorNick =
        nickname == "Wrong password" ||
        nickname == "Bad clan pass";

    const std::string normalized = normalizeNick(nickname);

    if (normalized.empty())
        return false;

    std::shared_lock lock(m_mutex);

    // 1. Проверяем точное совпадение защищённого ника.
    if (!isPasswordErrorNick &&
        m_passwordProtectedNicks.find(normalized) !=
        m_passwordProtectedNicks.end())
    {
        return true;
    }

    // 2. Если ник не найден, проверяем защищённые префиксы кланов.
    for (const std::string& prefix :
        m_passwordProtectedClanPrefixes)
    {
        if (prefix.size() <= normalized.size() &&
            normalized.compare(0, prefix.size(), prefix) == 0)
        {
            return true;
        }
    }

    return false;
}