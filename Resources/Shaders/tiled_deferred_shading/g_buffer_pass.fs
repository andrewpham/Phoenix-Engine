#version 460 core
layout (location = 0) out vec4 Normal;
layout (location = 1) out vec4 AlbedoSpecular;

in vec2 TexCoords;
in vec3 WorldNormal;

uniform sampler2D gDiffuseMap0;
uniform sampler2D gSpecularMap0;

void main()
{
    Normal.xyz = normalize(WorldNormal);
    Normal.w = gl_FragCoord.z;
    AlbedoSpecular.rgb = texture(gDiffuseMap0, TexCoords).rgb;
    AlbedoSpecular.a = texture(gSpecularMap0, TexCoords).r;
}
