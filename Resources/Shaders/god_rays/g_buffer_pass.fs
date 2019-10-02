#version 460 core
layout (location = 0) out vec3 Position;
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec4 AlbedoSpecular;

in vec3 WorldPos;
in vec2 TexCoords;
in vec3 WorldNormal;

uniform sampler2D gDiffuseMap0;
uniform sampler2D gSpecularMap0;

void main()
{
    Position = WorldPos;
    Normal = normalize(WorldNormal);
    AlbedoSpecular.rgb = texture(gDiffuseMap0, TexCoords).rgb;
    AlbedoSpecular.a = texture(gSpecularMap0, TexCoords).r;
}
