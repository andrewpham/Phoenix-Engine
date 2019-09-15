#version 460 core
layout (location = 0) in vec3 gPos;
layout (location = 1) in vec2 gTexCoords;

out vec2 TexCoords;

uniform mat4 gLightSpaceVP;
uniform mat4 gWorldMatrix;

void main()
{
    TexCoords = gTexCoords;
    gl_Position = gLightSpaceVP * gWorldMatrix * vec4(gPos, 1.0f);
}
