#version 460 core
layout (location = 0) in vec3 gPos;
layout (location = 1) in vec2 gTexCoords;

out vec3 WorldPos;
out vec2 TexCoords;

uniform mat4 gWorldMatrix;

void main()
{
    WorldPos = vec3(gWorldMatrix * vec4(gPos, 1.0f));
    TexCoords = vec2(gTexCoords.x * 2.0f - 1.0f, (1.0f - gTexCoords.y) * 2.0f - 1.0f);
    gl_Position = vec4(TexCoords, 0.0f, 1.0f);
}
