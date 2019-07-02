#version 460 core
layout (location = 0) in vec3 gPos;
layout (location = 2) in vec3 gNormal;

out vec3 WorldPos;
out vec3 WorldNormal;

uniform mat4 gWorldMatrix;
uniform mat4 gVP;
uniform mat3 gNormalMatrix;

void main()
{
    WorldPos = vec3(gWorldMatrix * vec4(gPos, 1.0f));
    WorldNormal = normalize(gNormalMatrix * gNormal);
    gl_Position = gVP * vec4(WorldPos, 1.0f);
}
