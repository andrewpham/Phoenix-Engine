#version 460 core
out vec4 FragColor;

const vec3 SPECULAR_COLOR = vec3(0.0001f);

in VS_OUT {
    vec3 WorldPos;
    vec3 WorldNormal;
    vec2 TexCoords;
    vec4 LightSpacePos;
} fs_in;

uniform sampler2D gDiffuseTexture;
uniform sampler2D gShadowMap;
uniform sampler2D gNormalMap;
uniform sampler2D gBeckmannTexture;
uniform sampler2D gRhoDTexture;
uniform sampler2D gStretchMap;
uniform sampler2D gSpecularTexture;
uniform sampler2D gIrradianceMaps[6];

uniform vec3 gLightPos;
uniform vec3 gViewPos;
uniform vec3 gLightColor;
uniform vec3 gLightShadow;
uniform float gAmbientFactor;
uniform float gSpecularFactor;

const vec3 T = normalize(dFdx(fs_in.WorldPos) * dFdy(fs_in.TexCoords).t - dFdy(fs_in.WorldPos) * dFdx(fs_in.TexCoords).t);
vec3 calcWorldSpaceNormal(vec3 tangentSpaceNormal)
{
    vec3 N = normalize(fs_in.WorldNormal);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentSpaceNormal);
}
const vec3 TANGENT_SPACE_N = texture(gNormalMap, fs_in.TexCoords).xyz * 2.0f - 1.0f;
const vec3 N = calcWorldSpaceNormal(TANGENT_SPACE_N);
const vec3 L = normalize(gLightPos - fs_in.WorldPos);

vec4 calcBlinnPhongColor()
{
    vec3 color = texture(gDiffuseTexture, fs_in.TexCoords).rgb;

    vec3 ambient = gAmbientFactor * color;

    vec3 diffuse = clamp(dot(N, L), 0.0f, 1.0f) * gLightColor;

    vec3 V = normalize(gViewPos - fs_in.WorldPos);
    vec3 H = normalize(L + V);
    vec3 specular = SPECULAR_COLOR * pow(clamp(dot(N, H), 0.0f, 1.0f), gSpecularFactor) * gLightColor;

    return vec4((ambient + diffuse + specular) * color, 1.0f);
}

void main()
{
    FragColor = calcBlinnPhongColor();

}
