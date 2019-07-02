#version 460 core
layout (location = 0) in vec3 gPos;
layout (location = 1) in vec2 gTexCoords;
layout (location = 2) in vec3 gNormal;

out vec3 WorldPos;
out vec2 TexCoords;
out vec3 WorldNormal;

uniform mat4 gWorldMatrix;
uniform mat3 gNormalMatrix;
uniform mat4 gVP;

void main()
{
    WorldPos = (gWorldMatrix * vec4(gPos, 1.0f)).xyz;
    TexCoords = gTexCoords;
    WorldNormal = gNormalMatrix * gNormal;
    gl_Position = gVP * vec4(WorldPos, 1.0f);
}
