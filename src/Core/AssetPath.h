#pragma once

#include <SDL3/SDL.h>

#include <filesystem>
#include <string>

namespace AssetPath
{
    // Корень, в котором лежит папка assets.
    // Порядок поиска:
    //   1) рядом с exe (релиз): <exeDir>/assets
    //   2) выше по дереву (запуск из build/Debug и т.п.): до 5 уровней вверх
    // Результат кэшируется.
    inline const std::filesystem::path& root()
    {
        static const std::filesystem::path cached = []
            {
                namespace fs = std::filesystem;

                const char* base = SDL_GetBasePath();   // SDL3: строка принадлежит SDL, освобождать не нужно
                fs::path dir = base ? fs::path(base) : fs::current_path();

                for (int i = 0; i < 6; ++i)
                {
                    if (fs::exists(dir / "assets"))
                        return dir;

                    if (!dir.has_parent_path() || dir.parent_path() == dir)
                        break;

                    dir = dir.parent_path();
                }

                return base ? fs::path(base) : fs::current_path();
            }();

        return cached;
    }

    // assetPath("fonts/arial.otf") -> <root>/assets/fonts/arial.otf
    inline std::string get(const std::string& relative)
    {
        return (root() / "assets" / relative).generic_string();
    }

    // Для путей, которые уже содержат "assets/..." (как gameModes[i].iconPath).
    inline std::string fromRoot(const std::string& relative)
    {
        return (root() / relative).generic_string();
    }
}