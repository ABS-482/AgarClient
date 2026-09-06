#pragma once

namespace RoundedPanelShader
{
    inline constexpr const char* vertex = R"(
        #version 330 core

        layout (location = 0) in vec2 aPos; // единичный квад -1..1

        uniform vec2 uCenter;    // экранные пиксели
        uniform vec2 uHalfSize;  // экранные пиксели

        uniform vec2 uScreenSize;

        out vec2 vLocalPixelPos;

        void main()
        {
            vLocalPixelPos = aPos * uHalfSize;

            vec2 screenPos = uCenter + vLocalPixelPos;

            vec2 ndc = vec2(
                (screenPos.x / uScreenSize.x) * 2.0 - 1.0,
                1.0 - (screenPos.y / uScreenSize.y) * 2.0
            );

            gl_Position = vec4(ndc, 0.0, 1.0);
        }
    )";

    inline constexpr const char* fragment = R"(
        #version 330 core

        in vec2 vLocalPixelPos;

        uniform vec2 uHalfSize;
        uniform float uCornerRadius;
        uniform vec4 uFillColor;
        uniform vec4 uBorderColor;
        uniform float uBorderWidth;

        out vec4 FragColor;

        float roundedBoxSDF(vec2 p, vec2 halfSize, float radius)
        {
            vec2 q = abs(p) - halfSize + radius;
            return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
        }

        void main()
        {
            float dist = roundedBoxSDF(vLocalPixelPos, uHalfSize, uCornerRadius);

            float shapeAlpha = 1.0 - smoothstep(-1.0, 1.0, dist);

            if (shapeAlpha <= 0.0)
                discard;

            float borderMask = smoothstep(-uBorderWidth - 1.0, -uBorderWidth + 1.0, dist);

            vec4 color = mix(uFillColor, uBorderColor, borderMask);
            color.a *= shapeAlpha;

            FragColor = color;
        }
    )";
}