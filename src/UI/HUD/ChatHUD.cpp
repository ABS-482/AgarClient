#include "ChatHUD.h"

#include <algorithm>
#include <string>
#include <vector>
#include <utility>

#include "../../Graphics/UIPanel.h"
#include "../../Graphics/TextRenderer.h"
#include "../../Graphics/Font.h"
#include "../../Graphics/Shader.h"
#include "../../Graphics/SkinMesh.h"
#include "../../Graphics/SkinManager.h"
#include "../../Game/World.h"
#include "../../Network/UserSkinDatabase.h"
#include "../../Fonts/UiFont.h"
#include "../../Fonts/UiTextRenderer.h"
#include <chrono>
#include "../../FrameProfiler.h"

ChatHUD::ChatHUD(
    UIPanel& uiPanel,
    TextRenderer& textRenderer,
    Font& font,
    World& world,
    Shader& chatSkinShader,
    SkinMesh& skinMesh,
    SkinManager& skinManager,
    UiFont& chatFont,
    UiTextRenderer& chatTextRenderer,
    UserSkinDatabase& userSkinDatabase
)
    : uiPanel(uiPanel),
    textRenderer(textRenderer),
    font(font),
    world(world),
    chatSkinShader(chatSkinShader),
    skinMesh(skinMesh),
    skinManager(skinManager),
    chatFont(chatFont),
    chatTextRenderer(chatTextRenderer),
    userSkinDatabase(userSkinDatabase)
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

void ChatHUD::draw(
    FrameProfiler& profiler,
    const std::vector<uint32_t>& ownedIds,
    float screenW,
    float screenH
)
{
    using ChatProfileClock = std::chrono::steady_clock;

    auto profileStageStart = ChatProfileClock::now();

    const auto recordChatStage =
        [&](FrameProfiler::Stage stage)
        {
            const auto now = ChatProfileClock::now();

            profiler.record(
                stage,
                std::chrono::duration<double, std::milli>(
                    now - profileStageStart
                ).count()
            );

            profileStageStart = now;
        };

    const World::ChatMessagesView chatView =
        world.getChatMessagesView();

    const auto chatMessagesSnapshot = chatView.messages;
    const auto& chatMessages = *chatMessagesSnapshot;

    const bool messagesChanged =
        !hasPreparedChatView ||
        chatView.version != preparedChatVersion ||
        chatView.resetVersion != preparedChatResetVersion;

    const bool databaseLoaded =
        userSkinDatabase.isLoaded();

    if (chatMessages.empty())
    {
        if (messagesChanged)
        {
            visualMessages.clear();

            preparedMessagesSnapshot = chatMessagesSnapshot;
            preparedChatVersion = chatView.version;
            preparedChatResetVersion = chatView.resetVersion;
            hasPreparedChatView = true;
        }

        if (databaseLoaded)
            verifiedMarksPrepared = true;

        recordChatStage(FrameProfiler::ChatPrepare);
        return;
    }

    // ------------------------------------------------------------
    // Размеры чата
    // ------------------------------------------------------------

    constexpr float lineHeight = 14.0f;

    constexpr float panelWidth = 300.0f;
    constexpr float panelPadding = 12.0f;
    constexpr float contentHeight = 305.0f;

    constexpr float avatarSize = 44.0f;
    constexpr float avatarRadius = 22.0f;
    constexpr float avatarBorderWidth = 2.0f;
    constexpr float avatarBoxWidth = 50.0f;
    constexpr float avatarGap = 10.0f;

    constexpr float enterAvatarSize = 44.0f;
    constexpr float enterAvatarGap = 10.0f;

    constexpr float bubbleWidth = 210.0f;
    constexpr float bubbleRadius = 10.0f;

    constexpr float bubblePaddingTop = 5.0f;
    constexpr float bubblePaddingBottom = 8.0f;
    constexpr float bubblePaddingLeft = 9.0f;
    constexpr float bubblePaddingRight = 5.0f;
    constexpr float nicknameGap = 5.0f;

    constexpr float enterPaddingX = 12.0f;
    constexpr float enterPaddingBottom = 4.0f;
    constexpr float enterIconWidth = 14.0f;
    constexpr float enterIconGap = 6.0f;
    constexpr float enterMarginTop = 12.0f;
    constexpr float enterMarginBottom = 16.0f;

    constexpr float messageMargin = 5.0f;
    constexpr int maxLinesPerMessage = 18;

    const float textWidth =
        bubbleWidth -
        bubblePaddingLeft -
        bubblePaddingRight;

    // ------------------------------------------------------------
    // UTF-8: чтение одного символа
    // ------------------------------------------------------------

    const auto getNextUtf8Char =
        [](const std::string& text, size_t& pos)
        {
            const size_t start = pos;

            if (pos >= text.size())
                return std::string();

            ++pos;

            while (
                pos < text.size() &&
                (
                    static_cast<unsigned char>(text[pos]) & 0xC0
                    ) == 0x80
                )
            {
                ++pos;
            }

            return text.substr(start, pos - start);
        };

    // ------------------------------------------------------------
    // Разбиение слишком длинного слова
    // ------------------------------------------------------------

    const auto splitLongWord =
        [&](const std::string& word, float maxWidth)
        {
            std::vector<std::string> parts;
            std::string current;

            size_t pos = 0;

            while (pos < word.size())
            {
                const std::string character =
                    getNextUtf8Char(word, pos);

                const std::string candidate =
                    current + character;

                const float width =
                    chatFont.measureWidth(candidate);

                if (!current.empty() && width > maxWidth)
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

    // ------------------------------------------------------------
    // Перенос обычного сообщения
    // ------------------------------------------------------------

    const auto wrapText =
        [&](const std::string& source)
        {
            std::vector<std::string> lines;
            std::string currentLine;

            size_t wordStart = 0;

            while (wordStart < source.size())
            {
                while (
                    wordStart < source.size() &&
                    source[wordStart] == ' '
                    )
                {
                    ++wordStart;
                }

                if (wordStart >= source.size())
                    break;

                size_t wordEnd = wordStart;

                while (
                    wordEnd < source.size() &&
                    source[wordEnd] != ' '
                    )
                {
                    ++wordEnd;
                }

                const std::string word =
                    source.substr(
                        wordStart,
                        wordEnd - wordStart
                    );

                wordStart = wordEnd;

                const std::string candidate =
                    currentLine.empty()
                    ? word
                    : currentLine + " " + word;

                const float candidateWidth =
                    chatFont.measureWidth(candidate);

                if (candidateWidth <= textWidth)
                {
                    currentLine = candidate;
                    continue;
                }

                if (!currentLine.empty())
                {
                    lines.push_back(currentLine);
                    currentLine.clear();
                }

                const auto parts =
                    splitLongWord(word, textWidth);

                for (size_t i = 0; i < parts.size(); ++i)
                {
                    if (i + 1 < parts.size())
                    {
                        lines.push_back(parts[i]);
                    }
                    else
                    {
                        currentLine = parts[i];
                    }
                }
            }

            if (!currentLine.empty())
                lines.push_back(currentLine);

            if (lines.empty())
                lines.emplace_back();

            // Не позволяем одному сообщению занять весь экран.
            if (lines.size() > maxLinesPerMessage)
            {
                lines.erase(
                    lines.begin(),
                    lines.end() - maxLinesPerMessage
                );
            }

            return lines;
        };

    // ------------------------------------------------------------
    // Инкрементальная подготовка сообщений чата
    // ------------------------------------------------------------

    constexpr std::size_t maxChatMessages = 50;

    const float enterAvailableWidth =
        panelWidth -
        panelPadding * 2.0f -
        enterAvatarSize -
        enterAvatarGap;

    const float enterSuffixWidth =
        chatFont.measureWidth(" enters the game");

    const auto updateEnterLayout =
        [&](ChatVisualMessage& visual)
        {
            visual.enterTextOnSecondLine =
                visual.enterNameLines.size() > 1 ||
                visual.enterLastNameWidth +
                enterSuffixWidth > enterAvailableWidth;

            const std::size_t enterLineCount =
                visual.enterNameLines.size() +
                (visual.enterTextOnSecondLine ? 1u : 0u);

            visual.boxHeight =
                lineHeight * static_cast<float>(enterLineCount) +
                enterPaddingBottom;
        };

    const auto makeVisualMessage =
        [&](const ChatMessage& msg,
            const ChatVisualMessage* previous)
        {
            ChatVisualMessage visual;

            visual.message = &msg;

            visual.displayName =
                msg.name == "Spectator"
                ? msg.name + std::to_string(msg.playerID)
                : msg.name;

            visual.isEnter = msg.isPlayerEnter;

            visual.isContinuation =
                !visual.isEnter &&
                previous != nullptr &&
                !previous->isEnter &&
                previous->message != nullptr &&
                previous->message->playerID == msg.playerID;

            visual.hasVerifiedMark =
                databaseLoaded &&
                !visual.isContinuation &&
                userSkinDatabase.requiresPassword(msg.name);

            if (!visual.isEnter && !visual.isContinuation)
            {
                visual.displayNameWidth =
                    chatFont.measureWidth(visual.displayName);
            }

            if (visual.isEnter)
            {
                visual.enterNameLines = wrapText(visual.displayName);

                visual.enterLastNameWidth =
                    chatFont.measureWidth(
                        visual.enterNameLines.back()
                    );

                updateEnterLayout(visual);

                visual.topMargin = enterMarginTop;
                visual.bottomMargin = enterMarginBottom;
            }
            else
            {
                visual.lines = wrapText(msg.message);

                const float nicknameHeight =
                    visual.isContinuation
                    ? 0.0f
                    : lineHeight + nicknameGap;

                visual.boxHeight =
                    bubblePaddingTop +
                    nicknameHeight +
                    lineHeight *
                    static_cast<float>(visual.lines.size()) +
                    bubblePaddingBottom;

                visual.topMargin =
                    visual.isContinuation ? 1.0f : messageMargin;

                visual.bottomMargin =
                    visual.isContinuation ? 2.0f : messageMargin;
            }

            return visual;
        };

    // ------------------------------------------------------------
    // Определяем, можно ли продолжить существующую разметку
    // ------------------------------------------------------------

    bool fullRebuild =
        !hasPreparedChatView ||
        chatView.resetVersion != preparedChatResetVersion ||
        !preparedMessagesSnapshot;

    std::size_t addedCount = 0;

    if (!fullRebuild &&
        chatView.version != preparedChatVersion)
    {
        if (chatView.version < preparedChatVersion)
        {
            fullRebuild = true;
        }
        else
        {
            const std::uint64_t versionDelta =
                chatView.version - preparedChatVersion;

            if (versionDelta == 0 ||
                versionDelta >= maxChatMessages ||
                versionDelta > chatMessages.size())
            {
                fullRebuild = true;
            }
            else
            {
                addedCount =
                    static_cast<std::size_t>(versionDelta);

                const std::size_t previousCount =
                    visualMessages.size();

                const std::size_t expectedCurrentCount =
                    (std::min)(
                        previousCount + addedCount,
                        maxChatMessages
                        );

                if (previousCount !=
                    preparedMessagesSnapshot->size() ||
                    chatMessages.size() != expectedCurrentCount)
                {
                    fullRebuild = true;
                }
            }
        }
    }
    else if (!fullRebuild &&
        chatMessagesSnapshot != preparedMessagesSnapshot)
    {
        fullRebuild = true;
    }

    // ------------------------------------------------------------
    // Полная перестройка нужна только при сбросе или рассинхронизации
    // ------------------------------------------------------------

    bool rebuiltAll = false;

    if (fullRebuild)
    {
        visualMessages.clear();
        visualMessages.reserve(maxChatMessages);

        for (const auto& msg : chatMessages)
        {
            const ChatVisualMessage* previous =
                visualMessages.empty()
                ? nullptr
                : &visualMessages.back();

            visualMessages.push_back(
                makeVisualMessage(msg, previous)
            );
        }

        rebuiltAll = true;
    }
    else if (addedCount > 0)
    {
        const std::size_t retainedCount =
            chatMessages.size() - addedCount;

        const std::size_t droppedCount =
            visualMessages.size() - retainedCount;

        // Удаляем визуальные данные сообщений, вытесненных из истории.
        if (droppedCount > 0)
        {
            visualMessages.erase(
                visualMessages.begin(),
                visualMessages.begin() +
                static_cast<std::vector<ChatVisualMessage>::difference_type>(
                    droppedCount
                    )
            );
        }

        // Перепривязываем указатели к новому неизменяемому снимку.
        for (std::size_t i = 0; i < visualMessages.size(); ++i)
        {
            visualMessages[i].message = &chatMessages[i];
        }

        // После удаления старейшего сообщения первый оставшийся
        // элемент мог перестать быть продолжением группы отправителя.
        if (droppedCount > 0 && !visualMessages.empty())
        {
            visualMessages.front() =
                makeVisualMessage(chatMessages.front(), nullptr);
        }

        // Готовим только новые сообщения.
        for (std::size_t i = retainedCount;
            i < chatMessages.size();
            ++i)
        {
            const ChatVisualMessage* previous =
                visualMessages.empty()
                ? nullptr
                : &visualMessages.back();

            visualMessages.push_back(
                makeVisualMessage(chatMessages[i], previous)
            );
        }
    }

    // Фиксируем снимок только после обновления всех указателей.
    if (messagesChanged || fullRebuild)
    {
        preparedMessagesSnapshot = chatMessagesSnapshot;
        preparedChatVersion = chatView.version;
        preparedChatResetVersion = chatView.resetVersion;
        hasPreparedChatView = true;
    }

    // Если база защитных ников загрузилась позднее сообщений,
    // пересчитываем отметки один раз для существующей истории.
    if (rebuiltAll)
    {
        verifiedMarksPrepared = databaseLoaded;
    }
    else if (databaseLoaded && !verifiedMarksPrepared)
    {
        for (auto& visual : visualMessages)
        {
            visual.hasVerifiedMark =
                !visual.isContinuation &&
                userSkinDatabase.requiresPassword(
                    visual.message->name
                );

            if (visual.isEnter)
                updateEnterLayout(visual);
        }

        verifiedMarksPrepared = true;
    }

    recordChatStage(FrameProfiler::ChatPrepare);

    // ------------------------------------------------------------
    // Выбираем последние сообщения с учётом частичной видимости
    // верхнего сообщения, как в Java ScrollPane.
    // ------------------------------------------------------------

    std::vector<size_t> visibleIndices;
    float usedHeight = 0.0f;

    for (size_t i = visualMessages.size(); i > 0; --i)
    {
        const size_t index = i - 1;

        const ChatVisualMessage& visual =
            visualMessages[index];

        const float requiredHeight =
            visual.topMargin +
            visual.boxHeight +
            visual.bottomMargin;

        visibleIndices.push_back(index);
        usedHeight += requiredHeight;

        // Включаем первое сообщение, которое пересекает
        // верхнюю границу области чата.
        if (usedHeight >= contentHeight)
            break;
    }

    std::reverse(
        visibleIndices.begin(),
        visibleIndices.end()
    );
    recordChatStage(FrameProfiler::ChatVisibility);

    // ------------------------------------------------------------
    // Общая полупрозрачная область чата
    // ------------------------------------------------------------

    const float panelHeight =
        contentHeight + panelPadding * 2.0f;

    const float panelCenterX =
        panelWidth * 0.5f + 10.0f;

    const float hudReservedHeight =
        ownedIds.empty() ? 16.0f : 52.0f;

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

        // rgba(0, 0, 0, 0.3)
        0.0f, 0.0f, 0.0f, 0.30f,

        // Белая граница с прозрачностью 15%.
        1.0f, 1.0f, 1.0f, 0.15f,

        1.0f,

        screenW,
        screenH
    );

    const float panelLeftX =
        panelCenterX - panelWidth * 0.5f;

    const float avatarLeftX =
        panelLeftX + 10.0f;

    const float avatarCenterX =
        avatarLeftX + avatarBoxWidth * 0.5f;

    const float bubbleLeftX =
        avatarLeftX + avatarBoxWidth + avatarGap;

    const float textX =
        bubbleLeftX + bubblePaddingLeft;


    // Границы области отображения сообщений.
    const float contentBottom =
        panelCenterY +
        panelHeight * 0.5f -
        panelPadding;

    const float contentTop =
        contentBottom - contentHeight;

    // OpenGL использует начало координат в левом нижнем углу
    // для glScissor, поэтому преобразуем Y из координат HUD.
    glEnable(GL_SCISSOR_TEST);

    glScissor(
        static_cast<GLint>(std::floor(panelLeftX)),
        static_cast<GLint>(std::floor(screenH - contentBottom)),
        static_cast<GLsizei>(std::ceil(panelWidth)),
        static_cast<GLsizei>(std::ceil(contentHeight))
    );

    float cursorY =
        contentBottom - usedHeight;
    std::vector<std::pair<float, float>> verifiedMarks;

    const auto queueVerifiedMarkOnAvatar =
        [&verifiedMarks](
            float avatarCenterX,
            float avatarCenterY,
            float avatarSize)
        {
            constexpr float markWidth = 12.0f;
            constexpr float markHeight = 9.0f;
            constexpr float inset = 2.0f;

            verifiedMarks.emplace_back(
                avatarCenterX + avatarSize * 0.5f
                - markWidth - inset,
                avatarCenterY + avatarSize * 0.5f
                - markHeight - inset
            );
        };

    // ------------------------------------------------------------
    // Уровни на верхних краях аватара
    // ------------------------------------------------------------

    const auto drawLevelBadges =
        [&](const ChatMessage& msg,
            float avatarCenterX,
            float avatarCenterY)
        {
            constexpr float badgeHeight = 14.0f;
            constexpr float badgePaddingX = 2.0f;
            constexpr float badgeMinWidth = 18.0f;
            constexpr float badgeOffsetX = 14.0f;

            constexpr float badgeTextScale = 0.8f;

            // Бейджи накладываются на верхний край аватара.
            const float badgeCenterY =
                avatarCenterY - avatarSize * 0.5f + 6.0f;

            const auto drawBadge =
                [&](const std::string& text,
                    float centerX,
                    float fillR,
                    float fillG,
                    float fillB,
                    float textR,
                    float textG,
                    float textB)
                {
                    const float textWidth =
                        chatFont.measureWidth(text) * badgeTextScale;

                    const float badgeWidth =
                        std::max(
                            badgeMinWidth,
                            textWidth + badgePaddingX * 2.0f
                        );

                    // Фон бейджа — скруглённая плашка.
                    uiPanel.draw(
                        centerX,
                        badgeCenterY,
                        badgeWidth,
                        badgeHeight,
                        badgeHeight * 0.5f,

                        // Заливка
                        fillR,
                        fillG,
                        fillB,
                        1.0f,

                        // Без границы
                        0.0f,
                        0.0f,
                        0.0f,
                        0.0f,
                        0.0f,

                        screenW,
                        screenH
                    );

                    // Число по центру плашки.
                    const float textX =
                        centerX - textWidth * 0.5f;

                    const float scaledTextHeight =
                        chatFont.lineHeight() * badgeTextScale;

                    const float textY =
                        badgeCenterY - scaledTextHeight * 0.5f;

                    chatTextRenderer.add(
                        chatFont,
                        text,
                        textX,
                        textY,
                        textR,
                        textG,
                        textB,
                        1.0f,
                        badgeTextScale
                    );
                };

            // --------------------------------------------------------
            // Слева — обычный уровень, золотистый фон.
            // Нулевой обычный уровень не отображаем, как в leaderboard.
            // --------------------------------------------------------

            if (msg.userLevel > 0)
            {
                drawBadge(
                    std::to_string(msg.userLevel),
                    avatarCenterX - badgeOffsetX,

                    0.855f,
                    0.647f,
                    0.125f,

                    0.0f,
                    0.0f,
                    0.0f
                );
            }

            // --------------------------------------------------------
            // Справа — сезонный уровень, зелёный фон.
            // Отображаем всегда, включая значение 1.
            // --------------------------------------------------------

            if (msg.userLevelSeason > 0)
            {
                drawBadge(
                    std::to_string(msg.userLevelSeason),
                    avatarCenterX + badgeOffsetX,

                    0.0f,
                    0.5f,
                    0.0f,

                    0.0f,
                    0.0f,
                    0.0f
                );
            }
        };

    // ------------------------------------------------------------
    // Отрисовка сообщений
    // ------------------------------------------------------------
    chatTextRenderer.beginProfile(profiler);

    for (const size_t index : visibleIndices)
    {
        const ChatVisualMessage& visual =
            visualMessages[index];

        const ChatMessage& msg =
            *visual.message;

        cursorY += visual.topMargin;

        const float boxTop = cursorY;
        const float boxCenterY =
            boxTop + visual.boxHeight * 0.5f;

        const float nicknameR =
            static_cast<float>(msg.colorR) / 255.0f;

        const float nicknameG =
            static_cast<float>(msg.colorG) / 255.0f;

        const float nicknameB =
            static_cast<float>(msg.colorB) / 255.0f;

        if (visual.isEnter)
        {
            // ----------------------------------------------------
            // Простое уведомление: аватар + ник + текст
            // ----------------------------------------------------

            const std::string enterText = " enters the game";

            constexpr float enterAvatarSize = 44.0f;
            constexpr float enterAvatarRadius = 22.0f;
            constexpr float enterAvatarBorderWidth = 2.0f;
            constexpr float enterAvatarGap = 10.0f;

            const float avatarLeftX =
                panelLeftX + panelPadding;

            const float avatarCenterX =
                avatarLeftX + enterAvatarSize * 0.5f;

            const float avatarCenterY = boxCenterY;

            // ----------------------------------------------------
            // Аватар игрока
            // ----------------------------------------------------

            GLuint avatarTexture = 0;

            if (msg.skin != 0)
                avatarTexture = skinManager.getTexture(msg.skin);

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
                    enterAvatarRadius
                );

                chatSkinShader.setVec2(
                    chatSkinScreenSize,
                    screenW,
                    screenH
                );

                chatSkinShader.setFloat(
                    chatSkinBorderWidth,
                    enterAvatarBorderWidth
                );

                chatSkinShader.setVec3(
                    chatSkinBorderColor,
                    1.0f,
                    1.0f,
                    1.0f
                );

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, avatarTexture);

                chatSkinShader.setInt(chatSkinTexture, 0);

                skinMesh.draw();
            }
            else
            {
                // Fallback, если скин отсутствует.
                uiPanel.draw(
                    avatarCenterX,
                    avatarCenterY,
                    enterAvatarSize,
                    enterAvatarSize,
                    enterAvatarRadius,

                    nicknameR,
                    nicknameG,
                    nicknameB,
                    1.0f,

                    1.0f,
                    1.0f,
                    1.0f,
                    1.0f,

                    enterAvatarBorderWidth,

                    screenW,
                    screenH
                );
            }

            drawLevelBadges(
                msg,
                avatarCenterX,
                avatarCenterY
            );

            if (visual.hasVerifiedMark)
            {
                queueVerifiedMarkOnAvatar(
                    avatarCenterX,
                    avatarCenterY,
                    enterAvatarSize
                );
            }

            // ----------------------------------------------------
            // Ник и текст уведомления с переносом строк
            // ----------------------------------------------------

            const float enterNameX =
                avatarLeftX +
                enterAvatarSize +
                enterAvatarGap;

            const float enterTextY =
                boxTop + enterPaddingBottom * 0.5f;

            // ----------------------------------------------------
            // Ник — цветной, может занимать несколько строк
            // ----------------------------------------------------

            for (size_t lineIndex = 0;
                lineIndex < visual.enterNameLines.size();
                ++lineIndex)
            {
                const float lineY =
                    enterTextY +
                    lineHeight * static_cast<float>(lineIndex);

                chatTextRenderer.add(
                    chatFont,
                    visual.enterNameLines[lineIndex],
                    enterNameX,
                    lineY,
                    nicknameR,
                    nicknameG,
                    nicknameB,
                    1.0f
                );
            }

            // ----------------------------------------------------
            // "enters the game" — белый текст
            // ----------------------------------------------------

            const size_t lastNameLineIndex =
                visual.enterNameLines.size() - 1;

            const float lastNameLineWidth =
                visual.enterLastNameWidth;

            const float enterSuffixX =
                visual.enterTextOnSecondLine
                ? enterNameX
                : enterNameX + lastNameLineWidth;

            const size_t enterSuffixLineIndex =
                lastNameLineIndex +
                (visual.enterTextOnSecondLine ? 1u : 0u);

            const float enterSuffixY =
                enterTextY +
                lineHeight * static_cast<float>(enterSuffixLineIndex);

            chatTextRenderer.add(
                chatFont,
                enterText,
                enterSuffixX,
                enterSuffixY,
                1.0f,
                1.0f,
                1.0f,
                1.0f
            );
        }
        else
        {
            // ----------------------------------------------------
            // Тип 2: обычное сообщение в собственном пузыре
            // ----------------------------------------------------

            const bool firstForSender =
                !visual.isContinuation;

            // ----------------------------------------------------
            // Хвостик у первого сообщения отправителя
            // Рисуем ДО пузыря, чтобы его внутренняя часть
            // перекрылась фоном сообщения.
            // ----------------------------------------------------

            if (firstForSender)
            {
                uiPanel.drawTriangle(
                    bubbleLeftX + 0.25f, boxTop + 11.0f,
                    bubbleLeftX - 9.0f, boxTop + 19.0f,
                    bubbleLeftX + 0.25f, boxTop + 25.0f,

                    // Цвет пузыря #80808057
                    128.0f / 255.0f,
                    128.0f / 255.0f,
                    128.0f / 255.0f,
                    87.0f / 255.0f,

                    screenW,
                    screenH
                );
            }

            // ----------------------------------------------------
            // Фон пузыря — все углы скруглённые
            // ----------------------------------------------------

            uiPanel.drawRoundedCorners(
                bubbleLeftX + bubbleWidth * 0.5f,
                boxCenterY,
                bubbleWidth,
                visual.boxHeight,

                bubbleRadius,
                bubbleRadius,
                bubbleRadius,
                bubbleRadius,

                // Фон #80808057
                128.0f / 255.0f,
                128.0f / 255.0f,
                128.0f / 255.0f,
                87.0f / 255.0f,

                // Без границы
                1.0f,
                1.0f,
                1.0f,
                0.0f,

                0.0f,

                screenW,
                screenH
            );

            // ----------------------------------------------------
            // Аватар: рисуем только у первого сообщения отправителя
            // ----------------------------------------------------

            if (!visual.isContinuation)
            {
                const float avatarCenterY =
                    boxTop + avatarSize * 0.5f;

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
                    // Цветной аватар, если скин отсутствует.
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
                drawLevelBadges(
                    msg,
                    avatarCenterX,
                    avatarCenterY
                );

                if (visual.hasVerifiedMark)
                {
                    queueVerifiedMarkOnAvatar(
                        avatarCenterX,
                        avatarCenterY,
                        avatarSize
                    );
                }
            }

            // ----------------------------------------------------
            // Ник отдельной строкой
            // ----------------------------------------------------
            if (!visual.isContinuation)
            {
                const float nicknameY =
                    boxTop + bubblePaddingTop;

                const float nicknameWidth =
                    visual.displayNameWidth;

                chatTextRenderer.add(
                    chatFont,
                    visual.displayName,
                    textX,
                    nicknameY,
                    nicknameR,
                    nicknameG,
                    nicknameB,
                    1.0f
                );

                // Двоеточие выводится после ника и, при необходимости, значка.
                chatTextRenderer.add(
                    chatFont,
                    ":",
                    textX + nicknameWidth,
                    nicknameY,
                    nicknameR,
                    nicknameG,
                    nicknameB,
                    1.0f
                );
            }

            // ----------------------------------------------------
            // Текст сообщения под ником
            // ----------------------------------------------------

            const float bodyFirstLineY =
                boxTop +
                bubblePaddingTop +
                (
                    visual.isContinuation
                    ? 0.0f
                    : lineHeight + nicknameGap
                    );

            for (size_t lineIndex = 0;
                lineIndex < visual.lines.size();
                ++lineIndex)
            {
                const float lineY =
                    bodyFirstLineY +
                    lineHeight * static_cast<float>(lineIndex);

                chatTextRenderer.add(
                    chatFont,
                    visual.lines[lineIndex],
                    textX,
                    lineY,
                    1.0f,
                    1.0f,
                    1.0f,
                    1.0f
                );
            }
        }

        cursorY +=
            visual.boxHeight +
            visual.bottomMargin;
    }
    chatTextRenderer.flush(
        chatFont,
        screenW,
        screenH
    );
    recordChatStage(FrameProfiler::ChatRender);

    // Рисуем галочки поверх текста, как отдельный слой.
    const auto drawCheckTriangle =
        [&](float x1, float y1,
            float x2, float y2,
            float x3, float y3)
        {
            uiPanel.drawTriangle(
                x1, y1,
                x2, y2,
                x3, y3,
                1.0f, 1.0f, 0.0f, 1.0f,
                screenW,
                screenH
            );
        };

    for (const auto& [x, y] : verifiedMarks)
    {
        // Короткий штрих.
        drawCheckTriangle(
            x, y + 5.0f,
            x + 1.7f, y + 3.4f,
            x + 5.0f, y + 6.2f
        );

        drawCheckTriangle(
            x, y + 5.0f,
            x + 5.0f, y + 6.2f,
            x + 3.5f, y + 7.8f
        );

        // Длинный штрих.
        drawCheckTriangle(
            x + 3.5f, y + 7.8f,
            x + 5.1f, y + 9.0f,
            x + 12.0f, y + 2.0f
        );

        drawCheckTriangle(
            x + 3.5f, y + 7.8f,
            x + 12.0f, y + 2.0f,
            x + 10.5f, y + 0.6f
        );
    }
    recordChatStage(FrameProfiler::ChatVerifiedMarks);

    glDisable(GL_SCISSOR_TEST);
}
