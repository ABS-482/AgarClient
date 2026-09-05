#pragma once

namespace CircleInstancedShader
{
    inline constexpr const char* vertex = R"(
        #version 330 core

        layout (location = 0) in vec2 aPos;      // единичный квад, на вершину

        layout (location = 1) in vec2 aCenter;   // на инстанс
        layout (location = 2) in float aRadius;  // на инстанс
        layout (location = 3) in vec3 aColor;    // на инстанс

        uniform vec2 uCameraPos;
        uniform float uZoom;
        uniform vec2 uScreenSize;

        out vec2 vLocalPos;
        out vec3 vColor;

        void main()
        {
            vLocalPos = aPos;
            vColor = aColor;

            vec2 worldPos = aCenter + aPos * aRadius;
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
        in vec3 vColor;

        out vec4 FragColor;

        void main()
        {
            float dist = length(vLocalPos);
            float edge = fwidth(dist);
            float alpha = 1.0 - smoothstep(1.0 - edge, 1.0 + edge, dist);

            if (alpha <= 0.0)
                discard;

            FragColor = vec4(vColor, alpha);
        }
    )";
}