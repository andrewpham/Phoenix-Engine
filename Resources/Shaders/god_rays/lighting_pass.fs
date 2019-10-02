#version 460 core
layout (location = 3) out vec4 FragColor;

const int SCREEN_WIDTH = 2460;
const int SCREEN_HEIGHT = 1440;
const int NUM_STEPS_INT = 15;
const float NUM_STEPS = float(NUM_STEPS_INT);
const float G = 0.7f; // Controls how much light will scatter in the forward direction
const float PI = 3.14159265359f;
const float SPECULAR_FACTOR = 16.0f;
const mat4 DITHER_PATTERN = mat4
    (vec4(0.0f, 0.5f, 0.125f, 0.625f),
     vec4(0.75f, 0.22f, 0.875f, 0.375f),
     vec4(0.1875f, 0.6875f, 0.0625f, 0.5625f),
     vec4(0.9375f, 0.4375f, 0.8125f, 0.3125f));

struct Light
{
    vec3 _color;
    float _intensity;
};

struct DirectLight
{
    struct Light _light;
    vec3 _direction;
};

in vec2 TexCoords;

uniform sampler2D gPositionMap;
uniform sampler2D gNormalMap;
uniform sampler2D gAlbedoSpecularMap;
uniform sampler2D gShadowMap;

uniform DirectLight gDirectLight;
uniform mat4 gLightSpaceVP;
uniform vec3 gViewPos;
uniform float gAmbientFactor;

const vec3 P = texture(gPositionMap, TexCoords).rgb;
const vec3 N = texture(gNormalMap, TexCoords).rgb;
const vec4 ALBEDO_SPECULAR = texture(gAlbedoSpecularMap, TexCoords);
const vec4 LIGHT_SPACE_POS = gLightSpaceVP * vec4(P, 1.0f);
const vec3 LIGHT_SPACE_POS_POST_W = LIGHT_SPACE_POS.xyz / LIGHT_SPACE_POS.w * 0.5f + 0.5f;
const vec3 L = normalize(-gDirectLight._direction);
const float NoL = dot(N, L);

// Mie-scattering phase function approximated by the Henyey-Greenstein phase function
float calcScattering(float cosTheta)
{
    return (1.0f - G * G) / (4.0f * PI * pow(1.0f + G * G - 2.0f * G * cosTheta, 1.5f));
}

float calcShadow()
{
    if (LIGHT_SPACE_POS_POST_W.z > 1.0f)
    {
        return 0.0f;
    }

    float bias = max(0.05f * (1.0f - NoL), 0.005f);
    float depth = texture(gShadowMap, LIGHT_SPACE_POS_POST_W.xy).r;
    return LIGHT_SPACE_POS_POST_W.z - bias > depth ? 1.0f : 0.0f;
}

void main()
{
    vec3 V = P - gViewPos;

    float stepSize = length(V) / NUM_STEPS;
    V = normalize(V);
    vec3 step = V * stepSize;

    vec3 position = gViewPos;
    position += step * DITHER_PATTERN[int(TexCoords.x * SCREEN_WIDTH) % 4][int(TexCoords.y * SCREEN_HEIGHT) % 4];

    vec3 volumetric = vec3(0.0f);
    for (int i = 0; i < NUM_STEPS_INT; ++i)
    {
        vec4 lightSpacePos = gLightSpaceVP * vec4(position, 1.0f);
        vec3 lightSpacePosPostW = lightSpacePos.xyz / lightSpacePos.w * 0.5f + 0.5f;
        float depth = texture(gShadowMap, lightSpacePosPostW.xy).r;
        if (depth > lightSpacePosPostW.z)
        {
            volumetric += calcScattering(dot(V, -L)) * gDirectLight._light._color;
        }
        position += step;
    }
    volumetric /= NUM_STEPS;

    V = normalize(gViewPos - P);
    vec3 diffuse = clamp(NoL, 0.0f, 1.0f) * gDirectLight._light._color * ALBEDO_SPECULAR.rgb;
    vec3 H = normalize(L + V);
    vec3 specular = pow(clamp(dot(N, H), 0.0f, 1.0f), SPECULAR_FACTOR) * gDirectLight._light._color * ALBEDO_SPECULAR.a;

    float shadow = calcShadow();
    if (shadow < 1.0f)
    {
        FragColor = vec4((1.0f - shadow) * (diffuse + specular) + volumetric, 1.0f);
    }
    else
    {
        FragColor = vec4(gAmbientFactor * ALBEDO_SPECULAR.rgb + volumetric, 1.0f);
    }
}
