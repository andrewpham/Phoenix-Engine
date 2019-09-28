#version 460 core
out vec4 FragColor;

const float SPECULAR_FACTOR = 16.0f;

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
uniform mat4 gInverseViewMatrix;
uniform mat4 gLightSpaceVP;
uniform vec3 gViewPos;
uniform float gAmbientFactor;

const vec3 P = vec3(gInverseViewMatrix * vec4(texture(gPositionMap, TexCoords).rgb, 1.0f));
const vec3 N = vec3(gInverseViewMatrix * vec4(texture(gNormalMap, TexCoords).rgb, 1.0f)); 
const vec4 ALBEDO_SPECULAR = texture(gAlbedoSpecularMap, TexCoords);
const vec4 LIGHT_SPACE_POS = gLightSpaceVP * vec4(P, 1.0f);
const vec3 LIGHT_SPACE_POS_POST_W = LIGHT_SPACE_POS.xyz / LIGHT_SPACE_POS.w * 0.5f + 0.5f;
const vec3 L = normalize(-gDirectLight._direction);
const float NoL = dot(N, L);

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
    vec3 V = normalize(gViewPos - P);
    vec3 diffuse = clamp(NoL, 0.0f, 1.0f) * gDirectLight._light._color * ALBEDO_SPECULAR.rgb;
    vec3 H = normalize(L + V);
    vec3 specular = pow(clamp(dot(N, H), 0.0f, 1.0f), SPECULAR_FACTOR) * gDirectLight._light._color * ALBEDO_SPECULAR.a;
    if (calcShadow() < 1.0f)
    {
        FragColor = vec4((1.0f - calcShadow()) * (diffuse + specular), 1.0f);
    }
    else
    {
        FragColor = vec4(gAmbientFactor * ALBEDO_SPECULAR.rgb, 1.0f);
    }
}
