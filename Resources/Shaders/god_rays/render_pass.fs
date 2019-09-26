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

uniform DirectLight gDirectLight;
uniform vec3 gViewPos;
uniform mat4 gInverseViewMatrix;

void main()
{
    vec3 N = vec3(gInverseViewMatrix * vec4(texture(gNormalMap, TexCoords).rgb, 1.0f)); 
    vec4 AlbedoSpecular = texture(gAlbedoSpecularMap, TexCoords);

    vec3 V = normalize(gViewPos - vec3(gInverseViewMatrix * vec4(texture(gPositionMap, TexCoords).rgb, 1.0f)));
    vec3 L = normalize(-gDirectLight._direction);
    vec3 diffuse = clamp(dot(N, L), 0.0f, 1.0f) * gDirectLight._light._color * AlbedoSpecular.rgb;
    vec3 H = normalize(L + V);
    vec3 specular = pow(clamp(dot(N, H), 0.0f, 1.0f), SPECULAR_FACTOR) * gDirectLight._light._color * AlbedoSpecular.a;

    FragColor = vec4(diffuse + specular, 1.0f);
}
