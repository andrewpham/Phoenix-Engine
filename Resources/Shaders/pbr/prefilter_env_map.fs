#version 460 core
out vec4 FragColor;

const float PI = 3.14159265359f;
const uint NUM_SAMPLES = 1024;

in vec3 WorldPos;

uniform samplerCube gEnvMap;
uniform float gRoughness;
uniform float gSpecConvTexWidth;

float GGXDistribution(vec3 N, vec3 H, float alpha)
{
    float NoH = clamp(dot(N, H), 0.0f, 1.0f);
    float alpha2 = alpha * alpha;
    float NoH2 = NoH * NoH;
    float den = NoH2 * (alpha2 - 1.0f) + 1.0f;
    return alpha2 / (PI * den * den);
}

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// Efficient van der Corput sequence generation
float radicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), radicalInverse_VdC(i));
}

vec3 importanceSampleGGX(vec2 Xi, float roughness, vec3 N)
{
    float a = roughness * roughness;

    float phi = 2.0f * PI * Xi.x;
    float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // Spherical polar angles to a point on the unit sphere, which will represent our half vector
    vec3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;

    vec3 upVector = abs(N.z) < 0.999f ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f);
    vec3 tangentX = normalize(cross(upVector, N));
    vec3 tangentY = cross(N, tangentX);
    // Tangent to world space
    return tangentX * H.x + tangentY * H.y + N * H.z;
}

vec3 prefilterEnvMap(vec3 R)
{
    vec3 N = R;
    vec3 V = R;

    vec3 prefilteredColor = vec3(0.0f);
    float totalWeight = 0.0f;
    for (uint i = 0; i < NUM_SAMPLES; ++i)
    {
        vec2 Xi = hammersley(i, NUM_SAMPLES);
        vec3 H = importanceSampleGGX(Xi, gRoughness, N);
        vec3 L = 2 * dot(V, H) * H - V;

        float NoL = clamp(dot(N, L), 0.0f, 1.0f);
        float NoH = clamp(dot(N, H), 0.0f, 1.0f);
        float HoV = clamp(dot(H, V), 0.0f, 1.0f);
        if (NoL > 0.0f)
        {
            float D = GGXDistribution(N, H, gRoughness * gRoughness);
            float pdf = D * NoH / (4 * HoV) + 0.0001f;

            float saTexel = 4.0f * PI / (6.0f * gSpecConvTexWidth * gSpecConvTexWidth);
            float saSample = 1.0f / (NUM_SAMPLES * pdf + 0.00001f);

            float mipLevel = gRoughness == 0.0f ? 0.0f : 0.5f * log2(saSample / saTexel);

            prefilteredColor += textureLod(gEnvMap, L, mipLevel).rgb * NoL;
            totalWeight += NoL;
        }
    }

    return prefilteredColor / totalWeight;
}

void main()
{
    FragColor = vec4(prefilterEnvMap(normalize(WorldPos)), 1.0f);
}
