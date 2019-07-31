#version 460 core
out vec4 FragColor;

const float NEAR = 0.1f;
const float KERNEL_SIZE = 143.36f;
const int NUM_SAMPLES = 64;
const int CORRECTION_FACTOR = 100;
const vec2 POISSON_DISK[NUM_SAMPLES] = vec2[]
    (vec2(-0.613392f, 0.617481f),
     vec2(0.170019f, -0.040254f),
     vec2(-0.299417f, 0.791925f),
     vec2(0.645680f, 0.493210f),
     vec2(-0.651784f, 0.717887f),
     vec2(0.421003f, 0.027070f),
     vec2(-0.817194f, -0.271096f),
     vec2(-0.705374f, -0.668203f),
     vec2(0.977050f, -0.108615f),
     vec2(0.063326f, 0.142369f),
     vec2(0.203528f, 0.214331f),
     vec2(-0.667531f, 0.326090f),
     vec2(-0.098422f, -0.295755f),
     vec2(-0.885922f, 0.215369f),
     vec2(0.566637f, 0.605213f),
     vec2(0.039766f, -0.396100f),
     vec2(0.751946f, 0.453352f),
     vec2(0.078707f, -0.715323f),
     vec2(-0.075838f, -0.529344f),
     vec2(0.724479f, -0.580798f),
     vec2(0.222999f, -0.215125f),
     vec2(-0.467574f, -0.405438f),
     vec2(-0.248268f, -0.814753f),
     vec2(0.354411f, -0.887570f),
     vec2(0.175817f, 0.382366f),
     vec2(0.487472f, -0.063082f),
     vec2(-0.084078f, 0.898312f),
     vec2(0.488876f, -0.783441f),
     vec2(0.470016f, 0.217933f),
     vec2(-0.696890f, -0.549791f),
     vec2(-0.149693f, 0.605762f),
     vec2(0.034211f, 0.979980f),
     vec2(0.503098f, -0.308878f),
     vec2(-0.016205f, -0.872921f),
     vec2(0.385784f, -0.393902f),
     vec2(-0.146886f, -0.859249f),
     vec2(0.643361f, 0.164098f),
     vec2(0.634388f, -0.049471f),
     vec2(-0.688894f, 0.007843f),
     vec2(0.464034f, -0.188818f),
     vec2(-0.440840f, 0.137486f),
     vec2(0.364483f, 0.511704f),
     vec2(0.034028f, 0.325968f),
     vec2(0.099094f, -0.308023f),
     vec2(0.693960f, -0.366253f),
     vec2(0.678884f, -0.204688f),
     vec2(0.001801f, 0.780328f),
     vec2(0.145177f, -0.898984f),
     vec2(0.062655f, -0.611866f),
     vec2(0.315226f, -0.604297f),
     vec2(-0.780145f, 0.486251f),
     vec2(-0.371868f, 0.882138f),
     vec2(0.200476f, 0.494430f),
     vec2(-0.494552f, -0.711051f),
     vec2(0.612476f, 0.705252f),
     vec2(-0.578845f, -0.768792f),
     vec2(-0.772454f, -0.090976f),
     vec2(0.504440f, 0.372295f),
     vec2(0.155736f, 0.065157f),
     vec2(0.391522f, 0.849605f),
     vec2(-0.620106f, -0.328104f),
     vec2(0.789239f, -0.419965f),
     vec2(-0.545396f, 0.538133f),
     vec2(-0.178564f, -0.596057f));

in VS_OUT {
    vec3 WorldPos;
    vec3 WorldNormal;
    vec2 TexCoords;
    vec4 LightSpacePos;
} fs_in;

uniform sampler2D gDiffuseTexture;
uniform sampler2D gPositionMap;
uniform sampler2D gNormalMap;
uniform sampler2D gFluxMap;
uniform sampler2D gShadowMap;
uniform sampler3D gAnglesTexture;

uniform vec3 gLightPos;
uniform vec3 gViewPos;
uniform vec3 gLightColor;
uniform float gAmbientFactor;
uniform float gSpecularFactor;
uniform float gCalibratedLightSize;
uniform int gRenderMode;

const vec2 RANDOM_VALUES = vec2(texture(gAnglesTexture, fs_in.WorldPos * CORRECTION_FACTOR).r,
                                texture(gAnglesTexture, fs_in.WorldPos * CORRECTION_FACTOR).g);
const vec2 ROTATION = RANDOM_VALUES * 2 - 1;
const vec2 TEXEL_SIZE = 1.0f / textureSize(gShadowMap, 0);
const vec3 N = normalize(fs_in.WorldNormal);
const vec3 L = normalize(gLightPos - fs_in.WorldPos);
const float NoL = dot(N, L);
const vec3 LIGHT_SPACE_POS_POST_W = fs_in.LightSpacePos.xyz / fs_in.LightSpacePos.w * 0.5f + 0.5f;

vec3 calcIndirectLighting()
{
    vec3 indirectLighting = vec3(0.0f);

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        vec2 offset = vec2(
            ROTATION.x * POISSON_DISK[i].x - ROTATION.y * POISSON_DISK[i].y,
            ROTATION.y * POISSON_DISK[i].x + ROTATION.x * POISSON_DISK[i].y);
        vec2 texCoords = LIGHT_SPACE_POS_POST_W.xy + offset * TEXEL_SIZE * KERNEL_SIZE;

        vec3 vPLWorldPos = texture(gPositionMap, texCoords).xyz;
        vec3 vPLWorldNormal = texture(gNormalMap, texCoords).xyz;
        vec3 flux = texture(gFluxMap, texCoords).xyz;

        vec3 lightContrib = flux * max(0.0f, dot(vPLWorldNormal, fs_in.WorldPos - vPLWorldPos)) * max(0.0f, dot(N, vPLWorldPos - fs_in.WorldPos)) / pow(length(fs_in.WorldPos - vPLWorldPos), 4);
        lightContrib *= offset.x * offset.x;
        indirectLighting += lightContrib;
    }

    return clamp(indirectLighting, 0.0f, 1.0f);
}

float calcSearchWidth(float receiverDepth)
{
    return gCalibratedLightSize * (receiverDepth - NEAR) / gViewPos.z;
}

float calcBlockerDistance(float bias)
{
    float sumBlockerDistances = 0.0f;
    int numBlockerDistances = 0;
    float receiverDepth = LIGHT_SPACE_POS_POST_W.z;

    int sw = int(calcSearchWidth(receiverDepth));
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        vec2 offset = vec2(
            ROTATION.x * POISSON_DISK[i].x - ROTATION.y * POISSON_DISK[i].y,
            ROTATION.y * POISSON_DISK[i].x + ROTATION.x * POISSON_DISK[i].y);

        float depth = texture(gShadowMap, LIGHT_SPACE_POS_POST_W.xy + offset * TEXEL_SIZE * sw).r;
        if (depth < receiverDepth - bias)
        {
            ++numBlockerDistances;
            sumBlockerDistances += depth;
        }
    }

    if (numBlockerDistances > 0)
    {
        return sumBlockerDistances / numBlockerDistances;
    }
    else
    {
        return -1;
    }
}

float calcPCFKernelSize(float bias)
{
    float receiverDepth = LIGHT_SPACE_POS_POST_W.z;
    float blockerDistance = calcBlockerDistance(bias);
    if (blockerDistance == -1)
    {
        return 1;
    }

    float penumbraWidth = (receiverDepth - blockerDistance) / blockerDistance;
    return penumbraWidth * gCalibratedLightSize * NEAR / receiverDepth;
}

float calcShadow()
{
    if (LIGHT_SPACE_POS_POST_W.z > 1.0f)
    {
        return 0.0f;
    }

    float shadow = 0.0f;

    float bias = max(0.05f * (1.0f - NoL), 0.005f);
    float pcfKernelSize = calcPCFKernelSize(bias);
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        vec2 offset = vec2(
            ROTATION.x * POISSON_DISK[i].x - ROTATION.y * POISSON_DISK[i].y,
            ROTATION.y * POISSON_DISK[i].x + ROTATION.x * POISSON_DISK[i].y);

        float pcfDepth = texture(gShadowMap, LIGHT_SPACE_POS_POST_W.xy + offset * TEXEL_SIZE * pcfKernelSize).r;
        shadow += LIGHT_SPACE_POS_POST_W.z - bias > pcfDepth ? 1.0f : 0.0f;
    }

    float l = clamp(smoothstep(0.0f, 0.2f, NoL), 0.0f, 1.0f);
    float t = smoothstep(RANDOM_VALUES.x * 0.5f, 1.0f, l);

    shadow /= (NUM_SAMPLES * t);

    return shadow;
}

void main()
{           
    // Blinn-Phong shading
    vec3 color = texture(gDiffuseTexture, fs_in.TexCoords).rgb;

    vec3 ambient = gAmbientFactor * color;

    vec3 diffuse = clamp(NoL, 0.0f, 1.0f) * gLightColor;

    vec3 V = normalize(gViewPos - fs_in.WorldPos);
    vec3 H = normalize(L + V);
    vec3 specular = pow(clamp(dot(N, H), 0.0f, 1.0f), gSpecularFactor) * gLightColor;

    vec3 indirectLighting = calcIndirectLighting();
    if (gRenderMode == 5)
    {
        FragColor = vec4(indirectLighting, 1.0f);
        return;
    }

    float shadow = calcShadow();
    vec3 directLighting = (ambient + (1.0f - shadow) * (diffuse + specular)) * color;
    if (gRenderMode == 6)
    {
        FragColor = vec4(directLighting, 1.0f);
        return;
    }

    FragColor = vec4(directLighting + indirectLighting, 1.0f);
}
