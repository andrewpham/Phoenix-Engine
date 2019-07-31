#version 460 core
out vec4 FragColor;

const float DIRECT_DIFFUSE_CORRECTION_FACTOR = 1.0f / 3.0f;
const float PI = 3.14159f;
const float VOXEL_OFFSET_CORRECTION_FACTOR = 1.732f; // sqrt(3.0f)
const float VOXEL_SIZE = 1.0f / 64.0f;
const float LIGHT_RADIUS = 3.0f;
const int NUM_STEPS = 200;

struct Light
{
    vec3 _color;
    float _intensity;
};

struct Attenuation
{
    float _constant;
    float _linear;
    float _quadratic;
};

struct PointLight
{
    struct Light _light;
    vec3 _position;
    Attenuation _attenuation;
};

struct Material
{
    vec3 _diffuseColor;
    vec3 _specularColor;
    float _specularReflectivity;
    float _diffuseReflectivity;
    float _emissivity;
    float _aperture;
};

in vec3 WorldPos;
in vec3 WorldNormal;

uniform Material gMaterial;
uniform PointLight gPointLight;
uniform vec3 gViewPos;
uniform sampler3D gTexture3D;

const vec3 N = normalize(WorldNormal);
const vec3 V = normalize(WorldPos - gViewPos);
const vec3 R = reflect(V, N);

bool isInsideUnitCube(vec3 p)
{
    return abs(p.x) < 1.0f && abs(p.y) < 1.0f && abs(p.z) < 1.0f;
}

vec3 coneTrace(vec3 direction, float aperture)
{
    vec3 start = WorldPos + VOXEL_OFFSET_CORRECTION_FACTOR * VOXEL_SIZE * N;

    vec4 Lv = vec4(0.0f);

    float tanHalfAperture = tan(aperture / 2.0f);
    float tanEighthAperture = tan(aperture / 8.0f);
    float stepSizeCorrectionFactor = (1.0f + tanEighthAperture) / (1.0f - tanEighthAperture);
    float step = stepSizeCorrectionFactor * VOXEL_SIZE / 2.0f;

    float distance = step;

    for (int i = 0; i < NUM_STEPS && Lv.a <= 0.9f; ++i)
    {
        vec3 position = start + distance * direction;
        if (!isInsideUnitCube(position))
        {
            break;
        }
        position = position * 0.5f + 0.5f;

        float diameter = 2.0f * tanHalfAperture * distance;
        float mipLevel = log2(diameter / VOXEL_SIZE);
        vec4 LvStep = 100.0f * step * textureLod(gTexture3D, position, mipLevel);
        if (LvStep.a > 0.0f)
        {
            LvStep.rgb /= LvStep.a;
            // Alpha blending
            Lv.rgb += (1.0f - Lv.a) * LvStep.a * LvStep.rgb;
            Lv.a += (1.0f - Lv.a) * LvStep.a;
        }
        distance += step;
    }
    return Lv.rgb;
}

vec3 calcDirectDiffuseLighting()
{
    vec3 L = gPointLight._position - WorldPos;
    float distance = length(L);
    L = normalize(L);
    float aperture = 2.0f * atan(LIGHT_RADIUS / distance);
    return DIRECT_DIFFUSE_CORRECTION_FACTOR * max(dot(N, L), 0.0f) * coneTrace(L, aperture) / (distance * distance);
}

vec3 calcDirectSpecularLighting()
{
    vec3 L = gPointLight._position - WorldPos;
    float distance = length(L);
    L = normalize(L);
    return gPointLight._light._intensity * gPointLight._light._color * pow(max(dot(R, L), 0.0f), 20.0f / (gMaterial._aperture + 0.1f));
}

vec3 calcIndirectDiffuseLighting()
{
    vec3 T = cross(N, vec3(0.0f, 1.0f, 0.0f));
    vec3 B = cross(T, N);

    vec3 Lo = vec3(0.0f);

    float aperture = PI / 3.0f;
    vec3 direction = N;
    Lo += coneTrace(direction, aperture);
    direction = 0.7071f * N + 0.7071f * T;
    Lo += coneTrace(direction, aperture);
    // Rotate the tangent vector about the normal using the 5th roots of unity to obtain the subsequent diffuse cone directions
    direction = 0.7071f * N + 0.7071f * (0.309f * T + 0.951f * B);
    Lo += coneTrace(direction, aperture);
    direction = 0.7071f * N + 0.7071f * (-0.809f * T + 0.588f * B);
    Lo += coneTrace(direction, aperture);
    direction = 0.7071f * N - 0.7071f * (-0.809f * T - 0.588f * B);
    Lo += coneTrace(direction, aperture);
    direction = 0.7071f * N - 0.7071f * (0.309f * T - 0.951f * B);
    Lo += coneTrace(direction, aperture);

    return Lo / 6.0f;
}

vec3 calcIndirectSpecularLighting()
{
    return coneTrace(R, gMaterial._aperture);
}

void main()
{
    FragColor = vec4(0.0f, 0.0f, 0.0f, 1.0f);
    FragColor.rgb += gMaterial._emissivity * gMaterial._diffuseColor;
    // We choose here to calculate the direct diffuse illumination from cone tracing
    FragColor.rgb += calcDirectDiffuseLighting() * gMaterial._diffuseColor * gMaterial._diffuseReflectivity * (1 - gMaterial._specularReflectivity);
    // Direct illumination associated with the point light
    FragColor.rgb += calcDirectSpecularLighting() * gMaterial._specularColor * gMaterial._diffuseReflectivity * gMaterial._specularReflectivity;
    // Indirect illumination associated with the point light is calculated with the remaining functions
    FragColor.rgb += calcIndirectDiffuseLighting() * gMaterial._diffuseColor * gMaterial._diffuseReflectivity * (1 - gMaterial._specularReflectivity);
    FragColor.rgb += calcIndirectSpecularLighting() * gMaterial._specularColor * gMaterial._diffuseReflectivity * gMaterial._specularReflectivity;
    FragColor.rgb = pow(FragColor.rgb, vec3(1.0f / 2.2f));
}
