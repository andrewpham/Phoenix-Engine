#version 460 core
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
};

in vec3 WorldPos;
in vec3 WorldNormal;

uniform Material gMaterial;
uniform PointLight gPointLight;
uniform vec3 gViewPos;
layout (RGBA8) uniform image3D gTexture3D;

float calcAttenuation(float distance)
{
    return gPointLight._attenuation._constant + gPointLight._attenuation._linear * distance + gPointLight._attenuation._quadratic * distance * distance;
}

vec3 calcPointLightContrib()
{
    vec3 L = normalize(gPointLight._position - WorldPos);
    return clamp(dot(normalize(WorldNormal), L), 0.0f, 1.0f) * gPointLight._light._intensity * gPointLight._light._color / calcAttenuation(distance(gPointLight._position, WorldPos));
};

bool isInsideUnitCube()
{
    return abs(WorldPos.x) < 1.0f && abs(WorldPos.y) < 1.0f && abs(WorldPos.z) < 1.0f;
}

void main()
{
    if(!isInsideUnitCube())
    {
        return;
    }

    vec3 diffuse = gMaterial._diffuseReflectivity * gMaterial._diffuseColor;
    vec3 specular = gMaterial._specularReflectivity * gMaterial._specularColor;
    vec3 color = (diffuse + specular) * calcPointLightContrib() + clamp(gMaterial._emissivity, 0.0f, 1.0f) * gMaterial._diffuseColor;

    vec3 position = WorldPos * 0.5f + 0.5f;
    imageStore(gTexture3D, ivec3(imageSize(gTexture3D) * position), vec4(vec3(color), 1.0f));
}
