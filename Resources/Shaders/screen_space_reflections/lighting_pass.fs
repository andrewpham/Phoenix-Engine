#version 460 core
layout (location = 3) out vec4 FragColor;

const float SPECULAR_FACTOR = 16.0f;
const int NUM_LIGHTS = 32;

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

in vec2 TexCoords;

uniform sampler2D gPositionMap;
uniform sampler2D gNormalMap;
uniform sampler2D gAlbedoSpecularMap;

uniform PointLight gPointLights[NUM_LIGHTS];
uniform vec3 gViewPos;
uniform mat4 gInverseViewMatrix;

float calcAttenuation(float distance, int index)
{
    return gPointLights[index]._attenuation._constant + gPointLights[index]._attenuation._linear * distance + gPointLights[index]._attenuation._quadratic * distance * distance;
}

void main()
{
    vec3 WorldPos = vec3(gInverseViewMatrix * vec4(texture(gPositionMap, TexCoords).rgb, 1.0f));
    vec3 N = vec3(gInverseViewMatrix * vec4(texture(gNormalMap, TexCoords).rgb, 1.0f)); 
    vec3 Albedo = texture(gAlbedoSpecularMap, TexCoords).rgb;
    float Specular = texture(gAlbedoSpecularMap, TexCoords).a;

    vec3 fragColor = 0.1f * Albedo;
    vec3 V = normalize(gViewPos - WorldPos);
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        vec3 L = normalize(gPointLights[i]._position - WorldPos);
        vec3 diffuse = clamp(dot(N, L), 0.0f, 1.0f) * gPointLights[i]._light._color * Albedo;

        vec3 H = normalize(L + V);  
        vec3 specular = pow(clamp(dot(N, H), 0.0f, 1.0f), SPECULAR_FACTOR) * gPointLights[i]._light._color * Specular;

        float distance = length(gPointLights[i]._position - WorldPos);
        fragColor += (diffuse + specular) / calcAttenuation(distance, i);
    }

    FragColor = vec4(fragColor, 1.0f);
}
