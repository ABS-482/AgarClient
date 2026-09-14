#pragma once

#include "../Game/ServerListEntry.h"

#include <string>
#include <vector>

class ServerListFetcher
{
public:
    // Синхронный запрос — вызывать один раз при старте, до открытия
    // основного окна/цикла (так же, как загрузка шрифта — блокирующе,
    // но один раз, не в игровом цикле).
    static std::vector<ServerListEntry> fetch();

private:
    static bool downloadText(const std::string& url, std::string& outText);
    static std::vector<ServerListEntry> parse(const std::string& html);
};