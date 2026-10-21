#pragma once

namespace UiTextureShader
{
    inline constexpr const char* vertex = R"(#version 330 core

        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aUV;

        uniform vec2 uCenter;
        uniform vec2 uHalfSize;
        uniform vec2 uScreenSize;

        out vec2 vUV;

        void main()
        {
            vec2 screenPos = uCenter + aPos * uHalfSize;

            vec2 ndc = vec2(
                screenPos.x / uScreenSize.x * 2.0 - 1.0,
                1.0 - screenPos.y / uScreenSize.y * 2.0
            );

            gl_Position = vec4(ndc, 0.0, 1.0);
            vUV = aUV;
        }
    )";

    inline constexpr const char* fragment = R"(#version 330 core

        in vec2 vUV;

        uniform sampler2D uTexture;
        uniform vec4 uTint;

        out vec4 FragColor;

        void main()
        {
            vec4 texColor = texture(uTexture, vUV);

            FragColor = vec4(
                texColor.rgb * uTint.rgb,
                texColor.a * uTint.a
            );

            if (FragColor.a <= 0.001)
                discard;
        }
    )";
}