#include "GameHUD.h"

#include <algorithm>
#include <sstream>
#include <string>

GameHUD::GameHUD(
    UIPanel& uiPanel,
    TextRenderer& textRenderer,
    Font& font,
    Camera& camera,
    World& world,
    GameState& gameState,
    Shader& chatSkinShader,
    SkinMesh& skinMesh,
    SkinManager& skinManager
)
    : uiPanel(uiPanel),
    textRenderer(textRenderer),
    font(font),
    camera(camera),
    world(world),
    gameState(gameState),
    chatSkinShader(chatSkinShader),
    skinMesh(skinMesh),
    skinManager(skinManager)
{
    chatSkinCenter =
        chatSkinShader.uniformLocation("uCenter");

    chatSkinRadius =
        chatSkinShader.uniformLocation("uRadius");

    chatSkinScreenSize =
        chatSkinShader.uniformLocation("uScreenSize");

    chatSkinTexture =
        chatSkinShader.uniformLocation("uSkin");

    chatSkinBorderWidth =
        chatSkinShader.uniformLocation("uBorderWidth");

    chatSkinBorderColor =
        chatSkinShader.uniformLocation("uBorderColor");
}
void GameHUD::draw(
    const std::vector<uint32_t>& ownedIds,
    const std::unordered_map<uint32_t, RenderState>& renderStates,
    float screenW,
    float screenH
)
{
    constexpr float rectR = 0.1647f;
    constexpr float rectG = 0.3922f;
    constexpr float rectB = 0.5882f;
    constexpr float rectA = 0.5f;

    textRenderer.begin();

    // -------------------------
    // Leaderboard
    // -------------------------

    auto leaderboard = world.getLeaderboard();

    if (!leaderboard.empty())
    {
        constexpr int maxVisible = 10;

        constexpr float rowHeight = 26.0f;
        constexpr float paddingX = 14.0f;
        constexpr float paddingY = 12.0f;

        const int visibleCount =
            std::min(
                static_cast<int>(leaderboard.size()),
                maxVisible
            );

        const float panelWidth = 250.0f;

        const float panelHeight =
            paddingY * 2.0f +
            rowHeight * visibleCount;

        // --------------------------------------------------------
        // Позиция
        // --------------------------------------------------------

        constexpr float marginRight = 10.0f;
        constexpr float marginTop = 10.0f;

        const float panelCenterX =
            screenW -
            marginRight -
            panelWidth * 0.5f;

        const float panelCenterY =
            marginTop +
            panelHeight * 0.5f;

        // --------------------------------------------------------
        // Тёмная панель в стиле JS
        // background: rgba(0, 0, 0, .30)
        // border: 1px solid rgba(255, 255, 255, .12)
        // --------------------------------------------------------

        uiPanel.draw(
            panelCenterX,
            panelCenterY,
            panelWidth,
            panelHeight,
            12.0f,

            // background
            0.0f,
            0.0f,
            0.0f,
            0.30f,

            // border
            1.0f,
            1.0f,
            1.0f,
            0.12f,

            1.0f,

            screenW,
            screenH
        );

        // --------------------------------------------------------
        // Текст
        // --------------------------------------------------------

        const float topY =
            panelCenterY -
            panelHeight * 0.5f +
            paddingY;

        const float rowX =
            panelCenterX -
            panelWidth * 0.5f +
            paddingX;

        for (int i = 0; i < visibleCount; ++i)
        {
            const auto& entry =
                leaderboard[i];

            const std::string line =
                std::to_string(i + 1) +
                ". " +
                entry.name;

            const float rowY =
                topY +
                rowHeight * i +
                rowHeight * 0.5f;

            textRenderer.addTextLeftAligned(
                font,
                line,
                rowX,
                rowY,
                0.24f
            );
        }
    }

    // -------------------------
    // Chat
    // -------------------------

    auto chatMessages =
        world.getChatMessages();

    if (!chatMessages.empty())
    {
        constexpr int maxVisibleLines = 7;

        // Расстояние между обычными визуальными строками.
        constexpr float rowHeight = 36.0f;

        // Дополнительный отступ только между разными сообщениями.
        constexpr float messageGap = 15.0f;

        constexpr float padding = 12.0f;
        constexpr float fontScale = 0.24f;

        constexpr float avatarSize = 44.0f;
        constexpr float avatarRadius = 22.0f;
        constexpr float avatarBorderWidth = 2.0f;
        constexpr float avatarGap = 10.0f;

        const float panelWidth = 340.0f;

        // ------------------------------------------------------------
        // Область текста находится справа от аватара
        // ------------------------------------------------------------

        const float contentWidth =
            panelWidth -
            padding * 2.0f -
            avatarSize -
            avatarGap;

        struct ChatVisualLine
        {
            const ChatMessage* message;
            std::string text;
            bool firstLine;
        };

        std::vector<ChatVisualLine> visualLines;

        // ------------------------------------------------------------
        // Разбиваем сообщения на визуальные строки
        // ------------------------------------------------------------

        for (const auto& msg : chatMessages)
        {
            const std::string displayName =
                msg.name == "Spectator"
                ? msg.name + std::to_string(msg.playerID)
                : msg.name;

            const std::string suffix =
                msg.isPlayerEnter
                ? " enters the game"
                : ": " + msg.message;

            const float nicknameWidth =
                font.measureWidth(displayName) *
                fontScale;

            // На первой строке место занимает ник.
            const float firstLineWidth =
                std::max(
                    1.0f,
                    contentWidth - nicknameWidth
                );

            // --------------------------------------------------------
            // Получение одного UTF-8 символа
            // --------------------------------------------------------

            auto getNextUtf8Char =
                [](const std::string& text, size_t& pos)
                {
                    const size_t start = pos;

                    if (pos >= text.size())
                        return std::string();

                    ++pos;

                    while (
                        pos < text.size() &&
                        (
                            static_cast<unsigned char>(
                                text[pos]
                                ) & 0xC0
                            ) == 0x80
                        )
                    {
                        ++pos;
                    }

                    return text.substr(
                        start,
                        pos - start
                    );
                };

            // --------------------------------------------------------
            // Разрезаем очень длинное слово
            // --------------------------------------------------------

            auto splitLongWord =
                [&](const std::string& word, float maxWidth)
                {
                    std::vector<std::string> parts;

                    std::string current;

                    size_t pos = 0;

                    while (pos < word.size())
                    {
                        const std::string character =
                            getNextUtf8Char(
                                word,
                                pos
                            );

                        const std::string candidate =
                            current + character;

                        const float width =
                            font.measureWidth(candidate) *
                            fontScale;

                        if (
                            !current.empty() &&
                            width > maxWidth
                            )
                        {
                            parts.push_back(current);
                            current = character;
                        }
                        else
                        {
                            current = candidate;
                        }
                    }

                    if (!current.empty())
                        parts.push_back(current);

                    return parts;
                };

            // --------------------------------------------------------
            // Разбиваем suffix на слова
            // --------------------------------------------------------

            std::vector<std::string> words;

            size_t wordStart = 0;

            while (wordStart < suffix.size())
            {
                while (
                    wordStart < suffix.size() &&
                    suffix[wordStart] == ' '
                    )
                {
                    ++wordStart;
                }

                if (wordStart >= suffix.size())
                    break;

                size_t wordEnd = wordStart;

                while (
                    wordEnd < suffix.size() &&
                    suffix[wordEnd] != ' '
                    )
                {
                    ++wordEnd;
                }

                words.push_back(
                    suffix.substr(
                        wordStart,
                        wordEnd - wordStart
                    )
                );

                wordStart = wordEnd;
            }

            std::vector<std::string> lines;

            std::string currentLine;

            bool firstWord = true;

            const bool suffixStartsWithSpace =
                !suffix.empty() &&
                suffix[0] == ' ';

            // --------------------------------------------------------
            // Формируем строки
            // --------------------------------------------------------

            for (const auto& word : words)
            {
                const float maxWidth =
                    lines.empty()
                    ? firstLineWidth
                    : contentWidth;

                const std::string prefix =
                    firstWord &&
                    suffixStartsWithSpace
                    ? " "
                    : "";

                const std::string candidate =
                    currentLine.empty()
                    ? prefix + word
                    : currentLine + " " + word;

                const float candidateWidth =
                    font.measureWidth(candidate) *
                    fontScale;

                if (candidateWidth <= maxWidth)
                {
                    currentLine = candidate;
                    firstWord = false;
                    continue;
                }

                // ----------------------------------------------------
                // Текущая строка закончена
                // ----------------------------------------------------

                if (!currentLine.empty())
                {
                    lines.push_back(currentLine);
                    currentLine.clear();
                }

                firstWord = false;

                // ----------------------------------------------------
                // Слово помещается целиком в новую строку
                // ----------------------------------------------------

                const float wordWidth =
                    font.measureWidth(word) *
                    fontScale;

                if (wordWidth <= contentWidth)
                {
                    currentLine = word;
                    continue;
                }

                // ----------------------------------------------------
                // Слово слишком длинное.
                // Разрезаем его посимвольно.
                // ----------------------------------------------------

                const auto parts =
                    splitLongWord(
                        word,
                        contentWidth
                    );

                for (
                    size_t partIndex = 0;
                    partIndex < parts.size();
                    ++partIndex
                    )
                {
                    const std::string& part =
                        parts[partIndex];

                    if (
                        partIndex + 1 <
                        parts.size()
                        )
                    {
                        lines.push_back(part);
                    }
                    else
                    {
                        currentLine = part;
                    }
                }
            }

            if (!currentLine.empty())
            {
                lines.push_back(currentLine);
            }

            // --------------------------------------------------------
            // Сохраняем визуальные строки
            // --------------------------------------------------------

            for (
                size_t lineIndex = 0;
                lineIndex < lines.size();
                ++lineIndex
                )
            {
                visualLines.push_back({
                    &msg,
                    lines[lineIndex],
                    lineIndex == 0
                    });
            }
        }

        // ------------------------------------------------------------
        // Показываем последние строки
        // ------------------------------------------------------------

        const int visibleLineCount =
            std::min(
                static_cast<int>(
                    visualLines.size()
                    ),
                maxVisibleLines
            );

        const int startLine =
            static_cast<int>(
                visualLines.size()
                ) -
            visibleLineCount;

        // ------------------------------------------------------------
        // Считаем количество дополнительных отступов
        // между сообщениями среди видимых строк.
        // ------------------------------------------------------------

        int messageGapCount = 0;

        for (int i = 1; i < visibleLineCount; ++i)
        {
            const ChatVisualLine& previousLine =
                visualLines[startLine + i - 1];

            const ChatVisualLine& currentLine =
                visualLines[startLine + i];

            if (
                currentLine.firstLine &&
                currentLine.message != previousLine.message
                )
            {
                ++messageGapCount;
            }
        }

        // ------------------------------------------------------------
        // Высота панели включает дополнительные отступы
        // ------------------------------------------------------------

        const float panelHeight =
            padding * 2.0f +
            rowHeight * maxVisibleLines +
            messageGap * (maxVisibleLines - 1);

        const float panelCenterX =
            panelWidth * 0.5f +
            10.0f;

        const float hudReservedHeight =
            ownedIds.empty()
            ? 16.0f
            : 52.0f;

        const float panelCenterY =
            screenH -
            hudReservedHeight -
            panelHeight * 0.5f;

        uiPanel.draw(
            panelCenterX,
            panelCenterY,
            panelWidth,
            panelHeight,
            25.0f,

            // background
            0.0f,
            0.0f,
            0.0f,
            0.30f,

            // border
            1.0f,
            1.0f,
            1.0f,
            0.15f,

            1.0f,

            screenW,
            screenH
        );

        // ------------------------------------------------------------
        // Верхняя точка списка
        // ------------------------------------------------------------

        const float totalContentHeight =
            rowHeight * visibleLineCount +
            messageGap * messageGapCount;

        const float topY =
            panelCenterY +
            panelHeight * 0.5f -
            padding -
            totalContentHeight;

        // ------------------------------------------------------------
        // Отрисовка
        // ------------------------------------------------------------

        for (
            int i = 0;
            i < visibleLineCount;
            ++i
            )
        {
            const ChatVisualLine& line =
                visualLines[startLine + i];

            // --------------------------------------------------------
            // Считаем дополнительные отступы перед текущей строкой.
            // Они появляются только при переходе к новому сообщению.
            // --------------------------------------------------------

            float additionalGap = 0.0f;

            for (int j = 1; j <= i; ++j)
            {
                const ChatVisualLine& previousLine =
                    visualLines[startLine + j - 1];

                const ChatVisualLine& currentLine =
                    visualLines[startLine + j];

                if (
                    currentLine.firstLine &&
                    currentLine.message != previousLine.message
                    )
                {
                    additionalGap += messageGap;
                }
            }

            const float rowY =
                topY +
                rowHeight * i +
                additionalGap +
                rowHeight * 0.5f;

            const float rowX =
                10.0f +
                padding;

            // --------------------------------------------------------
            // Текст начинается справа от аватара.
            // --------------------------------------------------------

            const float textX =
                rowX +
                avatarSize +
                avatarGap;

            // --------------------------------------------------------
            // Первая строка сообщения:
            // аватар + ник + текст.
            // --------------------------------------------------------

            if (line.firstLine)
            {
                const ChatMessage& msg =
                    *line.message;

                const std::string displayName =
                    msg.name == "Spectator"
                    ? msg.name +
                    std::to_string(msg.playerID)
                    : msg.name;

                const float nicknameR =
                    static_cast<float>(
                        msg.colorR
                        ) / 255.0f;

                const float nicknameG =
                    static_cast<float>(
                        msg.colorG
                        ) / 255.0f;

                const float nicknameB =
                    static_cast<float>(
                        msg.colorB
                        ) / 255.0f;

                // ----------------------------------------------------
                // Аватар
                // ----------------------------------------------------

                const float avatarCenterX =
                    rowX +
                    avatarSize * 0.5f;

                const float avatarCenterY =
                    rowY;

                GLuint avatarTexture = 0;

                if (msg.skin != 0)
                {
                    avatarTexture =
                        skinManager.getTexture(msg.skin);
                }

                if (avatarTexture != 0)
                {
                    chatSkinShader.use();

                    chatSkinShader.setVec2(
                        chatSkinCenter,
                        avatarCenterX,
                        avatarCenterY
                    );

                    chatSkinShader.setFloat(
                        chatSkinRadius,
                        avatarRadius
                    );

                    chatSkinShader.setVec2(
                        chatSkinScreenSize,
                        screenW,
                        screenH
                    );

                    chatSkinShader.setFloat(
                        chatSkinBorderWidth,
                        avatarBorderWidth
                    );

                    chatSkinShader.setVec3(
                        chatSkinBorderColor,
                        1.0f,
                        1.0f,
                        1.0f
                    );

                    glActiveTexture(GL_TEXTURE0);

                    glBindTexture(
                        GL_TEXTURE_2D,
                        avatarTexture
                    );

                    chatSkinShader.setInt(
                        chatSkinTexture,
                        0
                    );

                    skinMesh.draw();
                }
                else
                {
                    // ------------------------------------------------
                    // Skin ещё не загружен / отсутствует.
                    // Используем цветной fallback.
                    // ------------------------------------------------

                    uiPanel.draw(
                        avatarCenterX,
                        avatarCenterY,
                        avatarSize,
                        avatarSize,
                        avatarRadius,

                        nicknameR,
                        nicknameG,
                        nicknameB,
                        1.0f,

                        1.0f,
                        1.0f,
                        1.0f,
                        1.0f,

                        avatarBorderWidth,

                        screenW,
                        screenH
                    );
                }

                // ----------------------------------------------------
                // Ник
                // ----------------------------------------------------

                textRenderer.addTextLeftAlignedColored(
                    font,
                    displayName,
                    textX,
                    rowY,
                    fontScale,
                    nicknameR,
                    nicknameG,
                    nicknameB
                );

                const float nicknameWidth =
                    font.measureWidth(
                        displayName
                    ) * fontScale;

                // ----------------------------------------------------
                // Текст после ника
                // ----------------------------------------------------

                textRenderer.addTextLeftAligned(
                    font,
                    line.text,
                    textX + nicknameWidth,
                    rowY,
                    fontScale
                );
            }
            else
            {
                // ----------------------------------------------------
                // Продолжение длинного сообщения.
                // Ник и аватар не повторяем.
                // ----------------------------------------------------

                textRenderer.addTextLeftAligned(
                    font,
                    line.text,
                    textX,
                    rowY,
                    fontScale
                );
            }
        }
    }

    // -------------------------
    // Personal HUD
    // -------------------------

    if (!ownedIds.empty())
    {
        const auto firstRsIt =
            renderStates.find(ownedIds[0]);

        const float firstX =
            firstRsIt != renderStates.end()
            ? firstRsIt->second.x
            : 0.0f;

        const float firstY =
            firstRsIt != renderStates.end()
            ? firstRsIt->second.y
            : 0.0f;

        std::ostringstream hudStream;

        hudStream
            << "x:"
            << static_cast<int>(firstX)
            << " y:"
            << static_cast<int>(firstY)
            << ". Your best: "
            << static_cast<int>(gameState.bestScoreSoFar)
            << ". Now: "
            << static_cast<int>(gameState.currentScore)
            << " ("
            << ownedIds.size()
            << ")";

        const std::string hudText =
            hudStream.str();

        // --------------------------------------------------------
        // Typography
        // --------------------------------------------------------

        constexpr float hudFontScale =
            20.0f / 70.0f;

        const float textWidthPx =
            font.measureWidth(hudText) *
            hudFontScale;

        // Небольшие внутренние отступы по горизонтали.
        constexpr float paddingX = 14.0f;

        // Высота как у компактного HUD из JS.
        constexpr float panelHeight = 40.0f;

        const float panelWidth =
            textWidthPx +
            paddingX * 2.0f;

        // --------------------------------------------------------
        // Position
        // --------------------------------------------------------

        constexpr float marginLeft = 10.0f;
        constexpr float marginBottom = 10.0f;

        const float panelCenterX =
            marginLeft +
            panelWidth * 0.5f;

        const float panelCenterY =
            screenH -
            marginBottom -
            panelHeight * 0.5f;

        // --------------------------------------------------------
        // Dark strict panel
        // background: rgba(0, 0, 0, .30)
        // border:     rgba(255, 255, 255, .15)
        // --------------------------------------------------------

        uiPanel.draw(
            panelCenterX,
            panelCenterY,
            panelWidth,
            panelHeight,
            20.0f,

            // background
            0.0f,
            0.0f,
            0.0f,
            0.30f,

            // border
            1.0f,
            1.0f,
            1.0f,
            0.15f,

            1.0f,

            screenW,
            screenH
        );

        textRenderer.addTextLeftAligned(
            font,
            hudText,
            marginLeft + paddingX,
            panelCenterY,
            hudFontScale
        );
    }

    textRenderer.end(
        font,
        screenW,
        screenH,
        1.0f,
        1.0f,
        1.0f
    );
}