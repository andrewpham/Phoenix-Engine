#version 460 core
out vec2 FragColor;

const float PI = 3.14159265359f;
const uint NUM_SAMPLES = 1024;

in vec2 TexCoords;

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

float G_Schlick(float roughness, float NoV)
{
    float k = roughness * roughness / 2.0f;
    return NoV / (NoV * (1.0f - k) + k);
}

float G_Smith(float roughness, float NoV, float NoL)
{
    return G_Schlick(roughness, NoV) * G_Schlick(roughness, NoL);
}

vec2 integrateBRDF(float roughness, float NoV)
{
    vec3 V;
    V.x = sqrt(1.0f - NoV * NoV); // sin
    V.y = 0.0f;
    V.z = NoV;                    // cos

    float A = 0.0f;
    float B = 0.0f;

    vec3 N = vec3(0.0f, 0.0f, 1.0f);
    for (uint i = 0; i < NUM_SAMPLES; ++i)
    {
        vec2 Xi = hammersley(i, NUM_SAMPLES);
        vec3 H = importanceSampleGGX(Xi, roughness, N);
        vec3 L = 2 * dot(V, H) * H - V;

        float NoL = clamp(L.z, 0.0f, 1.0f);
        float NoH = clamp(H.z, 0.0f, 1.0f);
        float VoH = clamp(dot(V, H), 0.0f, 1.0f);

        if (NoL > 0.0f)
        {
            float G = G_Smith(roughness, NoV, NoL);
            float G_Vis = G * VoH / (NoH * NoV);
            float Fc = pow(1 - VoH, 5);

            A += (1 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    return vec2(A, B) / float(NUM_SAMPLES);
}

void main() 
{
    FragColor = integrateBRDF(TexCoords.y, TexCoords.x);
}
