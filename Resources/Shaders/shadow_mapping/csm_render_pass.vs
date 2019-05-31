#version 460 core
layout (location = 0) in vec3 gPos;
layout (location = 1) in vec2 gTexCoords;
layout (location = 2) in vec3 gNormal;

const int NUM_CASCADES = 3;

out VS_OUT {
    vec3 WorldPos;
    vec3 WorldNormal;
    vec2 TexCoords;
    vec4 LightSpacePos[NUM_CASCADES];
    float ClipSpacePosZ;
} vs_out;

uniform mat4 gWVP;
uniform mat4 gWorldMatrix;
uniform mat4 gLightSpaceVP[NUM_CASCADES];
uniform mat3 gNormalMatrix;

void main()
{
    vs_out.WorldPos = vec3(gWorldMatrix * vec4(gPos, 1.0f));
    vs_out.WorldNormal = gNormalMatrix * gNormal;
    vs_out.TexCoords = gTexCoords;
    for (int i = 0; i < NUM_CASCADES; ++i)
    {
        vs_out.LightSpacePos[i] = gLightSpaceVP[i] * vec4(vs_out.WorldPos, 1.0f);
    }
    gl_Position = gWVP * vec4(gPos, 1.0f);
    vs_out.ClipSpacePosZ = gl_Position.z;
}
