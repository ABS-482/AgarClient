#pragma once

class UIPanel;
class TextRenderer;
class Font;
class World;

class LeaderboardHUD
{
public:
    LeaderboardHUD(
        UIPanel& uiPanel,
        TextRenderer& textRenderer,
        Font& font,
        World& world
    );

    void draw(
        float screenW,
        float screenH
    );

private:
    UIPanel& uiPanel;
    TextRenderer& textRenderer;
    Font& font;
    World& world;
};