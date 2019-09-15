#version 460 core
layout (location = 0) in vec3 gPos;

uniform mat4 gWVP;

void main()
{
    gl_Position = gWVP * vec4(gPos, 1.0f);
}
