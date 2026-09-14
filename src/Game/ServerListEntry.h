#pragma once

#include <string>

struct ServerListEntry
{
    std::string id;
    std::string autorun;
    std::string sname;
    std::string descr;
    std::string location;
    std::string mode;
    std::string map;
    std::string food;
    std::string status;
    std::string address; // атрибут "value" — готовый host:port

    int modenumber = 0;
    int online = 0;
    int connectlimit = 0;
    int top = 0;
    int ai = 0;

    std::string displayText; // текст внутри <option>...</option>
};