#pragma once

namespace TextShader
{
    inline constexpr const char* vertex = R"(
        #version 330 core

        layout (location = 0) in vec2 aPos; // уже в абсолютных экранных пикселях
        layout (location = 1) in vec2 aUV;
        layout (location = 2) in vec3 aColor;

        uniform vec2 uScreenSize;

        out vec2 vUV;
        out vec3 vColor;

        void main()
        {
            vec2 ndc = vec2(
                (aPos.x / uScreenSize.x) * 2.0 - 1.0,
                1.0 - (aPos.y / uScreenSize.y) * 2.0
            );

            gl_Position = vec4(ndc, 0.0, 1.0);
            vUV = aUV;
            vColor = aColor;
        }
    )";

    inline constexpr const char* fragment = R"(
        #version 330 core

        in vec2 vUV;
        in vec3 vColor;

        uniform sampler2D uAtlas;

        uniform vec3 uColor;
        uniform vec3 uBorderColor;
        uniform float uAlphaMultiplier;

        uniform int uUseVertexColor;

        out vec4 FragColor;

        void main()
        {
            vec2 rg = texture(uAtlas, vUV).rg;

            float fill = rg.r;
            float shape = max(rg.g, rg.r);

            vec3 baseColor =
                uUseVertexColor != 0
                ? vColor
                : uColor;

            vec3 color =
                mix(
                    uBorderColor,
                    baseColor,
                    fill
                );

            float alpha =
                shape *
                uAlphaMultiplier;

            if (alpha <= 0.0)
                discard;

            FragColor =
                vec4(
                    color,
                    alpha
                );
        }
    )";
}