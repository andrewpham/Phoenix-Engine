#version 460 core
out vec3 L22;

in vec3 WorldPos;

uniform samplerCube gEnvMap;

void main()
{
    vec3 N = normalize(WorldPos);
    vec3 L = texture(gEnvMap, N).rgb;

    L22 = L * 0.546274f * (N.x * N.x - N.y * N.y);
}
