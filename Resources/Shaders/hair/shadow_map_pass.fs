#version 460 core
layout (location = 0) out vec3 Depth;

void main()
{
    Depth.r = gl_FragCoord.z;
}
