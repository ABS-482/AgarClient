#include "ServerListFetcher.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <random>
#include <sstream>

namespace
{
    // Универсальный разбор одного атрибута вида key='val', key="val" или key=val
    // начиная с позиции pos. Возвращает false, если атрибутов больше нет
    // (достигнут конец тега '>').
    bool parseNextAttribute(
        const std::string& text, size_t& pos,
        std::string& outKey, std::string& outValue
    )
    {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])))
            ++pos;

        if (pos >= text.size() || text[pos] == '>')
            return false;

        size_t keyStart = pos;

        while (pos < text.size() && text[pos] != '=' &&
            !std::isspace(static_cast<unsigned char>(text[pos])) && text[pos] != '>')
        {
            ++pos;
        }

        outKey = text.substr(keyStart, pos - keyStart);

        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])))
            ++pos;

        if (pos >= text.size() || text[pos] != '=')
        {
            outValue.clear();
            return true; // атрибут без значения — пропускаем как пустой
        }

        ++pos; // пропустить '='

        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])))
            ++pos;

        if (pos < text.size() && (text[pos] == '\'' || text[pos] == '"'))
        {
            char quote = text[pos];
            ++pos;
            size_t valStart = pos;

            while (pos < text.size() && text[pos] != quote)
                ++pos;

            outValue = text.substr(valStart, pos - valStart);

            if (pos < text.size())
                ++pos; // пропустить закрывающую кавычку
        }
        else
        {
            size_t valStart = pos;

            while (pos < text.size() &&
                !std::isspace(static_cast<unsigned char>(text[pos])) && text[pos] != '>')
            {
                ++pos;
            }

            outValue = text.substr(valStart, pos - valStart);
        }

        return true;
    }

    int toInt(const std::string& s)
    {
        try { return std::stoi(s); }
        catch (...) { return 0; }
    }
}

std::vector<ServerListEntry> ServerListFetcher::parse(const std::string& html)
{
    std::vector<ServerListEntry> result;

    size_t pos = 0;

    while (true)
    {
        size_t tagStart = html.find("<option", pos);

        if (tagStart == std::string::npos)
            break;

        size_t tagEnd = html.find('>', tagStart);

        if (tagEnd == std::string::npos)
            break;

        size_t closeStart = html.find("</option>", tagEnd);

        std::string innerText = (closeStart != std::string::npos)
            ? html.substr(tagEnd + 1, closeStart - tagEnd - 1)
            : "";

        ServerListEntry entry;
        entry.displayText = innerText;

        size_t attrPos = tagStart + 7; // длина "<option"

        std::string key, value;

        while (parseNextAttribute(html, attrPos, key, value))
        {
            if (key == "id") entry.id = value;
            else if (key == "autorun") entry.autorun = value;
            else if (key == "sname") entry.sname = value;
            else if (key == "descr") entry.descr = value;
            else if (key == "location") entry.location = value;
            else if (key == "mode") entry.mode = value;
            else if (key == "map") entry.map = value;
            else if (key == "food") entry.food = value;
            else if (key == "status") entry.status = value;
            else if (key == "value") entry.address = value;
            else if (key == "modenumber") entry.modenumber = toInt(value);
            else if (key == "online") entry.online = toInt(value);
            else if (key == "connectlimit") entry.connectlimit = toInt(value);
            else if (key == "top") entry.top = toInt(value);
            else if (key == "ai") entry.ai = toInt(value);
        }

        if (!entry.id.empty() && entry.id != "none")
        {
            result.push_back(std::move(entry));
        }

        pos = (closeStart != std::string::npos) ? closeStart + 9 : tagEnd + 1;
    }

    return result;
}

bool ServerListFetcher::downloadText(const std::string& url, std::string& outText)
{
    URL_COMPONENTS urlComp{};
    urlComp.dwStructSize = sizeof(urlComp);

    wchar_t hostName[256]{};
    wchar_t urlPath[2048]{};

    urlComp.lpszHostName = hostName;
    urlComp.dwHostNameLength = 256;
    urlComp.lpszUrlPath = urlPath;
    urlComp.dwUrlPathLength = 2048;

    std::wstring wideUrl(url.begin(), url.end());

    if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &urlComp))
        return false;

    HINTERNET session = WinHttpOpen(
        L"AgarClient/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!session)
        return false;

    HINTERNET connection = WinHttpConnect(session, hostName, urlComp.nPort, 0);

    if (!connection)
    {
        WinHttpCloseHandle(session);
        return false;
    }

    bool isHttps = (urlComp.nScheme == INTERNET_SCHEME_HTTPS);

    HINTERNET request = WinHttpOpenRequest(
        connection, L"GET", urlPath,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        isHttps ? WINHTTP_FLAG_SECURE : 0
    );

    if (!request)
    {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    bool success =
        WinHttpSendRequest(
            request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0
        ) &&
        WinHttpReceiveResponse(request, nullptr);

    if (success)
    {
        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);

        WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX
        );

        if (statusCode == 200)
        {
            DWORD bytesAvailable = 0;

            while (WinHttpQueryDataAvailable(request, &bytesAvailable) && bytesAvailable > 0)
            {
                std::vector<char> buffer(bytesAvailable);
                DWORD bytesRead = 0;

                if (WinHttpReadData(request, buffer.data(), bytesAvailable, &bytesRead))
                {
                    outText.append(buffer.data(), bytesRead);
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            success = false;
        }
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return success && !outText.empty();
}

std::vector<ServerListEntry> ServerListFetcher::fetch()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(1, 9);

    int dataServer = dist(gen);

    std::string url =
        "https://data" + std::to_string(dataServer) +
        ".petridish.info/engine/formobile/serversnew_full_en.txt";

    std::string text;

    if (!downloadText(url, text))
    {
        std::cerr << "ServerListFetcher: failed to download " << url << '\n';
        return {};
    }

    std::cout << "\n===== RAW SERVER RESPONSE =====\n";
    std::cout << text << '\n';
    std::cout << "===== END RAW SERVER RESPONSE =====\n\n";

    return parse(text);
}