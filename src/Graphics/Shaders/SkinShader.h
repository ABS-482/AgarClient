#pragma once

namespace SkinShader
{
    inline constexpr const char* vertex = R"(#version 330 core

        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aUV;

        uniform vec2 uCenter;
        uniform float uRadius;

        // JS camera
        uniform vec2 uCameraPos;   // offsetX / offsetY
        uniform float uZoom;       // camera.ratio
        uniform vec2 uScreenSize;  // реальные размеры окна
        uniform float uDepth;

        out vec2 vUV;

        void main()
        {
            vec2 worldPos =
                uCenter +
                aPos * uRadius;

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

            gl_Position = vec4(ndc, uDepth, 1.0);

            vUV = aUV;
        }
    )";

    inline constexpr const char* fragment = R"(#version 330 core

        in vec2 vUV;

        uniform sampler2D uSkin;
        uniform vec3 uBaseColor;

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

            // Прозрачные участки скина заполняются цветом клетки.
            vec3 color = mix(
                uBaseColor,
                texColor.rgb,
                texColor.a
            );

            // Круглая маска по-прежнему отвечает за сглаженный внешний край.
            FragColor = vec4(color, alpha);
        }
    )";
}