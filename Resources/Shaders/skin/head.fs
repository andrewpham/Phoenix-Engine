#version 460 core
out vec4 FragColor;

const float MIX = 0.5f;
const float CORRECTION_FACTOR = 100.0f;
const int NUM_CONVOLUTIONS = 6;
const vec3 GAUSSIAN_WEIGHTS[NUM_CONVOLUTIONS] = vec3[]
    (vec3(0.233f, 0.455f, 0.649f),
     vec3(0.1f, 0.336f, 0.344f),
     vec3(0.118f, 0.198f, 0.0f),
     vec3(0.113f, 0.007f, 0.007f),
     vec3(0.358f, 0.004f, 0.0f),
     vec3(0.078f, 0.0f, 0.0f));

in VS_OUT {
    vec3 WorldPos;
    vec3 WorldNormal;
    vec2 TexCoords;
    vec4 LightSpacePos;
} fs_in;

uniform sampler2D gDiffuseTexture;
uniform sampler2D gShadowMap;
uniform sampler2D gNormalMap;
uniform sampler2D gBeckmannTexture;
uniform sampler2D gStretchMap;
uniform sampler2D gSpecularTexture;
uniform sampler2D gIrradianceMaps[NUM_CONVOLUTIONS];

uniform vec3 gLightPos;
uniform vec3 gViewPos;
uniform vec3 gLightColor;
uniform float gAmbientFactor;
uniform float gSpecularFactor;
uniform int gRenderMode;

const vec3 T = normalize(dFdx(fs_in.WorldPos) * dFdy(fs_in.TexCoords).t - dFdy(fs_in.WorldPos) * dFdx(fs_in.TexCoords).t);
vec3 calcWorldSpaceNormal(vec3 tangentSpaceNormal)
{
    vec3 N = normalize(fs_in.WorldNormal);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentSpaceNormal);
}
const vec3 TANGENT_SPACE_N = texture(gNormalMap, fs_in.TexCoords).xyz * 2.0f - 1.0f;
const vec3 N = calcWorldSpaceNormal(TANGENT_SPACE_N);
const vec3 L = normalize(gLightPos - fs_in.WorldPos);
const vec3 V = normalize(gViewPos - fs_in.WorldPos);
const vec3 LIGHT_SPACE_POS_POST_W = fs_in.LightSpacePos.xyz / fs_in.LightSpacePos.w * 0.5f + 0.5f;
const vec2 FLIPPED_UV = vec2(fs_in.TexCoords.x, 1.0f - fs_in.TexCoords.y);

float fresnelReflectance(float cosTheta, float F0)
{
    cosTheta = 1.0f - cosTheta;
    cosTheta = pow(cosTheta, 5);
    return cosTheta + (1.0f - cosTheta) * F0;
}

float KS_Skin_Specular(float m, float rho_s)
{
    float result = 0.0f;
    float NoL = dot(N, L);

    if (NoL > 0.0f)
    {
        vec3 h = L + V;
        vec3 H = normalize(h);
        float NoH = dot(N, H);
        float PH = pow(2.0f * texture(gBeckmannTexture, vec2(NoH, m)).r, 10);
        float cosTheta = dot(H, V);
        float F = fresnelReflectance(cosTheta, 0.028f);
        float frSpec = max(PH * F / dot(h, h), 0);
        result = NoL * rho_s * frSpec; // BRDF * dot(N,L) * rho_s
    }
    return result;
}

vec4 finalSkinShader()
{
    vec3 color = texture(gDiffuseTexture, fs_in.TexCoords).rgb;

    vec3 ambientLight = gAmbientFactor * color;

    // The total diffuse light exiting the surface
    vec3 diffuseLight = vec3(0.0f);

    vec4 irradTaps[NUM_CONVOLUTIONS];
    for (int i = 0; i < NUM_CONVOLUTIONS; ++i)
    {
        irradTaps[i] = texture(gIrradianceMaps[i], FLIPPED_UV);
        diffuseLight += GAUSSIAN_WEIGHTS[i] * irradTaps[i].xyz;
    }

    // Renormalize diffusion profiles to white
    vec3 normConst = GAUSSIAN_WEIGHTS[0] + GAUSSIAN_WEIGHTS[1] + GAUSSIAN_WEIGHTS[2] + GAUSSIAN_WEIGHTS[3] + GAUSSIAN_WEIGHTS[4] + GAUSSIAN_WEIGHTS[5];
    diffuseLight /= normConst; // Renormalize to white diffuse light

    // Compute global scatter from modified TSM
    // TSMtap = (distance to light, u, v)
    vec3 TSMtap = texture(gShadowMap, LIGHT_SPACE_POS_POST_W.xy).xyz;

    // Four average thicknesses through the object (in mm)
    vec4 thickness_mm = 1.0f * -(1.0f / 0.2f) * log(vec4(irradTaps[1].w, irradTaps[2].w, irradTaps[3].w, irradTaps[4].w));

    vec2 stretchTap = texture(gStretchMap, FLIPPED_UV).xy;
    float stretchval = 0.5f * (stretchTap.x + stretchTap.y);

    vec4 a_values = vec4(0.433f, 0.753f, 1.412f, 2.722f);
    vec4 inv_a = -1.0f / (2.0f * a_values * a_values);
    vec4 fades = exp(thickness_mm * thickness_mm * inv_a);

    float textureScale = 1024.0f * 0.1f / stretchval;
    float blendFactor4 = clamp(textureScale * length(FLIPPED_UV - TSMtap.yz) / (a_values.y * 0.6f), 0.0f, 1.0f);
    float blendFactor5 = clamp(textureScale * length(FLIPPED_UV - TSMtap.yz) / (a_values.z * 0.6f), 0.0f, 1.0f);
    float blendFactor6 = clamp(textureScale * length(FLIPPED_UV - TSMtap.yz) / (a_values.w * 0.6f), 0.0f, 1.0f);

    diffuseLight += GAUSSIAN_WEIGHTS[3] / normConst * fades.y * blendFactor4 * texture(gIrradianceMaps[3], TSMtap.yz).xyz;
    diffuseLight += GAUSSIAN_WEIGHTS[4] / normConst * fades.z * blendFactor5 * texture(gIrradianceMaps[4], TSMtap.yz).xyz;
    diffuseLight += GAUSSIAN_WEIGHTS[5] / normConst * fades.w * blendFactor6 * texture(gIrradianceMaps[5], TSMtap.yz).xyz;

    // Determine skin color from a diffuseColor map
    diffuseLight *= pow(color, vec3(1.0f - MIX));

    vec4 specTap = texture(gSpecularTexture, FLIPPED_UV); // rho_s and m (roughness)
    float m = specTap.w * 0.09f + 0.23f;
    float rho_s = specTap.x * 0.16f + 0.18f;
    rho_s *= float(specTap.x > 0.1f);

    // Compute specular for each light
    vec3 specularLight = CORRECTION_FACTOR * gLightColor * KS_Skin_Specular(m, rho_s);

    return vec4(ambientLight + diffuseLight + specularLight, 1.0f);
}

vec4 calcBlinnPhongColor()
{
    vec3 color = texture(gDiffuseTexture, fs_in.TexCoords).rgb;

    vec3 ambient = gAmbientFactor * color;

    vec3 diffuse = clamp(dot(N, L), 0.0f, 1.0f) * gLightColor;

    vec3 V = normalize(gViewPos - fs_in.WorldPos);
    vec3 H = normalize(L + V);
    vec3 specular = pow(clamp(dot(N, H), 0.0f, 1.0f), gSpecularFactor) * gLightColor;

    return vec4((ambient + diffuse + specular) * color, 1.0f);
}

void main()
{
    if (gRenderMode == 7)
    {
        FragColor = calcBlinnPhongColor();
        return;
    }
    FragColor = finalSkinShader();
}
