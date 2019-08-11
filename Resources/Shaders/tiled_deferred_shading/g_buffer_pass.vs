#version 460 core
layout (location = 0) in vec3 gPos;
layout (location = 1) in vec2 gTexCoords;
layout (location = 2) in vec3 gNormal;

out vec2 TexCoords;
out vec3 WorldNormal;

uniform mat3 gNormalMatrix;
uniform mat4 gWVP;

void main()
{
    TexCoords = gTexCoords;
    WorldNormal = gNormalMatrix * gNormal;
    gl_Position = gWVP * vec4(gPos, 1.0f);
}
