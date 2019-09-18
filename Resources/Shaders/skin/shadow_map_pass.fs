#version 460 core
layout (location = 0) out vec3 ShadowMap;

in vec2 TexCoords;

void main()
{
    ShadowMap.x = gl_FragCoord.z;
    ShadowMap.y = TexCoords.x;
    ShadowMap.z = TexCoords.y;
}
