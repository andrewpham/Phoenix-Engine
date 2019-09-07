#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gRenderTarget;

void main()
{
    FragColor = vec4(vec3(texture(gRenderTarget, TexCoords).r), 1.0f);
}
