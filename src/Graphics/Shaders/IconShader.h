#pragma once

namespace IconShader
{
    inline constexpr const char* vertex = R"(
        #version 330 core

        layout (location = 0) in vec2 aPos;

        uniform vec2 uCenter;
        uniform vec2 uHalfSize;
        uniform vec2 uScreenSize;

        out vec2 vUV;

        void main()
        {
            vec2 pixelPos = aPos * uHalfSize + uCenter;

            vec2 ndc = vec2(
                (pixelPos.x / uScreenSize.x) * 2.0 - 1.0,
                1.0 - (pixelPos.y / uScreenSize.y) * 2.0
            );

            gl_Position = vec4(ndc, 0.0, 1.0);

            vUV = vec2(
                aPos.x * 0.5 + 0.5,
                aPos.y * 0.5 + 0.5
            );
        }
    )";

    inline constexpr const char* fragment = R"(
        #version 330 core

        in vec2 vUV;

        uniform sampler2D uIcon;

        out vec4 FragColor;

        void main()
        {
            FragColor = texture(uIcon, vUV);
        }
    )";
}