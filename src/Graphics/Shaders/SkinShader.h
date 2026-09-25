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
        uniform vec2 uViewportSize;

        out vec2 vUV;

        void main()
        {
            vec2 worldPos =
                uCenter +
                aPos * uRadius;

            vec2 ndc = vec2(
                ((worldPos.x - uCameraPos.x) /
                    (uViewportSize.x * 0.5)) / uZoom,

                ((worldPos.y - uCameraPos.y) /
                    (uViewportSize.y * 0.5)) / uZoom
            );

            gl_Position = vec4(ndc, 0.0, 1.0);

            vUV = vec2(
                aUV.x,
                1.0 - aUV.y
            );
        }
    )";

    inline constexpr const char* fragment = R"(
        #version 330 core

        in vec2 vUV;

        uniform sampler2D uSkin;

        out vec4 FragColor;

        void main()
        {
            vec2 centered = vUV - vec2(0.5);
            float dist = length(centered) * 2.0; // нормализуем к диапазону ~0..1, как в uv-радиусе 0.5

            float edge = fwidth(dist);
            float alpha = 1.0 - smoothstep(1.0 - edge, 1.0 + edge, dist);

            if (alpha <= 0.0)
                discard;

            vec4 texColor = texture(uSkin, vUV);
            FragColor = vec4(texColor.rgb, texColor.a * alpha);
        }
    )";
}