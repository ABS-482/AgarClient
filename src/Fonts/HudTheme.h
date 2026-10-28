#pragma once

#include <string>

#include "UiFont.h"
#include "UiTextRenderer.h"

// Общая тема интерфейса: HUD, лидерборд, чат, меню.
// Положите файл рядом с UiFonts.h (папка Fonts) или поправьте include.

namespace Ui
{
    struct Rgba
    {
        float r, g, b, a;
    };

    // Тот же цвет, но с другой прозрачностью.
    constexpr Rgba withAlpha(const Rgba& c, float a)
    {
        return { c.r, c.g, c.b, a };
    }

    constexpr Rgba rgb8(int r, int g, int b, float a = 1.0f)
    {
        return { r / 255.0f, g / 255.0f, b / 255.0f, a };
    }

    enum class ThemeMode
    {
        Light,
        Dark
    };

    struct HudPalette
    {
        // --- Текст ---
        Rgba label;        // подписи (x:, mass:)
        Rgba value;        // основные значения
        Rgba accent;       // акцент (рекорд, выделение)
        Rgba textMuted;    // второстепенный текст (serverInfo, подсказки)

        // --- Тень / ореол текста ---
        Rgba  shadow;
        float shadowOffset;   // в пикселях

        // --- Панели (плашки, лидерборд, чат, меню) ---
        Rgba panelBg;
        Rgba panelBorder;
        Rgba panelAccentLine; // тонкая линия сверху плашки

        // --- Пинг по порогам ---
        Rgba pingGood;
        Rgba pingWarn;
        Rgba pingBad;
    };

    namespace Themes
    {
        inline constexpr HudPalette Light
        {
            /* label           */ rgb8(104, 113, 128),
            /* value           */ rgb8(30,  35,  48),
            /* accent          */ rgb8(166, 124,  46),
            /* textMuted       */ rgb8(120, 128, 142),

            /* shadow          */ { 1.0f, 1.0f, 1.0f, 0.0f },   // выключено
            /* shadowOffset    */ 1.0f,

            /* panelBg         */ rgb8(247, 248, 252, 0.92f),
            /* panelBorder     */ rgb8(176, 186, 202),
            /* panelAccentLine */ rgb8(166, 124,  46, 0.80f),

            /* pingGood        */ rgb8(46, 125,  80),
            /* pingWarn        */ rgb8(176, 120,  30),
            /* pingBad         */ rgb8(176,  60,  60)
        };

        inline constexpr HudPalette Dark
        {
            /* label           */ rgb8(148, 158, 173, 0.90f),
            /* value           */ rgb8(237, 232, 218),
            /* accent          */ rgb8(212, 175, 105),
            /* textMuted       */ rgb8(148, 158, 173, 0.75f),

            /* shadow          */ { 0.0f, 0.0f, 0.0f, 0.55f },
            /* shadowOffset    */ 1.0f,

            /* panelBg         */ rgb8(10,  12,  18, 0.45f),
            /* panelBorder     */ rgb8(255, 255, 255, 0.10f),
            /* panelAccentLine */ rgb8(212, 175, 105, 0.80f),

            /* pingGood        */ rgb8(120, 190, 140),
            /* pingWarn        */ rgb8(220, 180, 100),
            /* pingBad         */ rgb8(210, 110, 110)
        };
    }

    // Текущая тема. Переключение: Ui::HudTheme::setMode(Ui::ThemeMode::Dark);
    class HudTheme
    {
    public:
        static const HudPalette& current()
        {
            return mode() == ThemeMode::Dark ? Themes::Dark : Themes::Light;
        }

        static ThemeMode getMode()
        {
            return mode();
        }

        static void setMode(ThemeMode m)
        {
            mode() = m;
        }

        // Цвет пинга по порогам. Отрицательный пинг (нет данных) — приглушённый.
        static const Rgba& pingColor(int pingMs)
        {
            const HudPalette& p = current();

            if (pingMs < 0)    return p.textMuted;
            if (pingMs < 60)   return p.pingGood;
            if (pingMs < 120)  return p.pingWarn;
            return p.pingBad;
        }

    private:
        static ThemeMode& mode()
        {
            static ThemeMode m = ThemeMode::Light;
            return m;
        }
    };

    // Добавляет текст с тенью (ореолом) в очередь рендерера.
    // Возвращает ширину текста. flush() вызывайте сами, как и раньше.
    inline float drawText(
        UiTextRenderer& renderer,
        UiFont& font,
        const std::string& text,
        float x,
        float y,
        const Rgba& color,
        float scale = 1.0f)
    {
        const HudPalette& t = HudTheme::current();
        const float o = t.shadowOffset;

        if (o > 0.0f && t.shadow.a > 0.0f)
        {
            renderer.add(font, text, x + o, y + o,
                t.shadow.r, t.shadow.g, t.shadow.b, t.shadow.a, scale);
        }

        renderer.add(font, text, x, y,
            color.r, color.g, color.b, color.a, scale);

        return font.measureWidth(text) * scale;
    }
}