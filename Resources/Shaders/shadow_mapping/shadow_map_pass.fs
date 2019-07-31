#version 460 core
layout (location = 0) out vec3 Position;
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec3 Flux;
layout (location = 3) out float Depth;

const vec3 LIGHT_COLOR = vec3(0.3f);

in vec3 WorldPos;
in vec2 TexCoords;
in vec3 WorldNormal;

uniform sampler2D gDiffuseTexture;

void main()
{
    Position = WorldPos;
    Normal = normalize(WorldNormal);
    Flux = texture(gDiffuseTexture, TexCoords).rgb * LIGHT_COLOR;
    Depth = gl_FragCoord.z;
}
