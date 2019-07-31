#version 460 core
out vec4 FragColor;

in VS_OUT {
    vec3 WorldPos;
    vec3 WorldNormal;
    vec2 TexCoords;
    vec4 LightSpacePos;
} fs_in;

uniform sampler2D gDiffuseTexture;
uniform sampler2D gShadowMap;

uniform vec3 gLightPos;
uniform vec3 gViewPos;
uniform vec3 gLightColor;
uniform float gAmbientFactor;
uniform float gSpecularFactor;
uniform float gCorrectionFactor;

float calcShadow(vec4 lightSpacePos)
{
    vec3 lightSpacePosPostW = lightSpacePos.xyz / lightSpacePos.w;
    lightSpacePosPostW = lightSpacePosPostW * 0.5f + 0.5f;

    if (lightSpacePosPostW.z > 1.0f)
    {
        return 1.0f;
    }

    vec2 moments = texture(gShadowMap, lightSpacePosPostW.xy).rg;
    float distance = lightSpacePosPostW.z;

    if (distance <= moments.x)
    {
        return 1.0f;
    }

    float variance = moments.y - (moments.x * moments.x);
    variance = max(variance, 0.00002f);

    float d = distance - moments.x;
    return variance / (variance + d * d); // pMax
}

float linstep(float min, float max, float v)
{
    return clamp((v - min) / (max - min), 0.0f, 1.0f);
}

float reduceLightBleeding(float pMax)
{
    // Remove the [0, gCorrectionFactor] tail and linearly rescale (gCorrectionFactor, 1].
    return linstep(gCorrectionFactor, 1.0f, pMax);
}

void main()
{
    // Blinn-Phong shading
    vec3 color = texture(gDiffuseTexture, fs_in.TexCoords).rgb;

    vec3 ambient = gAmbientFactor * color;

    vec3 N = normalize(fs_in.WorldNormal);
    vec3 L = normalize(gLightPos - fs_in.WorldPos);
    vec3 diffuse = clamp(dot(N, L), 0.0f, 1.0f) * gLightColor;

    vec3 V = normalize(gViewPos - fs_in.WorldPos);
    vec3 H = normalize(L + V);
    vec3 specular = pow(clamp(dot(N, H), 0.0f, 1.0f), gSpecularFactor) * gLightColor;

    float shadow = reduceLightBleeding(calcShadow(fs_in.LightSpacePos));
    vec3 fragColor = (ambient + shadow * (diffuse + specular)) * color;  

    FragColor = vec4(fragColor, 1.0f);
}
