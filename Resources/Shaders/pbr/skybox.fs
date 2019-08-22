#version 460 core
out vec4 FragColor;

const float PI = 3.14159265359f;
// Cosine lobe convolution factors
const float A0 = PI;
const float A1 = 2.094395f;
const float A2 = 0.785398f;

in vec3 WorldPos;

uniform samplerCube gEnvMap;
uniform int gRenderMode;
uniform vec3 gSH9Color[9];

vec3 calcSHIrradiance(vec3 N)
{
    vec3 irradiance = gSH9Color[0] * 0.282095f * A0
        + gSH9Color[1] * 0.488603f * N.y * A1
        + gSH9Color[2] * 0.488603f * N.z * A1
        + gSH9Color[3] * 0.488603f * N.x * A1
        + gSH9Color[4] * 1.092548f * N.x * N.y * A2
        + gSH9Color[5] * 1.092548f * N.y * N.z * A2
        + gSH9Color[6] * 0.315392f * (3.0f * N.z * N.z - 1.0f) * A2
        + gSH9Color[7] * 1.092548f * N.x * N.z * A2
        + gSH9Color[8] * 0.546274f * (N.x * N.x - N.y * N.y) * A2;
    return irradiance;
}

void main()
{		
    vec3 fragColor;

    if (gRenderMode == 3)
    {
        fragColor = calcSHIrradiance(WorldPos) / PI;
    }
    else
    {
        fragColor = textureLod(gEnvMap, WorldPos, 0.0f).rgb;
    }

    // Tone mapping and gamma correction
    fragColor = fragColor / (fragColor + vec3(1.0f));
    fragColor = pow(fragColor, vec3(1.0f / 2.2f));

    FragColor = vec4(fragColor, 1.0f);
}
