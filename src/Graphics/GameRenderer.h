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
        Camera& camera
    );

    void updatePositions(
        std::unordered_map<uint32_t, RenderState>& renderStates
    );

    void updateRenderStates(
        const WorldSnapshot& blobs,
        std::unordered_map<uint32_t, RenderState>& renderStates
    );

    InterpolationSettings& interpolation() { return m_interp; }

    void draw(
        const WorldSnapshot& blobs,
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
    std::vector<float> foodInstanceData;
    std::vector<GLuint> skinTexturesByDrawIndex;
};