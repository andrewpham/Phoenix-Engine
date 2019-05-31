#version 460 core
layout (location = 0) in vec3 gPos;

out vec3 WorldPos;

uniform mat4 gVP;

void main()
{
    WorldPos = gPos;
    gl_Position =  gVP * vec4(WorldPos, 1.0f);
}
