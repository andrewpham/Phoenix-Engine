#version 460 core
out vec4 FragColor;

const float PRIMARY_SHIFT = -1.0f;
const float SECONDARY_SHIFT = 1.5f;
const float PRIMARY_SPECULAR_EXP = 0.98f;
const float SECONDARY_SPECULAR_EXP = 0.49f;
const vec3 DIFFUSE_COLOR = vec3(0.416f, 0.424f, 0.431f);
const vec3 SECONDARY_SPECULAR_COLOR = DIFFUSE_COLOR;

in VS_OUT {
    vec3 WorldPos;
    vec3 WorldNormal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D gBaseTexture;
uniform sampler2D gNormalMap;
uniform sampler2D gAOMap;
uniform sampler2D gSpecularMap;
uniform sampler2D gAlphaTexture;
uniform sampler2D gShiftTexture;
uniform sampler2D gNoiseTexture;

uniform vec3 gLightPos;
uniform vec3 gViewPos;
uniform vec3 gLightColor;
uniform float gAmbientFactor;
uniform int gRenderMode;

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

vec3 shiftTangent(float shift)
{
    vec3 shiftedT = T + shift * N;
    return normalize(shiftedT);
}

float calcStrandSpecularLighting(vec3 T, float exponent)
{
    vec3 V = normalize(gViewPos - fs_in.WorldPos);
    vec3 H = normalize(L + V);
    float ToH = dot(T, H);
    return smoothstep(-1.0f, 0.0f, ToH) * pow(sqrt(1.0f - ToH * ToH), exponent);
}

vec4 calcHairColor()
{
    vec3 color = texture(gBaseTexture, fs_in.TexCoords).rgb;

    float baseShiftAmount = texture(gShiftTexture, fs_in.TexCoords).r - 0.5f;
    vec3 t1 = shiftTangent(PRIMARY_SHIFT + baseShiftAmount);
    vec3 t2 = shiftTangent(SECONDARY_SHIFT + baseShiftAmount);

    vec3 ambient = gAmbientFactor * color;

    vec3 diffuse = clamp(mix(0.25f, 1.0f, dot(N, L)), 0.0f, 1.0f) * DIFFUSE_COLOR * gLightColor;

    vec3 specular1 = texture(gSpecularMap, fs_in.TexCoords).rgb * calcStrandSpecularLighting(t1, PRIMARY_SPECULAR_EXP) * gLightColor;

    float mask = texture(gNoiseTexture, fs_in.TexCoords).r;
    vec3 specular2 = SECONDARY_SPECULAR_COLOR * calcStrandSpecularLighting(t2, SECONDARY_SPECULAR_EXP) * gLightColor;

    vec4 hairColor;
    hairColor.a = texture(gAlphaTexture, fs_in.TexCoords).r;
    if (gRenderMode == 2)
    {
        hairColor.rgb = diffuse * color;
        return hairColor;
    }
    else if (gRenderMode == 3)
    {
        hairColor.rgb = specular1 * color;
        return hairColor;
    }
    else if (gRenderMode == 4)
    {
        hairColor.rgb = specular2 * color;
        return hairColor;
    }
    else if (gRenderMode == 5)
    {
        hairColor.rgb = (specular1 + mask * specular2) * color;
        return hairColor;
    }
    else if (gRenderMode == 6)
    {
        hairColor.rgb = vec3(texture(gAOMap, fs_in.TexCoords).r);
        return hairColor;
    }
    hairColor.rgb = (ambient + diffuse + specular1 + mask * specular2) * color;
    hairColor.rgb *= texture(gAOMap, fs_in.TexCoords).r;
    return hairColor;
}

void main()
{
    FragColor = calcHairColor();
}
