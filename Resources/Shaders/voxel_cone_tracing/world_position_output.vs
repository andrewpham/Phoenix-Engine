#version 460 core
layout (location = 0) in vec3 gPos;

out vec3 WorldPos;

uniform mat4 gWorldMatrix;
uniform mat4 gVP;

void main()
{
    WorldPos = vec3(gWorldMatrix * vec4(gPos, 1.0f));
    gl_Position = gVP * vec4(WorldPos, 1.0f);
}
