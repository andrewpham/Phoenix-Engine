#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gOutput;

void main()
{
    FragColor = texture(gOutput, TexCoords);
}
