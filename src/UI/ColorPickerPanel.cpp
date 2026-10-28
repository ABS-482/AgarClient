#include "ColorPickerPanel.h"

#include "../Core/AssetPath.h"
#include "../Graphics/IconRenderer.h"
#include "../Graphics/UIPanel.h"
#include "../Fonts/UiFont.h"
#include "../Fonts/UiTextRenderer.h"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <utility>

namespace
{
    // --------------------------------------------------------
    // Общий масштаб всего окна выбора цвета.
    // 1.0 — оригинальный размер, 0.75 — на 25% меньше,
    // 0.6 — на 40% меньше. Меняйте только это значение.
    // --------------------------------------------------------
    constexpr float kScale = 0.85f;

    constexpr float kPanelHeight = 460.0f * kScale;
    constexpr float kCellSize = 46.0f * kScale;
    constexpr float kCellStepX = 43.0f * kScale;
    constexpr float kCellStepY = 39.0f * kScale;

    constexpr float kCellPaddingX = 25.0f * kScale;
    constexpr float kCellIndentX = 21.0f * kScale;
    constexpr float kCellTop = 14.0f * kScale;

    GLuint loadJavaTexture(
        IconRenderer& icons,
        const char* relativePath,
        int* width = nullptr,
        int* height = nullptr
    )
    {
        const std::string path = AssetPath::get(relativePath);

        if (!std::filesystem::exists(path))
        {
            std::cerr
                << "ColorPickerPanel: texture not found: "
                << path << '\n';

            return 0;
        }

        return icons.loadTexture(path, width, height);
    }
}

ColorPickerPanel::ColorPickerPanel(
    IconRenderer& icons,
    UIPanel& panels,
    UiTextRenderer& textRenderer,
    UiFont& titleFont
)
    : m_icons(icons),
    m_panels(panels),
    m_textRenderer(textRenderer),
    m_titleFont(titleFont)
{
    m_window1Texture = loadJavaTexture(
        m_icons,
        "ui/window1.png",
        &m_window1Width,
        &m_window1Height
    );

    m_window2Texture = loadJavaTexture(
        m_icons,
        "ui/window2.png",
        &m_window2Width
    );

    m_window3Texture = loadJavaTexture(
        m_icons,
        "ui/window3.png",
        &m_window3Width,
        &m_window3Height
    );

    m_glowTexture = loadJavaTexture(
        m_icons,
        "ui/glow.png"
    );

    m_blobTexture = loadJavaTexture(
        m_icons,
        "ui/blob.png"
    );
}

ColorPickerPanel::~ColorPickerPanel()
{
    const GLuint textures[] =
    {
        m_window1Texture,
        m_window2Texture,
        m_window3Texture,
        m_glowTexture,
        m_blobTexture
    };

    glDeleteTextures(
        static_cast<GLsizei>(std::size(textures)),
        textures
    );
}

void ColorPickerPanel::open()
{
    m_open = true;
}

bool ColorPickerPanel::isOpen() const
{
    return m_open;
}

int ColorPickerPanel::selectedColorIndex() const
{
    return m_selectedColorIndex;
}

RGB ColorPickerPanel::selectedColor() const
{
    return getPlayerColor(
        static_cast<uint8_t>(m_selectedColorIndex)
    );
}

bool ColorPickerPanel::contains(
    float x,
    float y,
    float left,
    float top,
    float width,
    float height
)
{
    return x >= left &&
        x <= left + width &&
        y >= top &&
        y <= top + height;
}

std::vector<ColorPickerPanel::ColorCell>
ColorPickerPanel::buildCells(
    float panelLeft,
    float panelTop
) const
{
    std::vector<ColorCell> cells;
    cells.reserve(127);

    int n = 0;

    for (int row = 0; row < 13; ++row)
    {
        // 7, 8, 9, 10, 11, 12, 13, 12, ... 7.
        const int rowCount =
            row < 7 ? 7 + row : 19 - row;

        const int indent = 13 - rowCount;

        for (int column = 0; column < rowCount; ++column)
        {
            // Сохраняем порядок индексов из Java:
            // 1...63, 0, 64...126.
            const int colorIndex =
                n < 63 ? n + 1 :
                (n > 63 ? n : 0);

            const float x =
                panelLeft +
                kCellPaddingX +
                static_cast<float>(indent) * kCellIndentX +
                static_cast<float>(column) * kCellStepX;

            // В C++ начало координат экрана находится сверху.
            // В Java позиции задаются относительно нижнего края.
            const float y =
                panelTop +
                kCellTop +
                static_cast<float>(row) * kCellStepY;

            cells.push_back({ colorIndex, x, y });
            ++n;
        }
    }

    return cells;
}

bool ColorPickerPanel::handleMouseClick(
    float mouseX,
    float mouseY,
    float screenW,
    float screenH
)
{
    if (!m_open)
        return false;

    const float centerX = screenW * 0.5f;
    const float centerY = screenH * 0.5f;

    // Размеры текстур с учётом масштаба.
    const float window1W = static_cast<float>(m_window1Width) * kScale;
    const float window1H = static_cast<float>(m_window1Height) * kScale;
    const float window3W = static_cast<float>(m_window3Width) * kScale;
    const float window3H = static_cast<float>(m_window3Height) * kScale;

    const float panelWidth =
        static_cast<float>(m_window2Width) * kScale;

    const float panelLeft =
        centerX - panelWidth * 0.5f;

    const float panelTop =
        centerY - kPanelHeight * 0.5f;

    const auto cells = buildCells(panelLeft, panelTop);

    // Обрабатываем цветовые ячейки прежде остальной панели.
    for (const ColorCell& cell : cells)
    {
        if (contains(
            mouseX,
            mouseY,
            cell.x,
            cell.y,
            kCellSize,
            kCellSize))
        {
            m_selectedColorIndex = cell.colorIndex;
            m_open = false;

            // Сообщаем владельцу панели о выборе цвета.
            if (m_onColorSelected)
                m_onColorSelected(m_selectedColorIndex);

            return true;
        }
    }

    const float headerLeft =
        centerX - window1W * 0.5f;

    const bool insideHeader = contains(
        mouseX,
        mouseY,
        headerLeft,
        panelTop - window1H,
        window1W,
        window1H
    );

    const bool insidePanel = contains(
        mouseX,
        mouseY,
        panelLeft,
        panelTop,
        panelWidth,
        kPanelHeight
    );

    const bool insideWindow3 = contains(
        mouseX,
        mouseY,
        panelLeft,
        panelTop + kPanelHeight,
        window3W,
        window3H
    );

    if (!insideHeader && !insidePanel && !insideWindow3)
        m_open = false;

    // Пока панель открыта, клики по меню под ней запрещены.
    return true;
}

void ColorPickerPanel::draw(
    float screenW,
    float screenH
)
{
    if (!m_open)
        return;

    const float centerX = screenW * 0.5f;
    const float centerY = screenH * 0.5f;

    // Размеры текстур с учётом масштаба.
    const float window1W = static_cast<float>(m_window1Width) * kScale;
    const float window1H = static_cast<float>(m_window1Height) * kScale;
    const float window3W = static_cast<float>(m_window3Width) * kScale;
    const float window3H = static_cast<float>(m_window3Height) * kScale;

    const float panelWidth =
        static_cast<float>(m_window2Width) * kScale;

    const float panelLeft =
        centerX - panelWidth * 0.5f;

    const float panelTop =
        centerY - kPanelHeight * 0.5f;

    // --------------------------------------------------------
    // Затемнение фона — Java Textures.GLOW
    // (на весь экран, масштаб не применяется)
    // --------------------------------------------------------

    if (m_glowTexture != 0)
    {
        m_icons.draw(
            m_glowTexture,
            centerX,
            centerY,
            screenW + 200.0f,
            screenH + 200.0f,
            screenW,
            screenH,
            0.0f, 0.0f, 0.0f, 0.68f
        );
    }
    else
    {
        m_panels.draw(
            centerX, centerY,
            screenW, screenH,
            0.0f,
            0.0f, 0.0f, 0.0f, 0.68f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f,
            screenW, screenH
        );
    }

    // --------------------------------------------------------
    // WINDOW1 — заголовок
    // --------------------------------------------------------

    const float headerY =
        panelTop - window1H * 0.5f;

    if (m_window1Texture != 0)
    {
        m_icons.draw(
            m_window1Texture,
            centerX,
            headerY,
            window1W,
            window1H,
            screenW,
            screenH
        );
    }
    else
    {
        m_panels.draw(
            centerX,
            headerY,
            window1W,
            window1H,
            12.0f * kScale,
            0.16f, 0.18f, 0.22f, 1.0f,
            0.30f, 0.34f, 0.40f, 1.0f,
            1.0f,
            screenW, screenH
        );
    }

    // --------------------------------------------------------
    // WINDOW2 — основная панель
    // --------------------------------------------------------

    if (m_window2Texture != 0)
    {
        m_icons.draw(
            m_window2Texture,
            centerX,
            centerY,
            panelWidth,
            kPanelHeight,
            screenW,
            screenH
        );
    }
    else
    {
        m_panels.draw(
            centerX, centerY,
            panelWidth, kPanelHeight,
            16.0f * kScale,
            0.12f, 0.14f, 0.18f, 1.0f,
            0.30f, 0.34f, 0.40f, 0.90f,
            1.0f,
            screenW, screenH
        );
    }

    // --------------------------------------------------------
    // WINDOW3 — нижний декоративный элемент
    // --------------------------------------------------------

    if (m_window3Texture != 0)
    {
        m_icons.draw(
            m_window3Texture,
            panelLeft + window3W * 0.5f,
            panelTop + kPanelHeight + window3H * 0.5f,
            window3W,
            window3H,
            screenW,
            screenH
        );
    }

    // --------------------------------------------------------
    // Цветовая палитра — Java Textures.BLOB
    // --------------------------------------------------------

    const auto cells = buildCells(panelLeft, panelTop);

    for (const ColorCell& cell : cells)
    {
        const RGB color = getPlayerColor(
            static_cast<uint8_t>(cell.colorIndex)
        );

        const float r = color.r / 255.0f;
        const float g = color.g / 255.0f;
        const float b = color.b / 255.0f;

        const float cellCenterX =
            cell.x + kCellSize * 0.5f;

        const float cellCenterY =
            cell.y + kCellSize * 0.5f;

        if (m_blobTexture != 0)
        {
            m_icons.draw(
                m_blobTexture,
                cellCenterX,
                cellCenterY,
                kCellSize,
                kCellSize,
                screenW,
                screenH,
                r, g, b, 1.0f
            );
        }
        else
        {
            m_panels.draw(
                cellCenterX,
                cellCenterY,
                kCellSize,
                kCellSize,
                kCellSize * 0.5f,
                r, g, b, 1.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f,
                screenW, screenH
            );
        }
    }

    // --------------------------------------------------------
    // Заголовок ColorPicker — центрирование и Java-style тень
    // --------------------------------------------------------

    constexpr float titleFontSize = 40.0f;

    // Java: Math.max(3.0f, fontSize / 12.0f)
    const float shadowOffset =
        std::max(3.0f, titleFontSize / 12.0f);

    constexpr float shadowAlpha = 85.0f / 255.0f;

    const std::string titleText = "Choose color";

    // Измеряем реальную ширину текста выбранным UiFont.
    const float titleWidth =
        m_titleFont.measureWidth(titleText);

    // Центрируем текст относительно центра WINDOW1.
    const float titleX =
        centerX - titleWidth * 0.5f;

    constexpr float titleVerticalOffset = 5.0f;

    const float titleY =
        m_titleFont.topForCenter(headerY)
        + titleVerticalOffset;

    m_textRenderer.begin();

    // 1. Тень: чёрная, с альфа-каналом из Colors.SCREEN_SHADOW.
    m_textRenderer.add(
        m_titleFont,
        titleText,
        titleX + shadowOffset,
        titleY + shadowOffset,
        0.0f, 0.0f, 0.0f,
        shadowAlpha,
        1.0f
    );

    // 2. Основной белый текст.
    m_textRenderer.add(
        m_titleFont,
        titleText,
        titleX,
        titleY,
        1.0f, 1.0f, 1.0f,
        1.0f,
        1.0f
    );

    m_textRenderer.flush(
        m_titleFont,
        screenW,
        screenH
    );
}

void ColorPickerPanel::setOnColorSelected(
    std::function<void(int)> callback
)
{
    m_onColorSelected = std::move(callback);
}