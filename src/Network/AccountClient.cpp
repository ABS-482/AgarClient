
#include "AccountClient.h"

#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace
{
    constexpr wchar_t UserAgent[] = L"AgarClient/0.1";
    constexpr DWORD TimeoutMs = 15000;

    constexpr wchar_t StandardUrl[] =
        L"https://donate.petridish.pw/master-login51.php";

    constexpr wchar_t MordaUrl[] =
        L"https://enter.petridish.pw/master-enter-ok.php";

    std::wstring widenAscii(const std::string& value)
    {
        return std::wstring(value.begin(), value.end());
    }

    std::string urlEncode(const std::string& value)
    {
        constexpr char hex[] = "0123456789ABCDEF";

        std::string out;
        out.reserve(value.size() * 3);

        for (unsigned char c : value)
        {
            const bool safe =
                (c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') ||
                c == '-' || c == '_' ||
                c == '.' || c == '~';

            if (safe)
            {
                out.push_back(static_cast<char>(c));
            }
            else
            {
                out.push_back('%');
                out.push_back(hex[c >> 4]);
                out.push_back(hex[c & 0x0F]);
            }
        }

        return out;
    }

    std::string makeBody(
        const std::string& id,
        const std::string& password)
    {
        return "action=list"
            "&id=" + urlEncode(id) +
            "&pass=" + urlEncode(password) +
            "&platform=Desktop"
            "&version=v4.5.5";
    }

    std::string readResponse(HINTERNET request)
    {
        std::string body;

        for (;;)
        {
            DWORD available = 0;

            if (!WinHttpQueryDataAvailable(request, &available))
                throw std::runtime_error("Failed to query HTTP response size");

            if (available == 0)
                break;

            const size_t oldSize = body.size();
            body.resize(oldSize + available);

            DWORD received = 0;

            if (!WinHttpReadData(
                request,
                body.data() + oldSize,
                available,
                &received))
            {
                body.resize(oldSize);
                throw std::runtime_error("Failed to read HTTP response");
            }

            body.resize(oldSize + received);

            if (received == 0)
                break;
        }

        return body;
    }

    std::string jsonValueToString(
        const nlohmann::json& value)
    {
        if (value.is_string())
            return value.get<std::string>();

        if (value.is_number())
            return value.dump();

        if (value.is_boolean())
            return value.get<bool>() ? "true" : "false";

        return {};
    }

    std::string getJsonString(
        const nlohmann::json& json,
        const char* key)
    {
        const auto it = json.find(key);

        if (it == json.end() || it->is_null())
            return {};

        return jsonValueToString(*it);
    }

    bool isAcceptedAccountStatus(const std::string& status)
    {
        return status == "200" ||
            status == "413" ||
            status == "902" ||
            status == "913";
    }

    std::string accountStatusMessage(
        const std::string& status,
        const std::string& description)
    {
        if (status == "803")
            return "Не найден файл HDSKIN.";

        if (status == "401")
            return "Не предоставлены все необходимые данные.";

        if (status == "402")
            return "Donate ID не найден.";

        if (status == "403")
            return "Требуется восстановление пароля.";

        if (status == "4031")
            return "Аккаунт заблокирован.";

        if (status == "405")
            return "Регистрация не удалась: недостаточно данных.";

        if (status == "406")
            return "Регистрация не удалась: пользователь уже существует.";

        if (status.empty() || status == "null")
            return "Операция не выполнена.";

        if (!description.empty())
            return description + " (" + status + ")";

        return "Операция не выполнена (" + status + ").";
    }
}

AccountClient::~AccountClient()
{
    if (m_worker.joinable())
        m_worker.join();
}

bool AccountClient::login(
    const std::string& donateId,
    const std::string& password,
    AccountLoginMethod method)
{
    std::lock_guard lock(m_mutex);

    if (m_busy)
        return false;

    if (donateId.empty() || password.empty())
    {
        m_status = AccountLoginStatus::Failed;
        m_lastResult = {
            AccountLoginStatus::Failed,
            {},
            "Введите Donate ID и пароль."
        };
        return false;
    }

    if (m_worker.joinable())
        m_worker.join();

    m_busy = true;
    m_status = AccountLoginStatus::InProgress;
    m_lastResult = {
        AccountLoginStatus::InProgress,
        {},
        "Выполняется вход..."
    };

    m_worker = std::thread(
        [this, donateId, password, method]
        {
            runLogin(donateId, password, method);
        });

    return true;
}

AccountClient::HttpResult AccountClient::postLogin(
    const std::string& url,
    const std::string& donateId,
    const std::string& password)
{
    HttpResult result;

    HINTERNET session = WinHttpOpen(
        UserAgent,
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!session)
    {
        result.error = "WinHttpOpen failed";
        return result;
    }

    WinHttpSetTimeouts(
        session,
        TimeoutMs,
        TimeoutMs,
        TimeoutMs,
        TimeoutMs);

    const std::wstring wideUrl = widenAscii(url);

    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = static_cast<DWORD>(-1);
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);

    if (!WinHttpCrackUrl(
        wideUrl.c_str(),
        static_cast<DWORD>(wideUrl.size()),
        0,
        &parts))
    {
        result.error = "Invalid login URL";
        WinHttpCloseHandle(session);
        return result;
    }

    const std::wstring host(
        parts.lpszHostName,
        parts.dwHostNameLength);

    std::wstring path(
        parts.lpszUrlPath,
        parts.dwUrlPathLength);

    if (parts.dwExtraInfoLength > 0)
    {
        path.append(
            parts.lpszExtraInfo,
            parts.dwExtraInfoLength);
    }

    HINTERNET connection = WinHttpConnect(
        session,
        host.c_str(),
        parts.nPort,
        0);

    if (!connection)
    {
        result.error = "WinHttpConnect failed";
        WinHttpCloseHandle(session);
        return result;
    }

    HINTERNET request = WinHttpOpenRequest(
        connection,
        L"POST",
        path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        parts.nScheme == INTERNET_SCHEME_HTTPS
        ? WINHTTP_FLAG_SECURE
        : 0);

    if (!request)
    {
        result.error = "WinHttpOpenRequest failed";
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return result;
    }

    const std::string body = makeBody(donateId, password);

    const wchar_t* headers =
        L"Content-Type: application/x-www-form-urlencoded\r\n";

    const BOOL sent = WinHttpSendRequest(
        request,
        headers,
        static_cast<DWORD>(-1),
        const_cast<char*>(body.data()),
        static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()),
        0);

    if (!sent || !WinHttpReceiveResponse(request, nullptr))
    {
        result.error = "HTTP request failed";
    }
    else
    {
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);

        if (WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_STATUS_CODE |
            WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode,
            &statusSize,
            WINHTTP_NO_HEADER_INDEX))
        {
            result.httpStatus = statusCode;
            try
            {
                result.body = readResponse(request);
            }
            catch (const std::exception& e)
            {
                result.error = e.what();
            }
        }
        else
        {
            result.error = "Failed to read HTTP status";
        }
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return result;
}

void AccountClient::runLogin(
    std::string donateId,
    std::string password,
    AccountLoginMethod method)
{
    AccountLoginResult finalResult;

    try
    {
        HttpResult response;

        if (method == AccountLoginMethod::Morda)
        {
            response = postLogin(
                "https://enter.petridish.pw/master-enter-ok.php",
                donateId,
                password
            );

            // В Java для Morda предусмотрен переход
            // на обычный способ входа в определённых случаях.
            bool fallback =
                !response.error.empty() ||
                response.httpStatus == 403 ||
                response.httpStatus == 404 ||
                response.httpStatus == 503;

            if (!response.body.empty())
            {
                try
                {
                    const auto json =
                        nlohmann::json::parse(response.body);

                    const std::string code =
                        getJsonString(json, "code");

                    if (code == "399.0" || code == "399")
                        fallback = true;
                }
                catch (const nlohmann::json::exception&)
                {
                    // Некорректный ответ сам по себе не является
                    // подтверждением необходимости fallback.
                    // Ошибки транспорта и HTTP проверяются отдельно.
                }
            }

            if (fallback)
            {
                response = postLogin(
                    "https://donate.petridish.pw/master-login51.php",
                    donateId,
                    password
                );
            }
        }
        else
        {
            response = postLogin(
                "https://donate.petridish.pw/master-login51.php",
                donateId,
                password
            );
        }

        // Сначала проверяем транспортный уровень.
        if (!response.error.empty())
        {
            finalResult = {
                AccountLoginStatus::Failed,
                {},
                response.error
            };
        }
        else if (response.httpStatus < 200 ||
            response.httpStatus >= 300)
        {
            finalResult = {
                AccountLoginStatus::Failed,
                {},
                "HTTP status: " +
                    std::to_string(response.httpStatus)
            };
        }
        else
        {
            // Успешный HTTP-ответ ещё не означает успешную
            // авторизацию: проверяем поле status в JSON.
            const auto json =
                nlohmann::json::parse(response.body);

            const std::string status =
                getJsonString(json, "status");

            const std::string description =
                getJsonString(json, "descr");

            if (isAcceptedAccountStatus(status))
            {
                finalResult = {
                    AccountLoginStatus::Success,
                    json,
                    "Сервер принял статус авторизации: " + status
                };
            }
            else
            {
                finalResult = {
                    AccountLoginStatus::Failed,
                    json,
                    accountStatusMessage(status, description)
                };
            }
        }
    }
    catch (const std::exception& e)
    {
        finalResult = {
            AccountLoginStatus::Failed,
            {},
            e.what()
        };
    }

    publishResult(std::move(finalResult));
}

void AccountClient::publishResult(AccountLoginResult result)
{
    std::lock_guard lock(m_mutex);
    m_results.push_back(std::move(result));
    m_busy = false;
}

void AccountClient::update()
{
    std::lock_guard lock(m_mutex);

    while (!m_results.empty())
    {
        m_lastResult = std::move(m_results.front());
        m_results.pop_front();
        m_status = m_lastResult.status;
    }
}

AccountLoginStatus AccountClient::status() const
{
    std::lock_guard lock(m_mutex);
    return m_status;
}

AccountLoginResult AccountClient::lastResult() const
{
    std::lock_guard lock(m_mutex);
    return m_lastResult;
}
