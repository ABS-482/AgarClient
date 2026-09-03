#pragma once

namespace CircleShader
{
    inline constexpr const char* vertex = R"(
        #version 330 core

        layout (location = 0) in vec2 aPos; // квад -1..1

        uniform vec2 uCenter;
        uniform float uRadius;
        uniform vec2 uCameraPos;
        uniform float uZoom;
        uniform vec2 uScreenSize;

        out vec2 vLocalPos;

        void main()
        {
            vLocalPos = aPos;

            vec2 worldPos = uCenter + aPos * uRadius;
            vec2 rel = worldPos - uCameraPos;

            vec2 ndc = vec2(
                (rel.x / (uScreenSize.x * 0.5)) * uZoom,
                -(rel.y / (uScreenSize.y * 0.5)) * uZoom
            );

            gl_Position = vec4(ndc, 0.0, 1.0);
        }
    )";

    inline constexpr const char* fragment = R"(
        #version 330 core

        in vec2 vLocalPos;

        uniform vec3 uColor;

        out vec4 FragColor;

        void main()
        {
            float dist = length(vLocalPos);

            // Ширина сглаживания адаптируется под масштаб экрана (fwidth) —
            // край остаётся чётким и гладким при любом зуме камеры.
            float edge = fwidth(dist);
            float alpha = 1.0 - smoothstep(1.0 - edge, 1.0 + edge, dist);

            if (alpha <= 0.0)
                discard;

            FragColor = vec4(uColor, alpha);
        }
    )";
}