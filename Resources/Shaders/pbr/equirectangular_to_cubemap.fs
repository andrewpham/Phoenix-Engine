#version 460 core
out vec4 FragColor;

const float PI = 3.14159265359f;
// (phi, theta) -> [-0.5f, 1.0f]
const vec2 POLAR_ANGLES_TO_RANGE = vec2(1.0f / (2.0f * PI), 1.0f / PI);
const float SEAM_WIDTH = 0.01f;

in vec3 WorldPos;

uniform sampler2D gEnvMap;

vec2 sampleEquirectangularTexture(vec3 r)
{
    // Creates a bijection from the normalized world position
    // to spherical polar angles to texture coordinates
    return vec2(atan(r.x, -r.z), acos(-r.y)) * POLAR_ANGLES_TO_RANGE + vec2(0.5f, 0.0f);
}

void main()
{
    vec3 r = normalize(WorldPos);
    // With corrections for sampling bias and seam artifacts
    float mask = max(0.0f, 1.0f - abs(r.x) / SEAM_WIDTH) * clamp(1.0f + r.z / SEAM_WIDTH, 0.0f, 1.0f);
    vec3 fragColor = texture(gEnvMap, sampleEquirectangularTexture(r), -2.0f * log2(1.0f + r.y * r.y) - 12.3f * mask).rgb;

    FragColor = vec4(fragColor, 1.0f);
}
