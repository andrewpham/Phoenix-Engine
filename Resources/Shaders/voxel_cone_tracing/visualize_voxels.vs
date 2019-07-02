#version 460 core
layout (location = 0) in vec3 gPos;

out vec2 TexCoords;

void main()
{
    TexCoords = gPos.xy * 0.5f + 0.5f;
    gl_Position = vec4(gPos, 1.0f);
}
