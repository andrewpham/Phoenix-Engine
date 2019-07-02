#version 460 core
layout (location = 0) out vec3 Position;
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec4 AlbedoSpecular;
layout (location = 4) out vec4 Metallic;

in vec3 WorldPos;
in vec2 TexCoords;
in vec3 WorldNormal;

uniform sampler2D gDiffuseMap0;
uniform sampler2D gSpecularMap0;

uniform mat4 gViewMatrix;
uniform float gMetalness;

void main()
{
    Position = vec3(gViewMatrix * vec4(WorldPos, 1.0f));
    Normal = vec3(gViewMatrix * vec4(normalize(WorldNormal), 1.0f));
    AlbedoSpecular.rgb = texture(gDiffuseMap0, TexCoords).rgb;
    AlbedoSpecular.a = texture(gSpecularMap0, TexCoords).r;
    Metallic.r = gMetalness;
}
