#pragma once

namespace BasicShader
{
    inline constexpr const char* vertex = R"(
        #version 330 core

        layout (location = 0) in vec2 aPos;

        uniform float offsetX;
        uniform float offsetY;
        uniform float scale;

        void main()
        {
            gl_Position = vec4(
                aPos.x * scale + offsetX,
                aPos.y * scale + offsetY,
                0.0,
                1.0
            );
        }
    )";

    inline constexpr const char* fragment = R"(
        #version 330 core

        out vec4 FragColor;

        uniform vec3 color;

        void main()
        {
            FragColor = vec4(color, 1.0);
        }
    )";
}