#version 460 core
out vec4 FragColor;

const float STEP_SIZE = 0.005f;

in vec2 TexCoords;

uniform sampler2D gBackfaceTexture;
uniform sampler2D gFrontfaceTexture;
uniform sampler3D gTexture3D;
uniform vec3 gViewPos;

bool isInsideUnitCube()
{
    return abs(gViewPos.x) < 1.2f && abs(gViewPos.y) < 1.2f && abs(gViewPos.z) < 1.2f;
}

void main() {
    vec3 start = isInsideUnitCube() ? gViewPos : texture(gFrontfaceTexture, TexCoords).xyz;
    vec3 direction = texture(gBackfaceTexture, TexCoords).xyz - start;
    uint numSteps = uint(length(direction) / STEP_SIZE);
    direction = normalize(direction);

    FragColor = vec4(0.0f);
    for(uint step = 0; step < numSteps && FragColor.a < 1.0f; ++step)
    {
        vec3 position = start + STEP_SIZE * step * direction;
        position = position * 0.5f + 0.5f;
        FragColor += (1.0f - FragColor.a) * textureLod(gTexture3D, position, 0);
    }
    FragColor.rgb = pow(FragColor.rgb, vec3(1.0f / 2.2f));
}
