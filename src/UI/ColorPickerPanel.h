#pragma once

#include <glad/glad.h>

#include <vector>
#include <functional>

#include "../Graphics/PlayerColors.h"

class IconRenderer;
class UIPanel;
class UiTextRenderer;
class UiFont;

class ColorPickerPanel final
{
public:
    ColorPickerPanel(
        IconRenderer& icons,
        UIPanel& panels,
        UiTextRenderer& textRenderer,
        UiFont& titleFont
    );

    ~ColorPickerPanel();

    ColorPickerPanel(const ColorPickerPanel&) = delete;
    ColorPickerPanel& operator=(const ColorPickerPanel&) = delete;

    void open();
    bool isOpen() const;

    // Возвращает true, если клик обработан панелью.
    bool handleMouseClick(
        float mouseX,
        float mouseY,
        float screenW,
        float screenH
    );

    void draw(float screenW, float screenH);

    int selectedColorIndex() const;
    RGB selectedColor() const;

    void setOnColorSelected(std::function<void(int)> callback);

private:
    struct ColorCell
    {
        int colorIndex;
        float x;
        float y;
    };

    std::vector<ColorCell> buildCells(
        float panelLeft,
        float panelTop
    ) const;

    static bool contains(
        float x,
        float y,
        float left,
        float top,
        float width,
        float height
    );

    IconRenderer& m_icons;
    UIPanel& m_panels;
    UiTextRenderer& m_textRenderer;
    UiFont& m_titleFont;

    bool m_open = false;

    // Индекс 30 соответствует исходному цвету RGB(51, 153, 255).
    int m_selectedColorIndex = 30;

    GLuint m_window1Texture = 0;
    GLuint m_window2Texture = 0;
    GLuint m_window3Texture = 0;
    GLuint m_glowTexture = 0;
    GLuint m_blobTexture = 0;

    // Размеры PNG-областей, извлечённых из Java-текстур.
    int m_window1Width = 420;
    int m_window1Height = 60;

    int m_window2Width = 612;
    int m_window3Width = 64;
    int m_window3Height = 40;

    std::function<void(int)> m_onColorSelected;
};