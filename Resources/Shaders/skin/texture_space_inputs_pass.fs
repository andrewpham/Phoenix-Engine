#version 460 core
layout (location = 0) out vec4 StretchMap;
layout (location = 1) out vec4 IrradianceMap;

const float SCALE = 0.0003f;

in vec3 WorldPos;
in vec2 TexCoords;

uniform sampler2D gDiffuseTexture;

vec2 computeStretchMap(vec3 worldCoord)
{
    vec3 derivu = dFdx(worldCoord);
    vec3 derivv = dFdy(worldCoord);
    float stretchU = SCALE / length(derivu);
    float stretchV = SCALE / length(derivv);
    return vec2(stretchU, stretchV);
}

void main()
{
    StretchMap = vec4(computeStretchMap(WorldPos), 0.0f, 1.0f);
    vec2 texCoords = TexCoords * 0.5f + 0.5f;
    texCoords.y = 1.0f - texCoords.y;
    IrradianceMap = texture(gDiffuseTexture, texCoords);
}
