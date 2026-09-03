#pragma once

namespace CircleShader
{
    inline constexpr const char* vertex = R"(
        #version 330 core

        layout (location = 0) in vec2 aPos;

        uniform vec2 uCenter;
        uniform float uRadius;
        uniform vec2 uCameraPos;
        uniform float uZoom;
        uniform vec2 uScreenSize;

        void main()
        {
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

        uniform vec3 uColor;

        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(uColor, 1.0);
        }
    )";
}