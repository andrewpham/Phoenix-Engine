#version 460 core
out vec4 FragColor;

const float PI = 3.14159265359f;
const float MAX_REFLECTION_LOD = 4.0f;
const int NUM_LIGHTS = 4;

in VS_OUT {
    vec3 WorldPos;
    vec3 WorldNormal;
    vec2 TexCoords;
} fs_in;

// PBR material textures
uniform sampler2D gAlbedoMap;
uniform sampler2D gNormalMap;
uniform sampler2D gMetallicMap;
uniform sampler2D gRoughnessMap;
uniform sampler2D gAOMap;

// Precomputed IBL data
uniform samplerCube gIrradianceMap;
uniform samplerCube gPrefilteredEnvMap;
uniform sampler2D gBRDFIntegrationMap;

uniform vec3 gViewPos;

uniform vec3 gLightPositions[NUM_LIGHTS];
uniform vec3 gLightColors[NUM_LIGHTS];

vec3 calcWorldSpaceNormal(vec3 tangentSpaceNormal)
{
    vec3 N = normalize(fs_in.WorldNormal);
    vec3 T = normalize(dFdx(fs_in.WorldPos) * dFdy(fs_in.TexCoords).t - dFdy(fs_in.WorldPos) * dFdx(fs_in.TexCoords).t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentSpaceNormal);
}

float GGXDistribution(vec3 N, vec3 H, float alpha)
{
    float NoH = clamp(dot(N, H), 0.0f, 1.0f);
    float alpha2 = alpha * alpha;
    float NoH2 = NoH * NoH;
    float den = NoH2 * (alpha2 - 1.0f) + 1.0f;
    return alpha2 / (PI * den * den);
}

float G_Schlick(float roughness, float NoV)
{
    float k = (roughness + 1) * (roughness + 1) / 8.0f;
    return NoV / (NoV * (1.0f - k) + k);
}

float G_Smith(float roughness, float NoV, float NoL)
{
    return G_Schlick(roughness, NoV) * G_Schlick(roughness, NoL);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1 - F0) * pow(1 - cosTheta, 5);
}

vec3 evalReflectanceEquation(vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness)
{
    float NoV = clamp(dot(N, V), 0.0f, 1.0f);

    vec3 Lo = vec3(0.0f);
    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        vec3 L = normalize(gLightPositions[i] - fs_in.WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(gLightPositions[i] - fs_in.WorldPos);
        vec3 Li = gLightColors[i] / (distance * distance);

        float NoL = clamp(dot(N, L), 0.0f, 1.0f);

        // Cook-Torrance microfacet specular shading model
        float D = GGXDistribution(N, H, roughness * roughness);
        vec3 F = fresnelSchlick(clamp(dot(H, V), 0.0f, 1.0f), F0);
        float G = G_Smith(roughness, NoV, NoL);

        vec3 f_spec = D * F * G / (4 * NoV * NoL + 0.001f);

        // Conservation of energy
        vec3 k_spec = F;
        vec3 k_diff = vec3(1.0f) - k_spec;
        // Only nonmetals factor in diffuse lighting data
        k_diff *= 1.0f - metallic;

        Lo += (k_diff * albedo / PI + f_spec) * Li * NoL;
    }

    return Lo;
}

vec3 approximateSpecularIBL(vec3 specularColor, float roughness, float NoV, vec3 R)
{
    vec3 prefilteredColor = textureLod(gPrefilteredEnvMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF = texture(gBRDFIntegrationMap, vec2(NoV, roughness)).rg;
    return prefilteredColor * (specularColor * envBRDF.x + envBRDF.y);
}

vec3 IBL(vec3 N, vec3 V, vec3 R, vec3 F0, vec3 albedo, float metallic, float roughness, float ao)
{
    float NoV = clamp(dot(N, V), 0.0f, 1.0f);

    // Specular IBL
    vec3 specularColor = fresnelSchlick(NoV, F0);
    specularColor = approximateSpecularIBL(specularColor, roughness, NoV, R);

    // Diffuse IBL
    vec3 diffuseColor = texture(gIrradianceMap, N).rgb * albedo;

    // Conservation of energy
    vec3 k_spec = specularColor;
    vec3 k_diff = 1.0f - k_spec;
    k_diff *= 1.0f - metallic;

    // IBL
    return (k_diff * diffuseColor + specularColor) * ao;
}

void main()
{
    vec3 albedo = pow(texture(gAlbedoMap, fs_in.TexCoords).rgb, vec3(2.2f));
    float metallic = texture(gMetallicMap, fs_in.TexCoords).r;
    float roughness = texture(gRoughnessMap, fs_in.TexCoords).r;
    float ao = texture(gAOMap, fs_in.TexCoords).r;

    vec3 tangentSpaceNormal = texture(gNormalMap, fs_in.TexCoords).xyz * 2.0f - 1.0f;
    vec3 N = calcWorldSpaceNormal(tangentSpaceNormal);
    vec3 V = normalize(gViewPos - fs_in.WorldPos);
    vec3 R = reflect(-V, N);

    float NoV = clamp(dot(N, V), 0.0f, 1.0f);

    // Interpolate between the Fresnel reflectance values for dielectric materials and metals,
    // given the value obtained from the metalness texture
    vec3 F0 = vec3(0.04f);
    F0 = mix(F0, albedo, metallic);

    vec3 fragColor = IBL(N, V, R, F0, albedo, metallic, roughness, ao);
    vec3 Lo = evalReflectanceEquation(N, V, F0, albedo, metallic, roughness);

    // Now add together the IBL color and the result from our physically based lighting calculations
    fragColor += Lo;

    // Tone mapping and gamma correction
    fragColor = fragColor / (fragColor + vec3(1.0f));
    fragColor = pow(fragColor, vec3(1.0f / 2.2f));

    FragColor = vec4(fragColor, 1.0f);
}
