#version 460 core
layout (location = 0) in vec3 gPos;
layout (location = 2) in vec3 gNormal;

uniform mat4 gWorldMatrix;
uniform mat3 gNormalMatrix;

out vec3 WorldPosGS;
out vec3 WorldNormalGS;

void main()
{
    WorldPosGS = vec3(gWorldMatrix * vec4(gPos, 1.0f));
    WorldNormalGS = normalize(gNormalMatrix * gNormal);
}
