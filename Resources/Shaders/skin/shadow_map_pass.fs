#version 460 core
layout (location = 0) out vec3 Output;

in vec2 TexCoords;

void main()
{
    Output.x = gl_FragCoord.z;
    Output.y = TexCoords.x;
    Output.z = TexCoords.y;
}
