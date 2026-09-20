#pragma once

namespace ChatSkinShader
{
    inline constexpr const char* vertex = R"(
#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;

uniform vec2 uCenter;
uniform float uRadius;
uniform vec2 uScreenSize;

out vec2 vUV;

void main()
{
    vec2 screenPos = uCenter + aPos * uRadius;

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
uniform float uRadius;
uniform float uBorderWidth;
uniform vec3 uBorderColor;

out vec4 FragColor;

void main()
{
    // Расстояние от центра круга в диапазоне примерно 0..1.
    vec2 centered = vUV * 2.0 - 1.0;
    float distanceFromCenter = length(centered);

    // Сглаживание внешнего края круга.
    float edge = fwidth(distanceFromCenter);

    if (distanceFromCenter > 1.0 + edge)
        discard;

    // Радиус внутренней части после вычета белой рамки.
    float innerRadius =
        1.0 - (uBorderWidth / uRadius);

    float borderMask = 1.0 - smoothstep(
        innerRadius - edge,
        innerRadius + edge,
        distanceFromCenter
    );

    vec4 skin = texture(uSkin, vUV);

    vec3 color = mix(
        uBorderColor,
        skin.rgb,
        borderMask
    );

    float alpha = 1.0 - smoothstep(
        1.0 - edge,
        1.0 + edge,
        distanceFromCenter
    );

    FragColor = vec4(color, alpha);
}
)";
}