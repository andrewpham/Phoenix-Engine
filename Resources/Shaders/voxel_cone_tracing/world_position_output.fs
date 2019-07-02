#version 460 core
out vec4 FragColor;

in vec3 WorldPos;

void main()
{
    FragColor.rgb = WorldPos;
}
