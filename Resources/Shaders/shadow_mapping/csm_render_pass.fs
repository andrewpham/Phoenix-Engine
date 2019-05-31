#version 460 core
out vec4 FragColor;

const int NUM_CASCADES = 3;

in VS_OUT {
    vec3 WorldPos;
    vec3 WorldNormal;
    vec2 TexCoords;
    vec4 LightSpacePos[NUM_CASCADES];
    float ClipSpacePosZ;
} fs_in;

uniform sampler2D gDiffuseTexture;
uniform sampler2D gShadowMap[NUM_CASCADES];

uniform vec3 gLightPos;
uniform vec3 gViewPos;
uniform vec3 gLightColor;
uniform float gAmbientFactor;
uniform float gSpecularFactor;
uniform float gCorrectionFactor;

uniform float gClipSpaceCascadeEnds[NUM_CASCADES];

float calcShadow(vec4 lightSpacePos, int cascadeIndex)
{
    vec3 lightSpacePosPostW = lightSpacePos.xyz / lightSpacePos.w;
    lightSpacePosPostW = lightSpacePosPostW * 0.5f + 0.5f;

    float closestDistance = texture(gShadowMap[cascadeIndex], lightSpacePosPostW.xy).r; 
    float distance = lightSpacePosPostW.z;

    vec3 N = normalize(fs_in.WorldNormal);
    vec3 L = normalize(gLightPos - fs_in.WorldPos);
    float bias = max(0.05f * (1.0f - dot(N, L)), 0.005f);

    // With correction for artifacts from extending beyond the light frustum's far plane.
    return mix(step(1.0f, distance), 1.0f, clamp(exp(gCorrectionFactor * (closestDistance - distance)), 0.0f, 1.0f));
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

    float shadow = 0.0f;
    for (int i = 0; i < NUM_CASCADES; ++i) {
        if (fs_in.ClipSpacePosZ <= gClipSpaceCascadeEnds[i])
        {
            shadow = calcShadow(fs_in.LightSpacePos[i], i);
            break;
        }
    }
    vec3 fragColor = (ambient + shadow * (diffuse + specular)) * color;

    FragColor = vec4(fragColor, 1.0f);
}
