#pragma once

namespace TextShader
{
    inline constexpr const char* vertex = R"(
        #version 330 core

        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aUV;

        uniform vec2 uScreenSize;
        uniform vec2 uOffset;
        uniform float uScale;

        out vec2 vUV;

        void main()
        {
            vec2 screenPos = aPos * uScale + uOffset;

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

        uniform sampler2D uAtlas;
        uniform vec3 uColor;

        out vec4 FragColor;

        void main()
        {
            float alpha = texture(uAtlas, vUV).r;
            FragColor = vec4(uColor, alpha);
        }
    )";
}