#version 460 core
out vec4 FragColor;

in vec3 WorldPos;

uniform samplerCube gEnvMap;

void main()
{		
    vec3 fragColor = textureLod(gEnvMap, WorldPos, 0.0f).rgb;

    // Tone mapping and gamma correction
    fragColor = fragColor / (fragColor + vec3(1.0f));
    fragColor = pow(fragColor, vec3(1.0f / 2.2f));

    FragColor = vec4(fragColor, 1.0f);
}
