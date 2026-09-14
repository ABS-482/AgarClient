#pragma once

namespace RoundedCornersPanelShader
{
    inline constexpr const char* vertex = R"(
        #version 330 core

        layout (location = 0) in vec2 aPos;

        uniform vec2 uCenter;
        uniform vec2 uHalfSize;
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

        // x = top-left
        // y = top-right
        // z = bottom-right
        // w = bottom-left
        uniform vec4 uCornerRadii;

        uniform vec4 uFillColor;
        uniform vec4 uBorderColor;
        uniform float uBorderWidth;

        out vec4 FragColor;

        float roundedBoxSDF(
            vec2 p,
            vec2 halfSize,
            vec4 radii
        )
        {
            float radius;

            if (p.x < 0.0)
            {
                if (p.y < 0.0)
                    radius = radii.x; // top-left
                else
                    radius = radii.w; // bottom-left
            }
            else
            {
                if (p.y < 0.0)
                    radius = radii.y; // top-right
                else
                    radius = radii.z; // bottom-right
            }

            radius = min(
                radius,
                min(halfSize.x, halfSize.y)
            );

            vec2 q = abs(p) - halfSize + radius;

            return length(max(q, 0.0))
                 + min(max(q.x, q.y), 0.0)
                 - radius;
        }

        void main()
        {
            float dist = roundedBoxSDF(
                vLocalPixelPos,
                uHalfSize,
                uCornerRadii
            );

            float shapeAlpha =
                1.0 - smoothstep(-1.0, 1.0, dist);

            if (shapeAlpha <= 0.0)
                discard;

            float borderMask =
                smoothstep(
                    -uBorderWidth - 1.0,
                    -uBorderWidth + 1.0,
                    dist
                );

            vec4 color =
                mix(
                    uFillColor,
                    uBorderColor,
                    borderMask
                );

            color.a *= shapeAlpha;

            FragColor = color;
        }
    )";
}