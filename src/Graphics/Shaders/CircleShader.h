#pragma once

namespace CircleShader
{
    inline constexpr const char* vertex = R"(#version 330 core

        layout (location = 0) in vec2 aPos; // квадрат -1..1

        uniform vec2 uCenter;
        uniform float uRadius;

        // Новая Camera.
        uniform vec2 uCameraPos;
        uniform float uZoom;
        uniform vec2 uScreenSize;
        uniform vec2 uViewportSize;
        uniform float uDepth;

        out vec2 vLocalPos;

        void main()
        {
            vLocalPos = aPos;

            // Локальные координаты круга -> мировые координаты.
            vec2 worldPos =
                uCenter +
                aPos * uRadius;

            vec2 rel =
                worldPos -
                uCameraPos;

            // Масштаб с учётом viewport и зума камеры.
            vec2 scale =
                (uScreenSize / uViewportSize) / uZoom;

            vec2 screenPos =
                uScreenSize * 0.5 +
                rel * scale;

            // Экранные координаты -> OpenGL NDC.
            vec2 ndc = vec2(
                (screenPos.x / uScreenSize.x) * 2.0 - 1.0,
                1.0 - (screenPos.y / uScreenSize.y) * 2.0
            );

            gl_Position = vec4(ndc, uDepth, 1.0);
        }
    )";

    inline constexpr const char* fragment = R"(#version 330 core

        in vec2 vLocalPos;

        uniform vec3 uColor;
        uniform float uOutlineWidth;

        out vec4 FragColor;

        void main()
        {
            float dist = length(vLocalPos);
            float edge = fwidth(dist);

            float alpha;

            // Обычный заполненный круг.
            if (uOutlineWidth <= 0.0)
            {
                alpha =
                    1.0 -
                    smoothstep(
                        1.0 - edge,
                        1.0 + edge,
                        dist
                    );
            }
            else
            {
                // Внешний край обводки.
                float outerAlpha =
                    1.0 -
                    smoothstep(
                        1.0 - edge,
                        1.0 + edge,
                        dist
                    );

                // Внутренний край обводки.
                float innerRadius =
                    1.0 - uOutlineWidth;

                float innerAlpha =
                    smoothstep(
                        innerRadius - edge,
                        innerRadius + edge,
                        dist
                    );

                alpha =
                    outerAlpha *
                    innerAlpha;
            }

            if (alpha <= 0.0)
                discard;

            FragColor = vec4(
                uColor,
                alpha
            );
        }
    )";
}