#pragma once

#include <glad/glad.h>

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "Shader.h"
#include "Camera.h"
#include "CircleMesh.h"
#include "Font.h"
#include "InstancedCircleRenderer.h"
#include "SkinManager.h"
#include "SkinMesh.h"
#include "TextRenderer.h"
#include "LineRenderer.h"

#include "../Game/RenderState.h"
#include "../Game/World.h"
#include "../Game/SplitPrediction.h"

class VirusGuard;
class FrameProfiler;
class UserSkinDatabase;
class GameRenderer
{
public:
    GameRenderer(
        Shader& circleShader,
        Shader& skinShader,
        Shader& circleInstancedShader,
        CircleMesh& circleMesh,
        SkinMesh& skinMesh,
        InstancedCircleRenderer& foodRenderer,
        SkinManager& skinManager,
        TextRenderer& textRenderer,
        Font& font,
        Camera& camera,
        UserSkinDatabase& userSkinDatabase
    );

    void updatePositions(
        std::unordered_map<uint32_t, RenderState>& renderStates,
        float deltaTime
    );

    void updateRenderStates(
        const WorldSnapshot& blobs,
        std::unordered_map<uint32_t, RenderState>& renderStates,
        float segDuration
    );

    InterpolationSettings& interpolation() { return m_interp; }

    void draw(
        const WorldSnapshot& blobs,
        std::uint64_t foodVersion,
        const std::vector<uint32_t>& ownedIds,
        std::unordered_map<uint32_t, RenderState>& renderStates,
        float screenW,
        float screenH,
        float targetWorldX,
        float targetWorldY,
        FrameProfiler& profiler,
        const VirusGuard* virusGuard = nullptr,
        bool splitPreviewHeld = false
    );

private:
    InterpolationSettings m_interp;
    Shader& circleShader;
    Shader& skinShader;
    Shader& circleInstancedShader;

    CircleMesh& circleMesh;
    SkinMesh& skinMesh;
    InstancedCircleRenderer& foodRenderer;
    UserSkinDatabase& userSkinDatabase;
    SkinManager& skinManager;

    TextRenderer& textRenderer;
    Font& font;
    Camera& camera;
    LineRenderer lineRenderer;
    SplitPrediction splitPrediction;
    GLint uCenter;
    GLint uRadius;
    GLint uCameraPos;
    GLint uZoom;
    GLint uScreenSize;
    GLint uColor;
    GLint uOutlineWidth;
    GLint uDepth;
    GLint skinCenter;
    GLint skinRadius;
    GLint skinCameraPos;
    GLint skinZoom;
    GLint skinScreenSize;
    GLint skinTexture;
    GLint skinBaseColor;
    GLint skinDepth;
    GLint iCameraPos;
    GLint iZoom;
    GLint iScreenSize;

    GLint uViewportSize;
    GLint skinViewportSize;
    GLint iViewportSize;

    std::vector<DrawEntry> drawList;

    // CPU-кэш статической еды. Перестраивается при изменении World.
    std::vector<float> m_staticFood;

    // Динамические круги: выброшенная масса и клетки без готового скина.
    std::vector<float> dynamicInstanceData;

    // Версия еды, загруженная в GPU-буфер.
    std::uint64_t m_foodVersionDrawn = 0;
    bool m_staticFoodInitialized = false;

    std::vector<GLuint> skinTexturesByDrawIndex;
};