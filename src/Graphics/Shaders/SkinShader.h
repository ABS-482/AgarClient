#pragma once

namespace SkinShader
{
    inline constexpr const char* vertex = R"(
        #version 330 core

        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aUV;

        uniform vec2 uCenter;
        uniform float uRadius;
        uniform vec2 uCameraPos;
        uniform float uZoom;
        uniform vec2 uScreenSize;

        out vec2 vUV;

        void main()
        {
            vec2 worldPos =
                uCenter +
                aPos * uRadius;

            vec2 screenPos =
                (worldPos - uCameraPos) * uZoom +
                uScreenSize * 0.5;

            vec2 ndc = vec2(
                (screenPos.x / uScreenSize.x) * 2.0 - 1.0,
                1.0 - (screenPos.y / uScreenSize.y) * 2.0
            );

            gl_Position = vec4(ndc, 0.0, 1.0);

            vUV = aUV;
        }
    )";

    inline constexpr const char* fragment = R"(
        #version 330 core

        in vec2 vUV;

        uniform sampler2D uSkin;

        out vec4 FragColor;

        void main()
        {
            FragColor = texture(uSkin, vUV);
        }
    )";
}