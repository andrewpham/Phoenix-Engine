#version 460 core
layout (location = 0) in vec3 gPos;
layout (location = 1) in vec2 gTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = gTexCoords;
    gl_Position = vec4(gPos, 1.0f);
}
