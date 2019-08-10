#version 460 core
layout (location = 0) out vec3 L00;
layout (location = 1) out vec3 L1_1;
layout (location = 2) out vec3 L10;
layout (location = 3) out vec3 L11;
layout (location = 4) out vec3 L2_2;
layout (location = 5) out vec3 L2_1;
layout (location = 6) out vec3 L20;
layout (location = 7) out vec3 L21;

in vec3 WorldPos;

uniform samplerCube gEnvMap;

void main()
{
    vec3 N = normalize(WorldPos);
    vec3 L = texture(gEnvMap, N).rgb;

    // L_l_m = Sum of All L(theta, phi) * Y_l_m(theta, phi) = Sum of All L(N) * Y_l_m(N)
    L00 = L * 0.282095f;
    L1_1 = L * 0.488603f * N.y;
    L10 = L * 0.488603f * N.z;
    L11 = L * 0.488603f * N.x;
    L2_2 = L * 1.092548f * N.x * N.y;
    L2_1 = L * 1.092548f * N.y * N.z;
    L20 = L * 0.315392f * (3.0f * N.z * N.z - 1.0f);
    L21 = L * 1.092548f * N.x * N.z;
}
