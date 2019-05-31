#version 460 core
out vec4 FragColor;

const float PI = 3.14159265359f;

in vec3 WorldPos;

uniform samplerCube gEnvMap;

void main()
{		
    vec3 N = normalize(WorldPos);

    vec3 up = vec3(0.0f, 1.0f, 0.0f);
    vec3 right = normalize(cross(up, N));
    up = cross(N, right);

    vec3 sampledColor = vec3(0.0f);
    float numSamples = 0.0f;
    for (float phi = 0.0f; phi < 2.0f * PI; phi += 0.025f)
    {
        for (float theta = 0.0f; theta < 0.5f * PI; theta += 0.1f)
        {
            // Convert from spherical polar angles to a point on the unit sphere
            vec3 temp = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // Convert from the point position to the cubemap position in the world,
            // so that we are essentially sampling the hemisphere relative to p
            vec3 sampleVector = temp.x * right + temp.y * up + temp.z * N;
            // With correction for sampling bias
            sampledColor += texture(gEnvMap, sampleVector).rgb * cos(theta) * sin(theta);
            ++numSamples;
        }
    }

    FragColor = vec4(PI * sampledColor / numSamples, 1.0f);
}
