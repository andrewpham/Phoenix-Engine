#version 460 core

const float NEAR = 0.1f;
const int NUM_SAMPLES = 64;
const int CORRECTION_FACTOR = 100;

out vec4 FragColor;

in VS_OUT {
    vec3 WorldPos;
    vec3 WorldNormal;
    vec2 TexCoords;
    vec4 LightSpacePos;
} fs_in;

uniform sampler2D gDiffuseTexture;
uniform sampler2D gShadowMap;
uniform sampler3D gCosinesTexture;
uniform sampler3D gSinesTexture;

uniform vec3 gLightPos;
uniform vec3 gViewPos;
uniform vec3 gLightColor;
uniform float gAmbientFactor;
uniform float gSpecularFactor;
uniform float gCalibratedLightSize;

float searchWidth(float receiverDepth)
{
    return gCalibratedLightSize * (receiverDepth - NEAR) / gViewPos.z;
}

float calcBlockerDistance(vec2 poissonDisk[NUM_SAMPLES], vec3 lightSpacePos, float bias)
{
    float sumBlockerDistances = 0.0f;
    int numBlockerDistances = 0;
    float receiverDepth = lightSpacePos.z;
    vec2 texelSize = 1.0f / textureSize(gShadowMap, 0);

    vec2 randomValues = vec2(texture(gCosinesTexture, fs_in.WorldPos * CORRECTION_FACTOR).r,
                             texture(gSinesTexture, fs_in.WorldPos * CORRECTION_FACTOR).r);
    vec2 rotation = randomValues * 2 - 1;

    int sw = int(searchWidth(receiverDepth));
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        vec2 poissonOffset = vec2(
            rotation.x * poissonDisk[i].x - rotation.y * poissonDisk[i].y,
            rotation.y * poissonDisk[i].x + rotation.x * poissonDisk[i].y);

        float depth = texture(gShadowMap, lightSpacePos.xy + poissonOffset * texelSize * sw).r;
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

float calcPCFKernel(vec2 poissonDisk[NUM_SAMPLES], vec3 lightSpacePos, float bias)
{
    float receiverDepth = lightSpacePos.z;
    float blockerDistance = calcBlockerDistance(poissonDisk, lightSpacePos, bias);
    if (blockerDistance == -1)
    {
        return 1;
    }

    float penumbraWidth = (receiverDepth - blockerDistance) / blockerDistance;
    return penumbraWidth * gCalibratedLightSize * NEAR / receiverDepth;
}

float calcShadow(vec4 lightSpacePos)
{
    vec3 lightSpacePosPostW = lightSpacePos.xyz / lightSpacePos.w;
    lightSpacePosPostW = lightSpacePosPostW * 0.5f + 0.5f;

    // Correction for artifacts from extending beyond the light frustum's far plane.
    if (lightSpacePosPostW.z > 1.0f)
        return 0.0f;

    float depth = lightSpacePosPostW.z;

    vec3 N = normalize(fs_in.WorldNormal);
    vec3 L = normalize(gLightPos - fs_in.WorldPos);
    float bias = max(0.05f * (1.0f - dot(N, L)), 0.005f);

    // PCF Using Rotated Poisson Disk Sampling
    vec2 poissonDisk[NUM_SAMPLES];
    poissonDisk[0] = vec2(-0.613392f, 0.617481f);
    poissonDisk[1] = vec2(0.170019f, -0.040254f);
    poissonDisk[2] = vec2(-0.299417f, 0.791925f);
    poissonDisk[3] = vec2(0.645680f, 0.493210f);
    poissonDisk[4] = vec2(-0.651784f, 0.717887f);
    poissonDisk[5] = vec2(0.421003f, 0.027070f);
    poissonDisk[6] = vec2(-0.817194f, -0.271096f);
    poissonDisk[7] = vec2(-0.705374f, -0.668203f);
    poissonDisk[8] = vec2(0.977050f, -0.108615f);
    poissonDisk[9] = vec2(0.063326f, 0.142369f);
    poissonDisk[10] = vec2(0.203528f, 0.214331f);
    poissonDisk[11] = vec2(-0.667531f, 0.326090f);
    poissonDisk[12] = vec2(-0.098422f, -0.295755f);
    poissonDisk[13] = vec2(-0.885922f, 0.215369f);
    poissonDisk[14] = vec2(0.566637f, 0.605213f);
    poissonDisk[15] = vec2(0.039766f, -0.396100f);
    poissonDisk[16] = vec2(0.751946f, 0.453352f);
    poissonDisk[17] = vec2(0.078707f, -0.715323f);
    poissonDisk[18] = vec2(-0.075838f, -0.529344f);
    poissonDisk[19] = vec2(0.724479f, -0.580798f);
    poissonDisk[20] = vec2(0.222999f, -0.215125f);
    poissonDisk[21] = vec2(-0.467574f, -0.405438f);
    poissonDisk[22] = vec2(-0.248268f, -0.814753f);
    poissonDisk[23] = vec2(0.354411f, -0.887570f);
    poissonDisk[24] = vec2(0.175817f, 0.382366f);
    poissonDisk[25] = vec2(0.487472f, -0.063082f);
    poissonDisk[26] = vec2(-0.084078f, 0.898312f);
    poissonDisk[27] = vec2(0.488876f, -0.783441f);
    poissonDisk[28] = vec2(0.470016f, 0.217933f);
    poissonDisk[29] = vec2(-0.696890f, -0.549791f);
    poissonDisk[30] = vec2(-0.149693f, 0.605762f);
    poissonDisk[31] = vec2(0.034211f, 0.979980f);
    poissonDisk[32] = vec2(0.503098f, -0.308878f);
    poissonDisk[33] = vec2(-0.016205f, -0.872921f);
    poissonDisk[34] = vec2(0.385784f, -0.393902f);
    poissonDisk[35] = vec2(-0.146886f, -0.859249f);
    poissonDisk[36] = vec2(0.643361f, 0.164098f);
    poissonDisk[37] = vec2(0.634388f, -0.049471f);
    poissonDisk[38] = vec2(-0.688894f, 0.007843f);
    poissonDisk[39] = vec2(0.464034f, -0.188818f);
    poissonDisk[40] = vec2(-0.440840f, 0.137486f);
    poissonDisk[41] = vec2(0.364483f, 0.511704f);
    poissonDisk[42] = vec2(0.034028f, 0.325968f);
    poissonDisk[43] = vec2(0.099094f, -0.308023f);
    poissonDisk[44] = vec2(0.693960f, -0.366253f);
    poissonDisk[45] = vec2(0.678884f, -0.204688f);
    poissonDisk[46] = vec2(0.001801f, 0.780328f);
    poissonDisk[47] = vec2(0.145177f, -0.898984f);
    poissonDisk[48] = vec2(0.062655f, -0.611866f);
    poissonDisk[49] = vec2(0.315226f, -0.604297f);
    poissonDisk[50] = vec2(-0.780145f, 0.486251f);
    poissonDisk[51] = vec2(-0.371868f, 0.882138f);
    poissonDisk[52] = vec2(0.200476f, 0.494430f);
    poissonDisk[53] = vec2(-0.494552f, -0.711051f);
    poissonDisk[54] = vec2(0.612476f, 0.705252f);
    poissonDisk[55] = vec2(-0.578845f, -0.768792f);
    poissonDisk[56] = vec2(-0.772454f, -0.090976f);
    poissonDisk[57] = vec2(0.504440f, 0.372295f);
    poissonDisk[58] = vec2(0.155736f, 0.065157f);
    poissonDisk[59] = vec2(0.391522f, 0.849605f);
    poissonDisk[60] = vec2(-0.620106f, -0.328104f);
    poissonDisk[61] = vec2(0.789239f, -0.419965f);
    poissonDisk[62] = vec2(-0.545396f, 0.538133f);
    poissonDisk[63] = vec2(-0.178564f, -0.596057f);

    // Setting Variable for PCSS
    float pcfKernel = calcPCFKernel(poissonDisk, lightSpacePosPostW, bias);

    float shadow = 0.0f;
    vec2 texelSize = 1.0f / textureSize(gShadowMap, 0);

    vec2 randomValues = vec2(texture(gCosinesTexture, fs_in.WorldPos * CORRECTION_FACTOR).r,
                             texture(gSinesTexture, fs_in.WorldPos * CORRECTION_FACTOR).r);
    vec2 rotation = randomValues * 2 - 1;

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        vec2 poissonOffset = vec2(
            rotation.x * poissonDisk[i].x - rotation.y * poissonDisk[i].y,
            rotation.y * poissonDisk[i].x + rotation.x * poissonDisk[i].y);

        float pcfDepth = texture(gShadowMap, lightSpacePosPostW.xy + poissonOffset * texelSize * pcfKernel).r;
        shadow += depth - bias > pcfDepth ? 1.0f : 0.0f;
    }

    float NoL = dot(N, L);
    float l = clamp(smoothstep(0.0f, 0.2f, dot(N, L)), 0.0f, 1.0f);
    float t = smoothstep(randomValues.x * 0.5f, 1.0f, l);

    shadow /= (NUM_SAMPLES * t);

    return shadow;
}

void main()
{           
    // Blinn-Phong shading
    vec3 color = texture(gDiffuseTexture, fs_in.TexCoords).rgb;

    vec3 ambient = gAmbientFactor * color;

    vec3 N = normalize(fs_in.WorldNormal);
    vec3 L = normalize(gLightPos - fs_in.WorldPos);
    vec3 diffuse = max(dot(N, L), 0.0f) * gLightColor;

    vec3 V = normalize(gViewPos - fs_in.WorldPos);
    vec3 H = normalize(L + V);
    vec3 specular = pow(clamp(dot(N, H), 0.0f, 1.0f), gSpecularFactor) * gLightColor;

    float shadow = calcShadow(fs_in.LightSpacePos);
    vec3 fragColor = (ambient + (1.0f - shadow) * (diffuse + specular)) * color;

    FragColor = vec4(fragColor, 1.0f);
}
