#version 460 core
out vec4 FragColor;

const float PI = 3.14159265359f;
// Cosine lobe convolution factors
const float A0 = 3.141593f;
const float A1 = 2.094395f;
const float A2 = 0.785398f;

in vec3 WorldPos;

uniform samplerCube gSampler0;
uniform samplerCube gSampler1;
uniform samplerCube gSampler2;
uniform samplerCube gSampler3;
uniform samplerCube gSampler4;
uniform samplerCube gSampler5;
uniform samplerCube gSampler6;
uniform samplerCube gSampler7;
uniform samplerCube gSampler8;

uniform int gRenderMode;

vec3 calcSHIrradiance(vec3 N)
{
    vec3 irradiance = vec3(0.0f);
    irradiance += (textureLod(gSampler0, vec3(1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler0, vec3(-1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler0, vec3(0.0f, 1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler0, vec3(0.0f, -1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler0, vec3(0.0f, 0.0f, 1.0f), 9.0f).rgb +
                   textureLod(gSampler0, vec3(0.0f, 0.0f, -1.0f), 9.0f).rgb) / 6.0f * 0.282095f * A0;
    irradiance += (textureLod(gSampler1, vec3(1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler1, vec3(-1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler1, vec3(0.0f, 1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler1, vec3(0.0f, -1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler1, vec3(0.0f, 0.0f, 1.0f), 9.0f).rgb +
                   textureLod(gSampler1, vec3(0.0f, 0.0f, -1.0f), 9.0f).rgb) / 6.0f * 0.488603f * N.y * A1;
    irradiance += (textureLod(gSampler2, vec3(1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler2, vec3(-1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler2, vec3(0.0f, 1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler2, vec3(0.0f, -1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler2, vec3(0.0f, 0.0f, 1.0f), 9.0f).rgb +
                   textureLod(gSampler2, vec3(0.0f, 0.0f, -1.0f), 9.0f).rgb) / 6.0f * 0.488603f * N.z * A1;
    irradiance += (textureLod(gSampler3, vec3(1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler3, vec3(-1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler3, vec3(0.0f, 1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler3, vec3(0.0f, -1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler3, vec3(0.0f, 0.0f, 1.0f), 9.0f).rgb +
                   textureLod(gSampler3, vec3(0.0f, 0.0f, -1.0f), 9.0f).rgb) / 6.0f * 0.488603f * N.x * A1;
    irradiance += (textureLod(gSampler4, vec3(1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler4, vec3(-1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler4, vec3(0.0f, 1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler4, vec3(0.0f, -1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler4, vec3(0.0f, 0.0f, 1.0f), 9.0f).rgb +
                   textureLod(gSampler4, vec3(0.0f, 0.0f, -1.0f), 9.0f).rgb) / 6.0f * 1.092548f * N.x * N.y * A2;
    irradiance += (textureLod(gSampler5, vec3(1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler5, vec3(-1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler5, vec3(0.0f, 1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler5, vec3(0.0f, -1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler5, vec3(0.0f, 0.0f, 1.0f), 9.0f).rgb +
                   textureLod(gSampler5, vec3(0.0f, 0.0f, -1.0f), 9.0f).rgb) / 6.0f * 1.092548f * N.y * N.z * A2;
    irradiance += (textureLod(gSampler6, vec3(1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler6, vec3(-1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler6, vec3(0.0f, 1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler6, vec3(0.0f, -1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler6, vec3(0.0f, 0.0f, 1.0f), 9.0f).rgb +
                   textureLod(gSampler6, vec3(0.0f, 0.0f, -1.0f), 9.0f).rgb) / 6.0f * 0.315392f * (3.0f * N.z * N.z - 1.0f) * A2;
    irradiance += (textureLod(gSampler7, vec3(1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler7, vec3(-1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler7, vec3(0.0f, 1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler7, vec3(0.0f, -1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler7, vec3(0.0f, 0.0f, 1.0f), 9.0f).rgb +
                   textureLod(gSampler7, vec3(0.0f, 0.0f, -1.0f), 9.0f).rgb) / 6.0f * 1.092548f * N.x * N.z * A2;
    irradiance += (textureLod(gSampler8, vec3(1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler8, vec3(-1.0f, 0.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler8, vec3(0.0f, 1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler8, vec3(0.0f, -1.0f, 0.0f), 9.0f).rgb +
                   textureLod(gSampler8, vec3(0.0f, 0.0f, 1.0f), 9.0f).rgb +
                   textureLod(gSampler8, vec3(0.0f, 0.0f, -1.0f), 9.0f).rgb) / 6.0f * 0.546274f * (N.x * N.x - N.y * N.y) * A2;
    return PI * irradiance;
}

void main()
{		
    vec3 fragColor;

    if (gRenderMode == 3)
    {
        fragColor = calcSHIrradiance(WorldPos);
    }
    else
    {
        fragColor = textureLod(gSampler0, WorldPos, 0.0f).rgb;
    }

    // Tone mapping and gamma correction
    fragColor = fragColor / (fragColor + vec3(1.0f));
    fragColor = pow(fragColor, vec3(1.0f / 2.2f));

    FragColor = vec4(fragColor, 1.0f);
}
