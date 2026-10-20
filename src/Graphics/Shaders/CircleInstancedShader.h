#pragma once

namespace CircleInstancedShader
{
    inline constexpr const char* vertex = R"(#version 330 core

        layout (location = 0) in vec2 aPos;

        // Данные отдельного экземпляра:
        layout (location = 1) in vec2 aCenter;
        layout (location = 2) in float aRadius;
        layout (location = 3) in vec3 aColor;
        layout (location = 4) in float aDepth;

        uniform vec2 uCameraPos;   // offsetX / offsetY
        uniform float uZoom;       // camera.ratio
        uniform vec2 uScreenSize;  // размеры окна

        out vec2 vLocalPos;
        out vec3 vColor;

        void main()
        {
            vLocalPos = aPos;
            vColor = aColor;

            vec2 worldPos =
                aCenter +
                aPos * aRadius;

            vec2 rel =
                worldPos -
                uCameraPos;

            vec2 screenPos =
                uScreenSize * 0.5 +
                rel * uZoom;

            vec2 ndc = vec2(
                (screenPos.x / uScreenSize.x) * 2.0 - 1.0,
                1.0 - (screenPos.y / uScreenSize.y) * 2.0
            );

            // У каждого экземпляра своя глубина.
            gl_Position = vec4(ndc, aDepth, 1.0);
        }
    )";

    inline constexpr const char* fragment = R"(#version 330 core

        in vec2 vLocalPos;
        in vec3 vColor;

        out vec4 FragColor;

        void main()
        {
            float dist = length(vLocalPos);
            float edge = fwidth(dist);

            float alpha =
                1.0 -
                smoothstep(
                    1.0 - edge,
                    1.0 + edge,
                    dist
                );

            if (alpha <= 0.0)
                discard;

            FragColor = vec4(vColor, alpha);
        }
    )";
}