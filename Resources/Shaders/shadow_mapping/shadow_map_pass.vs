#version 460 core
layout (location = 0) in vec3 gPos;

uniform mat4 gLightSpaceVP;
uniform mat4 gWorldMatrix;

void main()
{
    gl_Position = gLightSpaceVP * gWorldMatrix * vec4(gPos, 1.0f);
}
